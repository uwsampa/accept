#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/IRReader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include <cerrno>
#include "ExecutionEngine/Interpreter/Interpreter.h"

using namespace llvm;

namespace {
  cl::opt<std::string>
  InputFile(cl::desc("<input bitcode>"), cl::Positional, cl::init("-"));

  cl::list<std::string>
  InputArgv(cl::ConsumeAfter, cl::desc("<program arguments>..."));
}

int main(int argc, char **argv, char * const *envp) {
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  LLVMContext &Context = getGlobalContext();

  cl::ParseCommandLineOptions(argc, argv,
                              "EnerC execution tracer\n");

  // Load the bitcode.
  SMDiagnostic Err;
  Module *Mod = ParseIRFile(InputFile, Err, Context);
  if (!Mod) {
    Err.print(argv[0], errs());
    return 1;
  }

  // Load the whole bitcode file eagerly.
  std::string ErrorMsg;
  if (Mod->MaterializeAllPermanently(&ErrorMsg)) {
    errs() << argv[0] << ": bitcode didn't read correctly.\n";
    errs() << "Reason: " << ErrorMsg << "\n";
    exit(1);
  }

  // Create the interpreter.
  ExecutionEngine *EE = Interpreter::create(Mod, &ErrorMsg);
  if (!ErrorMsg.empty()) {
    errs() << argv[0] << ": error creating EE: " << ErrorMsg << "\n";
    exit(1);
  }

  // Remove ".bc" suffix from input bitcode name and use it as argv[0].
  if (StringRef(InputFile).endswith(".bc"))
    InputFile.erase(InputFile.length() - 3);
  InputArgv.insert(InputArgv.begin(), InputFile);

  // Get the main function from the module.
  Function *EntryFn = Mod->getFunction("main");
  if (!EntryFn) {
    errs() << '\'' << "main" << "\' function not found in module.\n";
    return -1;
  }

  // Reset errno to zero on entry to main.
  errno = 0;

  // Run main.
  return EE->runFunctionAsMain(EntryFn, InputArgv, envp);
}
