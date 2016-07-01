#include <sstream>
#include <string>
#include <iostream>

#include "llvm/IRBuilder.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"

#include "accept.h"

#define INSTRUMENT_FP false

using namespace llvm;

namespace {
  static unsigned bbIndex;
  static unsigned bbTotal;
  static unsigned fpTotal;
  static unsigned fpIndex;

  struct InstrumentBB : public FunctionPass {
    static char ID;
    ACCEPTPass *transformPass;
    Module *module;

    InstrumentBB();
    virtual const char *getPassName() const;
    virtual bool doInitialization(llvm::Module &M);
    virtual bool doFinalization(llvm::Module &M);
    virtual bool runOnFunction(llvm::Function &F);
    bool instrumentBasicBlocks(llvm::Function &F);
  };

  // Returns the string type
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

  // We'll insert the initialization call in main
  Function *Main = M.getFunction("main");
  assert(Main && "Error: count-bb requires a main function");

  // Initialization call, logbb_init() defined in run time
  LLVMContext &Ctx = Main->getContext();
  Constant *initFunc = Main->getParent()->getOrInsertFunction(
    "logbb_init", Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx), NULL
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
        }
      }
    }
  }

  Value* bbTotalVal = builder.getInt32(bbTotal);;
  Value* fpTotalVal = builder.getInt32(fpTotal);;
  Value* args[] = {bbTotalVal, fpTotalVal};
  builder.CreateCall(initFunc, args);

  return true; // modified IR
}

bool InstrumentBB::doFinalization(Module &M) {
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
  const std::string br_injectFn_name = "logbranch";
  Constant *brLogFunc = module->getOrInsertFunction(
    br_injectFn_name, voidty, stringty, int32ty, NULL
  );

  bool modified = false;

  // Iterate through all functions
  for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {

    // Only instrument if the function is white-listed
    if (transformPass->shouldInjectError(F)) {

      BasicBlock *bb = fi;

      for (BasicBlock::iterator bi = bb->begin(); bi != bb->end(); ++bi) {
        Instruction *inst = bi;
        Instruction *nextInst = next(bi, 1);

        // Load instruction
        if (isa<LoadInst>(inst)) {
          // assert(nextInst && "next inst is NULL");

          // Builder
          IRBuilder<> builder(module->getContext());
          builder.SetInsertPoint(nextInst);

          // Cast to load instruction
          LoadInst* load_inst = dyn_cast<LoadInst>(inst);

          // Obtain the instruction id
          StringRef iid = cast<MDString>(inst->getMetadata("iid")->getOperand(0))->getString();
          Value* instid_global_str = builder.CreateGlobalString(iid.str().c_str());
          Value* param_instid = builder.CreateBitCast(instid_global_str, stringty);

          // Obtain the type string
          Type* insnType = inst->getType();
          std::string type_str = getTypeStr(insnType, module);
          // assert(type_str != "" && "type string not found!");

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
            StringRef iid = cast<MDString>(inst->getMetadata("iid")->getOperand(0))->getString();
            // std::cout << iid.str() << std::endl;
            // inst->print(errs());
            Value* instid_global_str = builder.CreateGlobalString(iid.str().c_str());
            Value* param_instid = builder.CreateBitCast(instid_global_str, stringty);

            // Obtain the condition value
            Value* cond = br_inst->getCondition();
            Value* param_cond = builder.CreateZExtOrBitCast(cond, int32ty);

            // Initialize the argument vector
            Value* args[] = {
              // param_instid,
              param_instid,
              param_cond
            };
            // Inject function
            builder.CreateCall(brLogFunc, args);
            modified = true;

          }
        }
      }

#if INSTRUMENT_FP==true
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
            // IRBuilder<> builder(module->getContext());
            // builder.SetInsertPoint(nextInst);
            IRBuilder<> builder(inst);

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

            ++fpIndex;
          }
        }
      }
#endif //INSTRUMENT_FP

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
