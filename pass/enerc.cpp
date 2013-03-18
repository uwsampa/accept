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
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/IRBuilder.h"
#include "llvm/DebugInfo.h"
#include "llvm/IntrinsicInst.h"

#include <sstream>
#include <set>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#define ECQ_PRECISE 0
#define ECQ_APPROX 1

#define FUNC_TRACE "enerc_trace"
#define PERMIT "ACCEPT_PERMIT"

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
  std::string out;
  raw_string_ostream ss(out);

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
  return out;
}

// Describe an instruction.
std::string instDesc(const Module &mod, Instruction *inst) {
  std::string out;
  raw_string_ostream ss(out);
  ss << srcPosDesc(mod, inst->getDebugLoc()) << ": ";

  if (CallInst *call = dyn_cast<CallInst>(inst)) {
    Function *func = call->getCalledFunction();
    if (func) {
      StringRef name = func->getName();
      if (!name.empty() && name.front() == '_') {
        // C++ name. An extra leading underscore makes the name legible by
        // c++filt.
        ss << "call to _" << name;
      } else {
        ss << "call to " << name << "()";
      }
    }
    else
      ss << "indirect function call";
  } else if (StoreInst *store = dyn_cast<StoreInst>(inst)) {
    Value *ptr = store->getPointerOperand();
    StringRef name = ptr->getName();
    if (!name.empty() && name.front() == '_') {
      ss << "store to _" << ptr->getName().data();
    } else {
      ss << "store to " << ptr->getName().data();
    }
  } else {
    inst->print(ss);
  }

  return out;
}


/**** (NEW) ELIDABILITY ANALYSIS ***/

char const* _funcWhitelistArray[] = {
  // math.h
  "cos",
  "sin",
  "tan",
  "acos",
  "asin",
  "atan",
  "atan2",
  "cosh",
  "sinh",
  "tanh",
  "exp",
  "frexp",
  "ldexp",
  "log",
  "log10",
  "modf",
  "pow",
  "sqrt",
  "ceil",
  "fabs",
  "floor",
  "fmod",
};
const std::set<std::string> funcWhitelist(
  _funcWhitelistArray,
  _funcWhitelistArray + sizeof(_funcWhitelistArray) /
                        sizeof(_funcWhitelistArray[0])
);

struct ACCEPTAnalysis {
  std::map<Function*, bool> functionPurity;
  raw_fd_ostream *log;

  ACCEPTAnalysis(raw_fd_ostream *l) :
        log(l)
        {}

  // Conservatively check whether a store instruction can be observed by any
  // load instructions *other* than those in the specified set of instructions.
  bool storeEscapes(StoreInst *store, std::set<Instruction*> insts) {
    Value *ptr = store->getPointerOperand();

    // Make sure the pointer was created locally. That is, conservatively assume
    // that pointers coming from arguments or returned from other functions are
    // aliased somewhere else.
    if (!isa<AllocaInst>(ptr))
      return true;

    // Give up if the pointer is copied and leaves the function. This could be
    // smarter if it only looked *after* the store (flow-wise).
    if (PointerMayBeCaptured(ptr, true, true))
      return true;

    // Look for loads to the pointer not present in our exclusion set. Again, it
    // would be smarter to only look at the *successors* of the block in which
    // the store appears.
    Function *func = store->getParent()->getParent();
    for (Function::iterator bi = func->begin(); bi != func->end(); ++bi) {
      for (BasicBlock::iterator ii = bi->begin(); ii != bi->end(); ++ii) {
        if (LoadInst *load = dyn_cast<LoadInst>(ii)) {
          if (load->getPointerOperand() == ptr && !insts.count(load)) {
            return true;
          }
        }
      }
    }

    return false;
  }

