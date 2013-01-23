#ifndef EXTENSIBLE_INTERPRETER_H
#define EXTENSIBLE_INTERPRETER_H

#include <vector>
#include "llvm/Support/InstVisitor.h"
#include "ExecutionEngine/Interpreter/Interpreter.h"

// GIANT HACK
class PublicInterpreter : public llvm::ExecutionEngine,
                          public llvm::InstVisitor<llvm::Interpreter> {
public:
  llvm::GenericValue ExitValue;
  llvm::DataLayout TD;
  llvm::IntrinsicLowering *IL;
  std::vector<llvm::ExecutionContext> ECStack;
  std::vector<llvm::Function*> AtExitHandlers;
};


class ExtensibleInterpreter : public llvm::ExecutionEngine,
                              public llvm::InstVisitor<ExtensibleInterpreter> {
public:
  llvm::Interpreter *interp;
  PublicInterpreter *pubInterp;

  explicit ExtensibleInterpreter(llvm::Module *M);
  virtual ~ExtensibleInterpreter();

  // ExecutionEngine interface.
  virtual llvm::GenericValue runFunction(
      llvm::Function *F,
      const std::vector<llvm::GenericValue> &ArgValues
  );
  virtual void *getPointerToNamedFunction(const std::string &Name,
                                          bool AbortOnFailure = true) {
    return 0;
  }
  virtual void *recompileAndRelinkFunction(llvm::Function *F) {
    return getPointerToFunction(F);
  }
  virtual void freeMachineCodeForFunction(llvm::Function *F) { }
  virtual void *getPointerToFunction(llvm::Function *F) { return (void*)F; }
  virtual void *getPointerToBasicBlock(llvm::BasicBlock *BB) { return (void*)BB; }

  // Visitation entrance.
  virtual void run();

  // Instruction visitor methods.
  virtual void visitReturnInst(llvm::ReturnInst &I);
  virtual void visitBranchInst(llvm::BranchInst &I);
  virtual void visitSwitchInst(llvm::SwitchInst &I);
  virtual void visitIndirectBrInst(llvm::IndirectBrInst &I);
  virtual void visitBinaryOperator(llvm::BinaryOperator &I);
  virtual void visitICmpInst(llvm::ICmpInst &I);
  virtual void visitFCmpInst(llvm::FCmpInst &I);
  virtual void visitAllocaInst(llvm::AllocaInst &I);
  virtual void visitLoadInst(llvm::LoadInst &I);
  virtual void visitStoreInst(llvm::StoreInst &I);
  virtual void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
  virtual void visitPHINode(llvm::PHINode &PN);
  virtual void visitTruncInst(llvm::TruncInst &I);
  virtual void visitZExtInst(llvm::ZExtInst &I);
  virtual void visitSExtInst(llvm::SExtInst &I);
  virtual void visitFPTruncInst(llvm::FPTruncInst &I);
  virtual void visitFPExtInst(llvm::FPExtInst &I);
  virtual void visitUIToFPInst(llvm::UIToFPInst &I);
  virtual void visitSIToFPInst(llvm::SIToFPInst &I);
  virtual void visitFPToUIInst(llvm::FPToUIInst &I);
  virtual void visitFPToSIInst(llvm::FPToSIInst &I);
  virtual void visitPtrToIntInst(llvm::PtrToIntInst &I);
  virtual void visitIntToPtrInst(llvm::IntToPtrInst &I);
  virtual void visitBitCastInst(llvm::BitCastInst &I);
  virtual void visitSelectInst(llvm::SelectInst &I);
  virtual void visitCallSite(llvm::CallSite CS);
  virtual void visitUnreachableInst(llvm::UnreachableInst &I);
  virtual void visitShl(llvm::BinaryOperator &I);
  virtual void visitLShr(llvm::BinaryOperator &I);
  virtual void visitAShr(llvm::BinaryOperator &I);
  virtual void visitVAArgInst(llvm::VAArgInst &I);
  virtual void visitInstruction(llvm::Instruction &I);

  virtual void visitCallInst(llvm::CallInst &I) {
    visitCallSite (llvm::CallSite (&I));
  }
  virtual void visitInvokeInst(llvm::InvokeInst &I) {
    visitCallSite (llvm::CallSite (&I));
  }
};

#endif
