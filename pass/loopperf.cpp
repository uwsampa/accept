#include "accept.h"

#include "llvm/Analysis/LoopPass.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"

#include <sstream>

#define ACCEPT_LOG ACCEPT_LOG_(AI)

using namespace llvm;

namespace {
  struct LoopPerfPass : public LoopPass {
    static char ID;
    ACCEPTPass *transformPass;
    ApproxInfo *AI;
    Module *module;
    LoopInfo *LI;

    LoopPerfPass() : LoopPass(ID) {}

    virtual bool doInitialization(Loop *loop, LPPassManager &LPM) {
      transformPass = (ACCEPTPass*)sharedAcceptTransformPass;
      AI = transformPass->AI;
      return false;
    }
    virtual bool runOnLoop(Loop *loop, LPPassManager &LPM) {
      if (transformPass->shouldSkipFunc(*(loop->getHeader()->getParent())))
          return false;
      module = loop->getHeader()->getParent()->getParent();
      LI = &getAnalysis<LoopInfo>();
      return tryToOptimizeLoop(loop);
    }
    virtual bool doFinalization() {
      return false;
    }
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      LoopPass::getAnalysisUsage(AU);
      AU.addRequired<LoopInfo>();
    }

    IntegerType *getNativeIntegerType() {
      DataLayout layout(module->getDataLayout());
      return Type::getIntNTy(module->getContext(),
                              layout.getPointerSizeInBits());
    }

    // Assess whether a loop can be optimized and, if so, log some messages and
    // update the configuration map. If optimization is turned on, the
    // configuration map will be used to actually transform the loop. Returns a
    // boolean indicating whether the code was changed (i.e., the loop
    // perforated).
    bool tryToOptimizeLoop(Loop *loop) {
      Instruction *inst = loop->getHeader()->begin();
      Function *func = inst->getParent()->getParent();
      std::string funcName = func->getName().str();

      ACCEPT_LOG << "---\nloop within function _" << funcName << "\n";

      std::stringstream ss;
      ss << "loop at "
         << srcPosDesc(*module, loop->getHeader()->begin()->getDebugLoc());
      std::string loopName = ss.str();

      ACCEPT_LOG << loopName << "\n";

      // Look for ACCEPT_FORBID marker.
      if (AI->instMarker(loop->getHeader()->begin()) == markerForbid) {
        ACCEPT_LOG << "optimization forbidden\n";
        return false;
      }

      // We only consider loops for which there is a header (condition), a
      // latch (increment, in "for"), and a preheader (initialization).
      if (!loop->getHeader() || !loop->getLoopLatch()
          || !loop->getLoopPreheader()) {
        ACCEPT_LOG << "loop not in perforatable form\n";
        return false;
      }

      // Skip array constructor loops manufactured by Clang.
      if (loop->getHeader()->getName().startswith("arrayctor.loop")) {
        ACCEPT_LOG << "array constructor\n";
        return false;
      }

      // Skip empty-body loops (where perforation would do nothing anyway).
      if (loop->getNumBlocks() == 2
          && loop->getHeader() != loop->getLoopLatch()) {
        BasicBlock *latch = loop->getLoopLatch();
        if (&(latch->front()) == &(latch->back())) {
          ACCEPT_LOG << "empty body\n";
          return false;
        }
      }

      /*
      if (acceptUseProfile) {
        ProfileInfo &PI = getAnalysis<ProfileInfo>();
        double trips = PI.getExecutionCount(loop->getHeader());
        ACCEPT_LOG << "trips: " << trips << "\n";
      }
      */

      // Determine whether this is a for-like or while-like loop. This informs
      // the heuristic that determines which parts of the loop to perforate.
      bool isForLike = false;
      if (loop->getHeader()->getName().startswith("for.cond") || loop->getLoopLatch() != NULL) {
        ACCEPT_LOG << "for-like loop\n";
        isForLike = true;
      } else {
        ACCEPT_LOG << "while-like loop\n";
      }

      if (transformPass->relax) {
        int param = transformPass->relaxConfig[loopName];
        if (param) {
          ACCEPT_LOG << "perforating with factor 2^" << param << "\n";
          perforateLoop(loop, param, isForLike);
          return true;
        } else {
          ACCEPT_LOG << "not perforating\n";
          return false;
        }
      }

      // Get the body blocks of the loop: those that will not be executed during
      // some iterations of a perforated loop.
      std::set<BasicBlock*> bodyBlocks;
      for (Loop::block_iterator bi = loop->block_begin();
            bi != loop->block_end(); ++bi) {
        if (*bi == loop->getHeader()) {
          // Even in perforated loops, the header gets executed every time. So we
          // don't check it.
          continue;
        } else if (isForLike && *bi == loop->getLoopLatch()) {
          // When perforating for-like loops, we also execute the latch each
          // time.
          continue;
        }
        bodyBlocks.insert(*bi);
      }
      if (bodyBlocks.empty()) {
        ACCEPT_LOG << "empty body\n";
        return false;
      }

      // Check for control flow in the loop body. We don't perforate anything
      // with a break, continue, return, etc.
      for (std::set<BasicBlock*>::iterator i = bodyBlocks.begin();
            i != bodyBlocks.end(); ++i) {
        if (loop->isLoopExiting(*i)) {
          ACCEPT_LOG << "contains loop exit\n";
          return false;
        }
      }

      // Check whether the body of this loop is elidable (precise-pure).
      std::set<Instruction*> blockers = AI->preciseEscapeCheck(bodyBlocks);
      ACCEPT_LOG << "blockers: " << blockers.size() << "\n";
      for (std::set<Instruction*>::iterator i = blockers.begin();
            i != blockers.end(); ++i) {
        ACCEPT_LOG << " * " << instDesc(*module, *i) << "\n";
      }

      if (!blockers.size()) {
        ACCEPT_LOG << "can perforate loop\n";
        transformPass->relaxConfig[loopName] = 0;
      }

      return false;
    }

