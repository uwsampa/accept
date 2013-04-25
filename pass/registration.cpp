#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "accept.h"

using namespace llvm;

namespace {
  // Alias analysis.
  static void registerAA(const PassManagerBuilder &, PassManagerBase &PM) {
    PM.add(createAcceptAAPass());
  }
  // This extension point adds to the module pass manager.
  static RegisterStandardPasses
      RM(PassManagerBuilder::EP_ModuleOptimizerEarly,
         registerAA);
  // And this one adds to the function pass manager.
  static RegisterStandardPasses
      RF(PassManagerBuilder::EP_EarlyAsPossible,
         registerAA);
}
