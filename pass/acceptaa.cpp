#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Pass.h"
#include "llvm/DataLayout.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/GlobalVariable.h"
#include "accept.h"
#include <fstream>
#include <string>
#include <ctime>

using namespace llvm;

namespace {
  bool isMalloc(const Value *val) {
      if (const CallInst *call = dyn_cast<CallInst>(val)) {
          if (Function *func = call->getCalledFunction()) {
              return func->getName().find("alloc") != StringRef::npos;
          }
      }
      return false;
  }

  struct AcceptAA : public ImmutablePass, public AliasAnalysis {
    static char ID;
    ACCEPTPass *transformPass;
    int relaxId;
    int relaxParam;

    AcceptAA() : ImmutablePass(ID) {
      initializeAcceptAAPass(*PassRegistry::getPassRegistry());
      if (!sharedAcceptTransformPass) {
        errs() << "Alias analysis loaded without transform pass!\n";
        return;
      }
      transformPass = (ACCEPTPass*)sharedAcceptTransformPass;
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

      // One global optimization parameter controls whether we should enable
      // alias relaxation. (For now.)
      std::string relaxName = "alias relaxation";
      if (transformPass->relax) {
        relaxParam = transformPass->relaxConfig[relaxName];
      } else {
        relaxParam = 0;
        transformPass->relaxConfig[relaxName] = 0;
      }
    }

    bool approxLoc(const Value *val) {
      if (isApproxPtr(val))
        return true;
      if (const Instruction *inst = dyn_cast<Instruction>(val))
        if (isApprox(inst))
          return true;
      return false;
    }
    bool approxLoc(const Location &Loc) {
      return approxLoc(Loc.Ptr);
    }

    virtual bool pointToConstantMemory(const Location &Loc,
        bool OrLocal=false) {
      bool result = AliasAnalysis::pointsToConstantMemory(Loc, OrLocal);
      if (!relaxParam)
        return result;

      if (approxLoc(Loc)) {
        return true;
      }

      return result;
    }
    virtual ModRefBehavior getModRefBehavior (ImmutableCallSite CS) {
      ModRefBehavior result = AliasAnalysis::getModRefBehavior(CS);
      if (!relaxParam)
        return result;

      if (result == UnknownModRefBehavior) {
        const Function *func = CS.getCalledFunction();
        if (func && transformPass->AI->isPrecisePure(const_cast<Function*>(func))) {
          return OnlyReadsMemory;
        }
      }

      return result;
    }
    virtual ModRefBehavior getModRefBehavior (const Function *F) {
      ModRefBehavior result = AliasAnalysis::getModRefBehavior(F);
      if (!relaxParam)
        return result;

      if (result == UnknownModRefBehavior) {
        if (transformPass->AI->isPrecisePure(const_cast<Function*>(F))) {
          return OnlyReadsMemory;
        }
      }

      return result;
    }
    virtual ModRefResult getModRefInfo (ImmutableCallSite CS,
        const Location &Loc) {
      ModRefResult result = AliasAnalysis::getModRefInfo(CS, Loc);
      if (!relaxParam)
        return result;

      const Function *func = CS.getCalledFunction();
      if (func && transformPass->AI->isPrecisePure(const_cast<Function*>(func))) {
        return NoModRef;
      } else if (approxLoc(Loc)) {
        return NoModRef;
      }

      return result;
    }
    virtual ModRefResult getModRefInfo (ImmutableCallSite CS1,
        ImmutableCallSite CS2) {
      ModRefResult result = AliasAnalysis::getModRefInfo(CS1, CS2);
      if (!relaxParam)
        return result;

      const Function *func1 = CS1.getCalledFunction();
      const Function *func2 = CS2.getCalledFunction();
      if ((func1 && transformPass->AI->isPrecisePure(const_cast<Function*>(func1))) ||
          (func2 && transformPass->AI->isPrecisePure(const_cast<Function*>(func2)))) {
        return NoModRef;
      }

      return result;
    }