  int preciseEscapeCheckHelper(std::map<Instruction*, bool> &flags,
                               const std::set<Instruction*> &insts) {
    int changes = 0;
    for (std::map<Instruction*, bool>::iterator i = flags.begin();
        i != flags.end(); ++i) {
      // Only consider currently-untainted instructions.
      if (i->second) {
        continue;
      }

      // Precise store: check whether it escapes.
      if (StoreInst *store = dyn_cast<StoreInst>(i->first)) {
        if (!storeEscapes(store, insts)) {
          i->second = true;
          ++changes;
        }
        continue;
      }

      // Calls must be to precise-pure functions.
      if (CallInst *call = dyn_cast<CallInst>(i->first)) {
        if (!isa<DbgInfoIntrinsic>(call)) {

          Function *func = call->getCalledFunction();
          if (func && isPrecisePure(func)) {
            // The call itself is precise-pure, but now we need to make sure
            // that the uses are also tainted.
            // Fall through to usage check.
          } else {
            continue;
          }

        }
      }

      bool allUsesTainted = true;
      for (Value::use_iterator ui = i->first->use_begin();
            ui != i->first->use_end(); ++ui) {
        Instruction *user = dyn_cast<Instruction>(*ui);
        if (user && !flags[user]) {
          allUsesTainted = false;
          break;
        }
      }

      if (allUsesTainted) {
        ++changes;
        i->second = true;
      }
    }
    return changes;
  }

  bool hasPermit(Instruction *inst) {
    DebugLoc dl = inst->getDebugLoc();
    DIScope scope(dl.getScope(inst->getContext()));
    if (!scope.Verify())
      return false;

    // Read line N of the file.
    std::ifstream srcFile(scope.getFilename().data());
    int lineno = 1;
    std::string theLine;
    while (srcFile.good()) {
      std::string curLine;
      getline(srcFile, curLine);
      if (lineno == dl.getLine()) {
        theLine = curLine;
        break;
      } else {
        ++lineno;
      }
    }
    srcFile.close();

    return theLine.find(PERMIT) != std::string::npos;
  }

  bool approxOrLocal(std::set<Instruction*> &insts, Instruction *inst) {
    if (hasPermit(inst)) {
      return true;
    } else if (CallInst *call = dyn_cast<CallInst>(inst)) {

      if (isa<DbgInfoIntrinsic>(call)) {
        return true;
      }

      Function *func = call->getCalledFunction();
      if (func && isPrecisePure(func)) {
        if (isApprox(inst)) {
          return true;
        }
        // Otherwise, fall through and check usages for escape.
      } else {
        return false;
      }

    } else if (isApprox(inst)) {
      return true;
    } else if (isa<StoreInst>(inst) ||
               isa<ReturnInst>(inst) ||
               isa<BranchInst>(inst)) {
      return false;  // Never approximate.
    }

    for (Value::use_iterator ui = inst->use_begin();
          ui != inst->use_end(); ++ui) {
      Instruction *user = dyn_cast<Instruction>(*ui);
      if (user && insts.count(user) == 0) {
        return false;  // Escapes.
      }
    }
    return true;  // Does not escape.
  }

  std::set<Instruction*> preciseEscapeCheck(std::set<Instruction*> insts) {
    std::map<Instruction*, bool> flags;

    // Mark all approx and non-escaping instructions.
    for (std::set<Instruction*>::iterator i = insts.begin();
         i != insts.end(); ++i) {
      flags[*i] = approxOrLocal(insts, *i);
    }

    // Iterate to a fixed point.
    while (preciseEscapeCheckHelper(flags, insts)) {}

    // Construct a set of untainted instructions.
    std::set<Instruction*> untainted;
    for (std::map<Instruction*, bool>::iterator i = flags.begin();
        i != flags.end(); ++i) {
      if (!i->second)
        untainted.insert(i->first);
    }
    return untainted;
  }

  std::set<Instruction*> preciseEscapeCheck(std::set<BasicBlock*> blocks) {
    std::set<Instruction*> insts;
    for (std::set<BasicBlock*>::iterator bi = blocks.begin();
         bi != blocks.end(); ++bi) {
      for (BasicBlock::iterator ii = (*bi)->begin();
           ii != (*bi)->end(); ++ii) {
        insts.insert(ii);
      }
    }
    return preciseEscapeCheck(insts);
  }

