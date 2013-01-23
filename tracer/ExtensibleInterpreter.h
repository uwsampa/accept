#ifndef EXTENSIBLE_INTERPRETER_H
#define EXTENSIBLE_INTERPRETER_H

#include <vector>
#include "llvm/Support/InstVisitor.h"
#include "ExecutionEngine/Interpreter/Interpreter.h"

// GIANT HACK
class PublicInterpreter : public llvm::ExecutionEngine {
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

  // Instruction execution.
  virtual void execute(llvm::Instruction &I);
};

#endif
