#include "accept.h"

#include "llvm/Analysis/LoopPass.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Argument.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/InlineAsm.h"

#include <sstream>
#include <iostream>
#include <queue>
#include <fstream>

using namespace llvm;

namespace {
  struct LoopNPUPass : public LoopPass {
    static char ID;
    bool modified;
    ACCEPTPass *transformPass;
    ApproxInfo *AI;
    Module *module;
    llvm::raw_fd_ostream *log;
    LoopInfo *LI;
    //AliasAnalysis *AA;

    LoopNPUPass() : LoopPass(ID) {}

    virtual bool doInitialization(Loop *loop, LPPassManager &LPM) {
      transformPass = (ACCEPTPass*)sharedAcceptTransformPass;
      AI = transformPass->AI;
      log = AI->log;
      return false;
    }
    virtual bool runOnLoop(Loop *loop, LPPassManager &LPM) {
      modified = false;
      if (transformPass->shouldSkipFunc(*(loop->getHeader()->getParent())))
          return false;
      module = loop->getHeader()->getParent()->getParent();
      LI = &getAnalysis<LoopInfo>();
      //AA = &getAnalysis<AliasAnalysis>();
      return tryToOptimizeLoop(loop);
    }
    virtual bool doFinalization() {
      return false;
    }
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      LoopPass::getAnalysisUsage(AU);
      AU.addRequired<LoopInfo>();
      //AU.addRequiredTransitive<AliasAnalysis>();
    }

    /*
    void dumpFuncInsts(Function *f) {
    	std::cerr << "\nDumping function " << f->getName().str() << std::endl;
    	std::string type_str;
    	llvm::raw_string_ostream rso(type_str);
        for (Function::iterator bi = f->begin(); bi != f->end(); ++bi) {
          BasicBlock *bb = bi;
          for (BasicBlock::iterator ii = bb->begin();
              ii != bb->end(); ++ii) {
            Instruction *inst = ii;
            rso << *inst << "\n";
          }
        }
        std::cerr << rso.str() << std::endl;
    }
    */
    std::vector<Loop *> loops_to_npu;
    std::vector<Instruction *> calls_to_npu;
    bool find_inst(Instruction *inst) {
      for (int i = 0; i < calls_to_npu.size(); ++i)
        if (calls_to_npu[i] == inst)
          return true;
      return false;
    }

    // Not handling recursive or cyclic function calls.
    void testSubFunctions(Function *f, Loop *loop) {
      for (Function::iterator bi = f->begin(); bi != f->end(); ++bi) {
        BasicBlock *bb = bi;
        for (BasicBlock::iterator ii = bb->begin();
            ii != bb->end(); ++ii) {
          Instruction *inst = ii;
          const CallInst *c_inst = dyn_cast<CallInst>(inst);
          if (!c_inst) continue;

          Function *callee = c_inst->getCalledFunction();

          testSubFunctions(callee, loop);
          if (!isa<IntrinsicInst>(inst) && !AI->isWhiteList(callee->getName()) && AI->isPrecisePure(callee)) {
            std::cerr << callee->getName().str() << " is precise pure!\n\n" << std::endl;
            if (!find_inst(inst)) {
              loops_to_npu.push_back(loop);
              calls_to_npu.push_back(inst);
            }
          } else {
            std::cerr << callee->getName().str() << " is NOT precise pure!\n\n" << std::endl;
          }
        }
      }
    }

