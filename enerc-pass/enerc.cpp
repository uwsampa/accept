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

#define FUNC_BLOCK_COUNT "enerc_block_count"

// I'm not sure why, but I can't seem to enable debug output in the usual way
// (with command line flags). Here's a hack.
#undef DEBUG
#define DEBUG(s) s

using namespace llvm;

STATISTIC(ecElidableBlocks, "basic blocks elidable with approximation");
STATISTIC(ecTotalBlocks, "total basic blocks considered for elision");

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

  // Is every instruction in a block elidable?
  bool elidableBlock(BasicBlock *block) {
    for (BasicBlock::iterator ii = block->begin(); ii != block->end();
          ++ii) {
      DEBUG(errs() << "?");
      if ((Instruction*)ii == block->getTerminator()) {
        // Terminator instruction is not checked.
        continue;
      }

      if (!elidable(ii)) {
        DEBUG(errs() << "!\n");
        return false;
      }
    }

    DEBUG(errs() << ".\n");
    return true;
  }

  struct EnerC : public FunctionPass {
      static char ID;
      EnerC() : FunctionPass(ID) {
        blockInfoFile = NULL;
      }

      Constant *blockCountFunction;
      static unsigned int curBlockId;
      FILE *blockInfoFile;

      virtual bool runOnFunction(Function &F) {
        // DEBUG(errs() << "Analyzing func " << F.getName() << "\n");
        for (Function::iterator bbi = F.begin(); bbi != F.end(); ++bbi) {
          // Generate a "human-readable" name for the block (for debugging).
          int line = -1;
          Instruction *firstInst = bbi->getFirstNonPHI();
          if (firstInst) {
            DebugLoc dl = firstInst->getDebugLoc();
            line = dl.getLine();
          }
          std::stringstream ss;
          ss << std::string(F.getName()) << ":" << line;
          std::string blockname = ss.str();

          DEBUG(errs() << "block " << curBlockId << " " << blockname << "\n");

          bool elidable = elidableBlock(bbi);
          if (elidable) {
            DEBUG(llvm::errs() << "elidable block\n");
            ++ecElidableBlocks;
          }
          ++ecTotalBlocks;

          fprintf(blockInfoFile, "%i %i %s\n",
                  curBlockId, elidable, blockname.c_str());

          // Instrument the block.
          instrumentBlock(F, bbi->getTerminator(), curBlockId);

          curBlockId++;
        }
        return false;
      }

      virtual bool doInitialization(Module &M) {
        errs() << "initializing\n";

        // Add instrumentation function.
        blockCountFunction = M.getOrInsertFunction(
          FUNC_BLOCK_COUNT,
          Type::getVoidTy(M.getContext()), // return type
          Type::getInt32Ty(M.getContext()), // block ID
          NULL
        );

        // Open block info file.
        // TODO: don't clobber between modules
        if (blockInfoFile == NULL) {
          llvm::errs() << "opening\n";
          blockInfoFile = fopen("enerc_block_info.txt", "w");
        }

        return false;
      }

      ~EnerC() {
        llvm::errs() << "closing\n";
        if (blockInfoFile != NULL) {
          fclose(blockInfoFile);
          blockInfoFile = NULL;
        }
      }

    protected:
      void instrumentBlock(Function &F, Instruction* insertPoint,
                           unsigned int blockid) {
        std::vector<Value *> args;
        args.push_back(
            ConstantInt::get(Type::getInt32Ty(F.getContext()), blockid)
        );
        CallInst::Create(
          blockCountFunction,
          args,
          "",
          insertPoint
        );
      }
  };
  char EnerC::ID = 0;
  static RegisterPass<EnerC> X("enerc", "EnerC pass", false, false);
  unsigned int EnerC::curBlockId = 0;
}
