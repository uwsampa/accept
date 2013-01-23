#include "ExtensibleInterpreter.h"

using namespace llvm;

ExtensibleInterpreter::ExtensibleInterpreter(Module *M) :
      ExecutionEngine(M)
{
  interp = new Interpreter(M);
  pubInterp = (PublicInterpreter *)interp;  // GIANT HACK

  // Data layout is used by ExecutionEngine::runFunctionAsMain().
  setDataLayout(interp->getDataLayout());
}

ExtensibleInterpreter::~ExtensibleInterpreter() {
  delete interp;
}

void ExtensibleInterpreter::run() {
  while (!pubInterp->ECStack.empty()) {
    ExecutionContext &SF = pubInterp->ECStack.back();
    Instruction &I = *SF.CurInst++;
    execute(I);
  }
}

void ExtensibleInterpreter::execute(Instruction &I) {
  interp->visit(I);
}

GenericValue ExtensibleInterpreter::runFunction(
    Function *F,
    const std::vector<GenericValue> &ArgValues
) {
  assert (F && "Function *F was null at entry to run()");
  std::vector<GenericValue> ActualArgs;
  const unsigned ArgCount = F->getFunctionType()->getNumParams();
  for (unsigned i = 0; i < ArgCount; ++i)
    ActualArgs.push_back(ArgValues[i]);
  interp->callFunction(F, ActualArgs);
  run();
  return pubInterp->ExitValue;
}
