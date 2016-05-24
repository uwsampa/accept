#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/IRBuilder.h"
#include "llvm/ValueSymbolTable.h"
#include "llvm/Metadata.h"
#include "llvm/DebugInfo.h"

#include "accept.h"

// #define ALU_ONLY

using namespace llvm;

//cl::opt<bool> optInjectError ("inject-error",
//    cl::desc("ACCEPT: enable error injection"));

namespace {
  // Global bbIndex
  static unsigned bbIndex;

  typedef struct {
    Instruction* inst;
    int bb_index;
    int i_index;
  } InstId;

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

  void replaceAllUsesWithExcept(Instruction* oldInst, Value* newInst,
      std::vector<Value*>& except) {
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

  std::string get_mangled_fn_name(Module* m, std::string unmangled_fn_name) {
    const Module::FunctionListType& fn_list = m->getFunctionList();
    for (Module::FunctionListType::const_iterator fi = fn_list.begin();
        fi != fn_list.end(); ++fi) {
      const Function* fn = fi;
      std::string fn_name = fn->getName().str();
      if (fn_name.find(unmangled_fn_name) != std::string::npos) return fn_name;
    }
    return "Error: did not find the error injection function";
  }
}

struct ErrorInjection : public FunctionPass {
  static char ID;
  ACCEPTPass *transformPass;
  ApproxInfo *AI;
  Module *module;

  ErrorInjection();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual const char *getPassName() const;
  virtual bool doInitialization(llvm::Module &M);
  virtual bool doFinalization(llvm::Module &M);
  virtual bool runOnFunction(llvm::Function &F);

  bool injectError(Function &F);