  // Determine whether a function can only affect approximate memory (i.e., no
  // precise stores escape).
  bool isPrecisePure(Function *func) {
    // Check for cached result.
    if (functionPurity.count(func)) {
      return functionPurity[func];
    }

    *log << " - checking function " << func->getName() << "\n";

    // LLVM's own nominal purity analysis.
    if (func->onlyReadsMemory()) {
      *log << " - only reads memory\n";
      functionPurity[func] = true;
      return true;
    }

    // Whitelisted pure functions from standard libraries.
    if (func->empty() || funcWhitelist.count(func->getName())) {
        *log << " - whitelisted\n";
        functionPurity[func] = true;
        return true;
    }

    // Empty functions (those for which we don't have a definition) are
    // conservatively marked non-pure.
    if (func->empty()) {
      *log << " - definition not available\n";
      functionPurity[func] = false;
      return false;
    }

    // Begin by marking the function as non-pure. This avoids an infinite loop
    // for recursive function calls (but is, of course, conservative).
    functionPurity[func] = false;

    std::set<BasicBlock*> blocks;
    for (Function::iterator bi = func->begin(); bi != func->end(); ++bi) {
      blocks.insert(bi);
    }
    std::set<Instruction*> blockers = preciseEscapeCheck(blocks);

    *log << " - blockers: " << blockers.size() << "\n";
    for (std::set<Instruction*>::iterator i = blockers.begin();
         i != blockers.end(); ++i) {
      *log << " - * " << instDesc(*(func->getParent()), *i) << "\n";
    }
    if (blockers.empty()) {
      *log << " - precise-pure function: " << func->getName() << "\n";
    }

    functionPurity[func] = blockers.empty();
    return blockers.empty();
  }
};


/**** THE MAIN LLVM PASS ****/

struct ACCEPTPass : public FunctionPass {
  static char ID;

  Constant *blockCountFunction;
  unsigned long gApproxInsts;
  unsigned long gElidableInsts;
  unsigned long gTotalInsts;
  Module *module;
  std::map<int, int> relaxConfig;  // ident -> param
  std::map<int, std::string> configDesc;  // ident -> description
  int opportunityId;
  raw_fd_ostream *log;
  ACCEPTAnalysis *analysis;

  ACCEPTPass() : FunctionPass(ID) {
    gApproxInsts = 0;
    gElidableInsts = 0;
    gTotalInsts = 0;
    module = 0;
    opportunityId = 0;
    log = 0;
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

    std::string error;
    log = new raw_fd_ostream("accept_log.txt", error);

    analysis = new ACCEPTAnalysis(log);

    return false;
  }

