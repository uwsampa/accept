#include "accept.h"

#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "../llvm/lib/Transforms/Utils/LoopUnrollRuntime.cpp"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <algorithm>

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

    // This function adds a loop description to descTable.
    void addLoopDesc(
        const bool hasBlockers,
        const std::string fileName,
        const int lineNumber,
        const std::string prefix,
        const std::string postfix,
        const std::map< int, std::vector<std::string> > blockerEntries) {
      Location loc("Loop", hasBlockers, fileName, lineNumber);
      if (AI->descTable.count(loc) == 0) {
        AI->descTable[loc] = std::vector<Description>();
      }

      Description loopDesc(prefix, postfix, blockerEntries);
      AI->descTable[loc].push_back(loopDesc);
    }
      
    // Assess whether a loop can be optimized and, if so, log some messages and
    // update the configuration map. If optimization is turned on, the
    // configuration map will be used to actually transform the loop. Returns a
    // boolean indicating whether the code was changed (i.e., the loop
    // perforated).
    bool tryToOptimizeLoop(Loop *loop) {
      std::stringstream prefixStream;
      std::stringstream postfixStream;
      std::map< int, std::vector<std::string> > blockerEntries;

      std::string posDesc = srcPosDesc(*module, loop->getHeader()->begin()->getDebugLoc());
      std::string fileName, line;
      splitPosDesc(posDesc, fileName, line);
      int lineNumber = atoi(line.c_str());

      std::stringstream ss;
      ss << "loop at " << posDesc;
      std::string loopName = ss.str();

      prefixStream << loopName << "\n";

      Instruction *inst = loop->getHeader()->begin();
      Function *func = inst->getParent()->getParent();
      std::string funcName = func->getName().str();

      prefixStream << " - within function " << funcName << "\n";

      // Look for ACCEPT_FORBID marker.
      if (AI->instMarker(loop->getHeader()->begin()) == markerForbid) {
        prefixStream << " - optimization forbidden\n";
        addLoopDesc(false, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
        return false;
      }

      // We only consider loops for which there is a header (condition), a
      // latch (increment, in "for"), and a preheader (initialization).
      if (!loop->getHeader() || !loop->getLoopLatch()
          || !loop->getLoopPreheader()) {
        prefixStream << " - loop not in perforatable form\n";
        addLoopDesc(false, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
        return false;
      }

      // Skip array constructor loops manufactured by Clang.
      if (loop->getHeader()->getName().startswith("arrayctor.loop")) {
        prefixStream << " - array constructor\n";
        addLoopDesc(false, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
        return false;
      }

      // Skip empty-body loops (where perforation would do nothing anyway).
      if (loop->getNumBlocks() == 2
          && loop->getHeader() != loop->getLoopLatch()) {
        BasicBlock *latch = loop->getLoopLatch();
        if (&(latch->front()) == &(latch->back())) {
          prefixStream << " - empty body\n";
          addLoopDesc(false, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
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
      if (loop->getHeader()->getName().startswith("for.cond")) {
        prefixStream << " - for-like loop\n";
        isForLike = true;
      } else {
        prefixStream << " - while-like loop\n";
      }

      if (transformPass->relax) {
        int param = transformPass->relaxConfig[loopName];
        if (param) {
          prefixStream << " - perforating with factor 2^" << param << "\n";
          perforateLoop(loop, param, isForLike);
          addLoopDesc(false, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
          return true;
        } else {
          prefixStream << " - not perforating\n";
          addLoopDesc(false, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
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
        prefixStream << " - empty body\n";
        addLoopDesc(false, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
        return false;
      }

      // Check for control flow in the loop body. We don't perforate anything
      // with a break, continue, return, etc.
      for (std::set<BasicBlock*>::iterator i = bodyBlocks.begin();
            i != bodyBlocks.end(); ++i) {
        if (loop->isLoopExiting(*i)) {
          prefixStream << " - contains loop exit\n";
          prefixStream << " - cannot perforate loop\n";
          addLoopDesc(false, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
          return false;
        }
      }

      // Check whether the body of this loop is elidable (precise-pure).
      std::set<Instruction*> blockers = AI->preciseEscapeCheck(bodyBlocks);
      prefixStream << " - blockers: " << blockers.size() << "\n";
      for (std::set<Instruction*>::iterator i = blockers.begin();
            i != blockers.end(); ++i) {
        std::string blockerEntry = instDesc(*module, *i);
        int blockerLine = extractBlockerLine(blockerEntry);

        if (blockerEntries.count(blockerLine) == 0) {
          blockerEntries[blockerLine] = std::vector<std::string>();
        }

        std::stringstream blockerStream;
        blockerStream << "    * " << blockerEntry << "\n";
        blockerEntries[blockerLine].push_back(blockerStream.str());
      }

      if (!blockers.size()) {
        postfixStream << " - can perforate loop\n";
        transformPass->relaxConfig[loopName] = 0;
        addLoopDesc(false, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
      } else {
        postfixStream << " - cannot perforate loop\n";
        addLoopDesc(true, fileName, lineNumber, prefixStream.str(), postfixStream.str(), blockerEntries);
      }

      return false;
    }

    // Find the most recent StoreInst in the loop that precedes the LoadInst and has the same pointer operand.
    StoreInst *findMostRecentStoreInst(Instruction *inst, Loop *loop) {
      if (LoadInst *LI = dyn_cast<LoadInst>(inst)) {
        BasicBlock *bodyBlock = inst->getParent();

        BasicBlock::InstListType::reverse_iterator revInstItr;
        for (BasicBlock::InstListType::reverse_iterator i = bodyBlock->getInstList().rbegin(); i != bodyBlock->getInstList().rend(); i++) {
          if (&*i == inst) {
            revInstItr = ++i;
            break;
          }
        }
        for (BasicBlock::InstListType::reverse_iterator i = revInstItr; i != bodyBlock->getInstList().rend(); i++) {
          if (StoreInst *SI = dyn_cast<StoreInst>(&*i)) {
            if (SI->getPointerOperand() == LI->getPointerOperand()) {
              return SI;
            }
          }
        }
        typedef typename std::vector<BasicBlock *>::reverse_iterator reverse_block_iterator;
        std::vector<BasicBlock *> blkList = loop->getBlocks();
        reverse_block_iterator revBlkItr;
        for (reverse_block_iterator i = blkList.rbegin(); i != blkList.rend(); i++) {
          if (*i == bodyBlock) {
            revBlkItr = ++i;
            break;
          }
        }
        for (reverse_block_iterator i = revBlkItr; i != blkList.rend(); i++) {
          for (BasicBlock::InstListType::reverse_iterator j = (*i)->getInstList().rbegin(); j != (*i)->getInstList().rend(); j++) {
            if (StoreInst *SI = dyn_cast<StoreInst>(&*j)) {
              if (SI->getPointerOperand() == LI->getPointerOperand()) {
                return SI;
              }
            }
          }
        }
      }

      return NULL;
    }

    // This function slices the pointer operand of a store instruction to determine
    // which other instructions need to be preserved.
    void slicePointerOperand(StoreInst *SI, Loop *loop, std::set<Instruction *> &insts, std::set<Instruction *> &preserved) {        
      if (Instruction *inst = dyn_cast<Instruction>(SI->getPointerOperand())) {
        if (insts.count(inst)) {
          preserved.insert(inst);
          sliceOperandHelper(inst, loop, insts, preserved);
        }
      }
    }

    // This function is a helper function for slicing operands.
    void sliceOperandHelper(Instruction *inst, Loop *loop, std::set<Instruction *> &insts, std::set<Instruction *> &preserved) {        
      BasicBlock *bodyBlock = inst->getParent();

      if (LoadInst *LI = dyn_cast<LoadInst>(inst)) {
        if (StoreInst *SI = findMostRecentStoreInst(inst, loop)) {
          preserved.insert(SI);
          sliceOperandHelper(SI, loop, insts, preserved);
        }
      } else {
        for (Instruction::op_iterator i = inst->op_begin(); i != inst->op_end(); i++) {
          if (Instruction *tmp = dyn_cast<Instruction>(*i)) {
            if (insts.count(tmp)) {
              preserved.insert(tmp);
              sliceOperandHelper(tmp, loop, insts, preserved);
            }
          }
        }
      }
    }

    // Find the most recent add/fadd instruction.
    BinaryOperator *findAddInst(Instruction *inst, Loop *loop, Value *pointerOperand, std::set<Instruction *> insts, int &incOperand) {
      if (insts.count(inst)) {
        if (StoreInst *storeInst = dyn_cast<StoreInst>(inst)) {
          if (LoadInst *loadInst = dyn_cast<LoadInst>(storeInst->getValueOperand())) {
            return findAddInst(loadInst, loop, pointerOperand, insts, incOperand);
          } else if (BinaryOperator *opInst = dyn_cast<BinaryOperator>(storeInst->getValueOperand())) {
            if ((opInst->getOpcode() == Instruction::Add) || (opInst->getOpcode() == Instruction::FAdd)) {
              Value *op1 = opInst->getOperand(0);
              Value *op2 = opInst->getOperand(1);
              if (loadsAccumVariable(op1, loop, pointerOperand, insts)) {
                incOperand = 1;
                return opInst;
              } else if (loadsAccumVariable(op2, loop, pointerOperand, insts)) {
                incOperand = 0;
                return opInst;
              }
            }
          }
        } else if (LoadInst *loadInst = dyn_cast<LoadInst>(inst)) {
          if (StoreInst *storeInst = findMostRecentStoreInst(loadInst, loop)) {
            return findAddInst(storeInst, loop, pointerOperand, insts, incOperand);
          }
        }
      }

      return NULL;
    }

    // Determine whether an addend is the accumulation variable.
    bool loadsAccumVariable(Value *operand, Loop *loop, Value *pointerOperand, std::set<Instruction *> insts) {
      if (Instruction *inst = dyn_cast<Instruction>(operand)) {
        if (insts.count(inst)) {
          if (LoadInst *loadInst = dyn_cast<LoadInst>(inst)) {
            if (loadInst->getPointerOperand() == pointerOperand) {
              return true;
            } else if (StoreInst *storeInst = findMostRecentStoreInst(loadInst, loop)) {
              return loadsAccumVariable(storeInst, loop, pointerOperand, insts);
            }
          } else if (StoreInst *storeInst = dyn_cast<StoreInst>(inst)) {
            if (LoadInst *loadInst = dyn_cast<LoadInst>(storeInst->getValueOperand())) {
              return loadsAccumVariable(loadInst, loop, pointerOperand, insts);
            }
          }
        }
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
        errs() << "loop condition does no exit\n";
        return;
      }

      // Collect the set of instructions in the loop body.
      std::set<Instruction *> insts;
      std::set<StoreInst *> storeInsts;
      std::set<Instruction *> preserved;

      Loop::block_iterator bodyBlockItr;
      for (Loop::block_iterator bi = loop->block_begin(); *bi != loop->getLoopLatch(); bi++) {
        if (*bi == bodyBlock) {
          bodyBlockItr = bi;
          break;
        }
      }
      for (Loop::block_iterator bi = bodyBlockItr; *bi != loop->getLoopLatch(); bi++) {
        for (BasicBlock::iterator i = (*bi)->begin(); i != (*bi)->end(); i++) {
          insts.insert(i);
          // Collect the set of store instructions.
          if (StoreInst *SI = dyn_cast<StoreInst>(i)) {
            storeInsts.insert(SI);
          }
        }
      }

      // Collect the set of store instructions to preserve in the loop body.
      std::set<StoreInst *> preservedStores;
      for (std::set<StoreInst *>::iterator i = storeInsts.begin(); i != storeInsts.end(); i++) {
        if (AI->storeEscapes(*i, insts)) {
          preservedStores.insert(*i);
        }
      }

      // Map the set of store instructions that correspond to accumulations in the loop body.
      std::map< StoreInst *, std::pair< BinaryOperator *, int > > accumStores;
      for (std::set<StoreInst *>::iterator i = storeInsts.begin(); i != storeInsts.end(); i++) {
        int incOperand;
        if (BinaryOperator *addInst = findAddInst(*i, loop, (*i)->getPointerOperand(), insts, incOperand)) {
          preserved.insert(*i);
          preserved.insert(addInst);
          accumStores[*i] = std::make_pair(addInst, incOperand);
        }
      }

      // Remove all elements of accumStores from preservedStores, so that a StoreInst is not assessed for
      // optimization twice.
      for (std::map< StoreInst *, std::pair< BinaryOperator *, int> >::iterator i = accumStores.begin(); i != accumStores.end(); i++) {
        StoreInst *accumStore = i->first;
        if (preservedStores.count(accumStore)) {
          preservedStores.erase(accumStore);
        }
      }

      // Slice the pointer operand of each of the store instructions.
      for (std::set<StoreInst *>::iterator i = preservedStores.begin(); i != preservedStores.end(); i++) {
        preserved.insert(*i);
        slicePointerOperand(*i, loop, insts, preserved);
      }

      // Slice the condition of each of the branch instructions.
      for (std::set<Instruction *>::iterator i = insts.begin(); i != insts.end(); i++) {
        if (BranchInst *BI = dyn_cast<BranchInst>(*i)) {
          preserved.insert(BI);
          sliceOperandHelper(BI, loop, insts, preserved);
        }
      }

      BasicBlock *preheaderBlock = loop->getLoopPreheader();
      BasicBlock *latchBlock = loop->getLoopLatch();

      std::vector<BasicBlock *> blockClones;
      LoopBlocksDFS LoopBlocks(loop);
      LoopBlocks.perform(LI);
      ValueToValueMapTy VMap, LVMap;

      CloneLoopBlocks(loop, true, loop->getLoopPreheader(), loop->getLoopLatch(),
          blockClones, LoopBlocks, VMap, LVMap, LI);

      for (ValueToValueMapTy::iterator i = VMap.begin(); i != VMap.end(); i++) {
        if (Instruction *origInst = (Instruction *) dyn_cast<Instruction>(i->first)) {
          if (Instruction *clonedInst = dyn_cast<Instruction>(i->second)) {
            for (unsigned j = 0; j != clonedInst->getNumOperands(); j++) {
              if (VMap.count(origInst->getOperand(j))) {
                clonedInst->setOperand(j, VMap[origInst->getOperand(j)]);
              }
            }
          }
        }
      }

      std::set<Instruction *> removed;
      BasicBlock *clonedHeader = NULL;
      BasicBlock *clonedLatch = NULL;

      for (ValueToValueMapTy::iterator i = VMap.begin(); i != VMap.end(); i++) {
        if (Instruction *origInst = (Instruction *) dyn_cast<Instruction>(i->first)) {
          if (!preserved.count(origInst)) {
            if (Instruction *clonedInst = dyn_cast<Instruction>(i->second)) {
              if (origInst->getParent() == loop->getHeader()) {
                if (!clonedHeader) {
                  clonedHeader = clonedInst->getParent();
                }
              } else if (origInst->getParent() == loop->getLoopLatch()) {
                if (!clonedLatch) {
                  clonedLatch = clonedInst->getParent();
                }
              } else {
                removed.insert(clonedInst);
              }
            }
          }
        }
      }

      BranchInst *clonedCondBranch = dyn_cast<BranchInst>(
          clonedHeader->getTerminator()
      );
      BasicBlock *clonedBodyBlock;
      if (clonedCondBranch->getSuccessor(0) == loop->getExitBlock()) {
        clonedBodyBlock = clonedCondBranch->getSuccessor(1);
      } else if (clonedCondBranch->getSuccessor(1) == loop->getExitBlock()) {
        clonedBodyBlock = clonedCondBranch->getSuccessor(0);
      }

      BasicBlock *lastClonedBodyBlock = clonedLatch->getSinglePredecessor();
      std::vector<BasicBlock *>::iterator findItr;

      for (findItr = blockClones.begin(); findItr != blockClones.end(); findItr++) {
        if (*findItr == clonedHeader) {
          break;
        }
      }
      blockClones.erase(findItr);
      for (BasicBlock::iterator i = clonedHeader->begin(); i != clonedHeader->end(); i++) {
        i->dropAllReferences();
      }
      clonedHeader->removeFromParent();
      LI->removeBlock(clonedHeader);

      for (findItr = blockClones.begin(); findItr != blockClones.end(); findItr++) {
        if (*findItr == clonedLatch) {
          break;
        }
      }
      blockClones.erase(findItr);
      for (BasicBlock::iterator i = clonedLatch->begin(); i != clonedLatch->end(); i++) {
        i->dropAllReferences();
      }
      clonedLatch->removeFromParent();
      LI->removeBlock(clonedLatch);

      for (std::set<StoreInst *>::iterator i = preservedStores.begin(); i != preservedStores.end(); i++) {
        if (StoreInst *clonedStore = dyn_cast<StoreInst>(VMap[*i])) {
          clonedStore->setOperand(0, Constant::getNullValue((*i)->getValueOperand()->getType()));
        }
      }

      for (std::map< StoreInst *, std::pair< BinaryOperator *, int> >::iterator i = accumStores.begin(); i != accumStores.end(); i++) {
        StoreInst *origStore = i->first;
        if (StoreInst *clonedStore = dyn_cast<StoreInst>(VMap[origStore])) {
          clonedStore->setOperand(0, Constant::getNullValue(origStore->getValueOperand()->getType()));
        }

        BinaryOperator *origAdd = (i->second).first;
        if (BinaryOperator *clonedAdd = dyn_cast<BinaryOperator>(VMap[origAdd])) {
          clonedAdd->setOperand(0, Constant::getNullValue(origAdd->getOperand(0)->getType()));
          clonedAdd->setOperand(1, Constant::getNullValue(origAdd->getOperand(1)->getType()));
        }
      }

      for (std::set<Instruction *>::iterator i = removed.begin(); i != removed.end(); i++) {
        (*i)->dropAllReferences();
      }
      for (std::set<Instruction *>::iterator i = removed.begin(); i != removed.end(); i++) {
        (*i)->removeFromParent();
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

      // Change the preheader block to point to the condition block again.
      preheaderBlock->getTerminator()->setSuccessor(0, condBranch->getParent());

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
          clonedBodyBlock
      );

      // Change the condition block to point to our new condition
      // instead of the body.
      condBranch->setSuccessor(0, checkBlock);

      // Add condition block to the loop structure.
      loop->addBasicBlockToLoop(checkBlock, LI->getBase());

      // Change the last cloned body block to point to the increment block.
      lastClonedBodyBlock->getTerminator()->setSuccessor(0, latchBlock);

      // Add all cloned body blocks to the loop structure.
      for (std::vector<BasicBlock *>::iterator i = blockClones.begin(); i != blockClones.end(); i++) {
        if (const Loop *curLoop = (*LI)[*i]) {
          errs() << "Block " << (*i)->getName() << " is already in a loop:\n"
            << *curLoop << "...but we want it to be in:\n" << *loop;
          LI->removeBlock(*i);
        }
        loop->addBasicBlockToLoop(*i, LI->getBase());
      }

      // Insert instructions to store and load values saved from evaluated iterations.
      for (std::set<StoreInst *>::iterator i = preservedStores.begin(); i != preservedStores.end(); i++) {
        StoreInst *SI = *i;

        // Insert an AllocaInst for the saved value.
        builder.SetInsertPoint(
            loop->getLoopPreheader()->getParent()->getEntryBlock().begin()
        );

        AllocaInst *valueAlloca = builder.CreateAlloca(
            SI->getValueOperand()->getType(),
            0,
            "accept_value"
        );

        // Insert a StoreInst to store the saved value.
        builder.SetInsertPoint(SI);
        builder.CreateStore(
            SI->getValueOperand(),
            valueAlloca
        );
        
        // Insert a LoadInst to load the saved value.
        Instruction *clonedStore = (Instruction *) &*VMap[SI];
        builder.SetInsertPoint(clonedStore);
        result = builder.CreateLoad(
            valueAlloca,
            "accept_load"
        );

        // Store the saved value to the output variable.
        clonedStore->setOperand(0, result);
      }

      bodyBlock->getParent()->dump();

      // Insert instructions for the accumulation optimization.
      for (std::map< StoreInst *, std::pair<BinaryOperator *, int> >::iterator i = accumStores.begin(); i != accumStores.end(); i++) {
        StoreInst *storeInst = i->first;
        BinaryOperator *addInst = (i->second).first;
        int incOperand = (i->second).second;

        // Insert an AllocaInst for the accumulation increment value.
        builder.SetInsertPoint(
            loop->getLoopPreheader()->getParent()->getEntryBlock().begin()
        );

        AllocaInst *accumAlloca = builder.CreateAlloca(
            addInst->getOperand(incOperand)->getType(),
            0,
            "accept_accum"
        );

        // Insert a StoreInst to store the accumulation increment value.
        builder.SetInsertPoint(addInst);
        builder.CreateStore(
            addInst->getOperand(incOperand),
            accumAlloca
        );

        // Insert a LoadInst to load the accumulation increment value.
        Instruction *clonedAdd = (Instruction *) &*VMap[addInst];
        builder.SetInsertPoint(clonedAdd);
        result = builder.CreateLoad(
            accumAlloca,
            "accept_accumInc"
        );

        // Insert a LoadInst to load the accumulation variable.
        Value *result2 = builder.CreateLoad(
            storeInst->getPointerOperand(),
            "accept_accumVar"
        );

        // Change the operands of the BinaryOperator to be the two LoadInsts.
        clonedAdd->setOperand(0, result);
        clonedAdd->setOperand(1, result2);

        // Change the value operand of the cloned accumulation StoreInst to be
        // the cloned BinaryOperator.
        Instruction *clonedStore = (Instruction *) &*VMap[storeInst];
        clonedStore->setOperand(0, clonedAdd);
      }

      bodyBlock->getParent()->dump();
    }
  };
}

char LoopPerfPass::ID = 0;
LoopPass *llvm::createLoopPerfPass() { return new LoopPerfPass(); }