    virtual AliasResult alias(const Location &LocA, const Location &LocB) {
      AliasResult result = AliasAnalysis::alias(LocA, LocB);
      if (!relaxParam)
        return result;

      if (result == MustAlias) {
        return MustAlias;
      } else if (approxLoc(LocA) || approxLoc(LocB)) {
        return NoAlias;
      }

      // Mallocs, callocs...
      if (const Instruction *inst = dyn_cast<Instruction>(LocA.Ptr)) {
        if (isMalloc(inst)) {
            for (Value::const_use_iterator ui = inst->use_begin();
                  ui != inst->use_end();
                  ++ui) {
              if (const Instruction *user = dyn_cast<Instruction>(*ui)) {
                if (const StoreInst *si = dyn_cast<StoreInst>(user)) {
                  if (isApproxPtr(si) || isApprox(si)) {
                      return NoAlias;
                  }
                } else if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(user)) {
                  if (isApproxPtr(user)) {
                      return NoAlias;
                  }
                } else if (const BitCastInst *bc = dyn_cast<BitCastInst>(user)) {
                  for (Value::const_use_iterator bc_ui = user->use_begin();
                        bc_ui != user->use_end();
                        ++bc_ui) {
                    if (const Instruction *bc_user = dyn_cast<Instruction>(*bc_ui))
                      if (const StoreInst *si_bc_user = dyn_cast<StoreInst>(bc_user))
                        if (isApproxPtr(si_bc_user) || isApprox(si_bc_user)) {
                            return NoAlias;
                        }
                  }
                }
              }
            }
        }
      }
      if (const Instruction *inst = dyn_cast<Instruction>(LocB.Ptr)) {
        if (isMalloc(inst)) {
            for (Value::const_use_iterator ui = inst->use_begin();
                  ui != inst->use_end();
                  ++ui) {
              if (const Instruction *user = dyn_cast<Instruction>(*ui)) {
                if (const StoreInst *si = dyn_cast<StoreInst>(user)) {
                  if (isApproxPtr(si) || isApprox(si)) {
                      return NoAlias;
                  }
                } else if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(user)) {
                  if (isApproxPtr(user)) {
                      return NoAlias;
                  }
                } else if (const BitCastInst *bc = dyn_cast<BitCastInst>(user)) {
                  for (Value::const_use_iterator bc_ui = user->use_begin();
                        bc_ui != user->use_end();
                        ++bc_ui) {
                    if (const Instruction *bc_user = dyn_cast<Instruction>(*bc_ui))
                      if (const StoreInst *si_bc_user = dyn_cast<StoreInst>(bc_user))
                        if (isApproxPtr(si_bc_user) || isApprox(si_bc_user)) {
                            return NoAlias;
                        }
                  }
                }
              }
            }
        }
      }

      // Globals
      if (const GlobalValue *GV = dyn_cast<GlobalValue>(LocA.Ptr))
        if (const GlobalVariable *V = dyn_cast<GlobalVariable>(GV))
          if (isApproxPtr(V)) {
              return NoAlias;
          }
      if (const GlobalValue *GV = dyn_cast<GlobalValue>(LocB.Ptr))
        if (const GlobalVariable *V = dyn_cast<GlobalVariable>(GV))
          if (isApproxPtr(V)) {
              return NoAlias;
          }

      // Getelementptr
      if (const Instruction *inst = dyn_cast<Instruction>(LocA.Ptr))
        if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(inst))
          if (isApproxPtr(GEP)) {
              return NoAlias;
          }
      if (const Instruction *inst = dyn_cast<Instruction>(LocB.Ptr))
        if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(inst))
          if (isApproxPtr(GEP)) {
              return NoAlias;
          }

      // Bitcasts
      if (const Instruction *inst = dyn_cast<Instruction>(LocA.Ptr))
        if (const BitCastInst *BC = dyn_cast<BitCastInst>(inst)) {
          if (isMalloc(inst->getOperand(0))) {
            for (Value::const_use_iterator ui = inst->use_begin();
                  ui != inst->use_end();
                  ++ui) {
              const Instruction *user = dyn_cast<Instruction>(*ui);
              if (const StoreInst *st = dyn_cast<StoreInst>(user))
                if (isApprox(st) || isApproxPtr(st)) {
                    return NoAlias;
                }
            }
          }
        }
      if (const Instruction *inst = dyn_cast<Instruction>(LocB.Ptr))
        if (const BitCastInst *BC = dyn_cast<BitCastInst>(inst)) {
          if (isMalloc(inst->getOperand(0))) {
            for (Value::const_use_iterator ui = inst->use_begin();
                  ui != inst->use_end();
                  ++ui) {
              const Instruction *user = dyn_cast<Instruction>(*ui);
              if (const StoreInst *st = dyn_cast<StoreInst>(user))
                if (isApprox(st) || isApproxPtr(st)) {
                    return NoAlias;
                }
            }
          }
        }

      // Ordinary instructions
      if (const Instruction *inst = dyn_cast<Instruction>(LocA.Ptr))
        if (isApprox(inst) || isApproxPtr(inst)) {
            return NoAlias;
        }
      if (const Instruction *inst = dyn_cast<Instruction>(LocB.Ptr))
        if (isApprox(inst) || isApproxPtr(inst)) {
            return NoAlias;
        } 

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