    bool tryToOptimizeLoop(Loop *loop) {
      std::stringstream ss;
      ss << "loop at "
         << srcPosDesc(*module, loop->getHeader()->begin()->getDebugLoc());
      std::string loopName = ss.str();

      *log << "---\n" << loopName << "\n";

      // Look for ACCEPT_FORBID marker.
      if (AI->instMarker(loop->getHeader()->begin()) == markerForbid) {
        *log << "optimization forbidden\n";
        return false;
      }

      // Skip array constructor loops manufactured by Clang.
      if (loop->getHeader()->getName().startswith("arrayctor.loop")) {
        *log << "array constructor\n";
        return false;
      }

      /*
      std::cerr << "\n--------------- loop body only---------------------" << std::endl;
      for (Loop::block_iterator bi = loop->block_begin();
            bi != loop->block_end(); ++bi) {
        if (*bi == loop->getHeader()) {
          // Even in perforated loops, the header gets executed every time. So we
          // don't check it.
          continue;
        } else if (*bi == loop->getLoopLatch()) {
          // When perforating for-like loops, we also execute the latch each
          // time.
          continue;
        }
        BasicBlock *bb = *bi;
        for (BasicBlock::iterator ii = bb->begin();
          ii != bb->end(); ++ii) {
          Instruction *inst = ii;
    	    std::string type_str;
    	    llvm::raw_string_ostream rso(type_str);
          rso << *inst << "\n";
          std::cerr << rso.str() << std::endl;
        }
      }
      std::cerr << "\n--------------- end loop  body---------------------" << std::endl;
      */

      // Finding functions amenable to NPU.
      for (Loop::block_iterator bi = loop->block_begin();
            bi != loop->block_end(); ++bi) {
        BasicBlock *bb = *bi;
        for (BasicBlock::iterator ii = bb->begin();
          ii != bb->end(); ++ii) {
          Instruction *inst = ii;
          const CallInst *c_inst = dyn_cast<CallInst>(inst);
      std::string type_str;
      llvm::raw_string_ostream rso(type_str);
      rso << *inst;
      std::cerr << "\n" << rso.str() << std::endl;
          if (!c_inst) continue;
          Function *callee = c_inst->getCalledFunction();

          //testSubFunctions(callee, loop);
          if (!isa<IntrinsicInst>(inst) && !AI->isWhiteList(callee->getName()) && AI->isPrecisePure(callee)) {
            std::cerr << callee->getName().str() << " is precise pure!\n\n" << std::endl;
            if (!find_inst(inst)) {
              loops_to_npu.push_back(loop);
              calls_to_npu.push_back(inst);
            }
          } else {
            std::cerr << callee->getName().str() << " is NOT precise pure!\n\n" << std::endl;
          }

        } // for instructions

      } // for basic blocks

      for (int i = 0; i < calls_to_npu.size(); ++i) {
        std::cerr << "++++ begin" << std::endl;
        CallInst *c = dyn_cast<CallInst>(calls_to_npu[i]);
        std::cerr << "Function npu: " << (c->getCalledFunction()->getName()).str() << std::endl;
        modified = tryToNPU(loops_to_npu[i], calls_to_npu[i]);
        std::cerr << "++++ middle" << std::endl;
        for (Loop::block_iterator bi = loop->block_begin(); bi != loop->block_end(); ++bi) {
          BasicBlock *bb = *bi;
          for (BasicBlock::iterator ii = bb->begin(); ii != bb->end(); ++ii) {
            Instruction *inst = ii;
            std::string type_str;
            llvm::raw_string_ostream rso(type_str);
            rso << *inst;
            std::cerr << "\n" << rso.str() << std::endl;
          }
        }
        std::cerr << "++++ end" << std::endl;
      }
      calls_to_npu.clear();
      loops_to_npu.clear();

      return modified;

  } // tryToOptimizeLoop

  IntegerType *getNativeIntegerType() {
    DataLayout layout(module->getDataLayout());
    return Type::getIntNTy(module->getContext(),
                            layout.getPointerSizeInBits());
  }

