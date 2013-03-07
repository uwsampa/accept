#define DEBUG_TYPE "enerc"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Instructions.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IRBuilder.h"

#include <sstream>
#include <set>
#include <cstdio>

#define ECQ_PRECISE 0
#define ECQ_APPROX 1

#define FUNC_TRACE "enerc_trace"

// I'm not sure why, but I can't seem to enable debug output in the usual way
// (with command line flags). Here's a hack.
#undef DEBUG
#define DEBUG(s) s

using namespace llvm;
namespace {

// Command-line flags.
cl::opt<bool> optInstrument ("accept-inst",
    cl::desc("ACCEPT: enable profiling instrumentation"));
cl::opt<bool> optRelax ("accept-relax",
    cl::desc("ACCEPT: enable relaxations"));


/**** HELPERS ****/

// Look at the qualifier metadata on the instruction and determine whether it
// has approximate semantics.
bool isApprox(Instruction *instr) {
  MDNode *md = instr->getMetadata("quals");
  if (!md)
    return false;

  Value *val = md->getOperand(0);
  ConstantInt *ci = cast<ConstantInt>(val);
  if (ci) {
    APInt intval = ci->getValue();
    return intval == ECQ_APPROX;
  } else {
    llvm::errs() << "INVALID METADATA";
    return false;
  }
}

// Is it legal to elide this instruction?
bool elidable_helper(Instruction *instr,
                     std::set<Instruction*> &seen) {
  // Check for cycles.
  if (seen.count(instr)) {
    llvm::errs() << "CYCLE DETECTED\n";
    return false;
  }
  seen.insert(instr);

  if (isApprox(instr)) {
    return true;
  } else if (isa<StoreInst>(instr) ||
             isa<ReturnInst>(instr) ||
             isa<BranchInst>(instr)) {
    // Precise stores, returns, branches: never elidable.
    return false;
  } else {
    // Recursive case: all consumers are elidable.
    for (Value::use_iterator ui = instr->use_begin();
          ui != instr->use_end(); ++ui) {
      Instruction *user = dyn_cast<Instruction>(*ui);
      if (user && !elidable_helper(user, seen)) {
        return false;
      }
    }
    return true; // No non-elidable consumers.
  }
}
// Start with an empty "seen" (cycle-detection) set.
bool elidable(Instruction *instr) {
  std::set<Instruction*> seen;
  return elidable_helper(instr, seen);
}

// Format a source position.
std::string srcPosDesc(const Module &mod, const DebugLoc &dl) {
  std::stringstream ss;
  ss << mod.getModuleIdentifier() << ":";
  ss << dl.getLine();
  if (dl.getCol())
    ss << "," << dl.getCol();
  return ss.str();
}


/**** THE MAIN LLVM PASS ****/

struct ACCEPTPass : public FunctionPass {
  static char ID;
  ACCEPTPass() : FunctionPass(ID) {
    gApproxInsts = 0;
    gElidableInsts = 0;
    gTotalInsts = 0;
    module = 0;
  }

  Constant *blockCountFunction;
  unsigned long gApproxInsts;
  unsigned long gElidableInsts;
  unsigned long gTotalInsts;
  Module *module;

  virtual void getAnalysisUsage(AnalysisUsage &Info) const {
    Info.addRequired<LoopInfo>();
  }

  virtual bool runOnFunction(Function &F) {
    countAndInstrument(F);
    return perforateLoops(F);
  }


  /**** TRACING AND STATISTICS ****/

  void countAndInstrument(Function &F) {
    for (Function::iterator bbi = F.begin(); bbi != F.end(); ++bbi) {
      unsigned int approxInsts = 0;
      unsigned int elidableInsts = 0;
      unsigned int totalInsts = 0;

      for (BasicBlock::iterator ii = bbi->begin(); ii != bbi->end();
            ++ii) {
        if ((Instruction*)ii == bbi->getTerminator()) {
          // Terminator instruction is not checked.
          continue;
        }

        // Record information about this instruction.
        bool iApprox = isApprox(ii);
        bool iElidable = elidable(ii);
        ++gTotalInsts;
        ++totalInsts;
        if (iApprox) {
          ++gApproxInsts;
          ++approxInsts;
        }
        if (iElidable) {
          ++gElidableInsts;
          ++elidableInsts;
        }
      }

      // Call the runtime profiling library for this block.
      if (optInstrument)
        insertTraceCall(F, bbi, approxInsts, elidableInsts, totalInsts);
    }
  }

  virtual bool doInitialization(Module &M) {
    module = &M;

    // Add instrumentation function.
    if (optInstrument)
      blockCountFunction = M.getOrInsertFunction(
        FUNC_TRACE,
        Type::getVoidTy(M.getContext()), // return type
        Type::getInt32Ty(M.getContext()), // approx
        Type::getInt32Ty(M.getContext()), // elidable
        Type::getInt32Ty(M.getContext()), // total
        NULL
      );

    return false;
  }

  virtual bool doFinalization(Module &M) {
    FILE *results_file = fopen("enerc_static.txt", "w+");
    fprintf(results_file,
            "%lu %lu %lu\n", gApproxInsts, gElidableInsts, gTotalInsts);
    fclose(results_file);
    return false;
  }

  void insertTraceCall(Function &F, BasicBlock* bb,
                       unsigned int approx,
                       unsigned int elidable,
                       unsigned int total) {
    std::vector<Value *> args;
    args.push_back(
        ConstantInt::get(Type::getInt32Ty(F.getContext()), approx)
    );
    args.push_back(
        ConstantInt::get(Type::getInt32Ty(F.getContext()), elidable)
    );
    args.push_back(
        ConstantInt::get(Type::getInt32Ty(F.getContext()), total)
    );

    // For now, we're inserting calls at the end of basic blocks.
    CallInst::Create(
      blockCountFunction,
      args,
      "",
      bb->getTerminator()
    );
  }


