#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Pass.h"
#include "llvm/DataLayout.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/GlobalVariable.h"
#include "accept.h"
#include <fstream>
#include <string>

using namespace llvm;

namespace {
  struct AcceptAA : public ImmutablePass, public AliasAnalysis {
    static char ID;
    AcceptAA() : ImmutablePass(ID) {
      initializeAcceptAAPass(*PassRegistry::getPassRegistry());
    }

    virtual const char *getPassName() const {
      return "ACCEPT approximate alias analysis";
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AliasAnalysis::getAnalysisUsage(AU);
      // dependencies?
    }

    virtual void initializePass() {
      InitializeAliasAnalysis(this);
    }

    virtual AliasResult alias(const Location &LocA, const Location &LocB) {

      // Globals
      if (const GlobalValue *GV = dyn_cast<GlobalValue>(LocA.Ptr))
        if (const GlobalVariable *V = dyn_cast<GlobalVariable>(GV))
          if (isApproxPtr(V)) return NoAlias;
      if (const GlobalValue *GV = dyn_cast<GlobalValue>(LocB.Ptr))
        if (const GlobalVariable *V = dyn_cast<GlobalVariable>(GV))
          if (isApproxPtr(V)) return NoAlias;


      // Getelementptr
      if (const Instruction *inst = dyn_cast<Instruction>(LocA.Ptr))
        if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(inst))
          if (isApproxPtr(GEP)) return NoAlias;
      if (const Instruction *inst = dyn_cast<Instruction>(LocB.Ptr))
        if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(inst))
          if (isApproxPtr(GEP)) return NoAlias;

      // Bitcasts
      // In case of a precise store, we can check whether it's to
      // an APPROX global variable. In this case, the store is also approx
      // and we can return NoAlias.
      if (const Instruction *inst = dyn_cast<Instruction>(LocA.Ptr))
        if (const BitCastInst *BC = dyn_cast<BitCastInst>(inst)) {
          std::string type_str;
          llvm::raw_string_ostream rso(type_str);
          rso << *(inst->getOperand(0));
          if (rso.str().find("malloc") != std::string::npos) {
            for (Value::const_use_iterator ui = inst->use_begin();
                  ui != inst->use_end();
                  ++ui) {
              const Instruction *user = dyn_cast<Instruction>(*ui);
              if (const StoreInst *st = dyn_cast<StoreInst>(user))
                if (isApprox(st)) return NoAlias;
            }
          }
        }
      if (const Instruction *inst = dyn_cast<Instruction>(LocB.Ptr))
        if (const BitCastInst *BC = dyn_cast<BitCastInst>(inst)) {
          std::string type_str;
          llvm::raw_string_ostream rso(type_str);
          rso << *(inst->getOperand(0));
          if (rso.str().find("malloc") != std::string::npos) {
            for (Value::const_use_iterator ui = inst->use_begin();
                  ui != inst->use_end();
                  ++ui) {
              const Instruction *user = dyn_cast<Instruction>(*ui);
              if (const StoreInst *st = dyn_cast<StoreInst>(user))
                if (isApprox(st)) return NoAlias;
            }
          }
        }

      // Ordinary instructions
      if (const Instruction *inst = dyn_cast<Instruction>(LocA.Ptr))
        if (isApprox(inst)) return NoAlias;
      if (const Instruction *inst = dyn_cast<Instruction>(LocB.Ptr))
        if (isApprox(inst)) return NoAlias;


      /* DEBUG
      else if (instA && instB) {
        errs() << "instructions:\n";
        instA->dump();
        instB->dump();
      } else {
        const GlobalValue *gvA = dyn_cast<GlobalValue>(LocA.Ptr);
        const GlobalValue *gvB = dyn_cast<GlobalValue>(LocB.Ptr);
        if (gvA && gvB) {
          errs() << "global values:\n";
          gvA->dump();
          gvB->dump();
        }
      }
      */

      // Delegate to other alias analyses.
      return AliasAnalysis::alias(LocA, LocB);
    }

    // This required bit works around C++'s multiple inheritance weirdness.
    // Casting this to AliasAnalysis* gets the correct vtable for those calls.
    virtual void *getAdjustedAnalysisPointer(const void *ID) {
      if (ID == &AliasAnalysis::ID)
        return (AliasAnalysis*)this;
      return this;
    }
  };
}

char AcceptAA::ID = 0;
INITIALIZE_AG_PASS(AcceptAA, AliasAnalysis, "acceptaa",
                   "ACCEPT approximate alias analysis",
                   false, true, false)

ImmutablePass *llvm::createAcceptAAPass() { return new AcceptAA(); }