  bool tryToNPU(Loop *loop, Instruction *inst) {
    // We need a loop latch to jump to after reading oBuff
    // and executing the instructions after the function call.
    if (!loop->getLoopLatch() || !loop->getLoopPreheader())
      return false;

    if (!inst->getType()->isIntegerTy() &&
        !inst->getType()->isVoidTy() &&
        !inst->getType()->isFloatTy())
      return false;

    const CallInst *c_inst = dyn_cast<CallInst>(inst);
    Function *f = c_inst->getCalledFunction();

    // Region is the entire called function. All of its instructions.
    // "Stores" is all the store instructions in the called function.
    std::set<Instruction *> region;
    std::vector<StoreInst *> stores;
    for (Function::iterator bi = f->begin(); bi != f->end(); ++bi) {
      BasicBlock *bb = bi;
      Loop *in_loop = LI->getLoopFor(bb);

      // No escaping stores inside loops allowed for now.
      // We would need to determine how many times the store would
      // be executed and whether it would write to the same address
      // or not in order to know how much buffer space we need.
      if (in_loop) return false;

      for (BasicBlock::iterator ii = bb->begin(); ii != bb->end(); ++ii) {
        Instruction *inst = ii;
        region.insert(ii);
        if (StoreInst *s = dyn_cast<StoreInst>(inst))
          stores.push_back(s);
      }
    }
    int n_stores = stores.size();

    // Get a list of all the stores that escape the function
    // (approx or not).
    std::vector<StoreInst *> escaped_stores;
    for (int i = 0; i < n_stores; ++i)
      if (AI->storeEscapes(stores[i], region, false))
        escaped_stores.push_back(stores[i]);

    const int output_threshold = 5;
    if (escaped_stores.size() > output_threshold)
      escaped_stores.clear();

    // The list of parameters as in the function prototype.
    // Both in string and Value formats.
    // TODO: Not sure if the string version is needed. If not, remove it.
    typedef iplist<Argument> ArgumentListType;
    std::vector<std::string> callee_args_str;
    std::vector<Value *> callee_args_value;
    for (ArgumentListType::iterator ai = (f->getArgumentList()).begin();
          ai != (f->getArgumentList()).end();
          ++ai) {
      std::string type_str;
      llvm::raw_string_ostream rso(type_str);
      rso << *ai;
      callee_args_str.push_back(rso.str());
      callee_args_value.push_back(ai);
    }

    /*
    AliasSetTracker *st = new AliasSetTracker(*AA);
    for (Function::iterator bi = f->begin(); bi != f->end(); ++bi)
      st->add(*bi);
    //if (st->getAliasSetForPointerIfExists(inst, AA->getDataLayout()->getTypeStoreSize(callee_args_value[i]->getType()), NULL))
    std::cerr << "alias sets dump" << std::endl;
    for (AliasSetTracker::iterator I = st->begin(); I != st->end(); ++I) {
      AliasSet &AS = *I;
      std::cerr << "=============== one more set" << std::endl;
      AS.dump();
    }
    delete st;
    */

    // Number of inputs passed to the function call.
    // It looks like the code of the function itself is the last element.
    unsigned int n = inst->getNumOperands() - 1;

    // For pointer arguments, determine first which ones will
    // be passed as input to the NPU. Rules are:
    // 1 - If it can be determined from the function signature
    // how many elements the argument expects AND it's a small number.
    // e.g void f(int a[5]);
    // 2 - If it can be determined *exactly where the pointer points to*
    // and it's a small declaration.
    std::vector<int> op_size;
    const int input_size_threshold = 5;
    std::ifstream file("accept-npuArrayArgs-info.txt");
    if (file.is_open()) {
      while (file.good()) {
        std::string line;
        file >> line;
        if (line == f->getName().str())
          break;
      }
    }
    for (unsigned int i = 0; i < n; ++i) {
      int n_array;
      file >> n_array;
      op_size.push_back(n_array < input_size_threshold ? n_array : 0);
    }
    file.close();


    // Check the declaration (alloca) to see if it is a "normal" variable (i.e. not a pointer)
    // TODO: Also check whether it's an array whose size we can determine (and also check
    // whether p[] allow p to point to anywhere else...
    for (unsigned int i = 0; i < n; ++i) {
      Type *type = inst->getOperandUse(i)->getType();
      if (!type->isPointerTy())
        continue;

      if (!isa<AllocaInst>(*inst->getOperandUse(i)))
        continue;

      AllocaInst *allocai = dyn_cast<AllocaInst>(inst->getOperandUse(i));
      Type *allocat = allocai->getAllocatedType();
      if (type->isFloatingPointTy() || type->isIntegerTy())
        op_size[i] = 1;
    }


    // The list of arguments as in the instruction that calls
    // the function.
    std::vector<Value *> caller_args;
    for (int i = 0; i < c_inst->getNumArgOperands(); ++i)
      caller_args.push_back(c_inst->getArgOperand(i));

    std::vector<bool> is_output_arg;
    bool has_output_arg = false;
    for (int i = 0; i < caller_args.size(); ++i) {
      is_output_arg.push_back(false);
      for (int j = 0; j < escaped_stores.size(); ++j) {
        if ((callee_args_value[i])->getName().str() == (escaped_stores[j])->getPointerOperand()->getName().str()) {
          has_output_arg = true;
          is_output_arg[i] = true;
          break;
        }
      }
    }

    int first_output_arg, last_output_arg = -1;
    for (int i = 0; i < is_output_arg.size(); ++i) {
      if (is_output_arg[i]) {
        first_output_arg = i;
        break;
      }
    }
    for (int i = (is_output_arg.size() - 1); i >= 0; --i) {
      if (is_output_arg[i]) {
        last_output_arg = i;
        break;
      }
    }
    int n_ptr_args = 0;
    for (int i = 0; i < is_output_arg.size(); ++i)
      if (is_output_arg[i])
        ++n_ptr_args;

    // Assume a constant buffer size for now.
    int buffer_size = 64;
    unsigned int ibuff_addr = 0xFFFF8000;
    unsigned int obuff_addr = 0xFFFFF000;
    IntegerType *nativeInt = getNativeIntegerType();
    IRBuilder<> builder(module->getContext());

    // First insert all the needed allocas in the beginning of
    // the function (which is where they MUST be).
    builder.SetInsertPoint(loop->getHeader()->getParent()->getEntryBlock().begin());

    // A counter for buffering the function inputs.
    AllocaInst *counterAlloca = builder.CreateAlloca(nativeInt,
                                                      0,
                                                      "npu_ibuff_counter");

    // iBuff and oBuff
    AllocaInst *iBuffAlloca = builder.CreateAlloca(Type::getFloatPtrTy(module->getContext()),
                                                   0,
                                                   "npu_ibuff_alloca");
    AllocaInst *oBuffAlloca = builder.CreateAlloca(Type::getFloatPtrTy(module->getContext()),
                                                   0,
                                                   "npu_obuff_alloca");

    // A counter for reading the function outputs.
    AllocaInst *outLoopCounterAlloca = builder.CreateAlloca(nativeInt,
                                                            0,
                                                            "npu_out_loop_counter");

    // An auxiliary buffer to store function arguments' addresses.
    AllocaInst *addrBuffAlloca = builder.CreateAlloca(Type::getFloatPtrTy(module->getContext()),
                                                      ConstantInt::get(nativeInt, buffer_size, false),
                                                      "npu_addrBuff_alloca");

    AllocaInst *iAddrCounterAlloca = builder.CreateAlloca(nativeInt,
                                                          0,
                                                          "npu_iAddr_counter_alloca");

    AllocaInst *oAddrCounterAlloca = builder.CreateAlloca(nativeInt,
                                                          0,
                                                          "npu_oAddr_counter_alloca");

    // Initialize oBuff, iBuff and iBuff counter
    builder.SetInsertPoint(loop->getLoopPreheader()->getTerminator());
    Constant *constInt = ConstantInt::get(nativeInt, ibuff_addr, false);
    Value *constPtr = ConstantExpr::getIntToPtr(constInt,
                                                Type::getFloatPtrTy(module->getContext()));
    builder.CreateStore(constPtr, iBuffAlloca, true);
    builder.CreateStore(ConstantInt::get(nativeInt, 0, false), counterAlloca);

    if (n_ptr_args)
      builder.CreateStore(ConstantInt::get(nativeInt, 0, false), iAddrCounterAlloca);

    // Start inserting instructions to buffer the inputs of the function call
    // right before the call itself.
    builder.SetInsertPoint(inst);

    Value *v;
    Value *load;
    Value *iAddrChainCounter;
    Value *iAddrChainPosition;

    int first_input_arg, last_input_arg = -1;
    for (int i = 0; i < op_size.size(); ++i) {
      Type *type = inst->getOperandUse(i)->getType();
      if (!type->isPointerTy() || op_size[i]) {
        first_input_arg = i;
        break;
      }
    }
    for (int i = (op_size.size() - 1); i >= 0; --i) {
      Type *type = inst->getOperandUse(i)->getType();
      if (!type->isPointerTy() || op_size[i]) {
        last_input_arg = i;
        break;
      }
    }

    // The input buffer is always an array of floats. So, any input that is not a float
    // must be converted.
    int total_buffered = 0;
    for (unsigned int i = 0; i < n; ++i) {
      std::string s = "npu_conv_" + i;
      Type *type = inst->getOperandUse(i)->getType();

      // Get the next argument and convert it if it's not float.
      v = inst->getOperandUse(i);
      if (type->isIntegerTy()) {
        IntegerType *it = static_cast<IntegerType *>(type);
        if (it->getSignBit())
          v = builder.CreateSIToFP(v, Type::getFloatTy(module->getContext()), s.c_str());
        else
          v = builder.CreateUIToFP(v, Type::getFloatTy(module->getContext()), s.c_str());
      } else if (type->isDoubleTy()) {
        v = builder.CreateFPTrunc(v, Type::getFloatTy(module->getContext()), s.c_str());
      } else if (type->isPointerTy()) {
        // TODO: Didn't find a way to discover the type of the pointer.
        // If this is possible, convert the value loaded below.
        v = builder.CreateLoad(v);
      }

      // Only once (when i == 0) load iBuffAlloca
      // TODO: maybe this can be optimized. We already know the initial address
      // of iBuff and oBuff so we can just use a constant as the initial value
      // and go from there. At this point register allocation hasn't been
      // performed yet, so we have to load/store everything though.
      if (i == first_input_arg)
        load = builder.CreateLoad(iBuffAlloca, true, "npu_load_ibuff");
      if (i == first_output_arg)
        iAddrChainCounter = builder.CreateLoad(iAddrCounterAlloca, false, "npu_load_iAddrC");

      if (is_output_arg[i]) {
        s = "npu_iAddrGEP_" + i;
        iAddrChainPosition = builder.CreateInBoundsGEP(addrBuffAlloca,
                                                      iAddrChainCounter,
                                                      s.c_str());
        builder.CreateStore(inst->getOperandUse(i), iAddrChainPosition);
        s = "npu_iAddrCounter_add_" + i;
        iAddrChainCounter = builder.CreateAdd(iAddrChainCounter,
                                              ConstantInt::get(nativeInt, 1, false),
                                              s.c_str());
      }

      Value *GEP;
      if (!type->isPointerTy() || op_size[i]) {
        int n_to_buff = (type->isPointerTy()) ? op_size[i] : 1;

        for (int j = 0; j < n_to_buff; ++j) {
          // Now that the input has been converted and iBuff has been loaded, get
          // the address of the next iBuff element.
          s = "npu_iBuffGEP_" + i;
          // is the 1 signed? (the true parameter)
          GEP = builder.CreateInBoundsGEP(load,
                                          ConstantInt::get(nativeInt, 1, true),
                                          s.c_str());

          // Store the (converted) input in the current iBuff position.
          builder.CreateStore(v, load); //is this store volatile?

          // The next iBuff element to be written is the result of the
          // GEP instruction above.
          load = GEP;

          if (j != n_to_buff - 1) {
            s = "npu_auxGEP_" + (j+1);
            Value *auxGEP = builder.CreateInBoundsGEP(inst->getOperandUse(i),
                                                      ConstantInt::get(nativeInt, j+1),
                                                      s.c_str());
            v = builder.CreateLoad(auxGEP);
          }
        }
        total_buffered += n_to_buff;

      }

      // After storing the last input, store the final address of iBuff.
      if (i == last_input_arg)
        builder.CreateStore(GEP, iBuffAlloca, true);
      if (i == last_output_arg)
        builder.CreateStore(iAddrChainCounter, iAddrCounterAlloca);
    }

    // This will be used later in case the call instruction is
    // the last instruction of its basic block (it shouldn't be).
    // If this is the case, then the basic block cannot have
    // more than one successor.
    BasicBlock *after_callBB_tmp = *((AI->successorsOf(inst->getParent())).begin());

    // If we buffered any input (n != 0) we have to check whether
    // the buffer is not full yet.
    // TODO: for now we're assuming the buffer size is a multiple of
    // the number of inputs.
    // Since we have to check the iBuff counter at the end, we have to
    // split the current basic block. It's always safe to do so because
    // we have already buffered something (so there're instructions
    // *before* the call) and the call itself will be on the other BB.
    BasicBlock *callBB = inst->getParent();
    BasicBlock *before_callBB = inst->getParent();
    if (n != 0) {

      // First get an iterator to the call inst and split the BB there.
      for (BasicBlock::iterator ii = before_callBB->begin();
            ii != before_callBB->end();
            ++ii) {
        Instruction *i = ii;
        if (i != inst)
          continue;

        callBB = before_callBB->splitBasicBlock(
            ii,
            "npu_call_block"
        );
        break;
      }

      // The new BB (without the call inst) will have an unconditional
      // branch at the end to the call block. This will have to be replaced
      // by a conditional branch that will either jump to the call block
      // (if the iBuff is full) or to the loop latch to buffer more inputs.
      Instruction *new_uncond_branch_term = before_callBB->getTerminator();
      builder.SetInsertPoint(new_uncond_branch_term);

      // Load the input reading loop's induction variable.
      v = builder.CreateLoad(counterAlloca, "npu_counter_load");

      // Add the number of inputs stored in iBuff.
      v = builder.CreateAdd(v,
                            ConstantInt::get(nativeInt, total_buffered, false),
                            "npu_counter_add");

      // Store the result back.
      builder.CreateStore(v, counterAlloca);

      // Compare the new value of the induction variable to the buffer size.
      v = builder.CreateICmpEQ(v,
                                ConstantInt::get(nativeInt,
                                                  buffer_size,
                                                  false),
                                "npu_icmp_iBuffSize");

      // If they're the same, the buffer is full and we jump to
      // the call block. Otherwise, jump to the loop latch to buffer
      // more inputs.
      builder.CreateCondBr(v, callBB, loop->getLoopLatch());

      // Remove the unconditional branch and add the new block to the loop.
      new_uncond_branch_term->eraseFromParent();
      loop->addBasicBlockToLoop(callBB, LI->getBase());
    }

    // We will replace the call inst by the assembly code to invoke the
    // npu last because we will still use "inst" as a reference to the call inst.

    // =================================================================
    // Now we deal with what happens after the call.
    // Read the results in oBuff and execute whatever code that
    // comes after the call.
    // =================================================================

    // We need to insert a new BB right after the call *instruction*
    // which will read the output (and also be the
    // jump target of the loop that reads the output and executes remaining code.)
    //
    // The best way is to split the call BB. This will not work if the call is
    // the only instruction left in this BB. Again, this should not happen, but
    // we handle this case as well by creating an empty BB.
    BasicBlock *after_callBB;
    bool created_emptyBB = false;
    if (callBB->getTerminator() != inst) {
      // In this case the call is not the only instruction left, so we can split.
      Instruction *after_call_inst;
      for (BasicBlock::iterator ii = callBB->begin();
            ii != callBB->end();
            ++ii) {
        Instruction *i = ii;
        if (i != inst)
          continue;

        after_call_inst = ++ii;
        break;
      }


      after_callBB = callBB->splitBasicBlock(
          after_call_inst,
          "npu_after_call_block"
      );
    } else {
      // In this case we have to create a new BB which comes before
      // the BB that comes after the call BB.
      after_callBB = BasicBlock::Create(
          module->getContext(),
          "npu_after_call_block",
          loop->getHeader()->getParent(),
          after_callBB_tmp
      );
      created_emptyBB = true;
    }
    loop->addBasicBlockToLoop(after_callBB, LI->getBase());

    // We do, in the end of the call BB, everything we should do
    // only once i.e., not inside the loop that will read oBuff and
    // execute remaining code.
    builder.SetInsertPoint(callBB->getTerminator());

    Constant *constInt2 = ConstantInt::get(nativeInt, obuff_addr, false);
    Value *constPtr2 = ConstantExpr::getIntToPtr(constInt2,
                                             Type::getFloatPtrTy(module->getContext()));
    builder.CreateStore(constPtr2, oBuffAlloca, true);

    // Initialize "oBuff read" loop induction variable
    builder.CreateStore(
        ConstantInt::get(nativeInt, 0, false),
        outLoopCounterAlloca
    );
    builder.CreateStore(
        ConstantInt::get(nativeInt, 0, false),
        iAddrCounterAlloca
    );
    builder.CreateStore(
        ConstantInt::get(nativeInt, 0, false),
        oAddrCounterAlloca
    );

    // Load the iBuff writing induction variable so we know how many
    // positions we wrote to in the iBuff.
    // Then divide this by the number of inputs the function call uses.
    // This gives the number of times the function would have been called,
    // which is the number of times we should read from oBuff.
    // We'll use this latter
    Value *ibuff_used = builder.CreateLoad(counterAlloca,
                                           "npu_ibuff_used");
    ibuff_used = builder.CreateUDiv(ibuff_used,
        ConstantInt::get(nativeInt, total_buffered, false));


    // Reset "iBuff read counter" for the next time.
    // Also reset iBuff itself.
    builder.CreateStore(
        ConstantInt::get(nativeInt, 0, false),
        counterAlloca
    );
    constInt = ConstantInt::get(nativeInt, ibuff_addr, false);
    constPtr = ConstantExpr::getIntToPtr(constInt,
                                         Type::getFloatPtrTy(module->getContext()));
    builder.CreateStore(constPtr, iBuffAlloca, true);

    // Now we move to the block after the call BB (probably split
    // from it) to start reading the oBuff.

    // Insert instructions in the beginning (the other instructions in
    // in the BB are part of the "remaining" code that has to be executed
    // after the function call (and after oBuff is read).
    if (created_emptyBB)
      builder.SetInsertPoint(after_callBB);
    else
      builder.SetInsertPoint(after_callBB->begin());

    Value *oAddrChainCounter;
    Value *oAddrChainPosition;
    int ptr_args_i = 0;
    // Now we read all the output values generated by *one* call
    // For each output (escaped_stores) we:
    //
    // Alternatively, we can see whether anyone depends on inst
    bool gotRetVal = false;
    Value *retVal;
    if (f->getReturnType()->isIntegerTy() || f->getReturnType()->isFloatingPointTy()) {
      gotRetVal = true;
      load = builder.CreateLoad(oBuffAlloca, true, "npu_load_obuff");
      Value *GEP;
      std::string s1 = "npu_gepRetVal_oBuff";
      std::string s2 = "npu_loadRetVal_oBuffResult";
      GEP = builder.CreateInBoundsGEP(load,
                                      ConstantInt::get(nativeInt, 1, true),
                                      s1.c_str());
      retVal = builder.CreateLoad(load, s2.c_str());
      if (!escaped_stores.size())
        builder.CreateStore(GEP, oBuffAlloca, true);

      load = GEP;
    }
    for (int i = 0; i < escaped_stores.size(); ++i) {
      // First we store the function arguments.
      // For now we only consider escaped stores to function arguments.
      // TODO: remove this restriction.
      if (i == 0 && !gotRetVal)
        // Load the oBuff pointer.
        load = builder.CreateLoad(oBuffAlloca, true, "npu_load_obuff");
      if (i == 0 && n_ptr_args)
        oAddrChainCounter = builder.CreateLoad(oAddrCounterAlloca, false, "npu_load_oAddrC");

      // Then, get the address of next position of oBuff
      Value *GEP;
      std::string s1 = "npu_gep_oBuff";
      std::string s2 = "npu_load_oBuffResult";
      GEP = builder.CreateInBoundsGEP(load,
                                      ConstantInt::get(nativeInt, 1, true),
                                      s1.c_str());

      // Then, read the oBuff value
      Value *v = builder.CreateLoad(load, s2.c_str());

      if (ptr_args_i < n_ptr_args) {
        std::string s3 = "npu_oAddrGEP_" + ptr_args_i;
        oAddrChainPosition = builder.CreateInBoundsGEP(addrBuffAlloca,
                                                        oAddrChainCounter,
                                                        s3.c_str());
        Value *addr_buffer_value = builder.CreateLoad(oAddrChainPosition);
        builder.CreateStore(v, addr_buffer_value, false);
        s3 = "npu_oAddrCounter_add_" + ptr_args_i;
        oAddrChainCounter = builder.CreateAdd(oAddrChainCounter,
                                              ConstantInt::get(nativeInt, 1, false),
                                              s3.c_str());
        ++ptr_args_i;
      }

      if (i == (escaped_stores.size() - 1)) {
        builder.CreateStore(GEP, oBuffAlloca, true);
        if (n_ptr_args)
          builder.CreateStore(oAddrChainCounter, oAddrCounterAlloca);
      }

      // On the next iteration we read from the next oBuff position
      // (already found out by GEP).
      load = GEP;
    }

    // Replace all the uses of inst (the call) by retVal
    if (gotRetVal) {
      if (inst->getType()->isIntegerTy()) {
        IntegerType *it = static_cast<IntegerType *>(inst->getType());
        if (it->getSignBit())
          retVal = builder.CreateFPToSI(retVal, nativeInt, "npu_conv_retval");
        else
          retVal = builder.CreateFPToUI(retVal, nativeInt, "npu_conv_retval");
      }
      inst->replaceAllUsesWith(retVal);
    }

    // Now that we have read the outputs for *one* function call,
    // we have to:
    // (1) - execute the code that comes after the call
    // (2) - check whether we are done reading the buffer. If
    // so, we jump tp the loop latch. Otherwise, we jump
    // to this BB again.
    //
    // To accomplish this we simply create a new BB whose successor
    // is the loop latch and whose predecessors are all the BB's that
    // appear after the call inst and jump to the latch.

    // So, starting by the current BB, we find all the BB's that
    // jump to the latch by using BFS.
    std::set<BasicBlock *> jump_to_latch;
    
    std::queue<BasicBlock *> bfs_q;
    std::set<BasicBlock *> seen;

    seen.insert(after_callBB);
    bfs_q.push(after_callBB);

    while (!bfs_q.empty()) {
      BasicBlock *bb = bfs_q.front();
      bfs_q.pop();
      std::set<BasicBlock *> suc = AI->successorsOf(bb);
      for (std::set<BasicBlock *>::iterator it = suc.begin();
           it != suc.end();
           ++it) {
        BasicBlock *child = *it;
        if (seen.count(child))
          continue;

        if (child == loop->getLoopLatch()) {
          jump_to_latch.insert(bb);
        } else if (!loop->contains(child)) {
          continue;
        } else {
          seen.insert(child);
          bfs_q.push(child);
        }
      }
    }

    // Now that we have all the BB's that jump to the latch,
    // create the new BB that will come before the latch and
    // check whether we're done reading the oBuff.
    BasicBlock *checkBB = BasicBlock::Create(
        module->getContext(),
        "npu_checkBB",
        loop->getHeader()->getParent(),
        loop->getLoopLatch()
    );

    // For each BB that jumps to the latch, change the corresponding
    // successor to be the newly created BB.
    for (std::set<BasicBlock *>::iterator bi = jump_to_latch.begin();
         bi != jump_to_latch.end();
         ++bi) {

      if ((*bi) == before_callBB)
        continue;

      // Get the last inst, which should be a branch and change the
      // jump target to the new BB.
      Instruction *term = (*bi)->getTerminator();
      if (isa<BranchInst>(term)) {
        BranchInst *branch = static_cast<BranchInst *>(term);
        for (int i = 0; i < branch->getNumSuccessors(); ++i) {
          if (branch->getSuccessor(i) == loop->getLoopLatch())
            branch->setSuccessor(i, checkBB);
        }
      } else {
        // If the last inst is not a branch (weird), simply
        // insert an unconditional branch to the new BB.
        builder.SetInsertPoint(term);
        builder.CreateBr(checkBB);
      }
    }

    // Now, populate the new BB with the instructions needed.
    builder.SetInsertPoint(checkBB);

    // Load the induction variable that tells how many times we
    // have read *all* the outputs generated by *one* function call.
    v = builder.CreateLoad(
            outLoopCounterAlloca,
            "out_loop_count_load"
    );

    // Increment the value by one.
    v = builder.CreateAdd(
            v,
            ConstantInt::get(nativeInt, 1, false),
            "out_loop_count_add"
    );

    // Store the incremented value back
    builder.CreateStore(
        v,
        outLoopCounterAlloca
    );

    // Check whether we have read the outputs of a call
    // "number of calls" times.
    v = builder.CreateICmpEQ(v, ibuff_used, "npu_icmp_obuff_count");

    // If so, jump to the loop latch.
    // Otherwise, read more outputs.
    builder.CreateCondBr(v, loop->getLoopLatch(), after_callBB);

    loop->addBasicBlockToLoop(checkBB, LI->getBase());

    builder.SetInsertPoint(inst);

    InlineAsm *asm1 = InlineAsm::get(FunctionType::get(builder.getVoidTy(), false),
                                      StringRef("dsb"),
                                      StringRef(""),
                                      true);
    InlineAsm *asm2 = InlineAsm::get(FunctionType::get(builder.getVoidTy(), false),
                                      StringRef("SEV"),
                                      StringRef(""),
                                      true);
    InlineAsm *asm3 = InlineAsm::get(FunctionType::get(builder.getVoidTy(), false),
                                      StringRef("WFE"),
                                      StringRef(""),
                                      true);
    InlineAsm *asm4 = InlineAsm::get(FunctionType::get(builder.getVoidTy(), false),
                                      StringRef("WFE"),
                                      StringRef(""),
                                      true);
    builder.CreateCall(asm1);
    builder.CreateCall(asm2);
    builder.CreateCall(asm3);
    builder.CreateCall(asm4);

    inst->eraseFromParent();
    return true;
  }

};
}
/*
 *
            std::string type_str;
            llvm::raw_string_ostream rso(type_str);
            rso << *ii;
            std::cerr << "\n" << rso.str() << std::endl;
 */

char LoopNPUPass::ID = 0;
LoopPass *llvm::createLoopNPUPass() { return new LoopNPUPass(); }

