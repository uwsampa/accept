#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/LoopPass.h"
#include "accept.h"

using namespace llvm;

namespace {
  // Code transformations.
  static void registerACCEPT(const PassManagerBuilder &,
                             PassManagerBase &PM) {
    PM.add(new LoopInfo());
    PM.add(new ApproxInfo());
    PM.add(createAcceptTransformPass());
  }
  static RegisterStandardPasses
      RegisterACCEPT(PassManagerBuilder::EP_EarlyAsPossible,
                     registerACCEPT);

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