  bool instructionErrorInjection(Function& F);
  bool injectErrorInst(InstId iid, Instruction* nextInst, Function* injectFn);
  bool injectErrorRegion(InstId iid);
  bool injectRegionHooks(Instruction* inst, int param);
  bool injectHooks(Instruction* inst, Instruction* nextInst, int param,
      Function* injectFn);
  bool injectHooksBinOp(Instruction* inst, Instruction* nextInst, int param,
      Function* injectFn);
  bool injectHooksCast(Instruction* inst, Instruction* nextInst, int param,
      Function* injectFn);
  bool injectHooksStore(Instruction* inst, Instruction* nextInst, int param,
      Function* injectFn);
  bool injectHooksLoad(Instruction* inst, Instruction* nextInst, int param,
      Function* injectFn);
  bool injectHooksCall(Instruction* inst, Instruction* nextInst, int param,
      Function* injectFn);
  Type* intify(Type* dst_type);
};

void ErrorInjection::getAnalysisUsage(AnalysisUsage &AU) const {
  FunctionPass::getAnalysisUsage(AU);
  AU.addRequired<ApproxInfo>();
}

ErrorInjection::ErrorInjection() : FunctionPass(ID) {
  initializeErrorInjectionPass(*PassRegistry::getPassRegistry());
  module = 0;
}

const char *ErrorInjection::getPassName() const {
  return "ACCEPT error injection pass";
}

bool ErrorInjection::doInitialization(Module &M) {
  module = &M;
  transformPass = (ACCEPTPass*)sharedAcceptTransformPass;
  bbIndex = 0;
  return false;
}

bool ErrorInjection::doFinalization(Module &M) {
  return false;
}

bool ErrorInjection::runOnFunction(Function &F) {
  AI = &getAnalysis<ApproxInfo>();

  bool modified = false;

  // Skip optimizing functions that seem to be in standard libraries.
  if (!transformPass->shouldSkipFunc(F)) {
      modified = instructionErrorInjection(F);
  }

  return modified;
}

bool ErrorInjection::instructionErrorInjection(Function& F) {
  const std::string injectFn_unmangled_name = "injectInst";
  const std::string injectFn_mangled_name =
      get_mangled_fn_name(module, injectFn_unmangled_name);
  Function* injectFn = module->getFunction(injectFn_mangled_name);

  IRBuilder<> builder(module->getContext());

  bool modified = false;
  std::vector<InstId> all_insts;

  for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {
    BasicBlock *bb = fi;
    int icounter = 0;
    for (BasicBlock::iterator bi = bb->begin(); bi != bb->end(); ++bi) {
      Instruction *inst = bi;
      InstId iid;
      iid.inst = inst; iid.bb_index = bbIndex; iid.i_index = icounter;
      all_insts.push_back(iid);
      // Write metadata (instruction id & label)
      LLVMContext& C = inst->getContext();
      std::stringstream mss;
      mss << "bb" << bbIndex << "i" << icounter;
      Value* vals[] = {
        MDString::get(C, mss.str()),
        builder.getInt32(bbIndex),
        builder.getInt32(icounter)
      };
      MDNode* N1 = MDNode::get(C, vals);
      inst->setMetadata("iid", N1);
      MDNode* N2 = MDNode::get(C, MDString::get(C, "instruction label"));
      inst->setMetadata(mss.str(), N2);
      // Also add the source file line number for convenience
      MDNode* N3 = inst->getMetadata("dbg");
      if (N3) {
        DILocation Loc(N3);
        unsigned line = Loc.getLineNumber();
        unsigned col = Loc.getColumnNumber();
        StringRef fn = Loc.getFilename();
        std::stringstream locstring;
        locstring << fn.str() << ":" << line << ":" << col;
        Value* vals[] = {
          MDString::get(C, mss.str()),
          MDString::get(C, locstring.str())
        };
        MDNode* N4 = MDNode::get(C, vals);
        inst->setMetadata("iloc", N4);
      }
      ++icounter;
    }
    ++bbIndex;
  }

  if ((transformPass->relax && transformPass->shouldInjectError(F)) || !transformPass->relax) {
    // When injecting error during the relax phase, make sure F is in the white list
    int n_insts = all_insts.size();
    for (int i = 0; i < n_insts; ++i) {
      Instruction* nextInst = ((i == n_insts - 1) ? NULL : all_insts[i + 1].inst);
      if (injectErrorInst(all_insts[i], nextInst, injectFn)) modified = true;
    }
  }

  return modified;
}

bool ErrorInjection::injectHooksLoad(Instruction* inst, Instruction* nextInst,
    int param, Function* injectFn) {
  LoadInst* load_inst = dyn_cast<LoadInst>(inst);

  Type* orig_type = inst->getType();
  std::string orig_type_str = getTypeStr(orig_type, module);
  if (orig_type_str == "") return false;

  IRBuilder<> builder(module->getContext());
  builder.SetInsertPoint(nextInst);

  Type* int64ty = Type::getInt64Ty(module->getContext());
  Value* param_addr = builder.CreatePtrToInt(load_inst->getPointerOperand(),
      int64ty);
  Value* param_align = builder.CreateZExtOrBitCast(
      ConstantInt::get(int64ty, load_inst->getAlignment(), false), int64ty);

  Type* dst_type = NULL;
  if (orig_type_str == "Half")
    dst_type = Type::getInt16Ty(module->getContext());
  else if (orig_type_str == "Float")
    dst_type = Type::getInt32Ty(module->getContext());
  else if (orig_type_str == "Double")
    dst_type = Type::getInt64Ty(module->getContext());

  Value* ret_to_be_casted = inst;
  if (dst_type) ret_to_be_casted = builder.CreateBitCast(inst, dst_type);
  Value* param_ret = builder.CreateZExtOrBitCast(ret_to_be_casted, int64ty);

  Value* opcode_global_str = builder.CreateGlobalString(inst->getOpcodeName());
  Value* param_opcode = builder.CreateBitCast(opcode_global_str,
      Type::getInt8PtrTy(module->getContext()));
  Value* type_global_str = builder.CreateGlobalString(orig_type_str.c_str());
  Value* param_orig_type = builder.CreateBitCast(type_global_str,
      Type::getInt8PtrTy(module->getContext()));

  Value* param_knob = ConstantInt::get(int64ty, param, false);

  SmallVector<Value *, 6> Args;
  Args.push_back(param_opcode);
  Args.push_back(param_knob);
  Args.push_back(param_ret);
  Args.push_back(param_addr);
  Args.push_back(param_align);
  Args.push_back(param_orig_type);
  CallInst* call = builder.CreateCall(injectFn, Args);

  Value* final_result;
  if (dst_type && dst_type != int64ty) {
    Value* trunc = builder.CreateTrunc(call, dst_type);
    final_result = builder.CreateBitCast(trunc, orig_type);
  } else {
    final_result = builder.CreateTruncOrBitCast(call, orig_type);
  }

  std::vector<Value*> except;
  if (ret_to_be_casted != inst)
    except.push_back(ret_to_be_casted);
  else
    except.push_back(param_ret);
  except.push_back(call);
  replaceAllUsesWithExcept(inst, final_result, except);

  return true;
}

bool ErrorInjection::injectHooksStore(Instruction* inst, Instruction* nextInst,
    int param, Function* injectFn) {
  StoreInst* store_inst = dyn_cast<StoreInst>(inst);

  Type* orig_type = store_inst->getValueOperand()->getType();
  std::string orig_type_str = getTypeStr(orig_type, module);
  if (orig_type_str == "") return false;

  IRBuilder<> builder(module->getContext());
  builder.SetInsertPoint(inst);

  Value* opValue_to_be_casted = store_inst->getValueOperand();
  Type* dst_type = NULL;
  if (orig_type_str == "Half" || orig_type_str == "Float" || orig_type_str == "Double") {
    if (orig_type_str == "Half") {
      dst_type = Type::getInt16Ty(module->getContext());
    } else if (orig_type_str == "Float") {
      dst_type = Type::getInt32Ty(module->getContext());
    } else if (orig_type_str == "Double") {
      dst_type = Type::getInt64Ty(module->getContext());
    }
    opValue_to_be_casted = builder.CreateBitCast(opValue_to_be_casted, dst_type);
  }

  Type* int64ty = Type::getInt64Ty(module->getContext());
  Value* param_val = builder.CreateZExtOrBitCast(opValue_to_be_casted, int64ty);
  Value* param_addr = builder.CreatePtrToInt(store_inst->getPointerOperand(),
      int64ty);
  Value* param_align = builder.CreateZExtOrBitCast(
      ConstantInt::get(int64ty, store_inst->getAlignment(), false), int64ty);

  Value* opcode_global_str = builder.CreateGlobalString(inst->getOpcodeName());
  Value* param_opcode = builder.CreateBitCast(opcode_global_str,
      Type::getInt8PtrTy(module->getContext()));
  Value* type_global_str = builder.CreateGlobalString(orig_type_str.c_str());
  Value* param_orig_type = builder.CreateBitCast(type_global_str,
      Type::getInt8PtrTy(module->getContext()));

  Value* param_knob = ConstantInt::get(int64ty, param, false);

  SmallVector<Value *, 6> Args;
  Args.push_back(param_opcode);
  Args.push_back(param_knob);
  Args.push_back(param_val);
  Args.push_back(param_align);
  Args.push_back(param_addr);
  Args.push_back(param_orig_type);
  CallInst* call = builder.CreateCall(injectFn, Args);

  Value* final_result;
  if (dst_type && orig_type != int64ty) {
    Value* trunc = builder.CreateTrunc(call, dst_type);
    final_result = builder.CreateBitCast(trunc, orig_type);
  } else {
    final_result = builder.CreateTruncOrBitCast(call, orig_type);
  }

  // Now replace the operand value of the store instruction
  store_inst->setOperand(0, final_result);

  return true;
}

bool ErrorInjection::injectHooksBinOp(Instruction* inst, Instruction* nextInst,
    int param, Function* injectFn) {
  if (dyn_cast<Constant>(inst)) return false;

  Type* orig_type = inst->getType();
  std::string orig_type_str = getTypeStr(orig_type, module);
  if (orig_type_str == "") return false;

  IRBuilder<> builder(module->getContext());
  builder.SetInsertPoint(nextInst);

  Value* ret_to_be_casted = inst;
  Value* op1_to_be_casted = inst->getOperand(0);
  Value* op2_to_be_casted = inst->getOperand(1);
  Type* dst_type = NULL;
  if (orig_type_str == "Half" || orig_type_str == "Float"
      || orig_type_str == "Double") {
    if (orig_type_str == "Half")
      dst_type = Type::getInt16Ty(module->getContext());
    else if (orig_type_str == "Float")
      dst_type = Type::getInt32Ty(module->getContext());
    else
      dst_type = Type::getInt64Ty(module->getContext());

    ret_to_be_casted = builder.CreateBitCast(inst, dst_type);
    op1_to_be_casted = builder.CreateBitCast(inst->getOperand(0), dst_type);
    op2_to_be_casted = builder.CreateBitCast(inst->getOperand(1), dst_type);
  }

  Type* int64ty = Type::getInt64Ty(module->getContext());
  Value* param_ret = builder.CreateZExtOrBitCast(ret_to_be_casted, int64ty);
  Value* param_op1 = builder.CreateZExtOrBitCast(op1_to_be_casted, int64ty);
  Value* param_op2 = builder.CreateZExtOrBitCast(op2_to_be_casted, int64ty);

  Value* opcode_global_str = builder.CreateGlobalString(inst->getOpcodeName());
  Value* param_opcode = builder.CreateBitCast(opcode_global_str,
      Type::getInt8PtrTy(module->getContext()));
  Value* type_global_str = builder.CreateGlobalString(orig_type_str.c_str());
  Value* param_orig_type = builder.CreateBitCast(type_global_str,
      Type::getInt8PtrTy(module->getContext()));

  Value* param_knob = ConstantInt::get(int64ty, param, false);

  SmallVector<Value *, 6> Args;
  Args.push_back(param_opcode);
  Args.push_back(param_knob);
  Args.push_back(param_ret);
  Args.push_back(param_op1);
  Args.push_back(param_op2);
  Args.push_back(param_orig_type);
  CallInst* call = builder.CreateCall(injectFn, Args);

  Value* final_result;
  if (dst_type && dst_type != int64ty) {
    Value* trunc = builder.CreateTrunc(call, dst_type);
    final_result = builder.CreateBitCast(trunc, orig_type);
  } else {
    final_result = builder.CreateTruncOrBitCast(call, orig_type);
  }

  std::vector<Value*> except;
  if (ret_to_be_casted != inst)
    except.push_back(ret_to_be_casted);
  else
    except.push_back(param_ret);
  except.push_back(call);
  replaceAllUsesWithExcept(inst, final_result, except);

  return true;
}

Type* ErrorInjection::intify(Type* dst_type) {
  if (dst_type == Type::getHalfTy(module->getContext()))
    return Type::getInt16Ty(module->getContext());
  else if (dst_type == Type::getFloatTy(module->getContext()))
    return Type::getInt32Ty(module->getContext());
  else if (dst_type == Type::getDoubleTy(module->getContext()))
    return Type::getInt64Ty(module->getContext());
  else
    return dst_type;
}

bool ErrorInjection::injectHooksCast(Instruction* inst, Instruction* nextInst,
    int param, Function* injectFn) {
  if (dyn_cast<Constant>(inst)) return false;

  // Useful types
  Type* int64ty = Type::getInt64Ty(module->getContext());
  Type* stringty = Type::getInt8PtrTy(module->getContext());

  // Derive the source and destination types
  CastInst* cast_inst = dyn_cast<CastInst>(inst);
  Type* dst_type = cast_inst->getDestTy();
  Type* src_type = cast_inst->getSrcTy();
  std::string dst_type_str = getTypeStr(dst_type, module);
  std::string src_type_str = getTypeStr(src_type, module);
  if (dst_type_str == "" || src_type_str == "") return false;

  IRBuilder<> builder(module->getContext());
  builder.SetInsertPoint(nextInst);

  // Produce the opcode string parameter
  Value* opcode_global_str = builder.CreateGlobalString(inst->getOpcodeName());
  Value* param_opcode = builder.CreateBitCast(opcode_global_str, stringty);

  // Knob parameter for error injection
  Value* param_knob = ConstantInt::get(int64ty, param, false);

  // Procuce the parameter values (ret & op)
  Type* ret_type = intify(dst_type);
  Type* op_type = intify(src_type);
  Value* ret_to_be_casted = builder.CreateBitCast(cast_inst, ret_type);
  Value* op_to_be_casted = builder.CreateBitCast(cast_inst->getOperand(0), op_type);
  Value* param_ret = builder.CreateZExtOrBitCast(ret_to_be_casted, int64ty);
  Value* param_op = builder.CreateZExtOrBitCast(op_to_be_casted, int64ty);

  // Produce the type string parameter (destination)
  Value* type_global_str = builder.CreateGlobalString(dst_type_str.c_str());
  Value* param_orig_type = builder.CreateBitCast(type_global_str, stringty);


  SmallVector<Value *, 6> Args;
  Args.push_back(param_opcode);
  Args.push_back(param_knob);
  Args.push_back(param_ret);
  Args.push_back(param_op);
  Args.push_back(param_op); // Let's just push this twice in there
  Args.push_back(param_orig_type);
  CallInst* call = builder.CreateCall(injectFn, Args);

  Value* final_result;
  if (dst_type && dst_type != int64ty) {
    Value* trunc = builder.CreateTrunc(call, ret_type);
    final_result = builder.CreateBitCast(trunc, dst_type);
  } else {
    final_result = builder.CreateTruncOrBitCast(call, dst_type);
  }

  std::vector<Value*> except;
  if (ret_to_be_casted != inst)
    except.push_back(ret_to_be_casted);
  else
    except.push_back(param_ret);
  except.push_back(call);
  replaceAllUsesWithExcept(inst, final_result, except);

  return true;
}

bool ErrorInjection::injectHooksCall(Instruction* inst, Instruction* nextInst,
    int param, Function* injectFn) {
  CallInst* call_inst = dyn_cast<CallInst>(inst);
  assert(call_inst != NULL);

  // Useful types
  Type* int64ty = Type::getInt64Ty(module->getContext());
  Type* stringty = Type::getInt8PtrTy(module->getContext());

  // Derive the source and destination types
  Type* op_type = call_inst->getType();
  std::string op_type_str = getTypeStr(op_type, module);
  if (op_type_str == "") return false;

  IRBuilder<> builder(module->getContext());
  builder.SetInsertPoint(nextInst);

  // Produce the opcode string parameter
  Value* opcode_global_str = builder.CreateGlobalString(inst->getOpcodeName());
  Value* param_opcode = builder.CreateBitCast(opcode_global_str, stringty);

  // Knob parameter for error injection
  Value* param_knob = ConstantInt::get(int64ty, param, false);

  // Procuce the parameter values (ret & op)
  Type* ret_type = intify(op_type);
  Value* ret_to_be_casted = builder.CreateBitCast(call_inst, ret_type);
  Value* param_ret = builder.CreateZExtOrBitCast(ret_to_be_casted, int64ty);

  // Produce the type string parameter (destination)
  Value* type_global_str = builder.CreateGlobalString(op_type_str.c_str());
  Value* param_orig_type = builder.CreateBitCast(type_global_str, stringty);


  SmallVector<Value *, 6> Args;
  Args.push_back(param_opcode);
  Args.push_back(param_knob);
  Args.push_back(param_ret);
  Args.push_back(param_ret);// Let's just push this in there since all we care is the ret value
  Args.push_back(param_ret); // Let's just push this in there since all we care is the ret value
  Args.push_back(param_orig_type);
  CallInst* call = builder.CreateCall(injectFn, Args);

  Value* final_result;
  if (op_type && op_type != int64ty) {
    Value* trunc = builder.CreateTrunc(call, ret_type);
    final_result = builder.CreateBitCast(trunc, op_type);
  } else {
    final_result = builder.CreateTruncOrBitCast(call, op_type);
  }

  std::vector<Value*> except;
  if (ret_to_be_casted != inst)
    except.push_back(ret_to_be_casted);
  else
    except.push_back(param_ret);
  except.push_back(call);
  replaceAllUsesWithExcept(inst, final_result, except);

  return true;
}

bool ErrorInjection::injectHooks(Instruction* inst, Instruction* nextInst,
    int param, Function* injectFn) {
  if (isa<BinaryOperator>(inst))
    return injectHooksBinOp(inst, nextInst, param, injectFn);

#ifdef ALU_ONLY
  return false;
#endif //ALU_ONLY

  if (isa<CastInst>(inst))
    return injectHooksCast(inst, nextInst, param, injectFn);
  if (isa<StoreInst>(inst))
    return injectHooksStore(inst, nextInst, param, injectFn);
  if (isa<LoadInst>(inst))
    return injectHooksLoad(inst, nextInst, param, injectFn);
  if (isa<CallInst>(inst))
    return injectHooksCall(inst, nextInst, param, injectFn);

  assert(NULL && "Unsupported type!!!");

}

bool ErrorInjection::injectRegionHooks(Instruction* inst, int param) {
  CallInst* ci = dyn_cast<CallInst>(inst);
  assert(ci != NULL);

  const int nargs = ci->getNumArgOperands();

  const std::string injectFn_unmangled_name = "injectRegion";
  const std::string injectFn_mangled_name =
      get_mangled_fn_name(module, injectFn_unmangled_name);
  Function* injectFn = module->getFunction(injectFn_mangled_name);

  Type* int64ty = Type::getInt64Ty(module->getContext());
  Value* param_knob = ConstantInt::get(int64ty, param, false);
  Value* param_npairs = ConstantInt::get(int64ty, nargs / 2, false);

  SmallVector<Value *, 4> injectFn_args;
  injectFn_args.push_back(param_knob);
  injectFn_args.push_back(param_npairs);

  for (int i = 0; i < nargs; ++i)
    injectFn_args.push_back(ci->getArgOperand(i));

  Type* int32ty = Type::getInt32Ty(module->getContext());
  PointerType* int32ptrty = PointerType::getUnqual(int32ty);
  Value* param_dummy_int = ConstantInt::get(int32ty, 0, false);
  Value* param_dummy_ptr = ConstantPointerNull::get(int32ptrty);

  IRBuilder<> builder(module->getContext());
  builder.SetInsertPoint(inst);

  CallInst* injection_call = builder.CreateCall(injectFn, injectFn_args);
  //ci->eraseFromParent();

  return true; //code has been modified
}

bool ErrorInjection::injectErrorRegion(InstId iid) {
  Instruction* inst = iid.inst;
  std::stringstream ss;
  ss << "coarse " << inst->getParent()->getParent()->getName().str() <<
      ' ' << iid.bb_index << ' ' << iid.i_index << ' ' << inst->getOpcodeName();
  std::string instName = ss.str();

  LogDescription *desc = AI->logAdd("Coarse", inst);
  ACCEPT_LOG << instName << "\n";

  if (transformPass->relax) { // we're injecting error
    int param = transformPass->relaxConfig[instName];
    if (param) {
      ACCEPT_LOG << "injecting error " << param << "\n";
      // inst->getParent()->getParent()->viewCFG();
      return injectRegionHooks(inst, param);
    } else {
      ACCEPT_LOG << "not injecting error\n";
      return false;
    }
  } else { // we're just logging
    ACCEPT_LOG << "can inject error\n";
    transformPass->relaxConfig[instName] = 0;
  }

  return false;
}

bool ErrorInjection::injectErrorInst(InstId iid, Instruction* nextInst,
    Function* injectFn) {
  Instruction* inst = iid.inst;

  CallInst* ci = dyn_cast<CallInst>(inst);
  if (ci != NULL && !ci->isInlineAsm()) {
    std::string called_fn_name = ci->getCalledFunction()->getName().str();
    if (called_fn_name.find("ACCEPTRegion") != std::string::npos)
      return injectErrorRegion(iid);
  }

  if (isa<BinaryOperator>(inst) ||
      isa<StoreInst>(inst) ||
      isa<LoadInst>(inst) ||
      isa<CallInst>(inst)) {

    std::stringstream ss;
    Type* orig_type = inst->getType();

    if (isa<StoreInst>(inst)) {
      orig_type = dyn_cast<StoreInst>(inst)->getValueOperand()->getType();
    }

    std::string opCode;
    if (isa<CallInst>(inst) && ci != NULL && !ci->isInlineAsm()) {
      opCode = ci->getCalledFunction()->getName().str();
    } else {
      opCode = inst->getOpcodeName();
    }


    ss << "instruction " << inst->getParent()->getParent()->getName().str()
       << ":" << iid.bb_index << ":" << iid.i_index
       << ":" << opCode
       << ":" << getTypeStr(orig_type, module);

    std::string instName = ss.str();

    LogDescription *desc = AI->logAdd("Instruction", inst);
    ACCEPT_LOG << instName << "\n";

    bool approx = isApprox(inst);

    if (transformPass->relax && approx) { // we're injecting error
      int param = transformPass->relaxConfig[instName];
      if (isa<CallInst>(inst) && ci != NULL && !ci->isInlineAsm()) {
      }
      if (param) {
        ACCEPT_LOG << "injecting error " << param << "\n";
        // param tells which error injection will be done e.g. bit flipping
        return injectHooks(inst, nextInst, param, injectFn);
      } else {
        ACCEPT_LOG << "not injecting error\n";
        return false;
      }
    } else { // we're just logging
      if (approx) {
        ACCEPT_LOG << "can inject error\n";
        transformPass->relaxConfig[instName] = 0;
      } else {
        ACCEPT_LOG << "cannot inject error\n";
      }
    }
  }
  return false;
}

char ErrorInjection::ID = 0;
INITIALIZE_PASS_BEGIN(ErrorInjection, "error", "ACCEPT error injection pass", false, false)
INITIALIZE_PASS_DEPENDENCY(ApproxInfo)
INITIALIZE_PASS_END(ErrorInjection, "error", "ACCEPT error injection pass", false, false)
FunctionPass *llvm::createErrorInjectionPass() { return new ErrorInjection(); }
