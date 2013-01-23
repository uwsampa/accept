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
    visit(I);
  }
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

#define visit_meth(name, argt) \
  void ExtensibleInterpreter :: name (argt &I) { \
    interp->name(I); \
  }

visit_meth(visitReturnInst, ReturnInst)
visit_meth(visitBranchInst, BranchInst)
visit_meth(visitSwitchInst, SwitchInst)
visit_meth(visitIndirectBrInst, IndirectBrInst)
visit_meth(visitBinaryOperator, BinaryOperator)
visit_meth(visitICmpInst, ICmpInst)
visit_meth(visitFCmpInst, FCmpInst)
visit_meth(visitAllocaInst, AllocaInst)
visit_meth(visitLoadInst, LoadInst)
visit_meth(visitStoreInst, StoreInst)
visit_meth(visitGetElementPtrInst, GetElementPtrInst)
visit_meth(visitPHINode, PHINode)
visit_meth(visitTruncInst, TruncInst)
visit_meth(visitZExtInst, ZExtInst)
visit_meth(visitSExtInst, SExtInst)
visit_meth(visitFPTruncInst, FPTruncInst)
visit_meth(visitFPExtInst, FPExtInst)
visit_meth(visitUIToFPInst, UIToFPInst)
visit_meth(visitSIToFPInst, SIToFPInst)
visit_meth(visitFPToUIInst, FPToUIInst)
visit_meth(visitFPToSIInst, FPToSIInst)
visit_meth(visitPtrToIntInst, PtrToIntInst)
visit_meth(visitIntToPtrInst, IntToPtrInst)
visit_meth(visitBitCastInst, BitCastInst)
visit_meth(visitSelectInst, SelectInst)
visit_meth(visitUnreachableInst, UnreachableInst)
visit_meth(visitShl, BinaryOperator)
visit_meth(visitLShr, BinaryOperator)
visit_meth(visitAShr, BinaryOperator)
visit_meth(visitVAArgInst, VAArgInst)
visit_meth(visitInstruction, Instruction)

void ExtensibleInterpreter::visitCallSite(CallSite CS) {
  interp->Interpreter::visitCallSite(CS);
}
