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

#define ECQ_PRECISE 0
#define ECQ_APPROX 1

#define FUNC_TRACE "enerc_trace"

// I'm not sure why, but I can't seem to enable debug output in the usual way
// (with command line flags). Here's a hack.
#undef DEBUG
#define DEBUG(s) s

using namespace llvm;

STATISTIC(ecApproxInst, "approximate instructions");
STATISTIC(ecElidableInst, "elidable instructions");
STATISTIC(ecTotalInst, "total instructions");

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
        infoFile = NULL;
      }

      Constant *blockCountFunction;
      static unsigned int curInstId;
      FILE *infoFile;

      virtual bool runOnFunction(Function &F) {
        for (Function::iterator bbi = F.begin(); bbi != F.end(); ++bbi) {
          for (BasicBlock::iterator ii = bbi->begin(); ii != bbi->end();
                ++ii) {
            if ((Instruction*)ii == bbi->getTerminator()) {
              // Terminator instruction is not checked.
              continue;
            }

            // Record information about this instruction.
            bool iApprox = isApprox(ii);
            bool iElidable = elidable(ii);
            ++ecTotalInst;
            if (iApprox)
              ++ecApproxInst;
            if (iElidable)
              ++ecElidableInst;
            
            // Instrument the instruction and record information about it.
            // TODO: Only perform instrumentation if requested.
            insertTraceCall(F, ii, curInstId);
            recordInstInfo(curInstId, iApprox, iElidable);
            curInstId++;
          }
        }

        return false;
      }

      virtual bool doInitialization(Module &M) {
        errs() << "initializing\n";

        // Add instrumentation function.
        blockCountFunction = M.getOrInsertFunction(
          FUNC_TRACE,
          Type::getVoidTy(M.getContext()), // return type
          Type::getInt32Ty(M.getContext()), // block ID
          NULL
        );

        // Open program info file.
        // TODO: don't clobber between modules
        if (infoFile == NULL) {
          llvm::errs() << "opening\n";
          infoFile = fopen("enerc_info.txt", "w");
        }

        return false;
      }

      ~EnerC() {
        llvm::errs() << "closing\n";
        if (infoFile != NULL) {
          fclose(infoFile);
          infoFile = NULL;
        }
      }

    protected:
      void insertTraceCall(Function &F, Instruction* inst,
                           unsigned int instId) {
        std::vector<Value *> args;
        args.push_back(
            ConstantInt::get(Type::getInt32Ty(F.getContext()), instId)
        );
        CallInst::Create(
          blockCountFunction,
          args,
          "",
          inst
        );
      }

      void recordInstInfo(unsigned int instId, bool approx, bool elidable) {
        if (infoFile == NULL) {
          llvm::errs() << "couldn't write to info file\n";
          return;
        }
        fprintf(infoFile, "%u %u %u\n", instId, approx, elidable);
      }
  };
  char EnerC::ID = 0;
  static RegisterPass<EnerC> X("enerc", "EnerC pass", false, false);
  unsigned int EnerC::curInstId = 0;
}
