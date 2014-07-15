#include "llvm/Analysis/Passes.h"
#include "llvm/PassRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"
#include "llvm/Analysis/ProfileInfo.h"
#include "llvm/Analysis/LoopPass.h"

#include <set>
#include <map>
#include <string>
#include <cassert>

#define ECQ_PRECISE 0
#define ECQ_APPROX 1
#define ECQ_APPROX_PTR 2

// Logging macro.
#define ACCEPT_LOG_(ai) if (!(ai)->logEnabled) ; else (*(ai)->logFile)


namespace llvm {
  ImmutablePass *createAcceptAAPass();
  void initializeAcceptAAPass(PassRegistry &Registry);
  FunctionPass *createAcceptTransformPass();
  extern FunctionPass *sharedAcceptTransformPass;
  LoopPass *createLoopPerfPass();
  void initializeLoopNPUPass(PassRegistry &Registry);
  LoopPass *createLoopNPUPass();

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
  bool logEnabled;
  llvm::raw_fd_ostream *logFile;

  std::set<llvm::Instruction*> preciseEscapeCheck(
      std::set<llvm::Instruction*> insts,
      std::set<llvm::Instruction*> *blessed=NULL);
  std::set<llvm::Instruction*> preciseEscapeCheck(
      std::set<llvm::BasicBlock*> blocks);
  bool isPrecisePure(llvm::Function *func);
  bool pointerCaptured(const llvm::Value *ptr,
      const std::set<llvm::Instruction*> &region,
      bool approx);

  std::map< std::string, std::map<int, LineMarker> > lineMarkers;
  LineMarker markerAtLine(std::string filename, int line);
  LineMarker instMarker(llvm::Instruction *inst);

  bool isWhiteList(llvm::StringRef s);
  std::set<llvm::BasicBlock*> successorsOf(llvm::BasicBlock *block);
  std::set<llvm::BasicBlock*> imSuccessorsOf(llvm::BasicBlock *block);
  bool storeEscapes(llvm::StoreInst *store,
                    const std::set<llvm::Instruction*> &insts,
                    bool approx=true);

private:
  void successorsOfHelper(llvm::BasicBlock *block,
                          std::set<llvm::BasicBlock*> &succ);
  int preciseEscapeCheckHelper(std::map<llvm::Instruction*, bool> &flags,
                               const std::set<llvm::Instruction*> &insts);
  bool approxOrLocal(std::set<llvm::Instruction*> &insts,
                     llvm::Instruction *inst);
};

// The pass that actually performs optimizations.
struct ACCEPTPass : public llvm::FunctionPass {
  static char ID;

  llvm::Module *module;
  std::map<std::string, int> relaxConfig;  // ident -> param
  int opportunityId;
  std::map<llvm::Function*, llvm::DISubprogram> funcDebugInfo;
  ApproxInfo *AI;
  bool relax;

  ACCEPTPass();
  virtual void getAnalysisUsage(llvm::AnalysisUsage &Info) const;
  virtual const char *getPassName() const;
  virtual bool runOnFunction(llvm::Function &F);
  virtual bool doInitialization(llvm::Module &M);
  virtual bool doFinalization(llvm::Module &M);

  bool shouldSkipFunc(llvm::Function &F);
  std::string siteName(std::string kind, llvm::Instruction *at);

  void collectFuncDebug(llvm::Module &M);
  void collectSubprogram(llvm::DISubprogram sp);
  void collectType(llvm::DIType ty);

  void dumpRelaxConfig();
  void loadRelaxConfig();

  bool optimizeSync(llvm::Function &F);
  bool optimizeAcquire(llvm::Instruction *inst);
  bool optimizeBarrier(llvm::Instruction *bar1);
  llvm::Instruction *findCritSec(llvm::Instruction *acq, std::set<llvm::Instruction*> &cs);
  llvm::Instruction *findApproxCritSec(llvm::Instruction *acq);

  bool nullifyApprox(llvm::Function &F);
};

// Information about individual instructions is always available.
bool isApprox(const llvm::Instruction *instr);
bool isApproxPtr(const llvm::Value *value);
bool isCallOf(llvm::Instruction *inst, const char *fname);
bool isAcquire(llvm::Instruction *inst);
bool isRelease(llvm::Instruction *inst);
