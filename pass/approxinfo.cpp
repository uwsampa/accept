#include "accept.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

ApproxInfo::ApproxInfo() : FunctionPass(ID) {}

bool ApproxInfo::doInitialization(Module &M) {
  return false;
}

bool ApproxInfo::runOnFunction(Function &F) {
  errs() << "running on function " << F.getName() << "\n";
  return false;
}

bool ApproxInfo::doFinalization(Module &M) {
  return false;
}

char ApproxInfo::ID = 0;
