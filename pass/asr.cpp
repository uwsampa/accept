#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/IRBuilder.h"
#include "llvm/ValueSymbolTable.h"

#include "accept.h"

using namespace llvm;

struct ApproxStrengthReduction : public FunctionPass {
  static char ID;
  ACCEPTPass *transformPass;
  ApproxInfo *AI;
  Module *module;

  ApproxStrengthReduction();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual const char *getPassName() const;
  virtual bool doInitialization(llvm::Module &M);
  virtual bool doFinalization(llvm::Module &M);
  virtual bool runOnFunction(llvm::Function &F);
};

void ApproxStrengthReduction::getAnalysisUsage(AnalysisUsage &AU) const {
  FunctionPass::getAnalysisUsage(AU);
  AU.addRequired<ApproxInfo>();
}

ApproxStrengthReduction::ApproxStrengthReduction() : FunctionPass(ID) {
  // initializeApproxStrengthReductionPass(*PassRegistry::getPassRegistry());
  module = 0;
}

const char *ApproxStrengthReduction::getPassName() const {
  return "ACCEPT tutorial example";
}

bool ApproxStrengthReduction::doInitialization(Module &M) {
  module = &M;
  transformPass = (ACCEPTPass*)sharedAcceptTransformPass;
  return false;
}

bool ApproxStrengthReduction::doFinalization(Module &M) {
  return false;
}

bool ApproxStrengthReduction::runOnFunction(Function &F) {
  AI = &getAnalysis<ApproxInfo>();

  // Skip optimizing functions that seem to be in standard libraries.
  if (transformPass->shouldSkipFunc(F))
    return false;

  errs() << "hi\n";

  return true;
}

char ApproxStrengthReduction::ID = 0;
FunctionPass *llvm::createASRPass() {
  return new ApproxStrengthReduction();
}
