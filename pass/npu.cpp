#include "accept.h"

#include "llvm/Analysis/LoopPass.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"

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
            //tryToNPU(loop, inst, prev_inst);
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

  void tryToNPU(Loop *loop, Instruction *inst, Instruction *prev_inst) {
    IRBuilder<> builder(module->getContext());
    builder.SetInsertPoint(loop->getLoopPreheader()->getParent()->getEntryBlock().begin());
    IntegerType *nativeInt = getNativeIntegerType();
    AllocaInst *counterAlloca = builder.CreateAlloca(nativeInt, 0, "npu_ibuff_counter");

    unsigned int n = inst->getNumOperands();
    //std::vector<AllocaInst *> funcIn_allocas;

    for (unsigned int i = 0; i < n; ++i) {
      std::string alloca_name = "npu_funcIn_alloca_" + i;
      AllocaInst *funcIn_alloca = builder.CreateAlloca(Type::getFloatTy(), 0, alloca_name.c_str());
      //funcIn_allocas.push_back(funcIn_alloca);

      rso << inst->getOperandUse(i);
    }

    builder.SetInsertPoint(prev_inst);
    Value *v;
    Value *load;
    for (unsigned int i = 0; i < n; ++i) {
      std::string s = "npu_conv_" + i;
      Type *type = inst->getOperandUse(i)->getType();
      v = inst->getOperandUse(i);
      if (type->isIntegerTy()) {
        IntegerType *it = static_cast<IntegerType *>(type);
        if (it->getSignBit())
          v = builder.CreateSIToFP(v, Type::getFloatTy(), s.c_str());
        else
          v = builder.CreateUIToFP(v, Type::getFloatTy(), s.c_str());
      } else if (type->isDoubleTy()) {
        v = builder.CreateFPTrunc(v, Type::getFloatTy(), s.c_str());
      }

      if (i == 0)
        load = builder.CreateLoad(iBuffAlloca, true, "npu_load_ibuff");

      s = "npu_iBuffGEP_" + i;
      Value *GEP;
      // is the 1 signed? (the true parameter)
      GEP = builder.CreateInBoundsGEP(load, ConstantInt::get(nativeInt, 1, true), s.c_str());

      if (i == (n - 1))
        builder.CreateStore(GEP, iBuffAlloca, true);

      builder.CreateStore(v, load, false); //is this store volatile?

      load = GEP;
    }


    if (n == 0) {
      // Invoke npu. No inputs...
    } else {
      v = builder.CreateLoad(counterAlloca, "npu_counter_load");
      v = builder.CreateAdd(v, ConstantInt::get(nativeInt, n, false), "npu_counter_add");
      builder.CreateStore(v, counterAlloca);
    }

  }


    /*
    const CallInst *c_inst = dyn_cast<CallInst>(inst);
    unsigned int n = c_inst->getNumArgOperands();
    for (unsigned int i = 0; i < n; ++i) {
      std::string type_str;
      llvm::raw_string_ostream rso(type_str);
      rso << c_inst->
      std::cerr << "\n" << rso.str() << std::endl;
    }
    */



























  void build_region(BasicBlock *bb, std::set<BasicBlock *> &region) {
    region.insert(bb);
    std::set<BasicBlock *> suc = AI->successorsOf(bb);
    BasicBlock *sucBB = *(suc.begin());
    while ((suc.size() == 1) && sucBB->getUniquePredecessor()) {
      region.insert(sucBB);
      suc = AI->successorsOf(sucBB);
    }
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

