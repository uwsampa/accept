#define DEBUG_TYPE "enerc"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Instructions.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

#include <sstream>
#include <set>
#include <cstdio>

#define ECQ_PRECISE 0
#define ECQ_APPROX 1

#define FUNC_TRACE "enerc_trace"

// I'm not sure why, but I can't seem to enable debug output in the usual way
// (with command line flags). Here's a hack.
#undef DEBUG
#define DEBUG(s) s

using namespace llvm;

namespace {
  // Look at the qualifier metadata on the instruction and determine whether it
  // has approximate semantics.
  bool isApprox(Instruction *instr) {
    MDNode *md = instr->getMetadata("quals");
    if (!md)
      return false;

    Value *val = md->getOperand(0);
    ConstantInt *ci = cast<ConstantInt>(val);
    if (ci) {
      APInt intval = ci->getValue();
      return intval == ECQ_APPROX;
    } else {
      llvm::errs() << "INVALID METADATA";
      return false;
    }
  }

  // Is it legal to elide this instruction?
  bool elidable_helper(Instruction *instr,
                       std::set<Instruction*> &seen) {
    // Check for cycles.
    if (seen.count(instr)) {
      llvm::errs() << "CYCLE DETECTED\n";
      return false;
    }
    seen.insert(instr);

    if (isApprox(instr)) {
      return true;
    } else if (isa<StoreInst>(instr) ||
               isa<ReturnInst>(instr) ||
               isa<BranchInst>(instr)) {
      // Precise stores, returns, branches: never elidable.
      return false;
    } else {
      // Recursive case: all consumers are elidable.
      for (Value::use_iterator ui = instr->use_begin();
            ui != instr->use_end(); ++ui) {
        Instruction *user = dyn_cast<Instruction>(*ui);
        if (user && !elidable_helper(user, seen)) {
          return false;
        }
      }
      return true; // No non-elidable consumers.
    }
  }
  // Start with an empty "seen" (cycle-detection) set.
  bool elidable(Instruction *instr) {
    std::set<Instruction*> seen;
    return elidable_helper(instr, seen);
  }

  struct EnerC : public FunctionPass {
      static char ID;
      EnerC() : FunctionPass(ID) {
        gApproxInsts = 0;
        gElidableInsts = 0;
        gTotalInsts = 0;
      }

      Constant *blockCountFunction;
      unsigned long gApproxInsts;
      unsigned long gElidableInsts;
      unsigned long gTotalInsts;

      virtual bool runOnFunction(Function &F) {
        for (Function::iterator bbi = F.begin(); bbi != F.end(); ++bbi) {
          unsigned int approxInsts = 0;
          unsigned int elidableInsts = 0;
          unsigned int totalInsts = 0;

          for (BasicBlock::iterator ii = bbi->begin(); ii != bbi->end();
                ++ii) {
            if ((Instruction*)ii == bbi->getTerminator()) {
              // Terminator instruction is not checked.
              continue;
            }

            // Record information about this instruction.
            bool iApprox = isApprox(ii);
            bool iElidable = elidable(ii);
            ++gTotalInsts;
            ++totalInsts;
            if (iApprox) {
              ++gApproxInsts;
              ++approxInsts;
            }
            if (iElidable) {
              ++gElidableInsts;
              ++elidableInsts;
            }
          }

          // Call the runtime profiling library for this block.
          insertTraceCall(F, bbi, approxInsts, elidableInsts, totalInsts);
        }

        return false;
      }

      virtual bool doInitialization(Module &M) {
        errs() << "initializing\n";

        // Add instrumentation function.
        blockCountFunction = M.getOrInsertFunction(
          FUNC_TRACE,
          Type::getVoidTy(M.getContext()), // return type
          Type::getInt32Ty(M.getContext()), // approx
          Type::getInt32Ty(M.getContext()), // elidable
          Type::getInt32Ty(M.getContext()), // total
          NULL
        );

        return false;
      }

      virtual bool doFinalization(Module &M) {
        FILE *results_file = fopen("enerc_static.txt", "w+");
        fprintf(results_file,
                "%lu %lu %lu\n", gApproxInsts, gElidableInsts, gTotalInsts);
        fclose(results_file);
        return false;
      }

    protected:
      void insertTraceCall(Function &F, BasicBlock* bb,
                           unsigned int approx,
                           unsigned int elidable,
                           unsigned int total) {
        std::vector<Value *> args;
        args.push_back(
            ConstantInt::get(Type::getInt32Ty(F.getContext()), approx)
        );
        args.push_back(
            ConstantInt::get(Type::getInt32Ty(F.getContext()), elidable)
        );
        args.push_back(
            ConstantInt::get(Type::getInt32Ty(F.getContext()), total)
        );

        // For now, we're inserting calls at the end of basic blocks.
        CallInst::Create(
          blockCountFunction,
          args,
          "",
          bb->getTerminator()
        );
      }
  };
  char EnerC::ID = 0;
  static RegisterPass<EnerC> X("enerc", "EnerC pass", false, false);
}
