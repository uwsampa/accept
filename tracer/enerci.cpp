#define DEBUG_TYPE "lli"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/IRReader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/Debug.h"
#include <cerrno>

// Hacky! This is not included in the LLVM installed headers. I should probably
// change the build files to make this explicit path unnecessary.
#include "../llvm/lib/ExecutionEngine/Interpreter/Interpreter.h"

using namespace llvm;

namespace {
  cl::opt<std::string>
  InputFile(cl::desc("<input bitcode>"), cl::Positional, cl::init("-"));

  cl::list<std::string>
  InputArgv(cl::ConsumeAfter, cl::desc("<program arguments>..."));
}

//===----------------------------------------------------------------------===//
// main Driver function
//
int main(int argc, char **argv, char * const *envp) {
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  LLVMContext &Context = getGlobalContext();

  cl::ParseCommandLineOptions(argc, argv,
                              "EnerC execution tracer\n");

  // Load the bitcode...
  SMDiagnostic Err;
  Module *Mod = ParseIRFile(InputFile, Err, Context);
  if (!Mod) {
    Err.print(argv[0], errs());
    return 1;
  }

  // If not jitting lazily, load the whole bitcode file eagerly too.
  std::string ErrorMsg;
  if (Mod->MaterializeAllPermanently(&ErrorMsg)) {
    errs() << argv[0] << ": bitcode didn't read correctly.\n";
    errs() << "Reason: " << ErrorMsg << "\n";
    exit(1);
  }

  ExecutionEngine *EE = 0;
  EE = Interpreter::create(Mod, &ErrorMsg);
  if (!ErrorMsg.empty()) {
    errs() << argv[0] << ": error creating EE: " << ErrorMsg << "\n";
    exit(1);
  }

  // Otherwise, if there is a .bc suffix on the executable strip it off, it
  // might confuse the program.
  if (StringRef(InputFile).endswith(".bc"))
    InputFile.erase(InputFile.length() - 3);

  // Add the module's name to the start of the vector of arguments to main().
  InputArgv.insert(InputArgv.begin(), InputFile);

  // Call the main function from M as if its signature were:
  //   int main (int argc, char **argv, const char **envp)
  // using the contents of Args to determine argc & argv, and the contents of
  // EnvVars to determine envp.
  //
  Function *EntryFn = Mod->getFunction("main");
  if (!EntryFn) {
    errs() << '\'' << "main" << "\' function not found in module.\n";
    return -1;
  }

  // If the program doesn't explicitly call exit, we will need the Exit
  // function later on to make an explicit call, so get the function now.
  Constant *Exit = Mod->getOrInsertFunction("exit", Type::getVoidTy(Context),
                                                    Type::getInt32Ty(Context),
                                                    NULL);

  // Reset errno to zero on entry to main.
  errno = 0;

  int Result;
  // Trigger compilation separately so code regions that need to be 
  // invalidated will be known.
  (void)EE->getPointerToFunction(EntryFn);

  // Run main.
  Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);

  return Result;
}
