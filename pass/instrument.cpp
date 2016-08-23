#include <sstream>
#include <string>
#include <iostream>
#include <fstream>

#include "llvm/IRBuilder.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"

#include "accept.h"

#define INSTRUMENT_FP true
#define STATICANALYSIS
// #define DYNTRACE

using namespace llvm;

namespace {
  static unsigned bbIndex;
  static unsigned bbTotal;
  static unsigned fpIndex;
  static unsigned fpTotal;
  static unsigned brIndex;
  static unsigned brTotal;
  static unsigned regIndex;

  struct InstrumentBB : public FunctionPass {
    static char ID;
    ACCEPTPass *transformPass;
    Module *module;
    std::ofstream staticdump;

    InstrumentBB();
    virtual const char *getPassName() const;
    virtual bool doInitialization(llvm::Module &M);
    virtual bool doFinalization(llvm::Module &M);
    virtual bool runOnFunction(llvm::Function &F);
    bool instrumentBasicBlocks(llvm::Function &F);
  };

  // Returns the string type
  std::string getTypeStr(Type* orig_type, Module* module) {
    if (orig_type == Type::getVoidTy(module->getContext()))
      return "Void";
    else if (orig_type == Type::getLabelTy(module->getContext()))
      return "Label";
    else if (orig_type == Type::getHalfTy(module->getContext()))
      return "Half";
    else if (orig_type == Type::getFloatTy(module->getContext()))
      return "Float";
    else if (orig_type == Type::getDoubleTy(module->getContext()))
      return "Double";
    else if (orig_type == Type::getMetadataTy(module->getContext()))
      return "Metadata";
    else if (orig_type == Type::getX86_FP80Ty(module->getContext()))
      return "X86_FP80";
    else if (orig_type == Type::getFP128Ty(module->getContext()))
      return "FP128";
    else if (orig_type == Type::getPPC_FP128Ty(module->getContext()))
      return "PPC_FP128";
    else if (orig_type == Type::getX86_MMXTy(module->getContext()))
      return "X86_MMX";
    // else if (orig_type == Type::getTokenTy(module->getContext()))
    //   return "Token";
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
    // else if (orig_type == Type::getInt128Ty(module->getContext()))
    //   return "Int128";
    else if (orig_type == Type::getHalfPtrTy(module->getContext()))
      return "HalfPtr";
    else if (orig_type == Type::getFloatPtrTy(module->getContext()))
      return "FloatPtr";
    else if (orig_type == Type::getDoublePtrTy(module->getContext()))
      return "DoublePtr";
    else if (orig_type == Type::getX86_FP80PtrTy(module->getContext()))
      return "X86_FP80Ptr";
    else if (orig_type == Type::getFP128PtrTy(module->getContext()))
      return "FP128Ptr";
    else if (orig_type == Type::getPPC_FP128PtrTy(module->getContext()))
      return "PPC_FP128Ptr";
    else if (orig_type == Type::getX86_MMXPtrTy(module->getContext()))
      return "X86_MMXPtr";
    else if (orig_type == Type::getInt1PtrTy(module->getContext()))
      return "Int1Ptr";
    else if (orig_type == Type::getInt8PtrTy(module->getContext()))
      return "Int8Ptr";
    else if (orig_type == Type::getInt16PtrTy(module->getContext()))
      return "Int16Ptr";
    else if (orig_type == Type::getInt32PtrTy(module->getContext()))
      return "Int32Ptr";
    else if (orig_type == Type::getInt64PtrTy(module->getContext()))
      return "Int64Ptr";
    else
      return "";
  }

  // Returns the integer type equivalent
  Type* getIntType(Type* orig_type, Module* module) {
    if (orig_type == Type::getHalfTy(module->getContext()))
      return Type::getInt16Ty(module->getContext());
    else if (orig_type == Type::getFloatTy(module->getContext()))
      return Type::getInt32Ty(module->getContext());
    else if (orig_type == Type::getDoubleTy(module->getContext()))
      return Type::getInt64Ty(module->getContext());
    else
      return NULL;
  }

