#include "llvm/Analysis/Passes.h"
#include "llvm/PassRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Function.h"

namespace llvm {
  ImmutablePass *createAcceptAAPass();
  void initializeAcceptAAPass(PassRegistry &Registry);
  FunctionPass *createAcceptTransformPass();
}

// This class represents an analysis this determines whether functions and
// chunks are approximate. It is consumed by our various optimizations.
class ApproxInfo : public llvm::FunctionPass {
public:
  static char ID;
  ApproxInfo();
  virtual bool doInitialization(llvm::Module &M);
  virtual bool runOnFunction(llvm::Function &F);
  virtual bool doFinalization(llvm::Module &M);
};
