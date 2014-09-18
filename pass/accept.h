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
#include <iostream>
#include <cassert>
#include <cstdlib>

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

// Logging: a section of the ACCEPT log.
class Description {
public:
  Description() : prefix(""), postfix(""), stream(prefix) {}
  Description(const std::string myPrefix, const std::string myPostfix,
      const std::map< int, std::vector<std::string> > myBlockers) :
      prefix(myPrefix), postfix(myPostfix), blockers(myBlockers),
      stream(prefix) {}
  Description(const Description &d) :
    prefix(d.prefix), postfix(d.postfix), blockers(d.blockers),
    stream(prefix) {}
  bool operator==(const Description &rhs) {
    return (this->prefix == rhs.prefix) &&
        (this->postfix == rhs.postfix) &&
        (this->blockers == rhs.blockers);
  }
  void operator=(const Description &d) {
    prefix = d.prefix;
    postfix = d.postfix;
    blockers = d.blockers;
    stream.str() = prefix;
  }
  llvm::raw_ostream &operator<<(llvm::StringRef s) {
    return stream << s;
  }
  void blocker(int lineno, llvm::StringRef s) {
    blockers[lineno].push_back(s);
  }
  std::string prefix;
  std::string postfix;
  std::map< int, std::vector<std::string> > blockers;
  llvm::raw_string_ostream stream;
};

// Logging: the location of a Description for positioning in the log.
class Location {
  public:
    Location() : kind(""), hasBlockers(false), fileName(""), lineNumber(0) {}
    Location(llvm::StringRef myKind, const bool myHasBlockers,
        llvm::StringRef myFile, const int myNum) :
        kind(myKind), hasBlockers(myHasBlockers), fileName(myFile),
        lineNumber(myNum) {}
    bool operator==(const Location &rhs) {
      return (this->kind == rhs.kind) &&
          (this->hasBlockers == rhs.hasBlockers) &&
          (this->fileName == rhs.fileName) &&
          (this->lineNumber == rhs.lineNumber);
    }
    std::string kind;
    bool hasBlockers;
    std::string fileName;
    int lineNumber;
};

// Logging: comparator for Location sorting.
class cmpLocation {
  public:
    bool operator() (const Location a, const Location b) const {
      if (a.kind != b.kind) {
        return (a.kind < b.kind);
      } else if (a.hasBlockers != b.hasBlockers) {
        return (a.hasBlockers && !b.hasBlockers);
      } else if (a.fileName != b.fileName) {
        return (a.fileName < b.fileName);
      } else {
        return (a.lineNumber < b.lineNumber);
      }
    }
};

// This static function splits a description returned by srcPosDesc
// into two strings: the file name and the line number.
static void splitPosDesc(
    const std::string posDesc,
    std::string &file,
    std::string &line) {
  size_t i = posDesc.find(":");
  size_t length = posDesc.length();

  file = posDesc.substr(0, i);
  line = posDesc.substr(i + 1, length - i - 1);
}

// This static function extracts the line number from a description returned
// by instDesc.
static int extractBlockerLine(const std::string instDesc) {
  size_t i = instDesc.find(":");
  size_t length = instDesc.length();

  std::string line = instDesc.substr(i + 1, length - i - 1);
  return atoi(line.c_str());
}

// This class represents an analysis this determines whether functions and
// chunks are approximate. It is consumed by our various optimizations.
class ApproxInfo : public llvm::FunctionPass {
public:
  static char ID;
  std::map< llvm::Function*, std::pair<std::string, int> > functionLocs;
  ApproxInfo();
  virtual ~ApproxInfo();
  virtual const char *getPassName() const;

  // Required FunctionPass interface.
  virtual void findFunctionLocs(llvm::Module &mod);
  virtual bool doInitialization(llvm::Module &M);
  virtual bool runOnFunction(llvm::Function &F);
  virtual bool doFinalization(llvm::Module &M);

  std::map<llvm::Function*, bool> functionPurity;

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

  bool isWhitelistedPure(llvm::StringRef s);
  std::set<llvm::BasicBlock*> successorsOf(llvm::BasicBlock *block);
  std::set<llvm::BasicBlock*> imSuccessorsOf(llvm::BasicBlock *block);
  bool storeEscapes(llvm::StoreInst *store,
                    const std::set<llvm::Instruction*> &insts,
                    bool approx=true);

  // Logging.
  Description *logAdd(llvm::StringRef kind, llvm::StringRef filename,
      const int lineno);

private:
  void successorsOfHelper(llvm::BasicBlock *block,
                          std::set<llvm::BasicBlock*> &succ);
  int preciseEscapeCheckHelper(std::map<llvm::Instruction*, bool> &flags,
                               const std::set<llvm::Instruction*> &insts);
  bool approxOrLocal(std::set<llvm::Instruction*> &insts,
                     llvm::Instruction *inst);

  // Logging.
  std::map<Location, std::vector<Description>, cmpLocation> logDescs;
  bool logEnabled;
  llvm::raw_fd_ostream *logFile;
  void dumpLog();
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
  std::string siteName(std::string kind, llvm::Instruction *at,
      std::string &fileName, std::string &line);

  void collectFuncDebug(llvm::Module &M);
  void collectSubprogram(llvm::DISubprogram sp);
  void collectType(llvm::DIType ty);

  void dumpRelaxConfig();
  void loadRelaxConfig();

  void addSyncDesc(const bool hasBlockers, const std::string fileName,
      const int lineNumber, const std::string prefix, const std::string postfix,
      const std::map< int, std::vector<std::string> > blockerEntries);
  bool optimizeSync(llvm::Function &F);
  bool optimizeAcquire(llvm::Instruction *inst);
  bool optimizeBarrier(llvm::Instruction *bar1);
  llvm::Instruction *findCritSec(llvm::Instruction *acq,
      std::set<llvm::Instruction*> &cs, Description *desc);
  llvm::Instruction *findApproxCritSec(llvm::Instruction *acq,
      Description *desc);
  bool nullifyApprox(llvm::Function &F);
};

// Information about individual instructions is always available.
bool isApprox(const llvm::Instruction *instr);
bool isApproxPtr(const llvm::Value *value);
bool isCallOf(llvm::Instruction *inst, const char *fname);
bool isAcquire(llvm::Instruction *inst);
bool isRelease(llvm::Instruction *inst);