  /**** EXPERIMENT ****/

  bool perforateLoops(Function &F) {
    int perforatedLoops = 0;
    LoopInfo &loopInfo = getAnalysis<LoopInfo>();

    for (LoopInfo::iterator li = loopInfo.begin();
         li != loopInfo.end(); ++li) {
      Loop *loop = *li;

      // We only consider loops for which there is a header (condition), a
      // latch (increment, in "for"), and a preheader (initialization).
      if (!loop->getHeader() || !loop->getLoopLatch()
          || !loop->getLoopPreheader())
        continue;

      // Count elidable instructions in the loop.
      int cTotal = 0;
      int cElidable = 0;
      for (Loop::block_iterator bi = loop->block_begin();
           bi != loop->block_end(); ++bi) {
        BasicBlock *block = *bi;

        // Don't count the loop control.
        if (block == loop->getHeader() || block == loop->getLoopLatch())
          continue;

        for (BasicBlock::iterator ii = block->begin();
             ii != block->end(); ++ii) {
          if ((Instruction*)ii == block->getTerminator())
            continue;
          if (elidable(ii))
            ++cElidable;
          ++cTotal;
        }
      }

      // How elidable is the loop body as a whole?
      errs() << "loop at "
             << srcPosDesc(*module, loop->getHeader()->begin()->getDebugLoc())
             << " - " << cElidable << "/" << cTotal << "\n";

      /*
      #define DUMPNN(e) if (e) (e)->dump(); else errs() << "null\n";
      errs() << "header: ";
      DUMPNN(loop->getHeader())
      errs() << "preheader: ";
      DUMPNN(loop->getLoopPreheader())
      errs() << "predecessor: ";
      DUMPNN(loop->getLoopPredecessor())
      errs() << "latch: ";
      DUMPNN(loop->getLoopLatch())
      errs() << "exit: ";
      DUMPNN(loop->getExitBlock())
      errs() << "exiting: ";
      DUMPNN(loop->getExitingBlock())
      errs() << "all contained blocks: ";
      for (Loop::block_iterator bi = loop->block_begin();
           bi != loop->block_end(); ++bi) {
        BasicBlock *block = *bi;
        block->dump();
      }
      */

      if (cElidable == cTotal) {
        errs() << "can perforate.\n";
        if (optRelax) {
          perforateLoop(loop);
          ++perforatedLoops;
        }
      }
    }

    return perforatedLoops > 0;
  }

  void perforateLoop(Loop *loop) {
    // Check whether this loop is perforatable.
    // First, check for required blocks.
    if (!loop->getHeader() || !loop->getLoopLatch()
        || !loop->getLoopPreheader() || !loop->getExitBlock()) {
      llvm::errs() << "malformed loop\n";
      return;
    }
    // Next, make sure the header (condition block) ends with a body/exit
    // conditional branch.
    BranchInst *condBranch = dyn_cast<BranchInst>(
        loop->getHeader()->getTerminator()
    );
    if (!condBranch || condBranch->getNumSuccessors() != 2) {
      llvm::errs() << "malformed loop condition\n";
      return;
    }
    BasicBlock *bodyBlock;
    if (condBranch->getSuccessor(0) == loop->getExitBlock()) {
      bodyBlock = condBranch->getSuccessor(1);
    } else if (condBranch->getSuccessor(1) == loop->getExitBlock()) {
      bodyBlock = condBranch->getSuccessor(0);
    } else {
      llvm::errs() << "loop condition does not exit\n";
      return;
    }

    IRBuilder<> builder(module->getContext());
    Value *result;

    // Add our own counter to the preheader.
    builder.SetInsertPoint(loop->getLoopPreheader()->getTerminator());
    AllocaInst *counterAlloca = builder.CreateAlloca(
        builder.getInt32Ty(),
        0,
        "accept_counter"
    );
    builder.CreateStore(
        builder.getInt32(0),
        counterAlloca
    );

    // Increment the counter in the latch.
    builder.SetInsertPoint(loop->getLoopLatch()->getTerminator());
    result = builder.CreateLoad(
        counterAlloca,
        "something"
    );
    result = builder.CreateAdd(
        result,
        builder.getInt32(1),
        "anotherthing"
    );
    builder.CreateStore(
        result,
        counterAlloca
    );

    // Get the first body block.

    // Check the counter before the loop's body.
    BasicBlock *checkBlock = BasicBlock::Create(
        module->getContext(),
        "perforate",
        bodyBlock->getParent(),
        bodyBlock
    );
    builder.SetInsertPoint(checkBlock);
    result = builder.CreateLoad(
        counterAlloca,
        "thectr"
    );
    result = builder.CreateAnd(
        result,
        builder.getInt32(1),
        "ctrlowbit"
    );
    result = builder.CreateIsNull(
        result,
        "ctrtest"
    );
    result = builder.CreateCondBr(
        result,
        bodyBlock,
        loop->getLoopLatch()
    );

    // Change the latch (condition block) to point to our new condition
    // instead of the body.
    condBranch->setSuccessor(0, checkBlock);
  }

};
char ACCEPTPass::ID = 0;


/**** PASS REGISTRATION FOOTNOTE ***/

// Register ACCEPT as a "standard pass". This allows the pass to run without
// running opt explicitly (e.g., as part of running `clang`).
static void registerACCEPTPass(const PassManagerBuilder &,
                               PassManagerBase &PM) {
  PM.add(new LoopInfo());
  PM.add(new ACCEPTPass());
}
static RegisterStandardPasses
    RegisterACCEPT(PassManagerBuilder::EP_EarlyAsPossible,
                   registerACCEPTPass);

}
