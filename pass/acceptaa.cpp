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

    static bool read_file;
    static std::vector<std::string> approx_vars;

    void parse_globals() {
      if (!read_file) {
        read_file = true;
        std::ifstream f("accept-globals-info.txt");
        if (f.is_open()) {
          while (f.good()) {
            std::string line;
            std::getline(f, line);
            if (!line.empty()) approx_vars.push_back(line);
          }
          f.close();
        }
      }
    }

    bool is_gv_approx(std::string name) const {
      for (int i = 0; i < approx_vars.size(); ++i)
        if ((approx_vars[i] == name) && !approx_vars[i].empty() && !name.empty()) return true;
      return false;
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
      parse_globals();
      if (const GlobalValue *GV = dyn_cast<GlobalValue>(LocA.Ptr))
        if (const GlobalVariable *V = dyn_cast<GlobalVariable>(GV))
          if (is_gv_approx(V->getName().str())) return NoAlias;
      if (const GlobalValue *GV = dyn_cast<GlobalValue>(LocB.Ptr))
        if (const GlobalVariable *V = dyn_cast<GlobalVariable>(GV))
          if (is_gv_approx(V->getName().str())) return NoAlias;

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
  bool AcceptAA::read_file = false;
  std::vector<std::string> AcceptAA::approx_vars;
}

char AcceptAA::ID = 0;
INITIALIZE_AG_PASS(AcceptAA, AliasAnalysis, "acceptaa",
                   "ACCEPT approximate alias analysis",
                   false, true, false)

ImmutablePass *llvm::createAcceptAAPass() { return new AcceptAA(); }
