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

#include "accept.h"

using namespace llvm;

// Command-line flags.
cl::opt<bool> optRelax ("accept-relax",
    cl::desc("ACCEPT: enable relaxations"));

ACCEPTPass::ACCEPTPass() : FunctionPass(ID) {
  module = 0;
  opportunityId = 0;
  log = 0;
}

void ACCEPTPass::getAnalysisUsage(AnalysisUsage &Info) const {
  Info.addRequired<LoopInfo>();
  Info.addRequired<ApproxInfo>();
  if (acceptUseProfile)
    Info.addRequired<ProfileInfo>();
}

bool ACCEPTPass::shouldSkipFunc(Function &F) {
  if (F.getName().startswith("accept_")) {
    return true;
  }

  // If we're missing debug info for the function, give up.
  if (!funcDebugInfo.count(&F)) {
    return false;
  }

  DISubprogram funcInfo = funcDebugInfo[&F];
  StringRef filename = funcInfo.getFilename();
  if (filename.startswith("/usr/include/") ||
      filename.startswith("/usr/lib/"))
    return false; // true;
  if (AI->markerAtLine(filename, funcInfo.getLineNumber()) == markerForbid) {
    return true;
  }
  return false;
}

bool ACCEPTPass::runOnFunction(Function &F) {
  AI = &getAnalysis<ApproxInfo>();
  log = AI->log;

  // Skip optimizing functions that seem to be in standard libraries.
  if (shouldSkipFunc(F))
    return false;

  return optimizeLoops(F) || optimizeSync(F);
}

// Find the debug info for every "subprogram" (i.e., function).
// Inspired by DebugInfoFinder::procuessModule, but actually examines
// multiple compilation units.
void ACCEPTPass::collectFuncDebug(Module &M) {
  NamedMDNode *cuNodes = module->getNamedMetadata("llvm.dbg.cu");
  if (!cuNodes) {
    errs() << "ACCEPT: debug information not available\n";
    return;
  }

  for (unsigned i = 0; i != cuNodes->getNumOperands(); ++i) {
    DICompileUnit cu(cuNodes->getOperand(i));

    DIArray spa = cu.getSubprograms();
    for (unsigned j = 0; j != spa.getNumElements(); ++j) {
      collectSubprogram(DISubprogram(spa.getElement(j)));
    }

    DIArray tya = cu.getRetainedTypes();
    for (unsigned j = 0; j != tya.getNumElements(); ++j) {
      collectType(DIType(spa.getElement(i)));
    }
  }
}
void ACCEPTPass::collectSubprogram(DISubprogram sp) {
  if (Function *func = sp.getFunction()) {
    funcDebugInfo[func] = sp;
  }
}
void ACCEPTPass::collectType(DIType ty) {
  if (ty.isCompositeType()) {
    DICompositeType cty(ty);
    collectType(cty.getTypeDerivedFrom());
    DIArray da = cty.getTypeArray();
    for (unsigned i = 0; i != da.getNumElements(); ++i) {
      DIDescriptor d = da.getElement(i);
      if (d.isType())
        collectType(DIType(d));
      else if (d.isSubprogram())
        collectSubprogram(DISubprogram(d));
    }
  }
}

bool ACCEPTPass::doInitialization(Module &M) {
  module = &M;

  if (optRelax)
    loadRelaxConfig();

  collectFuncDebug(M);

  return false;
}

bool ACCEPTPass::doFinalization(Module &M) {
  if (!optRelax)
    dumpRelaxConfig();
  return false;
}

IntegerType *ACCEPTPass::getNativeIntegerType() {
  DataLayout layout(module->getDataLayout());
  return Type::getIntNTy(module->getContext(),
                          layout.getPointerSizeInBits());
}

const char *ACCEPTPass::getPassName() const {
  return "ACCEPT relaxation";
}


/**** RELAXATION CONFIGURATION ***/

void ACCEPTPass::dumpRelaxConfig() {
  std::ofstream configFile("accept_config.txt", std::ios_base::out);
  for (std::map<int, int>::iterator i = relaxConfig.begin();
        i != relaxConfig.end(); ++i) {
    configFile << i->first << " "
                << i->second << "\n";
  }
  configFile.close();

  std::ofstream descFile("accept_config_desc.txt", std::ios_base::out);
  for (std::map<int, std::string>::iterator i = configDesc.begin();
        i != configDesc.end(); ++i) {
    descFile << i->first << " "
              << i->second << "\n";
  }
  descFile.close();
}

void ACCEPTPass::loadRelaxConfig() {
  std::ifstream configFile("accept_config.txt");;
  if (!configFile.good()) {
    errs() << "no config file; no optimizations will occur\n";
    return;
  }

  while (configFile.good()) {
    int ident;
    int param;
    configFile >> ident >> param;
    relaxConfig[ident] = param;
  }

  configFile.close();
}


/**** LOOP PERFORATION ****/

// Attempt to optimize the loops in a function.
bool ACCEPTPass::optimizeLoops(Function &F) {
  int perforatedLoops = 0;
  LoopInfo &loopInfo = getAnalysis<LoopInfo>();

  for (LoopInfo::iterator li = loopInfo.begin();
        li != loopInfo.end(); ++li) {
    Loop *loop = *li;
    optimizeLoopsHelper(loop, perforatedLoops);
  }
  return perforatedLoops > 0;
}
void ACCEPTPass::optimizeLoopsHelper(Loop *loop, int &perforatedLoops) {
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
bool ACCEPTPass::tryToOptimizeLoop(Loop *loop, int id) {
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

  // Skip array constructor loops manufactured by Clang.
  if (loop->getHeader()->getName().startswith("arrayctor.loop")) {
    *log << "array constructor\n";
    return false;
  }

  // Skip empty-body loops (where perforation would do nothing anyway).
  if (loop->getNumBlocks() == 2
      && loop->getHeader() != loop->getLoopLatch()) {
    BasicBlock *latch = loop->getLoopLatch();
    if (&(latch->front()) == &(latch->back())) {
      *log << "empty body\n";
      return false;
    }
  }

  /*
  if (acceptUseProfile) {
    ProfileInfo &PI = getAnalysis<ProfileInfo>();
    double trips = PI.getExecutionCount(loop->getHeader());
    *log << "trips: " << trips << "\n";
  }
  */

  // Determine whether this is a for-like or while-like loop. This informs
  // the heuristic that determines which parts of the loop to perforate.
  bool isForLike = false;
  if (loop->getHeader()->getName().startswith("for.cond")) {
    *log << "for-like loop\n";
    isForLike = true;
  } else {
    *log << "while-like loop\n";
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

  // Check for control flow in the loop body. We don't perforate anything
  // with a break, continue, return, etc.
  for (std::set<BasicBlock*>::iterator i = bodyBlocks.begin();
        i != bodyBlocks.end(); ++i) {
    if (loop->isLoopExiting(*i)) {
      *log << "contains loop exit\n";
      return false;
    }
  }

  // Check whether the body of this loop is elidable (precise-pure).
  std::set<Instruction*> blockers = AI->preciseEscapeCheck(bodyBlocks);
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
        perforateLoop(loop, param, isForLike);
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
void ACCEPTPass::perforateLoop(Loop *loop, int logfactor, bool isForLike) {
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
}


/**** SYNCHRONIZATION ELISION ****/

bool ACCEPTPass::optimizeSync(Function &F) {
  return false;
}

char ACCEPTPass::ID = 0;

FunctionPass *llvm::createAcceptTransformPass() {
  return new ACCEPTPass();
}
