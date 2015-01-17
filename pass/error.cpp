#include <iostream>
#include <sstream>

#include "accept.h"
#include "liberror.h"

using namespace llvm;

//cl::opt<bool> optInjectError ("inject-error",
//    cl::desc("ACCEPT: enable error injection"));

struct ErrorInjection : public FunctionPass {
  static char ID;
  ACCEPTPass *transformPass;
  ApproxInfo *AI;
  Module *module;

  ErrorInjection();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual const char *getPassName() const;
  virtual bool doInitialization(llvm::Module &M);
  virtual bool doFinalization(llvm::Module &M);
  virtual bool runOnFunction(llvm::Function &F);

  bool injectError(Function &F);

  bool instructionErrorInjection(Function& F);
  bool injectErrorInst(Instruction* inst);
};

void ErrorInjection::getAnalysisUsage(AnalysisUsage &AU) const {
  FunctionPass::getAnalysisUsage(AU);
  AU.addRequired<ApproxInfo>();
}

ErrorInjection::ErrorInjection() : FunctionPass(ID) {
  initializeErrorInjectionPass(*PassRegistry::getPassRegistry());
  module = 0;
  //relax = optInjectError;
  // Assume this has been done by transform.cpp
  //if (relax)
  //  loadRelaxConfig();
}

const char *ErrorInjection::getPassName() const {
  return "ACCEPT error injection pass";
}

bool ErrorInjection::doInitialization(Module &M) {
  module = &M;
  transformPass = (ACCEPTPass*)sharedAcceptTransformPass;
  return false;
}

bool ErrorInjection::doFinalization(Module &M) {
  return false;
}

bool ErrorInjection::runOnFunction(Function &F) {
  AI = &getAnalysis<ApproxInfo>();

  // Skip optimizing functions that seem to be in standard libraries.
  if (transformPass->shouldSkipFunc(F))
    return false;

  bool modified = instructionErrorInjection(F);
  return modified;
}

bool ErrorInjection::instructionErrorInjection(Function& F) {
  bool modified = false;
  for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {
    BasicBlock *bb = fi;
    for (BasicBlock::iterator bi = bb->begin(); bi != bb->end(); ++bi) {
      Instruction *inst = bi;
      if (injectErrorInst(inst)) modified = true;
    }
  }
  return modified;
}

bool ErrorInjection::injectErrorInst(Instruction* inst) {
  std::stringstream ss;
  ss << "instruction " << inst->getOpcodeName() << " at " <<
      srcPosDesc(*module, inst->getDebugLoc());
  std::string instName = ss.str();

  LogDescription *desc = AI->logAdd("Instruction", inst);
  ACCEPT_LOG << instName << "\n";

  bool approx = isApprox(inst);

  if (transformPass->relax) { // we're injecting error
    int param = transformPass->relaxConfig[instName];
    if (param) {
      ACCEPT_LOG << "injecting error " << param << "\n";
      // param tells which error injection will be done e.g. bit flipping
      InjectError::injectInst(inst, param);
      return true;
    } else {
      ACCEPT_LOG << "not injecting error\n";
      return false;
    }
  } else { // we're just logging
    if (approx) {
      ACCEPT_LOG << "can inject error\n";
      transformPass->relaxConfig[instName] = 0;
    } else {
      ACCEPT_LOG << "cannot inject error\n";
    }
  }
  return false;
}

char ErrorInjection::ID = 0;
INITIALIZE_PASS_BEGIN(ErrorInjection, "error", "ACCEPT error injection pass", false, false)
INITIALIZE_PASS_DEPENDENCY(ApproxInfo)
INITIALIZE_PASS_END(ErrorInjection, "error", "ACCEPT error injection pass", false, false)
FunctionPass *llvm::createErrorInjectionPass() { return new ErrorInjection(); }
