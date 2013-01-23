#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "ExtensibleInterpreter.h"

using namespace llvm;

namespace {
  cl::opt<std::string>
  InputFile(cl::desc("<input bitcode>"), cl::Positional, cl::init("-"));

  cl::list<std::string>
  InputArgv(cl::ConsumeAfter, cl::desc("<program arguments>..."));
}

class MyInterpreter : public ExtensibleInterpreter {
public:
  MyInterpreter(Module *M) : ExtensibleInterpreter(M) {};
  virtual void execute(llvm::Instruction &I) {
    I.dump();
    ExtensibleInterpreter::execute(I);
  }
};

int main(int argc, char **argv, char * const *envp) {
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  cl::ParseCommandLineOptions(argc, argv,
                              "EnerC execution tracer\n");

  return interpret<MyInterpreter>(InputFile, InputArgv, envp);
}