    // Transform a loop to skip iterations.
    // The loop should already be validated as perforatable, but checks will be
    // performed nonetheless to ensure safety.
    void perforateLoop(Loop *loop, int logfactor, bool isForLike) {
      // Check whether this loop is perforatable.
      // First, check for required blocks.
      if (!loop->getHeader() || !loop->getLoopLatch()
          || !loop->getLoopPreheader() || !loop->getExitBlock()) {
        errs() << "malformed loop\n";
        return;
      }
      // Next, make sure the header (condition block) ends with a body/exit
      // conditional branch.
      BranchInst *condBranch = dyn_cast<BranchInst>(
          loop->getHeader()->getTerminator()
      );
      if (!condBranch || condBranch->getNumSuccessors() != 2) {
        errs() << "malformed loop condition\n";
        return;
      }
      BasicBlock *bodyBlock;
      if (condBranch->getSuccessor(0) == loop->getExitBlock()) {
        bodyBlock = condBranch->getSuccessor(1);
      } else if (condBranch->getSuccessor(1) == loop->getExitBlock()) {
        bodyBlock = condBranch->getSuccessor(0);
      } else {
        errs() << "loop condition does not exit\n";
        return;
      }

      // Get the shortcut for the destination. In for-like loop perforation, we
      // shortcut to the latch (increment block). In while-like perforation, we
      // jump to the header (condition block).
      BasicBlock *shortcutDest;
      if (isForLike) {
        shortcutDest = loop->getLoopLatch();
      } else {
        shortcutDest = loop->getHeader();
      }

      IRBuilder<> builder(module->getContext());
      Value *result;

      // Allocate stack space for the counter.
      // LLVM "alloca" instructions go in the function's entry block. Otherwise,
      // they have to adjust the frame size dynamically (and, in my experience,
      // can actually segfault!). And we only want one of these per static loop
      // anyway.
      builder.SetInsertPoint(
          loop->getLoopPreheader()->getParent()->getEntryBlock().begin()
      );

      IntegerType *nativeInt = getNativeIntegerType();
      AllocaInst *counterAlloca = builder.CreateAlloca(
          nativeInt,
          0,
          "accept_counter"
      );

      // Initialize the counter in the preheader.
      builder.SetInsertPoint(loop->getLoopPreheader()->getTerminator());
      builder.CreateStore(
          ConstantInt::get(nativeInt, 0, false),
          counterAlloca
      );

      // Increment the counter in the latch.
      builder.SetInsertPoint(loop->getLoopLatch()->getTerminator());
      result = builder.CreateLoad(
          counterAlloca,
          "accept_tmp"
      );
      result = builder.CreateAdd(
          result,
          ConstantInt::get(nativeInt, 1, false),
          "accept_inc"
      );
      builder.CreateStore(
          result,
          counterAlloca
      );

      // Get the first body block.

      // Check the counter before the loop's body.
      BasicBlock *checkBlock = BasicBlock::Create(
          module->getContext(),
          "accept_cond",
          bodyBlock->getParent(),
          bodyBlock
      );
      builder.SetInsertPoint(checkBlock);
      result = builder.CreateLoad(
          counterAlloca,
          "accept_tmp"
      );
      // Check whether the low n bits of the counter are zero.
      result = builder.CreateTrunc(
          result,
          Type::getIntNTy(module->getContext(), logfactor),
          "accept_trunc"
      );
      result = builder.CreateIsNull(
          result,
          "accept_cmp"
      );
      result = builder.CreateCondBr(
          result,
          bodyBlock,
          loop->getLoopLatch()
      );

      // Change the condition block to point to our new condition
      // instead of the body.
      condBranch->setSuccessor(0, checkBlock);

      // Add block to the loop structure.
      loop->addBasicBlockToLoop(checkBlock, LI->getBase());
    }

  };
}

char LoopPerfPass::ID = 0;
LoopPass *llvm::createLoopPerfPass() { return new LoopPerfPass(); }

