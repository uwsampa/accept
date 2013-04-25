#include "llvm/Analysis/Passes.h"

namespace llvm {
  ImmutablePass *createAcceptAAPass();
  void initializeAcceptAAPass(PassRegistry &Registry);
  FunctionPass *createAcceptTransformPass();
}
