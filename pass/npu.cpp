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

      // Finding functions amenable to NPU.
      for (Loop::block_iterator bi = loop->block_begin();
            bi != loop->block_end(); ++bi) {
        BasicBlock *bb = *bi;
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

        } // for instructions

      } // for basic blocks

      // Finding smaller regions inside the loop.
      for (Loop::block_iterator bi = loop->block_begin();
            bi != loop->block_end(); ++bi) {
        std::set<BasicBlock *> region;
        build_region(*bi, region);
        if ((AI->preciseEscapeCheck(region)).size() == 0) { // no blockers
          std::cerr << "\nFound region of approx code" << std::endl;
        }
      } // end basic blocks

  } // tryToOptimizeLoop


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

