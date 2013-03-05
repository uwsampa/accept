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
    findElidableBlocks(F);
    return false;
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

  void findElidableBlocks(Function &F) {
    LoopInfo &loopInfo = getAnalysis<LoopInfo>();
    loopInfo.runOnFunction(F);

    for (LoopInfo::iterator li = loopInfo.begin();
         li != loopInfo.end(); ++li) {
      Loop *loop = *li;

      // Count elidable instructions in the loop.
      int cTotal = 0;
      int cElidable = 0;
      for (Loop::block_iterator bi = loop->block_begin();
           bi != loop->block_end(); ++bi) {
        BasicBlock *block = *bi;
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

    }
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
