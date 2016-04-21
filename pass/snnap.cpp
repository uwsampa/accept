#include <sstream>
#include <string>
#include <iostream>

#include "llvm/IRBuilder.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Attributes.h"

#include "accept.h"

using namespace llvm;

namespace {

  std::string getTypeStr(Type* orig_type, Module* module) {
    if (orig_type == Type::getHalfTy(module->getContext()))
      return "Half";
    else if (orig_type == Type::getFloatTy(module->getContext()))
      return "Float";
    else if (orig_type == Type::getDoubleTy(module->getContext()))
      return "Double";
    else if (orig_type == Type::getInt1Ty(module->getContext()))
      return "Int1";
    else if (orig_type == Type::getInt8Ty(module->getContext()))
      return "Int8";
    else if (orig_type == Type::getInt16Ty(module->getContext()))
      return "Int16";
    else if (orig_type == Type::getInt32Ty(module->getContext()))
      return "Int32";
    else if (orig_type == Type::getInt64Ty(module->getContext()))
      return "Int64";
    else
      return "";
  }


  struct NPUInst : public FunctionPass {
    static char ID;
    ACCEPTPass *transformPass;
    Module *module;
    std::set<Function*> npu_fns;

    NPUInst();
    virtual const char *getPassName() const;
    virtual bool doInitialization(llvm::Module &M);
    virtual bool doFinalization(llvm::Module &M);
    virtual bool runOnFunction(llvm::Function &F);
    bool instrumentBasicBlocks(llvm::Function &F);
  };
}

NPUInst::NPUInst() : FunctionPass(ID) {
  initializeNPUInstPass(*PassRegistry::getPassRegistry());
}

const char *NPUInst::getPassName() const {
  return "Basic Block instrumentation";
}
bool NPUInst::doInitialization(Module &M) {
  module = &M;

  // ACCEPT shared transform pass
  transformPass = (ACCEPTPass*)sharedAcceptTransformPass;

  // Identify function annotated with "npu"
  GlobalVariable * global_annos = M.getNamedGlobal("llvm.global.annotations");
  if (global_annos) {
    ConstantArray * a = cast<ConstantArray>(global_annos->getOperand(0));
    for (int i=0; i<a->getNumOperands(); i++) {
      ConstantStruct * e = cast<ConstantStruct>(a->getOperand(i));

      if (Function * fn = dyn_cast<Function>(e->getOperand(0)->getOperand(0))) {
        StringRef anno = cast<ConstantDataArray>(cast<GlobalVariable>(e->getOperand(1)->getOperand(0))->getOperand(0))->getAsCString();
        if (anno == "npu") {
          npu_fns.insert(fn);
        }
      }
    }
  }

  // We'll insert the initialization call in main
  Function *Main = M.getFunction("main");
  assert(Main && "Error: npu-instrumentation requires a main function");

  // Initialization call, logbb_init() defined in run time
  LLVMContext &Ctx = Main->getContext();
  Constant *initFunc = Main->getParent()->getOrInsertFunction(
    "npu_init", Type::getVoidTy(Ctx), NULL
  );
  BasicBlock *bb = &Main->front();
  Instruction *op = &bb->front();
  IRBuilder<> builder(op);
  builder.SetInsertPoint(bb, builder.GetInsertPoint());
  builder.CreateCall(initFunc);

  return true; // modified IR
}

bool NPUInst::doFinalization(Module &M) {
  return false;
}

bool NPUInst::runOnFunction(Function &F) {
  bool modified = false;

  // Skip optimizing functions that seem to be in standard libraries.
  if (!transformPass->shouldSkipFunc(F)) {
    assert (!llvm::verifyFunction(F) && "Verification failed before code alteration");
    modified = instrumentBasicBlocks(F);
    assert (!llvm::verifyFunction(F) && "Verification failed after code alteration");
  }

  return modified;
}

bool NPUInst::instrumentBasicBlocks(Function & F){

  LLVMContext &Ctx = F.getContext();
  Type* voidty = Type::getVoidTy(Ctx);
  Type* int16ty = Type::getInt16Ty(Ctx);
  Type* int32ty = Type::getInt32Ty(Ctx);
  Type* int64ty = Type::getInt64Ty(Ctx);
  Type* stringty = Type::getInt8PtrTy(Ctx);
  Type* halfty = Type::getHalfTy(Ctx);
  Type* floatty = Type::getFloatTy(Ctx);
  Type* doublety = Type::getDoubleTy(Ctx);

  const std::string npu_injectFn_name = "lognpu";
  Function* npuLogFunc = module->getFunction(npu_injectFn_name);

  bool modified = false;

  for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {

    BasicBlock *bb = fi;

    for (BasicBlock::iterator bi = bb->begin(); bi != bb->end(); ++bi) {
      Instruction *inst = bi;
      Instruction *nextInst = next(bi, 1);

      if (isa<CallInst>(inst)) {
        CallInst* call_inst = dyn_cast<CallInst>(inst);
        if (npu_fns.find(call_inst->getCalledFunction()) != npu_fns.end() ) {
          std::string called_fn_name = call_inst->getCalledFunction()->getName().str();
          // outs() << F.getName() << " calls " << called_fn_name << " which is NPU-able!\n";

          // IR builder
          IRBuilder<> builder(module->getContext());
          builder.SetInsertPoint(nextInst);

          // Return Type Parameter
          Type* ret_type = call_inst->getType();
          std::string ret_type_str = getTypeStr(ret_type, module);
          if (ret_type_str == "") return false;
          Value* type_global_str = builder.CreateGlobalString(ret_type_str.c_str());
          Value* param_ret_type = builder.CreateBitCast(type_global_str,
              Type::getInt8PtrTy(module->getContext()));

          // Return Value Parameter
          if (ret_type == halfty)
            ret_type = int16ty;
          else if (ret_type == floatty)
            ret_type = int32ty;
          else if (ret_type == doublety)
            ret_type = int64ty;
          Value* ret_to_be_casted = builder.CreateBitCast(call_inst, ret_type);
          Value* param_ret = builder.CreateZExtOrBitCast(ret_to_be_casted, int64ty);

          // Arguments Paramters
          const int nargs = call_inst->getNumArgOperands();
          Value* param_args = ConstantInt::get(int32ty, nargs, false);

          // Instrumentation function
          SmallVector<Value *, 3> args;
          args.push_back(param_ret_type);
          args.push_back(param_ret);
          args.push_back(param_args);

          for (int i = 0; i < nargs; ++i)
            args.push_back(call_inst->getArgOperand(i));

          // Insert Function Call
          CallInst* call = builder.CreateCall(npuLogFunc, args);

          modified = true;

        }
      }
    }
  }

  return modified;
}

char NPUInst::ID = 0;
INITIALIZE_PASS_BEGIN(NPUInst, "npu instrumentation", "ACCEPT npu instrumentation pass", false, false)
INITIALIZE_PASS_DEPENDENCY(ApproxInfo)
INITIALIZE_PASS_END(NPUInst, "npu instrumentation", "ACCEPT npu instrumentation pass", false, false)
FunctionPass *llvm::createNPUInstPass() { return new NPUInst(); }
