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
#include "llvm/DebugInfo.h"

#include <sstream>
#include <set>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

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
  LLVMContext &ctx = mod.getContext();
  std::stringstream ss;

  // Try to get filename from debug location.
  DIScope scope(dl.getScope(ctx));
  if (scope.Verify()) {
    ss << scope.getFilename().data();
  } else {
    // Fall back to the compilation unit name.
    ss << "(" << mod.getModuleIdentifier() << ")";
  }
  ss << ":";

  ss << dl.getLine();
  if (dl.getCol())
    ss << "," << dl.getCol();
  return ss.str();
}


/**** THE MAIN LLVM PASS ****/

struct ACCEPTPass : public FunctionPass {
  static char ID;

  Constant *blockCountFunction;
  unsigned long gApproxInsts;
  unsigned long gElidableInsts;
  unsigned long gTotalInsts;
  Module *module;
  std::map<std::pair<int, int>, int> relaxConfig;  // kind, num -> param

  ACCEPTPass() : FunctionPass(ID) {
    gApproxInsts = 0;
    gElidableInsts = 0;
    gTotalInsts = 0;
    module = 0;
  }

  virtual void getAnalysisUsage(AnalysisUsage &Info) const {
    Info.addRequired<LoopInfo>();
  }

  virtual bool runOnFunction(Function &F) {
    countAndInstrument(F);
    return optimizeLoops(F);
  }

  virtual bool doInitialization(Module &M) {
    module = &M;

    if (optInstrument)
      setUpInstrumentation();

    if (optRelax)
      loadRelaxConfig();

    return false;
  }

  virtual bool doFinalization(Module &M) {
    dumpStaticStats();
    if (!optRelax)
      dumpRelaxConfig();
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

  void setUpInstrumentation() {
    blockCountFunction = module->getOrInsertFunction(
      FUNC_TRACE,
      Type::getVoidTy(module->getContext()), // return type
      Type::getInt32Ty(module->getContext()), // approx
      Type::getInt32Ty(module->getContext()), // elidable
      Type::getInt32Ty(module->getContext()), // total
      NULL
    );
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

  void dumpStaticStats() {
    FILE *results_file = fopen("enerc_static.txt", "a");
    fprintf(results_file,
            "%lu %lu %lu\n", gApproxInsts, gElidableInsts, gTotalInsts);
    fclose(results_file);
  }


  /**** RELAXATION CONFIGURATION ***/

  typedef enum {
    rkPerforate
  } relaxKind;

  void dumpRelaxConfig() {
    std::ofstream configFile("accept_config.txt", std::ios_base::app);;
    for (std::map<std::pair<int, int>, int>::iterator i = relaxConfig.begin();
         i != relaxConfig.end(); ++i) {
      configFile << i->first.first << " "
                 << i->first.second << " "
                 << i->second << "\n";
    }
    configFile.close();
  }

  void loadRelaxConfig() {
    std::ifstream configFile("accept_config.txt");;
    if (!configFile.good()) {
      errs() << "no config file; no optimizations will occur\n";
      return;
    }

    while (configFile.good()) {
      int kind;
      int ident;
      int param;
      configFile >> kind >> ident >> param;
      relaxConfig[std::pair<int, int>(kind, ident)] = param;
    }

    configFile.close();
  }


  /**** EXPERIMENT ****/

  // Attempt to optimize the loops in a function.
  bool optimizeLoops(Function &F) {
    int perforatedLoops = 0;
    int loopId = 0;
    LoopInfo &loopInfo = getAnalysis<LoopInfo>();

    for (LoopInfo::iterator li = loopInfo.begin();
         li != loopInfo.end(); ++li, ++loopId) {
      Loop *loop = *li;
      optimizeLoopsHelper(loop, loopId, perforatedLoops);
    }
    return perforatedLoops > 0;
  }
  void optimizeLoopsHelper(Loop *loop, int &loopId, int &perforatedLoops) {
    std::vector<Loop*> subLoops = loop->getSubLoops();
    for (int i = 0; i < subLoops.size(); ++i)
      // Recurse into subloops.
      optimizeLoopsHelper(subLoops[i], loopId, perforatedLoops);
    if (tryToOptimizeLoop(loop, loopId))
      ++perforatedLoops;
    ++loopId;
  }

  // Assess whether a loop can be optimized and, if so, log some messages and
  // update the configuration map. If optimization is turned on, the
  // configuration map will be used to actually transform the loop.
  bool tryToOptimizeLoop(Loop *loop, int &loopId) {
    int perforatedLoops = 0;

    // We only consider loops for which there is a header (condition), a
    // latch (increment, in "for"), and a preheader (initialization).
    if (!loop->getHeader() || !loop->getLoopLatch()
        || !loop->getLoopPreheader())
      return false;
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

    if (cElidable == cTotal) {
      errs() << "can perforate loop " << loopId << "\n";
      if (optRelax) {
        int param = relaxConfig[std::pair<int, int>(rkPerforate, loopId)];
        if (param) {
          errs() << "perforating with factor 2^" << param << "\n";
          perforateLoop(loop, param);
          ++perforatedLoops;
        }
      } else {
        relaxConfig[std::pair<int, int>(rkPerforate, loopId)] = 0;
      }
    }

    return perforatedLoops > 0;
  }

  // Transform a loop to skip iterations.
  // The loop should already be validated as perforatable, but checks will be
  // performed nonetheless to ensure safety.
  void perforateLoop(Loop *loop, int logfactor=1) {
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
        "accept_tmp"
    );
    result = builder.CreateAdd(
        result,
        builder.getInt32(1),
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