  // Returns the Value Register name
  std::string getRegName(Value* reg, LLVMContext& Ctx) {
    // Set the register name if unnamed
    if (!reg->hasName()) {
      std::string regName;
      llvm::raw_string_ostream rso(regName);
      rso << "r" << regIndex++;
      reg->setName(Twine(rso.str()));
    }

    // Special case: constant
    if (isa<Constant>(reg)) {
      if (dyn_cast<Constant>(reg)->isNullValue()) {
        return "null";
      } else if (isa<UndefValue>(reg)) {
        return "undef";
      }

      // Dumpt the constant value to a string
      std::string cstVal;
      llvm::raw_string_ostream rso(cstVal);
      // If half or float, print as float
      if (reg->getType()->isDoubleTy()) {
        ConstantFP* cst = dyn_cast<ConstantFP>(reg);
        rso << cst->getValueAPF().convertToDouble();
        return rso.str();
      }
      // If double, print as double
      else if (reg->getType()->isFloatTy() || reg->getType()->isHalfTy()) {
        ConstantFP* cst = dyn_cast<ConstantFP>(reg);
        rso << cst->getValueAPF().convertToFloat();
        return rso.str();
      }
      // If integer
      else if (reg->getType()->isIntegerTy()) {
        ConstantInt* cst = dyn_cast<ConstantInt>(reg);
        rso << cst->getValue().getZExtValue();
        return rso.str();
      }
    }

    return reg->getName().str();
  }

  // Get the instruction ID of the instruction
  Value* getIID(Instruction* inst, int idx, Module *module) {
    IRBuilder<> builder(module->getContext());
    builder.SetInsertPoint(inst);
    std::string iid_str = "";
    if (inst->getMetadata("iid")) {
      iid_str = cast<MDString>(inst->getMetadata("iid")->getOperand(idx))->getString().str();
    }
    Value* iid_global_str = builder.CreateGlobalString(iid_str.c_str());
    Value* iid = builder.CreateBitCast(iid_global_str, Type::getInt8PtrTy(module->getContext()));
    return iid;
  }
}

InstrumentBB::InstrumentBB() : FunctionPass(ID) {
  initializeInstrumentBBPass(*PassRegistry::getPassRegistry());
}

const char *InstrumentBB::getPassName() const {
  return "Basic Block instrumentation";
}
bool InstrumentBB::doInitialization(Module &M) {
  module = &M;

  // ACCEPT shared transform pass
  transformPass = (ACCEPTPass*)sharedAcceptTransformPass;

  // Static instruction dump for DFG/CFG construction
  staticdump.open("accept_static.txt");
  if (!staticdump) {
    errs() << "Failed to open accept_static.txt\n";
    exit(-1);
  }

  // We'll insert the initialization call in main
  Function *Main = M.getFunction("main");
  assert(Main && "Error: count-bb requires a main function");

  // Initialization call, logbb_init() defined in run time
  LLVMContext &Ctx = Main->getContext();
  Constant *initFunc = Main->getParent()->getOrInsertFunction(
    "logbb_init", Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx), NULL
  );
  BasicBlock *bb = &Main->front();
  Instruction *op = &bb->front();
  IRBuilder<> builder(op);
  builder.SetInsertPoint(bb, builder.GetInsertPoint());

  // Initialize statics
  bbIndex = 0;
  bbTotal = 0;
  fpIndex = 0;
  fpTotal = 0;
  brIndex = 0;
  brTotal = 0;
  regIndex = 0;
  // Determine the number of basic blocks in the module
  for (Module::iterator mi = M.begin(); mi != M.end(); ++mi) {
    Function *F = mi;
    // if (!transformPass->shouldSkipFunc(*F))
    bbTotal += F->size();
    for (Function::iterator fi = F->begin(); fi != F->end(); ++fi) {
      BasicBlock *bb = fi;
      // Count the number fp
      for (BasicBlock::iterator bi = bb->begin(); bi != bb->end(); ++bi) {
        Instruction *inst = bi;
        // Check if we are dealing with a fp instruction
        if (isa<BinaryOperator>(inst) ||
            isa<StoreInst>(inst) ||
            isa<LoadInst>(inst) ||
            isa<CallInst>(inst)) {
          Type * opType = inst->getType();
          if (opType == Type::getHalfTy(Ctx) ||
              opType == Type::getFloatTy(Ctx) ||
              opType == Type::getDoubleTy(Ctx)) {
            fpTotal++;
          }
        } else if (isa<BranchInst>(inst)) {
            BranchInst* br_inst = dyn_cast<BranchInst>(inst);
            if (br_inst->isConditional()) {
              brTotal++;
            }
          }
      }
    }
  }

  Value* bbTotalVal = builder.getInt32(bbTotal);
  Value* fpTotalVal = builder.getInt32(fpTotal);
  Value* brTotalVal = builder.getInt32(brTotal);
  Value* args[] = {bbTotalVal, fpTotalVal, brTotalVal};
  builder.CreateCall(initFunc, args);

  return true; // modified IR
}

