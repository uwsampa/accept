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
  bool acceptEnableNPU;
}

namespace {
  cl::opt<bool, true> optProf("accept-prof",
      cl::desc("ACCEPT: use profiling information"),
      cl::location(acceptUseProfile));

  // The NPU pass is enabled explicitly because it is platform-specific. x86
  // and WISP benchmarks cannot use this optimization.
  cl::opt<bool, true> optEnableNPU("accept-npu",
      cl::desc("ACCEPT: use NPU transformation"),
      cl::location(acceptEnableNPU));

  // Code transformations.
  static void registerEarlyACCEPT(const PassManagerBuilder &,
                             PassManagerBase &PM) {
    PM.add(new LoopInfo());
    PM.add(new DominatorTree());
    PM.add(new PostDominatorTree());
    /*
    if (acceptUseProfile)
      PM.add(createProfileLoaderPass());
    */
    // PM.add(createBBCountPass());
    // PM.add(createErrorInjectionPass());
    // PM.add(createLoopPerfPass());
    // if (acceptEnableNPU)
    //   PM.add(createLoopNPUPass());
  }
  static void registerLateACCEPT(const PassManagerBuilder &,
                             PassManagerBase &PM) {
    PM.add(new ApproxInfo());
    PM.add(createAcceptTransformPass());
    PM.add(createErrorInjectionPass());
    PM.add(createInstrumentBBPass());
  }
  static RegisterStandardPasses
      RegisterEarlyACCEPT(PassManagerBuilder::EP_EarlyAsPossible ,
                     registerEarlyACCEPT);

  static RegisterStandardPasses
      RegisterLateACCEPT(PassManagerBuilder::EP_OptimizerLast,
                     registerLateACCEPT);


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
