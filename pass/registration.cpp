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
  bool acceptEnableInjection;
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

  // Error injection is also optional (it is for simulation experiments).
  cl::opt<bool, true> optEnableInjection("accept-inject",
      cl::desc("ACCEPT: instrument for error injection"),
      cl::location(acceptEnableInjection));

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
    if (acceptEnableInjection)
      PM.add(createErrorInjectionPass());
    PM.add(createLoopPerfPass());
    if (acceptEnableNPU)
      PM.add(createLoopNPUPass());
    // NPU instrumentation
    PM.add(createNPUInstPass());
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