bool InstrumentBB::doFinalization(Module &M) {
  staticdump.close();
  return false;
}

bool InstrumentBB::runOnFunction(Function &F) {
  bool modified = false;

  // Skip optimizing functions that seem to be in standard libraries.
  if (!transformPass->shouldSkipFunc(F)) {
    assert (!llvm::verifyFunction(F) && "Verification failed before code alteration");
    modified = instrumentBasicBlocks(F);
    assert (!llvm::verifyFunction(F) && "Verification failed after code alteration");
  }

  return modified;
}

bool InstrumentBB::instrumentBasicBlocks(Function & F){

  // Context
  LLVMContext &Ctx = F.getContext();
  // Handy types
  Type* voidty = Type::getVoidTy(Ctx);
  Type* int16ty = Type::getInt16Ty(Ctx);
  Type* int32ty = Type::getInt32Ty(Ctx);
  Type* int64ty = Type::getInt64Ty(Ctx);
  Type* stringty = Type::getInt8PtrTy(Ctx);
  Type* halfty = Type::getHalfTy(Ctx);
  Type* floatty = Type::getFloatTy(Ctx);
  Type* doublety = Type::getDoubleTy(Ctx);

  const std::string bb_injectFn_name = "logbb";
  Constant *bbLogFunc = module->getOrInsertFunction(
    bb_injectFn_name, voidty, int32ty, NULL
  );
  const std::string fp_injectFn_name = "logfloat";
  Constant *fpLogFunc = module->getOrInsertFunction(
    fp_injectFn_name, voidty, int32ty, stringty,
    int32ty, int64ty, NULL
  );
  // Function used to log loads
  const std::string ld_injectFn_name = "logload";
  Constant *ldLogFunc = module->getOrInsertFunction(
    ld_injectFn_name, voidty, stringty, stringty,
    int64ty, int64ty, int64ty, NULL
  );
  // Function used to log conditional branches
  const std::string cbr_injectFn_name = "logcondbranch";
  Constant *cbrLogFunc = module->getOrInsertFunction(
    cbr_injectFn_name, voidty, int32ty, stringty, int32ty, NULL
  );
  // Function used to log unconditional branches
  const std::string ucbr_injectFn_name = "loguncondbranch";
  Constant *ucbrLogFunc = module->getOrInsertFunction(
    ucbr_injectFn_name, voidty, stringty, stringty,  NULL
  );
  // Function used to log phi instructions
  const std::string phi_injectFn_name = "logphi";
  Constant *phiLogFunc = module->getOrInsertFunction(
    phi_injectFn_name, voidty, stringty, int64ty, stringty, stringty, NULL
  );

  bool modified = false;

#ifdef STATICANALYSIS
  // Let's print the instruction list to reconstruct a DDDG
  if (!transformPass->shouldSkipFunc(F)) {
    for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {
      BasicBlock *bb = fi;
      for (BasicBlock::iterator bi = bb->begin(); bi != bb->end(); ++bi) {
        Instruction *inst = bi;

        // Get function name
        std::string fn_name = inst->getParent()->getParent()->getName().str();

        // Get the op string
        std::string op_str = inst->getOpcodeName();

        // Get instruction ID if it exists
        std::string iid = "";
        if (inst->getMetadata("iid")) {
          iid = cast<MDString>(inst->getMetadata("iid")->getOperand(0))->getString().str();
        }

        // Uncomment for debugging purposes
        // inst->print(errs());
        // errs() << "\n";

        // Get the qualifier
        std::string qual = (isApprox(inst)) ? "approx" : "precise";

        // Arithmetic instructions
        if (isa<CmpInst>(inst)) {

          CmpInst* cmp_inst = dyn_cast<CmpInst>(inst);

          Value* dst = cmp_inst;
          Value* src0 = inst->getOperand(0);
          Value* src1 = inst->getOperand(1);

          std::string ty_str = getTypeStr(cmp_inst->getType(), module);
          std::string dst_str = getRegName(dst, Ctx);
          std::string src0_str = getRegName(src0, Ctx);
          std::string src1_str = getRegName(src1, Ctx);

          staticdump << fn_name << ", ";
          staticdump << op_str << ", ";
          staticdump << iid << ", ";
          staticdump << qual << ", ";
          staticdump << ty_str << ", ";
          staticdump << dst_str << ", ";
          staticdump << src0_str << ", ";
          staticdump << src1_str << "\n";

        } else if (isa<BinaryOperator>(inst)) {

          Value* dst = inst;
          Value* src0 = inst->getOperand(0);
          Value* src1 = inst->getOperand(1);

          std::string ty_str = getTypeStr(inst->getType(), module);
          std::string dst_str = getRegName(dst, Ctx);
          std::string src0_str = getRegName(src0, Ctx);
          std::string src1_str = getRegName(src1, Ctx);

          staticdump << fn_name << ", ";
          staticdump << op_str << ", ";
          staticdump << iid << ", ";
          staticdump << qual << ", ";
          staticdump << ty_str << ", ";
          staticdump << dst_str << ", ";
          staticdump << src0_str << ", ";
          staticdump << src1_str << "\n";

        } else if (isa<LoadInst>(inst)) {

          LoadInst* load_inst = dyn_cast<LoadInst>(inst);
          Value* dst = load_inst;
          Value* adr = load_inst->getPointerOperand();

          std::string ty_str = getTypeStr(inst->getType(), module);
          std::string dst_str = getRegName(dst, Ctx);
          std::string adr_str = getRegName(adr, Ctx);

          staticdump << fn_name << ", ";
          staticdump << op_str << ", ";
          staticdump << iid << ", ";
          staticdump << qual << ", ";
          staticdump << ty_str << ", ";
          staticdump << dst_str << ", [";
          staticdump << adr_str << "]" << "\n";

        } else if (isa<StoreInst>(inst)) {

          StoreInst* store_inst = dyn_cast<StoreInst>(inst);
          Value* src = store_inst->getValueOperand();
          Value* adr = store_inst->getPointerOperand();

          std::string ty_str = getTypeStr(store_inst->getValueOperand()->getType(), module);
          std::string src_str = getRegName(src, Ctx);
          std::string adr_str = getRegName(adr, Ctx);

          staticdump << fn_name << ", ";
          staticdump << op_str << ", ";
          staticdump << iid << ", ";
          staticdump << qual << ", ";
          staticdump << ty_str << ", ";
          staticdump << src_str << ", [";
          staticdump << adr_str << "]" << "\n";

        } else if (isa<CastInst>(inst)) {

          CastInst* cast_inst = dyn_cast<CastInst>(inst);
          Value *dst = cast_inst;
          Value *src = cast_inst->getOperand(0);

          std::string src_ty_str = getTypeStr(cast_inst->getSrcTy(), module);
          std::string dst_ty_str = getTypeStr(cast_inst->getDestTy(), module);
          std::string src_str = getRegName(src, Ctx);
          std::string dst_str = getRegName(dst, Ctx);

          staticdump << fn_name << ", ";
          staticdump << op_str << ", ";
          staticdump << iid << ", ";
          staticdump << qual << ", ";
          staticdump << dst_ty_str << ", ";
          staticdump << src_ty_str << ", ";
          staticdump << dst_str << ", ";
          staticdump << src_str << "\n";

        } else if (isa<PHINode>(inst)) {

          PHINode* phy_node = dyn_cast<PHINode>(inst);
          Value* dst = phy_node;

          std::string ty_str = getTypeStr(phy_node->getType(), module);
          std::string dst_str = getRegName(dst, Ctx);
          unsigned num_vals = phy_node->getNumIncomingValues();

          staticdump << fn_name << ", ";
          staticdump << op_str << ", ";
          staticdump << iid << ", ";
          staticdump << qual << ", ";
          staticdump << ty_str << ", ";
          staticdump << dst_str << ", ";
          staticdump << num_vals;

          for (unsigned val_idx=0; val_idx<num_vals; val_idx++) {
            // Retrieve incoming value
            Value* src = phy_node->getIncomingValue(val_idx);
            std::string src_str = getRegName(src, Ctx);
            // Retrieve incoming block ID
            BasicBlock* srcBB = phy_node->getIncomingBlock(val_idx);
            Instruction* srcBBi = srcBB->getFirstNonPHI();
            StringRef srcBB_str = cast<MDString>(srcBBi->getMetadata("iid")->getOperand(1))->getString();
            staticdump << ", " << src_str << ", " << srcBB_str.str();
          }
          staticdump << "\n";

        } else if (isa<ReturnInst>(inst)) {
          ReturnInst* ret_inst = dyn_cast<ReturnInst>(inst);
          Value *src = ret_inst->getReturnValue();

          if (!ret_inst->getType()->isVoidTy()) {

            std::string ty_str = getTypeStr(ret_inst->getType(), module);
            std::string src_str = getRegName(src, Ctx);

            staticdump << fn_name << ", ";
            staticdump << op_str << ", ";
            staticdump << iid << ", ";
            staticdump << qual << ", ";
            staticdump << ty_str << ", ";
            staticdump << src_str << "\n";

          }

        } else if (isa<CallInst>(inst)) {
          CallInst* call_inst = dyn_cast<CallInst>(inst);
          Function *callee = call_inst->getCalledFunction();


          std::string fn_str = callee->getName().str();
          std::string ty_str = getTypeStr(call_inst->getType(), module);

          if (transformPass->AI->isWhitelistedPure(fn_str)) {

            staticdump << fn_name << ", ";
            staticdump << op_str << ", ";
            staticdump << iid << ", ";
            staticdump << qual << ", ";
            staticdump << fn_str << ", ";
            staticdump << ty_str;

            if (! callee->getReturnType()->isVoidTy()) {
              Value *dst = call_inst;
              std::string dst_str = getRegName(dst, Ctx);
              staticdump << ", " << dst_str;
            }

            unsigned nargs = call_inst->getNumArgOperands();
            staticdump << ", " << nargs;

            for (int i = 0; i < nargs; ++i) {
              Value *arg = call_inst->getArgOperand(i);

              std::string arg_ty_str = getTypeStr(arg->getType(), module);

              staticdump << ", " << arg_ty_str;

              if (! arg->getType()->isVoidTy()) {
                std::string arg_str = getRegName(arg, Ctx);
                staticdump << ", " << arg_str;
              }
            }

            staticdump << "\n";
          }

        } else if (isa<BranchInst>(inst)) {

          BranchInst* br_inst = dyn_cast<BranchInst>(inst);

          StringRef src = cast<MDString>(inst->getMetadata("iid")->getOperand(1))->getString();

          if (br_inst->isConditional()) {

            // Determine the successor based on the condition
            BasicBlock* succ_0 = br_inst->getSuccessor(0); // True
            BasicBlock* succ_1 = br_inst->getSuccessor(1); // False
            Instruction* first_0 = succ_0->getFirstNonPHI();
            Instruction* first_1 = succ_1->getFirstNonPHI();
            StringRef dst_0 = cast<MDString>(first_0->getMetadata("iid")->getOperand(1))->getString();
            StringRef dst_1 = cast<MDString>(first_1->getMetadata("iid")->getOperand(1))->getString();

            staticdump << fn_name << ", " << op_str << ", " << iid << ", " << src.str() << "->" << dst_1.str() << "\n";
            staticdump << fn_name << ", " << op_str << ", " << iid << ", " << src.str() << "->" << dst_0.str() << "\n";

          } else if (br_inst->isUnconditional()) {

            // Determine the successor based on the condition
            BasicBlock* succ_0 = br_inst->getSuccessor(0);
            Instruction* first_0 = succ_0->getFirstNonPHI();
            StringRef dst_0 = cast<MDString>(first_0->getMetadata("iid")->getOperand(1))->getString();

            staticdump << fn_name << ", " << op_str << ", " << iid << ", " << src.str() << "->" << dst_0.str() << "\n";

          }
        }
      }
    }
  }
#endif

#ifdef DYNTRACE
  if (transformPass->relax && transformPass->shouldInjectError(F)) {
    // Iterate through all functions
    for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {

      // Only instrument if the function is white-listed
      if (transformPass->shouldInjectError(F)) {

        BasicBlock *bb = fi;

        for (BasicBlock::iterator bi = bb->begin(); bi != bb->end(); ++bi) {
          Instruction *inst = bi;
          Instruction *nextInst = next(bi, 1);

          // Load instruction
          if (isa<LoadInst>(inst) && isApprox(inst)) {
            // assert(nextInst && "next inst is NULL");

            // Builder
            IRBuilder<> builder(module->getContext());
            builder.SetInsertPoint(nextInst);

            // Cast to load instruction
            LoadInst* load_inst = dyn_cast<LoadInst>(inst);

            // Obtain the instruction id
            Value* param_instid = getIID(inst, 0, module);

            // Obtain the type string
            Type* insnType = inst->getType();
            std::string type_str = getTypeStr(insnType, module);

            if (type_str!="") {

              // Obtain the type string value
              Value* type_str_val = builder.CreateGlobalString(type_str.c_str());
              // Cast to a char array
              Value* param_type = builder.CreateBitCast(type_str_val,
                  Type::getInt8PtrTy(module->getContext()));

              // Obtain the load address
              Value* param_addr = builder.CreatePtrToInt(load_inst->getPointerOperand(),
                  int64ty);

              // Obtain the load alignment
              Value* param_align = builder.CreateZExtOrBitCast(
                  ConstantInt::get(int64ty, load_inst->getAlignment(), false), int64ty);

              // Obtain the load value
              // Cast the value to a 64-bit integer
              Value* int_value = inst;
              // If the value is a float, cast to integer
              Type* dst_type = getIntType(insnType, module);
              if (dst_type) int_value = builder.CreateBitCast(inst, dst_type);
              // Now zeroextend to 64-bits
              Value* param_value = builder.CreateZExtOrBitCast(int_value, int64ty);

              // Initialize the argument vector
              Value* args[] = {
                  param_instid,
                  param_type,
                  param_addr,
                  param_align,
                  param_value
                };
              // Inject function
              builder.CreateCall(ldLogFunc, args);
              modified = true;

            }

          } else if (isa<BranchInst>(inst)) {

            // Cast to branch instruction
            BranchInst* br_inst = dyn_cast<BranchInst>(inst);

            if (br_inst->isConditional()) {

              // Builder
              IRBuilder<> builder(module->getContext());
              builder.SetInsertPoint(inst);

               // Obtain the instruction id
              Value* param_instid = getIID(inst, 1, module);

              // Obtain the condition value
              Value* cond = br_inst->getCondition();
              Value* param_cond = builder.CreateZExtOrBitCast(cond, int32ty);

              // Obtain the branch ID
              Value* param_brIdx = builder.getInt32(brIndex);

              // // Obtain the number of successors
              // unsigned successors = br_inst->getNumSuccessors();
              // Value* param_succ = ConstantInt::get(int32ty, successors, false);

              // // Determine the successor based on the condition
              // BasicBlock* succ_0 = br_inst->getSuccessor(0);
              // BasicBlock* succ_1 = br_inst->getSuccessor(1);
              // Instruction* first_0 = succ_0->getFirstNonPHI();
              // Instruction* first_1 = succ_1->getFirstNonPHI();

              // Value* param_succ_0 = getIID(first_0, 1, module);
              // Value* param_succ_1 = getIID(first_1, 1, module);

              // Initialize the argument vector
              Value* args[] = {
                param_brIdx,
                param_instid,
                param_cond
                // param_succ_0,
                // param_succ_1
              };
              // Inject function
              builder.CreateCall(cbrLogFunc, args);
              modified = true;

              brIndex++;

            }
            // else if (br_inst->isUnconditional()) {

            //   // Builder
            //   IRBuilder<> builder(module->getContext());
            //   builder.SetInsertPoint(inst);

            //    // Obtain the instruction id
            //   Value* param_instid = getIID(inst, 1, module);

            //   // Determine the successor based on the condition
            //   BasicBlock* succ_0 = br_inst->getSuccessor(0);
            //   Instruction* first_0 = succ_0->getFirstNonPHI();

            //   Value* param_succ_0 = getBBID(first_0, module);

            //   // Initialize the argument vector
            //   Value* args[] = {
            //     param_instid,
            //     param_succ_0
            //   };
            //   // Inject function
            //   builder.CreateCall(ucbrLogFunc, args);
            //   modified = true;

            // }
          }
        }
      }
    }
  }
#endif //DYNTRACE

#if INSTRUMENT_FP==true
  // Iterate through all functions
  for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {

    // Only instrument if the function is white-listed
    if (transformPass->shouldInjectError(F)) {

      BasicBlock *bb = fi;
      for (BasicBlock::iterator bi = bb->begin(); bi != bb->end(); ++bi) {
        Instruction *inst = bi;
        Instruction *nextInst = next(bi, 1);

        // Check that the instruction is of interest to us
        if ( (isa<BinaryOperator>(inst) ||
              isa<StoreInst>(inst) ||
              isa<LoadInst>(inst) ||
              isa<CallInst>(inst)) && inst->getMetadata("iid") )
        {
          assert(nextInst && "next inst is NULL");

          // Check if the float is double or half precision op
          Type * opType;
          if (isa<StoreInst>(inst)) {
            StoreInst* store_inst = dyn_cast<StoreInst>(inst);
            opType = store_inst->getValueOperand()->getType();
          } else {
            opType = inst->getType();
          }
          if (opType == halfty ||
              opType == floatty ||
              opType == doublety) {

            // Builder
            IRBuilder<> builder(module->getContext());
            builder.SetInsertPoint(nextInst);

            // Identify the type
            int opTypeEnum;
            Type* dst_type;
            if (opType == halfty) {
              opTypeEnum = 1;
              dst_type = int16ty;
            }
            else if (opType == floatty) {
              opTypeEnum = 2;
              dst_type = int32ty;
            }
            else if (opType == doublety) {
              opTypeEnum = 3;
              dst_type = int64ty;
            } else {
              assert(NULL && "Type unknown!");
            }

            // Arg1: Type enum
            Value* param_opType = builder.getInt32(opTypeEnum);
            // Arg2: Instruction ID string
            StringRef iid = cast<MDString>(inst->getMetadata("iid")->getOperand(0))->getString();
            Value* instIdx_global_str = builder.CreateGlobalString(iid.str().c_str());
            Value* param_instIdx = builder.CreateBitCast(instIdx_global_str, stringty);
            // Arg3: FP instruction index
            Value* param_fpIdx = builder.getInt32(fpIndex);
            // Arg4: Destination value and type
            Value* dst_to_be_casted;
            if (isa<StoreInst>(inst)) {
              StoreInst* store_inst = dyn_cast<StoreInst>(inst);
              dst_to_be_casted = store_inst->getValueOperand();
              dst_to_be_casted = builder.CreateBitCast(dst_to_be_casted, dst_type);
            } else {
              dst_to_be_casted = builder.CreateBitCast(inst, dst_type);
            }
            Value* param_val = builder.CreateZExtOrBitCast(dst_to_be_casted, int64ty);

            // Create vector
            Value* args[] = {
              param_opType,
              param_instIdx,
              param_fpIdx,
              param_val
            };

            // Inject function
            builder.CreateCall(fpLogFunc, args);
            modified = true;

            ++fpIndex;
          }
        }
      }
    }
  }
#endif //INSTRUMENT_FP

  // Iterate through all functions
  for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {

    // Only instrument if the function is white-listed
    if (transformPass->shouldInjectError(F)) {

      BasicBlock *bb = fi;
      // Insert logbb call before the BB terminator
      Instruction *op = bb->getTerminator();
      IRBuilder<> builder(op);
      builder.SetInsertPoint(bb, builder.GetInsertPoint());

      // Pass the BB id (global)
      assert(bbIndex<bbTotal && "bbIndex exceeds bbTotal");
      Value* bbIdxValue = builder.getInt32(bbIndex);
      Value* args[] = {bbIdxValue};
      builder.CreateCall(bbLogFunc, args);

      modified = true;
    }

    ++bbIndex;
  }

  return modified;
}

char InstrumentBB::ID = 0;
INITIALIZE_PASS_BEGIN(InstrumentBB, "bb count", "ACCEPT basic block instrumentation pass", false, false)
INITIALIZE_PASS_DEPENDENCY(ApproxInfo)
INITIALIZE_PASS_END(InstrumentBB, "bb count", "ACCEPT basic block instrumentation pass", false, false)
FunctionPass *llvm::createInstrumentBBPass() { return new InstrumentBB(); }
