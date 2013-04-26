#include "llvm/Analysis/Passes.h"
#include "llvm/PassRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"

#include <set>
#include <map>
#include <string>

#define ECQ_PRECISE 0
#define ECQ_APPROX 1

#define PERMIT "ACCEPT_PERMIT"

namespace llvm {
  ImmutablePass *createAcceptAAPass();
  void initializeAcceptAAPass(PassRegistry &Registry);
  FunctionPass *createAcceptTransformPass();

  std::string srcPosDesc(const Module &mod, const DebugLoc &dl);
  std::string instDesc(const Module &mod, Instruction *inst);
}

// This class represents an analysis this determines whether functions and
// chunks are approximate. It is consumed by our various optimizations.
class ApproxInfo : public llvm::FunctionPass {
public:
  static char ID;
  ApproxInfo();
  virtual ~ApproxInfo();

  // Required FunctionPass interface.
  virtual bool doInitialization(llvm::Module &M);
  virtual bool runOnFunction(llvm::Function &F);
  virtual bool doFinalization(llvm::Module &M);

  std::map<llvm::Function*, bool> functionPurity;
  llvm::raw_fd_ostream *log;

  std::set<llvm::Instruction*> preciseEscapeCheck(
      std::set<llvm::Instruction*> insts);
  std::set<llvm::Instruction*> preciseEscapeCheck(
      std::set<llvm::BasicBlock*> blocks);
  bool isPrecisePure(llvm::Function *func);

private:
  void successorsOfHelper(llvm::BasicBlock *block,
                          std::set<llvm::BasicBlock*> &succ);
  std::set<llvm::BasicBlock*> successorsOf(llvm::BasicBlock *block);
  bool storeEscapes(llvm::StoreInst *store,
                    std::set<llvm::Instruction*> insts);
  int preciseEscapeCheckHelper(std::map<llvm::Instruction*, bool> &flags,
                               const std::set<llvm::Instruction*> &insts);
  bool hasPermit(llvm::Instruction *inst);
  bool approxOrLocal(std::set<llvm::Instruction*> &insts,
                     llvm::Instruction *inst);
};
