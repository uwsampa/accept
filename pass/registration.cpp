#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "accept.h"

using namespace llvm;

namespace llvm {
  bool acceptUseProfile;
}

namespace {
  cl::opt<bool, true> optProf("accept-prof",
      cl::desc("ACCEPT: use profiling information"),
      cl::location(acceptUseProfile));

  // Code transformations.
  static void registerACCEPT(const PassManagerBuilder &,
                             PassManagerBase &PM) {
    PM.add(new LoopInfo());
    PM.add(new DominatorTree());
    PM.add(new PostDominatorTree());
    /*
    if (acceptUseProfile)
      PM.add(createProfileLoaderPass());
    */
    PM.add(new ApproxInfo());
    PM.add(createAcceptTransformPass());
    PM.add(createLoopPerfPass());
  }
  static RegisterStandardPasses
      RegisterACCEPT(PassManagerBuilder::EP_EarlyAsPossible,
                     registerACCEPT);

  // Alias analysis.
  /*
  static void registerAA(const PassManagerBuilder &, PassManagerBase &PM) {
    PM.add(createAcceptAAPass());
  }
  // This extension point adds to the module pass manager.
  static RegisterStandardPasses
      RM(PassManagerBuilder::EP_ModuleOptimizerEarly,
         registerAA);
  */
}
