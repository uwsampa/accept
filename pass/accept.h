#include "llvm/Analysis/Passes.h"
#include "llvm/PassRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"
#include "llvm/Analysis/ProfileInfo.h"

#include <set>
#include <map>
#include <string>

#define ECQ_PRECISE 0
#define ECQ_APPROX 1


namespace llvm {
  ImmutablePass *createAcceptAAPass();
  void initializeAcceptAAPass(PassRegistry &Registry);
  FunctionPass *createAcceptTransformPass();

  std::string srcPosDesc(const Module &mod, const DebugLoc &dl);
  std::string instDesc(const Module &mod, Instruction *inst);

  extern bool acceptUseProfile;
}

#define PERMIT "ACCEPT_PERMIT"
#define FORBID "ACCEPT_FORBID"
typedef enum {
  markerNone,
  markerPermit,
  markerForbid
} LineMarker;

// This class represents an analysis this determines whether functions and
// chunks are approximate. It is consumed by our various optimizations.
class ApproxInfo : public llvm::FunctionPass {
public:
  static char ID;
  ApproxInfo();
  virtual ~ApproxInfo();
  virtual const char *getPassName() const;

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

  std::map< std::string, std::map<int, LineMarker> > lineMarkers;
  LineMarker markerAtLine(std::string filename, int line);
  LineMarker instMarker(llvm::Instruction *inst);

private:
  void successorsOfHelper(llvm::BasicBlock *block,
                          std::set<llvm::BasicBlock*> &succ);
  std::set<llvm::BasicBlock*> successorsOf(llvm::BasicBlock *block);
  bool storeEscapes(llvm::StoreInst *store,
                    std::set<llvm::Instruction*> insts);
  int preciseEscapeCheckHelper(std::map<llvm::Instruction*, bool> &flags,
                               const std::set<llvm::Instruction*> &insts);
  bool approxOrLocal(std::set<llvm::Instruction*> &insts,
                     llvm::Instruction *inst);
};

// Information about individual instructions is always available.
bool isApprox(const llvm::Instruction *instr);