  virtual bool doFinalization(Module &M) {
    dumpStaticStats();
    if (!optRelax)
      dumpRelaxConfig();
    log->close();
    delete log;
    delete analysis;
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

  IntegerType *getNativeIntegerType() {
    DataLayout layout(module->getDataLayout());
    return Type::getIntNTy(module->getContext(),
                           layout.getPointerSizeInBits());
  }

  void setUpInstrumentation() {
    LLVMContext &C = module->getContext();
    IntegerType *nativeInt = getNativeIntegerType();
    blockCountFunction = module->getOrInsertFunction(
      FUNC_TRACE,
      Type::getVoidTy(C), // return type
      nativeInt, // approx
      nativeInt, // elidable
      nativeInt, // total
      NULL
    );
  }

  void insertTraceCall(Function &F, BasicBlock* bb,
                       unsigned int approx,
                       unsigned int elidable,
                       unsigned int total) {
    std::vector<Value *> args;
    IntegerType *nativeInt = getNativeIntegerType();
    args.push_back(ConstantInt::get(nativeInt, approx));
    args.push_back(ConstantInt::get(nativeInt, elidable));
    args.push_back(ConstantInt::get(nativeInt, total));

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
    std::ofstream configFile("accept_config.txt", std::ios_base::app);
    for (std::map<int, int>::iterator i = relaxConfig.begin();
         i != relaxConfig.end(); ++i) {
      configFile << module->getModuleIdentifier() << " "
                 << i->first << " "
                 << i->second << "\n";
    }
    configFile.close();

    std::ofstream descFile("accept_config_desc.txt", std::ios_base::app);
    for (std::map<int, std::string>::iterator i = configDesc.begin();
         i != configDesc.end(); ++i) {
      descFile << module->getModuleIdentifier() << " "
               << i->first << " "
               << i->second << "\n";
    }
    descFile.close();
  }

  void loadRelaxConfig() {
    std::ifstream configFile("accept_config.txt");;
    if (!configFile.good()) {
      errs() << "no config file; no optimizations will occur\n";
      return;
    }

    while (configFile.good()) {
      std::string modname;
      int ident;
      int param;
      configFile >> modname >> ident >> param;
      if (modname == module->getModuleIdentifier())
        relaxConfig[ident] = param;
    }

    configFile.close();
  }


  /**** EXPERIMENT ****/

  // Attempt to optimize the loops in a function.
  bool optimizeLoops(Function &F) {
    int perforatedLoops = 0;
    LoopInfo &loopInfo = getAnalysis<LoopInfo>();

    for (LoopInfo::iterator li = loopInfo.begin();
         li != loopInfo.end(); ++li) {
      Loop *loop = *li;
      optimizeLoopsHelper(loop, perforatedLoops);
    }
    return perforatedLoops > 0;
  }
  void optimizeLoopsHelper(Loop *loop, int &perforatedLoops) {
    std::vector<Loop*> subLoops = loop->getSubLoops();
    for (int i = 0; i < subLoops.size(); ++i)
      // Recurse into subloops.
      optimizeLoopsHelper(subLoops[i], perforatedLoops);
    if (tryToOptimizeLoop(loop, opportunityId))
      ++perforatedLoops;
    ++opportunityId;;
  }

  // Assess whether a loop can be optimized and, if so, log some messages and
  // update the configuration map. If optimization is turned on, the
  // configuration map will be used to actually transform the loop. Returns a
  // boolean indicating whether the code was changed (i.e., the loop
  // perforated).
  bool tryToOptimizeLoop(Loop *loop, int id) {
    bool transformed = false;

    std::stringstream ss;
    ss << "loop at "
       << srcPosDesc(*module, loop->getHeader()->begin()->getDebugLoc());
    std::string loopName = ss.str();

    *log << "---\n" << loopName << "\n";

    // We only consider loops for which there is a header (condition), a
    // latch (increment, in "for"), and a preheader (initialization).
    if (!loop->getHeader() || !loop->getLoopLatch()
        || !loop->getLoopPreheader()) {
      *log << "loop not in perforatable form\n";
      return false;
    }

    // Check whether the body of this loop is elidable (precise-pure).
    std::set<BasicBlock*> loopBlocks;
    for (Loop::block_iterator bi = loop->block_begin();
         bi != loop->block_end(); ++bi) {
      if (*bi == loop->getHeader() || *bi == loop->getLoopLatch())
        // Don't count the loop control.
        continue;
      loopBlocks.insert(*bi);
    }
    std::set<Instruction*> blockers = analysis->preciseEscapeCheck(loopBlocks);
    *log << "blockers: " << blockers.size() << "\n";
    for (std::set<Instruction*>::iterator i = blockers.begin();
         i != blockers.end(); ++i) {
      *log << " * " << instDesc(*module, *i) << "\n";
    }

    if (!blockers.size()) {
      *log << "can perforate loop " << id << "\n";
      if (optRelax) {
        int param = relaxConfig[id];
        if (param) {
          *log << "perforating with factor 2^" << param << "\n";
          perforateLoop(loop, param);
          transformed = true;
        }
      } else {
        relaxConfig[id] = 0;
        configDesc[id] = loopName;
      }
    }

    return transformed;
  }

  // Transform a loop to skip iterations.
  // The loop should already be validated as perforatable, but checks will be
  // performed nonetheless to ensure safety.
  void perforateLoop(Loop *loop, int logfactor=1) {
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
