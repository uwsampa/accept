#ifndef EXTENSIBLE_INTERPRETER_H
#define EXTENSIBLE_INTERPRETER_H

#include <vector>
#include "llvm/Support/InstVisitor.h"
#include "ExecutionEngine/Interpreter/Interpreter.h"

// This is a GIANT HACK that should NOT BE TRUSTED. I need access to the
// private fields of Interpreter, so here I'm redefining a similar-looking
// class and hoping that the compiler lays it out the same as Interpreter. Then
// I cast an Interpreter* to a PublicInterpreter* to access its
// otherwise-inaccessible fields.
//
// This will almost certainly break at some point.
class PublicInterpreter : public llvm::ExecutionEngine {
public:
  llvm::GenericValue ExitValue;
  llvm::DataLayout TD;
  llvm::IntrinsicLowering *IL;
  std::vector<llvm::ExecutionContext> ECStack;
  std::vector<llvm::Function*> AtExitHandlers;
};


// ExtensibleInterpreter is a facade around Interpreter that allows subclasses
// to override one important method, execute(). This method is responsible for
// interpreting a single instruction. Subclasses can override it to do whatever
// they like, including calling the superclass' version of execute() to make
// interpretation continue as usual.
class ExtensibleInterpreter : public llvm::ExecutionEngine,
                              public llvm::InstVisitor<ExtensibleInterpreter> {
public:
  llvm::Interpreter *interp;
  PublicInterpreter *pubInterp;  // GIANT HACK

  explicit ExtensibleInterpreter(llvm::Module *M);
  virtual ~ExtensibleInterpreter();

  // Interpreter execution loop.
  virtual void run();
  virtual void execute(llvm::Instruction &I);

  // Satisfy the ExecutionEngine interface.
  llvm::GenericValue runFunction(
      llvm::Function *F,
      const std::vector<llvm::GenericValue> &ArgValues
  );
  void *getPointerToNamedFunction(const std::string &Name,
                                  bool AbortOnFailure = true);
  void *recompileAndRelinkFunction(llvm::Function *F);
  void freeMachineCodeForFunction(llvm::Function *F);
  void *getPointerToFunction(llvm::Function *F);
  void *getPointerToBasicBlock(llvm::BasicBlock *BB);

};

#endif
