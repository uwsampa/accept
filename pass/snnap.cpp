#include <sstream>
#include <string>
#include <iostream>

#include "llvm/IRBuilder.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Attributes.h"
#include "llvm/IntrinsicInst.h"

#include "accept.h"

using namespace llvm;

namespace {

  int getTypeEnum(Type* orig_type, Module* module) {
    if (orig_type == Type::getVoidTy(module->getContext()))
      return 0;
    else if (orig_type == Type::getHalfTy(module->getContext()))
      return 1;
    else if (orig_type == Type::getFloatTy(module->getContext()))
      return 2;
    else if (orig_type == Type::getDoubleTy(module->getContext()))
      return 3;
    else if (orig_type == Type::getInt1Ty(module->getContext()))
      return 4;
    else if (orig_type == Type::getInt8Ty(module->getContext()))
      return 5;
    else if (orig_type == Type::getInt16Ty(module->getContext()))
      return 6;
    else if (orig_type == Type::getInt32Ty(module->getContext()))
      return 7;
    else if (orig_type == Type::getInt64Ty(module->getContext()))
      return 8;
    else if (orig_type == Type::getHalfPtrTy(module->getContext()))
      return 9;
    else if (orig_type == Type::getFloatPtrTy(module->getContext()))
      return 10;
    else if (orig_type == Type::getDoublePtrTy(module->getContext()))
      return 11;
    else if (orig_type == Type::getInt1PtrTy(module->getContext()))
      return 12;
    else if (orig_type == Type::getInt8PtrTy(module->getContext()))
      return 13;
    else if (orig_type == Type::getInt16PtrTy(module->getContext()))
      return 14;
    else if (orig_type == Type::getInt32PtrTy(module->getContext()))
      return 15;
    else if (orig_type == Type::getInt64PtrTy(module->getContext()))
      return 16;
    else
      return -1;
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
    bool instrumentFunction(llvm::Function &F);
  };

  void replaceAllUsesWithExcept(Instruction* oldInst, Value* newInst, std::vector<Value*>& except) {
    std::vector<User*> users;
    for (Value::use_iterator ui = oldInst->use_begin();
        ui != oldInst->use_end(); ++ui)
      users.push_back(*ui);

    for (int i = 0; i < users.size(); ++i) {
      User* user = users[i];

      bool shouldSkip = false;
      for (int j = 0; j < except.size(); ++j) {
        if (except[j] == cast<Instruction>(user)) {
          shouldSkip = true;
          break;
        }
      }
      if (shouldSkip) continue;

      if (Constant *C = dyn_cast<Constant>(user)) {
        if (!isa<GlobalValue>(C)) {
          int j;
          for (j = 0; j < user->getNumOperands(); ++j)
            if (user->getOperand(j) == oldInst) break;

          C->replaceUsesOfWithOnConstant(oldInst, newInst,
              &(user->getOperandUse(j)));
          continue;
        }
      }

      user->replaceUsesOfWith(oldInst, newInst);
    }

    if (BasicBlock *BB = dyn_cast<BasicBlock>(oldInst))
      BB->replaceSuccessorsPhiUsesWith(cast<BasicBlock>(newInst));
  }

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

  // Initialization call, log_init()/npu_init() defined in run time
  // log_init initializes the logging process (during run_orig)
  // npu_init initializes the npu (during run_opt)
  std::string npu_injectFn_name;
  if (transformPass->relax) {
    npu_injectFn_name = "npu_init";
  } else {
    npu_injectFn_name = "log_init";
  }
  LLVMContext &Ctx = Main->getContext();
  Constant *initFunc = Main->getParent()->getOrInsertFunction(
    npu_injectFn_name, Type::getVoidTy(Ctx), NULL
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
    modified = instrumentFunction(F);
    assert (!llvm::verifyFunction(F) && "Verification failed after code alteration");
  }

  return modified;
}

