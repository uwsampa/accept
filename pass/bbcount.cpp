#include "llvm/IRBuilder.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"

#include "accept.h"

using namespace llvm;

namespace {
  static unsigned bbIndex;
  static unsigned bbTotal;

  struct BBCount : public FunctionPass {
    static char ID;

    BBCount();
    virtual const char *getPassName() const;
    virtual bool doInitialization(llvm::Module &M);
    virtual bool doFinalization(llvm::Module &M);
    virtual bool runOnFunction(llvm::Function &F);
    bool instrumentBasicBlocks(llvm::Function &F);
  };
}

BBCount::BBCount() : FunctionPass(ID) {
  initializeBBCountPass(*PassRegistry::getPassRegistry());
}

const char *BBCount::getPassName() const {
  return "Basic Block instrumentation";
}
bool BBCount::doInitialization(Module &M) {
  // We'll insert the initialization call in main
  Function *Main = M.getFunction("main");
  assert(Main && "Error: count-bb requires a main function");

  // Initialization call, logbb_init() defined in run time
  LLVMContext &Ctx = Main->getContext();
  Constant *initFunc = Main->getParent()->getOrInsertFunction(
    "logbb_init", Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx), NULL
  );
  BasicBlock *bb = &Main->front();
  Instruction *op = &bb->front();
  IRBuilder<> builder(op);
  builder.SetInsertPoint(bb, builder.GetInsertPoint());

  // Initialize statics
  bbIndex = 0;
  bbTotal = 0;
  // Determine the number of basic blocks in the module
  for (Module::iterator mi = M.begin(); mi != M.end(); ++mi) {
    Function *F = mi;
    assert(F && "Error, function pointer is NULL");
    bbTotal += F->size();
  }

  Value* bbTotalVal = builder.getInt32(bbTotal);;
  Value* args[] = {bbTotalVal};
  builder.CreateCall(initFunc, args);

  return true; // modified IR
}

bool BBCount::doFinalization(Module &M) {
  return false;
}

bool BBCount::runOnFunction(Function &F) {
  assert (!llvm::verifyFunction(F) && "Verification failed before code alteration");
  bool modified = instrumentBasicBlocks(F);
  assert (!llvm::verifyFunction(F) && "Verification failed after code alteration");

  return modified;
}

bool BBCount::instrumentBasicBlocks(Function & F){
  const std::string injectFn_name = "logbb";
  LLVMContext &Ctx = F.getContext();
  Constant *logFunc = F.getParent()->getOrInsertFunction(
    injectFn_name, Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx), NULL
  );

  bool modified = false;

  // We don't want to instrument the instrumentation function
  if (F.getName().str() != injectFn_name) {

    for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {
      BasicBlock *bb = fi;

      // Insert logbb call before the BB terminator
      Instruction *op = bb->getTerminator();
      IRBuilder<> builder(op);
      builder.SetInsertPoint(bb, builder.GetInsertPoint());

      // Pass the BB id (global)
      assert(bbIndex<bbTotal && "bbIndex exceeds bbTotal");
      Value* bbIdxValue = builder.getInt32(bbIndex);
      Value* args[] = {bbIdxValue};
      builder.CreateCall(logFunc, args);

      modified = true;

      ++bbIndex;
    }

  }

  return modified;
}

char BBCount::ID = 0;
INITIALIZE_PASS_BEGIN(BBCount, "bb count", "ACCEPT basic block instrumentation pass", false, false)
INITIALIZE_PASS_DEPENDENCY(ApproxInfo)
INITIALIZE_PASS_END(BBCount, "bb count", "ACCEPT basic block instrumentation pass", false, false)
FunctionPass *llvm::createBBCountPass() { return new BBCount(); }
