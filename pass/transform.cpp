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
#include "llvm/DebugInfo.h"
#include "llvm/DataLayout.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"

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
  log = 0;

  relax = optRelax;

  if (relax)
    loadRelaxConfig();
}

void ACCEPTPass::getAnalysisUsage(AnalysisUsage &Info) const {
  Info.addRequired<LoopInfo>();
  Info.addRequired<DominatorTree>();
  Info.addRequired<PostDominatorTree>();
  Info.addRequired<ApproxInfo>();
  if (acceptUseProfile)
    Info.addRequired<ProfileInfo>();
}

bool ACCEPTPass::shouldSkipFunc(Function &F) {
  // ACCEPT internal functions.
  if (F.getName().startswith("accept_")) {
    return true;
  }

  // LLVM intrinsics.
  if (F.isIntrinsic()) {
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

  return optimizeSync(F);
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

  collectFuncDebug(M);

  return false;
}

bool ACCEPTPass::doFinalization(Module &M) {
  if (!relax)
    dumpRelaxConfig();
  return false;
}

const char *ACCEPTPass::getPassName() const {
  return "ACCEPT relaxation";
}


/**** RELAXATION CONFIGURATION ***/

void ACCEPTPass::dumpRelaxConfig() {
  std::ofstream configFile("accept_config.txt", std::ios_base::out);
  for (std::map<std::string, int>::iterator i = relaxConfig.begin();
        i != relaxConfig.end(); ++i) {
    configFile << i->second << " "
               << i->first << "\n";
  }
  configFile.close();
}

void ACCEPTPass::loadRelaxConfig() {
  std::ifstream configFile("accept_config.txt");;
  if (!configFile.good()) {
    errs() << "no config file; no optimizations will occur\n";
    return;
  }

  while (configFile.good()) {
    std::string ident;
    int param;
    configFile >> param;
    if (!configFile.good())
      break;
    configFile.ignore(); // Skip space.
    getline(configFile, ident);
    relaxConfig[ident] = param;
  }

  configFile.close();
}

char ACCEPTPass::ID = 0;

FunctionPass *llvm::sharedAcceptTransformPass = NULL;
FunctionPass *llvm::createAcceptTransformPass() {
  sharedAcceptTransformPass = new ACCEPTPass();
  return sharedAcceptTransformPass;
}