bool NPUInst::instrumentFunction(Function & F){

  LLVMContext &Ctx = F.getContext();
  Type* voidty = Type::getVoidTy(Ctx);
  Type* int16ty = Type::getInt16Ty(Ctx);
  Type* int32ty = Type::getInt32Ty(Ctx);
  Type* int64ty = Type::getInt64Ty(Ctx);
  Type* stringty = Type::getInt8PtrTy(Ctx);
  Type* halfty = Type::getHalfTy(Ctx);
  Type* floatty = Type::getFloatTy(Ctx);
  Type* doublety = Type::getDoubleTy(Ctx);

  bool modified = false;
  std::vector<Instruction *> remList;

  for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {

    BasicBlock *bb = fi;

    for (BasicBlock::iterator bi = bb->begin(); bi != bb->end(); ++bi) {
      Instruction *inst = bi;
      Instruction *nextInst = next(bi, 1);

      if (isa<CallInst>(inst)) {
        CallInst* call_inst = dyn_cast<CallInst>(inst);
        Function *callee = call_inst->getCalledFunction();
        if (callee) {
          std::string called_fn_name = callee->getName().str();
          // if (npu_fns.find(callee) != npu_fns.end() ) {
          if (!isa<IntrinsicInst>(inst) && !transformPass->shouldSkipFunc(*callee)) {
            if (transformPass->AI->isPrecisePure(const_cast<Function*>(callee))
              && !transformPass->AI->isWhitelistedPure(called_fn_name)) {

              // outs() << F.getName() << " calls " << called_fn_name << " which is NPU-able\n";

              // Function to inject
              bool approx = isApprox(inst);
              std::string npu_injectFn_name;
              if (transformPass->relax) {
                npu_injectFn_name = "invokenpu";
              } else {
                npu_injectFn_name = "lognpu";
              }
              Function* npuLogFunc = module->getFunction(npu_injectFn_name);

              // IR builder
              IRBuilder<> builder(module->getContext());
              builder.SetInsertPoint(nextInst);

              // // Return Type Parameter
              // Type* ret_type = call_inst->getType();
              // std::string ret_type_str = getTypeStr(ret_type, module);
              // if (ret_type_str == "") return false;
              // Value* ret_type_global_str = builder.CreateGlobalString(ret_type_str.c_str());
              // Value* param_ret_type = builder.CreateBitCast(ret_type_global_str,
              //     Type::getInt8PtrTy(module->getContext()));

              // // Return Value Parameter
              // Type* int_ret_type = ret_type;
              // if (ret_type == halfty)
              //   int_ret_type = int16ty;
              // else if (ret_type == floatty)
              //   int_ret_type = int32ty;
              // else if (ret_type == doublety)
              //   int_ret_type = int64ty;
              // Value* ret_to_be_casted = builder.CreateBitCast(call_inst, int_ret_type);
              // Value* param_ret_value = builder.CreateZExtOrBitCast(ret_to_be_casted, int64ty);

              // Arguments Paramters
              const int nargs = call_inst->getNumArgOperands();
              Value* param_args = ConstantInt::get(int32ty, nargs, false);

              // Derive the number of inputs and outputs
              int ninputs = 0;
              int noutputs = 0;
              // Input and Output vectors
              SmallVector<Value *, 0> input_args;
              SmallVector<Value *, 0> output_args;
              for (int i = 0; i < nargs; ++i) {
                // Get Argument Type
                // If pointer, it's an output else it's an input
                Type* arg_type = call_inst->getArgOperand(i)->getType();
                const int typeEnum = getTypeEnum(arg_type, module);
                if (typeEnum>8) {
                  noutputs ++;
                  output_args.push_back(call_inst->getArgOperand(i));
                } else if (typeEnum>0) {
                  ninputs ++;
                  input_args.push_back(call_inst->getArgOperand(i));
                }
              }
              Value* ninput_args = ConstantInt::get(int32ty, ninputs, false);
              Value* noutput_args = ConstantInt::get(int32ty, noutputs, false);

              // Instrumentation function
              SmallVector<Value *, 0> args;
              // args.push_back(param_ret_type);
              // args.push_back(param_ret_value);
              args.push_back(ninput_args);
              args.push_back(noutput_args);
              args.push_back(param_args);

              for (int i = 0; i < ninputs; ++i) {
                args.push_back(input_args[i]);
              }
              for (int i = 0; i < noutputs; ++i) {
                args.push_back(output_args[i]);
              }

              // Insert Function Call
              CallInst* call = builder.CreateCall(npuLogFunc, args);

              // if (transformPass->relax) {
              //   // Use the return value to replace all instances
              //   Value* final_result;
              //   if (int_ret_type != int64ty) {
              //     Value* trunc = builder.CreateTrunc(call, int_ret_type);
              //     final_result = builder.CreateBitCast(trunc, ret_type);
              //   } else {
              //     final_result = builder.CreateTruncOrBitCast(call, ret_type);
              //   }

              //   std::vector<Value*> except;
              //   if (ret_to_be_casted != inst)
              //     except.push_back(ret_to_be_casted);
              //   else
              //     except.push_back(param_ret_value);
              //   except.push_back(call);
              //   replaceAllUsesWithExcept(inst, final_result, except);
              // }

              // Remove from parent
              if (transformPass->relax) {
                remList.push_back(inst);
              }

              modified = true;
            }
          }
        }
      }
    }
  }

  // Remove the function calls
  for (std::vector<Instruction *>::iterator it = remList.begin() ; it != remList.end(); ++it) {
    Instruction *inst = *it;
    inst->eraseFromParent();
  }

  return modified;
}

char NPUInst::ID = 0;
INITIALIZE_PASS_BEGIN(NPUInst, "npu instrumentation", "ACCEPT npu instrumentation pass", false, false)
INITIALIZE_PASS_DEPENDENCY(ApproxInfo)
INITIALIZE_PASS_END(NPUInst, "npu instrumentation", "ACCEPT npu instrumentation pass", false, false)
FunctionPass *llvm::createNPUInstPass() { return new NPUInst(); }
