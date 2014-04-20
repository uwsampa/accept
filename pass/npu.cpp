#include "accept.h"

#include "llvm/Analysis/LoopPass.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Argument.h"

#include <sstream>
#include <iostream>

using namespace llvm;

namespace {
  struct LoopNPUPass : public LoopPass {
    static char ID;
    ACCEPTPass *transformPass;
    ApproxInfo *AI;
    Module *module;
    llvm::raw_fd_ostream *log;
    LoopInfo *LI;

    LoopNPUPass() : LoopPass(ID) {}

    virtual bool doInitialization(Loop *loop, LPPassManager &LPM) {
      transformPass = (ACCEPTPass*)sharedAcceptTransformPass;
      AI = transformPass->AI;
      log = AI->log;
      return false;
    }
    virtual bool runOnLoop(Loop *loop, LPPassManager &LPM) {
      if (transformPass->shouldSkipFunc(*(loop->getHeader()->getParent())))
          return false;
      module = loop->getHeader()->getParent()->getParent();
      LI = &getAnalysis<LoopInfo>();
      return tryToOptimizeLoop(loop);
    }
    virtual bool doFinalization() {
      return false;
    }
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      LoopPass::getAnalysisUsage(AU);
      AU.addRequired<LoopInfo>();
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

    // Not handling recursive or cyclic function calls.
    void testSubFunctions(Function *f) {
      for (Function::iterator bi = f->begin(); bi != f->end(); ++bi) {
        BasicBlock *bb = bi;
        for (BasicBlock::iterator ii = bb->begin();
            ii != bb->end(); ++ii) {
          Instruction *inst = ii;
          const CallInst *c_inst = dyn_cast<CallInst>(inst);
          if (!c_inst) continue;

          Function *callee = c_inst->getCalledFunction();

          testSubFunctions(callee);
          if (AI->isPrecisePure(callee)) {
            std::cerr << callee->getName().str() << " is precise pure!\n\n" << std::endl;
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
      Instruction *prev_inst = NULL;
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
          /*
    std::cerr << "------------------------ uses --------------" << std::endl;
      for (Loop::block_iterator bi2 = loop->block_begin();
            bi2 != loop->block_end(); ++bi2) {
        BasicBlock *bb2 = *bi2;
        for (BasicBlock::iterator ii2 = bb2->begin();
          ii2 != bb2->end(); ++ii2) {
          Instruction *inst2 = ii2;
          Value *v = (Value*) inst2;
      std::string type_str;
      llvm::raw_string_ostream rso(type_str);
      rso << *inst2 << "\n";
      rso << "===== value: " << *v;
      std::cerr << "\n" << rso.str() << std::endl;
        }
      }


    unsigned int n = inst->getNumOperands();
    for (unsigned int i = 0; i < n; ++i) {
      std::string type_str;
      llvm::raw_string_ostream rso(type_str);
      rso << *(inst->getOperandUse(i));
      std::cerr << "\n" << rso.str() << std::endl;
    }
    std::cerr << "------------------------ end uses --------------" << std::endl;
    */

          Function *callee = c_inst->getCalledFunction();

          testSubFunctions(callee);
          if (AI->isPrecisePure(callee)) {
            std::cerr << callee->getName().str() << " is precise pure!\n\n" << std::endl;
            tryToNPU(loop, inst, prev_inst);
          } else {
            std::cerr << callee->getName().str() << " is NOT precise pure!\n\n" << std::endl;
          }

        prev_inst = inst;
        } // for instructions

      } // for basic blocks

      // Finding smaller regions inside the loop.
      /*
      for (Loop::block_iterator bi = loop->block_begin();
            bi != loop->block_end(); ++bi) {
        std::set<BasicBlock *> region;
        build_region(*bi, region);
        if ((AI->preciseEscapeCheck(region)).size() == 0) { // no blockers
          std::cerr << "\nFound region of approx code" << std::endl;
        }
      } // end basic blocks
      */

  } // tryToOptimizeLoop

  IntegerType *getNativeIntegerType() {
    DataLayout layout(module->getDataLayout());
    return Type::getIntNTy(module->getContext(),
                            layout.getPointerSizeInBits());
  }

  void tryToNPU(Loop *loop, Instruction *inst, Instruction *prev_inst) {
    if (!loop->getLoopLatch())
      return;

    const CallInst *c_inst = dyn_cast<CallInst>(inst);
    Function *f = c_inst->getCalledFunction();

    std::set<Instruction *> region;
    std::vector<StoreInst *> stores;
    for (Function::iterator bi = f->begin(); bi != f->end(); ++bi) {
      BasicBlock *bb = bi;
      Loop *in_loop = LI->getLoopFor(bb);
      if (in_loop) return;
      for (BasicBlock::iterator ii = bb->begin(); ii != bb->end(); ++ii) {
        Instruction *inst = ii;
        region.insert(ii);
        if (StoreInst *s = dyn_cast<StoreInst>(inst))
          stores.push_back(s);
      }
    }
    int n_stores = stores.size();
    std::vector<StoreInst *> escaped_stores;
    for (int i = 0; i < n_stores; ++i)
      if (AI->storeEscapes(stores[i], region, false))
        escaped_stores.push_back(stores[i]);

    std::vector<std::string> outputs;
    std::vector<int> n_written;

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

    std::vector<Value *> caller_args;
    for (int i = 0; i < c_inst->getNumArgOperands(); ++i)
      caller_args.push_back(c_inst->getArgOperand(i));


    int buffer_size = 1024;
    IRBuilder<> builder(module->getContext());
    builder.SetInsertPoint(loop->getLoopPreheader()->getParent()->getEntryBlock().begin());
    IntegerType *nativeInt = getNativeIntegerType();
    AllocaInst *counterAlloca = builder.CreateAlloca(nativeInt,
                                                      0,
                                                      "npu_ibuff_counter");
    AllocaInst *iBuffAlloca = builder.CreateAlloca(Type::getFloatPtrTy(module->getContext()),
                                                   0,
                                                   "npu_ibuff_alloca");
    AllocaInst *oBuffAlloca = builder.CreateAlloca(Type::getFloatPtrTy(module->getContext()),
                                                   0,
                                                   "npu_obuff_alloca");
    AllocaInst *outLoopCounterAlloca = builder.CreateAlloca(nativeInt,
                                                            0,
                                                            "npu_out_loop_counter");

    unsigned int n = inst->getNumOperands();

    builder.SetInsertPoint(prev_inst);
    builder.CreateStore(ConstantInt::get(nativeInt, 0, false), counterAlloca);
    Value *v;
    Value *load;
    std::vector<Type *> conv_ty;
    std::vector<int> converted;
    // 1 - SI
    // 2 - UI
    // 3 - Double truncation
    // 4 - No conversion
    for (unsigned int i = 0; i < n; ++i) {
      std::string s = "npu_conv_" + i;
      Type *type = inst->getOperandUse(i)->getType();
      conv_ty[i] = type;
      v = inst->getOperandUse(i);
      if (type->isIntegerTy()) {
        IntegerType *it = static_cast<IntegerType *>(type);
        if (it->getSignBit()) {
          converted[i] = 1;
          v = builder.CreateSIToFP(v,
                                    Type::getFloatTy(module->getContext()),
                                    s.c_str());
        }
        else {
          converted[i] = 2;
          v = builder.CreateUIToFP(v,
                                    Type::getFloatTy(module->getContext()),
                                    s.c_str());
        }
      } else if (type->isDoubleTy()) {
        converted[i] = 3;
        v = builder.CreateFPTrunc(v,
                                  Type::getFloatTy(module->getContext()),
                                  s.c_str());
      } else {
        converted[i] = 4;
      }

      unsigned int ibuff_addr = 0xFFFF8000;
      unsigned int obuff_addr = 0xFFFFF000;
      if (i == 0) {
        Constant *constInt = ConstantInt::get(nativeInt, ibuff_addr, false);
        Value *constPtr = ConstantExpr::getIntToPtr(constInt,
                                                    Type::getFloatPtrTy(module->getContext()));
        builder.CreateStore(constPtr, iBuffAlloca, true);
        constInt = ConstantInt::get(nativeInt, obuff_addr, false);
        constPtr = ConstantExpr::getIntToPtr(constInt,
                                             Type::getFloatPtrTy(module->getContext()));
        builder.CreateStore(constPtr, oBuffAlloca, true);
        load = builder.CreateLoad(iBuffAlloca, true, "npu_load_ibuff");
      }

      s = "npu_iBuffGEP_" + i;
      Value *GEP;
      // is the 1 signed? (the true parameter)
      GEP = builder.CreateInBoundsGEP(load,
                                      ConstantInt::get(nativeInt, 1, true),
                                      s.c_str());

      if (i == (n - 1))
        builder.CreateStore(GEP, iBuffAlloca, true);

      builder.CreateStore(v, load, false); //is this store volatile?

      load = GEP;
    }

    // This will be used later in case the call instruction is
    // the last instruction of its basic block.
    // If this is the case, then the basic block cannot have
    // more than one successor.
    BasicBlock *after_callBB_tmp = (successorsOf(inst->getParent())).begin();

    BasicBlock *newBB;
    if (n != 0) {
      BasicBlock *currBB = inst->getParent();
      for (BasicBlock::iterator ii = currBB->begin();
            ii != currBB->end();
            ++ii) {
        Instruction *i = ii;
        if (i != inst)
          continue;

        newBB = currBB->splitBasicBlock(
            ii,
            "npu_call_block"
        );
        break;
      }
      Instruction *new_uncond_branch_term = currBB->getTerminator();
      builder.SetInsertPoint(new_uncond_branch_term);
      v = builder.CreateLoad(counterAlloca, "npu_counter_load");
      v = builder.CreateAdd(v,
                            ConstantInt::get(nativeInt, n, false),
                            "npu_counter_add");
      builder.CreateStore(v, counterAlloca);
      v = builder.CreateICmpEQ(v,
                                ConstantInt::get(nativeInt,
                                                  buffer_size,
                                                  false),
                                "npu_icmp_iBuffSize");
      builder.CreateCondBr(v, newBB, loop->getLoopLatch());

      new_uncond_branch_term->eraseFromParent();
      loop->addBasicBlockToLoop(newBB, LI->getBase());
    }

    BasicBlock *callBB = newBB;
    BasicBlock *after_callBB;
    bool created_emptyBB = false;
    if (callBB->getTerminator() != inst) {
      Instruction *after_call_inst;
      for (BasicBlock::iterator ii = callBB->begin();
            ii != currBB->end();
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
      after_callBB = BasicBlock::Create(
          module->getContext(),
          "npu_after_call_block",
          loop->getParent(),
          after_callBB_tmp
      );
      created_emptyBB = true;
    }
    loop->addBasicBlockToLoop(after_callBB, LI->getBase());

    // Invoke npu. No inputs, or buffered inputs.
    // ==============================================

    builder.setInsertPoint(callBB->getTerminator());

    // Initialize "oBuff read" loop induction variable
    builder.CreateStore(
        ConstantInt::get(nativeInt, 0, false),
        outLoopCounterAlloca
    );
    load = builder.CreateLoad(oBuffAlloca, true, "npu_load_obuff");

    // We'll use this latter
    Value *ibuff_used = builder.CreateLoad(counterAlloca,
                                           "npu_ibuff_used");
    ibuff_used = builder.CreateUDiv(ibuff_used,
        ConstantInt::get(nativeInt, n, false));


    if (created_emptyBB)
      builder.setInsertPoint(after_callBB);
    else
      builder.setInsertPoint(after_callBB->begin());

    for (int i = 0; i < escaped_stores.size(); ++i) {
      // First find out where we are storing to (for now it *must*
      // be one of the function arguments.
      Value *arg = escaped_stores[i]->getPointerOperand();
      int j;
      for (j = 0; j < callee_args_value.size(); ++j)
        if (callee_args_value[j] == arg)
          break;

      // Now:
      // 2 - Advance oBuff by one
      // 3 - Load the value oBuff had in step (1)
      // 4 - Convert the value from float to the original type
      // 5 - Store the oBuff value into the argument

      // (2)
      Value *GEP;
      std::string s1 = "npu_gep_oBuff";
      std::string s2 = "npu_load_oBuffResult";
      GEP = builder.CreateInBoundsGEP(load,
                                      ConstantInt::get(nativeInt, 1, true),
                                      s1.c_str());

      // (3)
      Value *v = builder.CreateLoad(load, s2.c_str());

      // (4)
      // No need to convert back for now
      //if (converted[i] == 1)
      //  v = builder.CreateFPToSI(v, conv_ty[i], "npu_conv_oBuff");
      //else if (converted[i] == 2)
      //  v = builder.CreateFPToUI(v, conv_ty[i], "npu_conv_oBuff");
      //else if (converted[i] == 3) 
      //  v = builder.CreateFPExt(v, conv_ty[i], "npu_conv_oBuff");

      builder.CreateStore(v, caller_args[j], false);

      load = GEP;

      // Code that is *inside* the loop and depends on either
      // the outputs of the function call or something that was
      // buffered before making the call goes here.
    }

    std::set<BasicBlock *> jump_to_latch;
    
    std::queue<BasicBlock *> bfs_q;
    std::set<BasicBlock *> seen;

    seen.insert(after_callBB);
    bfs_q.push(after_callBB);

    while (!bfs_q.empty()) {
      BasicBlock *bb = bfs_q.pop();
      std::set<BasicBlock *> suc = successorsOf(bb);
      for (std::set<BasicBlock *>::iterator it = suc.begin();
           it != suc.end();
           ++suc) {
        BasicBlock *child = it;
        if (seen.count(child))
          continue;

        if (child == loop->getLoopLatch()) {
          jump_to_latch.insert(bb);
        } else if (!loop->contains(child)) {
          continue;
        } else {
          seen.insert(child);
          bfs_q.insert(child);
        }
      }
    }

    BasicBlock *checkBB = BasicBlock::Create(
        module->getContext(),
        "npu_checkBB"
    );

    for (std::set<BasicBlock *>::iterator bi = jump_to_latch.begin();
         bi != jump_to_latch.end();
         ++bi) {
      Instruction *term = bi->getTerminator();
      if (isa<BranchInst>(term)) {
        BranchInst *branch = term;
        for (int i = 0; i < branch->getNumSuccessors(); ++i) {
          if (branch->getSuccessor(i) == loop->getLoopLatch())
            branch->setSuccessor(i, checkBB);
        }
      } else {
        builder.setInsertPoint(term);
        builder.CreateBr(checkBB);
      }
    }

    builder.setInsertPoint(checkBB);

    v = builder.CreateLoad(
            outLoopCounterAlloca,
            "out_loop_count_load"
    );

    v = builder.CreateAdd(
            v,
            ConstantInt::get(nativeInt, 1, false),
            "out_loop_count_add"
    );

    builder.CreateStore(
        v,
        outLoopCounterAlloca
    );

    // Function would be called ibuff_used times
    v = builder.CreateICmpEQ(v, ibuff_used, "npu_icmp_obuff_count");

    builder.CreateCondBr(v, loop->getLoopLatch(), after_callBB);

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

