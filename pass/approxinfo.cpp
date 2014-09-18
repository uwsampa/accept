#include "accept.h"
#include "llvm/Pass.h"
#include "llvm/Metadata.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/DebugInfo.h"
#include "llvm/Module.h"
#include "llvm/Support/CommandLine.h"

#include <fstream>
#include <sstream>
#include <cctype>
#include <utility>

using namespace llvm;


/**** DESCRIBE SOURCE INFORMATION ****/

// Format a source position.
std::string llvm::srcPosDesc(const Module &mod, const DebugLoc &dl) {
  LLVMContext &ctx = mod.getContext();
  std::string out;
  raw_string_ostream ss(out);

  // Try to get filename from debug location.
  DIScope scope(dl.getScope(ctx));
  if (scope.Verify()) {
    ss << scope.getFilename().data();
  } else {
    // Fall back to the compilation unit name.
    ss << "(" << mod.getModuleIdentifier() << ")";
  }
  ss << ":";

  ss << dl.getLine();
  return out;
}

// Describe an instruction.
std::string llvm::instDesc(const Module &mod, Instruction *inst) {
  std::string out;
  raw_string_ostream ss(out);
  ss << srcPosDesc(mod, inst->getDebugLoc()) << ": ";

  // call and invoke instructions
  Function *calledFunc = NULL;
  bool isCall = false;
  if (CallInst *call = dyn_cast<CallInst>(inst)) {
    calledFunc = call->getCalledFunction();
    isCall = true;
  } else if (InvokeInst *invoke = dyn_cast<InvokeInst>(inst)) {
    calledFunc = invoke->getCalledFunction();
    isCall = true;
  }

  if (isCall) {
    if (calledFunc) {
      StringRef name = calledFunc->getName();
      if (!name.empty() && name.front() == '_') {
        ss << "call to " << name;
      } else {
        ss << "call to " << name << "()";
      }
    }
    else
      ss << "indirect function call";
  } else if (StoreInst *store = dyn_cast<StoreInst>(inst)) {
    Value *ptr = store->getPointerOperand();
    StringRef name = ptr->getName();
    if (name.empty()) {
      ss << "store to intermediate:";
      inst->print(ss);
    } else {
      ss << "store to " << ptr->getName().data();
    }
  } else {
    inst->print(ss);
  }

  return out;
}


/**** ANALYSIS HELPERS ****/

// Look at the qualifier metadata on the instruction and determine whether it
// has approximate semantics.
bool isApprox(const Instruction *instr) {
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

// Global names list used by isApproxPtr.
std::set<std::string> _approx_vars;
bool _read_file;
void _parse_globals() {
  if (!_read_file) {
    _read_file = true;
    std::ifstream f("accept-globals-info.txt");
    if (f.is_open()) {
      while (f.good()) {
        std::string line;
        std::getline(f, line);
        if (!line.empty()) _approx_vars.insert(line);
      }
      f.close();
    }
  }
}

bool isApproxPtr(const Value *value, std::set<const Value *> &seen) {
  // Avoid infinite loops through phi nodes.
  if (seen.count(value))
    return false;
  seen.insert(value);

  // Special cases.
  if (const BitCastInst *cast = dyn_cast<BitCastInst>(value)) {
    if (isApproxPtr(cast->getOperand(0), seen))
      return true;
  } else if (const GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(value)) {
    if (isApproxPtr(gep->getPointerOperand(), seen))
      return true;
  } else if (const PHINode *phi = dyn_cast<PHINode>(value)) {
    for (unsigned i = 0; i != phi->getNumIncomingValues(); ++i) {
      if (isApproxPtr(phi->getIncomingValue(i), seen))
        return true;
    }
  }

  // General instructions and globals.
  if (const Instruction *instr = dyn_cast<Instruction>(value)) {
    MDNode *md = instr->getMetadata("quals");
    if (!md)
      return false;
    Value *val = md->getOperand(0);
    ConstantInt *ci = cast<ConstantInt>(val);
    if (ci)
      return ci->getValue() == ECQ_APPROX_PTR;
  } else if (const GlobalVariable *gv = dyn_cast<GlobalVariable>(value)) {
    // Use the global names list.
    _parse_globals();
    return _approx_vars.count(gv->getName());
  }
  return false;
}

bool isApproxPtr(const Value *value) {
  std::set<const Value *> seen;
  return isApproxPtr(value, seen);
}

// Identification of lock acquire and release calls.
const char *FUNC_ACQUIRE = "pthread_mutex_lock";
const char *FUNC_RELEASE = "pthread_mutex_unlock";
bool isCallOf(Instruction *inst, const char *fname) {
  CallInst *call = dyn_cast<CallInst>(inst);
  if (call) {
    Function *func = call->getCalledFunction();
    if (func) {
      return fname == func->getName();
    }
  }
  return false;
}
bool isAcquire(Instruction *inst) {
  return isCallOf(inst, FUNC_ACQUIRE);
}
bool isRelease(Instruction *inst) {
  return isCallOf(inst, FUNC_RELEASE);
}

// An internal whitelist for functions considered to be pure.
char const* _funcWhitelistArray[] = {
  // math.h
  "cos",
  "atoi",
  "sin",
  "tan",
  "acos",
  "asin",
  "atan",
  "atan2",
  "cosh",
  "sinh",
  "tanh",
  "exp",
  "frexp",
  "ldexp",
  "log",
  "log10",
  "modf",
  "pow",
  "sqrt",
  "sqrtf",
  "ceil",
  "fabs",
  "floor",
  "fmod",
};
const std::set<std::string> funcWhitelist(
  _funcWhitelistArray,
  _funcWhitelistArray + sizeof(_funcWhitelistArray) /
                        sizeof(_funcWhitelistArray[0])
);



/**** ANALYSIS PASS WORKFLOW ****/

bool acceptLogEnabled;
cl::opt<bool, true> acceptLogEnabledOpt("accept-log",
    cl::desc("ACCEPT: write analysis log"),
    cl::location(acceptLogEnabled));

ApproxInfo::ApproxInfo() : FunctionPass(ID) {
  std::string error;
  logEnabled = acceptLogEnabled;
  if (logEnabled) {
    logFile = new raw_fd_ostream("accept_log.txt", error);
  }
}

Description *ApproxInfo::logAdd(llvm::StringRef kind,
      StringRef filename, const int lineno) {
  // TODO return NULL if the log is disabled
  Location loc(kind, filename, lineno);
  std::vector<Description*> &descs = logDescs[loc];
  Description *desc = new Description();
  descs.push_back(desc);
  return desc;
}

void ApproxInfo::dumpLog() {
  int numKind = 0;
  std::string prevKind;

  // For each location, print all the descriptions to the log.
  for (std::map<Location, std::vector<Description*>, cmpLocation>::iterator
      i = logDescs.begin(); i != logDescs.end(); i++) {
    std::string newKind = i->first.kind;
    if (newKind != prevKind) {
      if (numKind != 0) {
        *logFile << "\n\n";
      }
      numKind++;

      // Descriptions are organized into sections according to their kinds.
      // Include a header if the description is of a different kind than the
      // previous description.
      if (newKind == "Function") {
        *logFile << "ANALYZING FUNCTIONS FOR PRECISE-PURITY:\n";
      } else if (newKind == "Loop") {
        *logFile << "ANALYZING LOOP BODIES FOR PERFORABILITY:\n";
      } else if (newKind == "NPU Region") {
        *logFile << "ANALYZING NPU REGIONS FOR PRECISE-PURITY:\n";
      } else if (newKind == "Synchronization") {
        *logFile << "ANALYZING CRITICAL SECTIONS FOR PRECISE-PURITY:\n";
      } else {
        for (size_t ch = 0; ch < newKind.length(); ch++) {
          newKind[ch] = toupper(newKind[ch]);
        }
        *logFile << "ANALYZING " << newKind << "S FOR PRECISE-PURITY:\n";
      }
    }

    prevKind = newKind;

    std::vector<Description*> descVector = i->second;
    for (std::vector<Description*>::iterator j = descVector.begin();
        j != descVector.end(); j++) {
      // Within the section for a kind, descriptions with blockers are
      // printed before descriptions with no blockers, in order to help
      // the programmer identify any possible annotations that remain.
      // Five additional dashes are included for the demarcation between
      // the last description with blockers and the first description
      // without blockers within a section.
      Description *desc = *j;
      *logFile << "-----\n" << desc->getText();

      std::map< int, std::vector<std::string> > blockers = desc->blockers;
      for (std::map< int, std::vector<std::string> >::iterator
          k = blockers.begin(); k != blockers.end(); k++) {
        std::vector<std::string> entryVector = k->second;
        for (std::vector<std::string>::iterator l = entryVector.begin();
            l != entryVector.end(); l++) {
          *logFile << "  * " << *l << "\n";
        }
      }

      // Free the description. We're done.
      delete desc;
    }
  }
}

ApproxInfo::~ApproxInfo() {
  if (logEnabled) {
    dumpLog();
    logFile->close();
    delete logFile;
  }
}

bool ApproxInfo::runOnFunction(Function &F) {
  return false;
}

bool ApproxInfo::doInitialization(Module &M) {
  findFunctionLocs(M);
  // Analyze the purity of each function in the module up-front.
  for (Module::iterator i = M.begin(); i != M.end(); ++i) {
    isPrecisePure(&*i);
  }
  return false;
}

bool ApproxInfo::doFinalization(Module &M) {
  return false;
}

const char *ApproxInfo::getPassName() const {
  return "ACCEPT approximateness analysis";
}


/**** ANALYSIS CORE ****/

void ApproxInfo::successorsOfHelper(BasicBlock *block,
                        std::set<BasicBlock*> &succ) {
  TerminatorInst *term = block->getTerminator();
  if (!term)
    return;
  for (int i = 0; i < term->getNumSuccessors(); ++i) {
    BasicBlock *sb = term->getSuccessor(i);
    if (!succ.count(sb)) {
      succ.insert(sb);
      successorsOfHelper(sb, succ);
    }
  }
}
std::set<BasicBlock*> ApproxInfo::successorsOf(BasicBlock *block) {
  // TODO memoize
  std::set<BasicBlock*> successors;
  successorsOfHelper(block, successors);
  return successors;
}
std::set<BasicBlock*> ApproxInfo::imSuccessorsOf(BasicBlock *block) {
  std::set<BasicBlock*> successors;
  TerminatorInst *term = block->getTerminator();
  if (!term)
    return successors;
  for (int i = 0; i < term->getNumSuccessors(); ++i) {
    BasicBlock *sb = term->getSuccessor(i);
    successors.insert(sb);
  }
  return successors;
}

struct ApproxCaptureTracker : public CaptureTracker {
  bool Captured;
  const std::set<Instruction *> *region;
  ApproxInfo *ai;
  bool approx;

  explicit ApproxCaptureTracker(const std::set<Instruction *> *_region,
                                ApproxInfo *_ai, bool _approx)
    : Captured(false), region(_region), ai(_ai), approx(_approx) {}

  void tooManyUses() {
    Captured = true;
  }

  bool captured(Use *U) {
    if (approx) {
      // Permit captures by precise-pure functions in the region.
      CallInst *call = dyn_cast<CallInst>(U->getUser());
      if (call && region->count(call)) {
        Function *func = call->getCalledFunction();
        if (func && ai->isPrecisePure(func)) {
          return false;
        }
      }
    }

    Captured = true;
    return true;
  }
};

bool ApproxInfo::pointerCaptured(const Value *ptr,
                                 const std::set<Instruction*> &region,
                                 bool approx) {
  ApproxCaptureTracker ct(&region, this, approx);
  PointerMayBeCaptured(ptr, &ct);
  return ct.Captured;
}

// Conservatively check whether a store instruction can be observed by any
// load instructions *other* than those in the specified set of instructions.
bool ApproxInfo::storeEscapes(StoreInst *store,
                              const std::set<Instruction*> &insts,
                              bool approx) {
  Value *ptr = store->getPointerOperand();

  // Traverse bitcasts from one pointer type to another.
  BitCastInst *bitcast = dyn_cast<BitCastInst>(ptr);
  if (bitcast) {
    // Check that the operand (the original pointer) does not escape and,
    // if that passes, continue checking the cast value.
    if (!bitcast->getDestTy()->isPointerTy())
      return true;
    Value *innerPtr = bitcast->getOperand(0);
    // We should probably recurse into the value here (e.g., through multiple
    // bitcasts), but for now we just do a shallow escape check.
    if (!isa<AllocaInst>(innerPtr) || pointerCaptured(innerPtr, insts, approx))
      return true;

  // Make sure the pointer was created locally. That is, conservatively assume
  // that pointers coming from arguments or returned from other functions are
  // aliased somewhere else.
  } else if (!isa<AllocaInst>(ptr)) {
    return true;
  }

  // Give up if the pointer is copied and leaves the function.
  if (pointerCaptured(ptr, insts, approx))
    return true;

  // Look for loads to the pointer not present in our exclusion set. We
  // only look for loads in successors to this block. This could be made
  // more precise by detecting anti-dependencies (i.e., stores that shadow
  // this store).
  // First, check the current block.
  BasicBlock *parent = store->getParent();
  bool sawStore = false;
  for (BasicBlock::iterator ii = parent->begin(); ii != parent->end(); ++ii) {
    if (sawStore) {
      if (LoadInst *load = dyn_cast<LoadInst>(ii)) {
        if (load->getPointerOperand() == ptr && !insts.count(load)) {
          return true;
        }
      }
    } else if (store == ii) {
      sawStore = true;
    }
  }
  // Next, check all the successors of the current block.
  std::set<BasicBlock*> successors = successorsOf(parent);
  for (std::set<BasicBlock*>::iterator bi = successors.begin();
        bi != successors.end(); ++bi) {
    for (BasicBlock::iterator ii = (*bi)->begin(); ii != (*bi)->end(); ++ii) {
      if (LoadInst *load = dyn_cast<LoadInst>(ii)) {
        if (load->getPointerOperand() == ptr && !insts.count(load)) {
          return true;
        }
      }
    }
  }

  return false;
}

int ApproxInfo::preciseEscapeCheckHelper(std::map<Instruction*, bool> &flags,
                              const std::set<Instruction*> &insts) {
  int changes = 0;
  for (std::map<Instruction*, bool>::iterator i = flags.begin();
      i != flags.end(); ++i) {
    // Only consider currently-untainted instructions.
    if (i->second) {
      continue;
    }

    // Precise store: check whether it escapes.
    if (StoreInst *store = dyn_cast<StoreInst>(i->first)) {
      if (!storeEscapes(store, insts)) {
        i->second = true;
        ++changes;
      }
      continue;
    }

    // Check for balanced synchronization. This is a bit of an ugly hack since
    // we don't look at where the calls occur or what they lock, but it should
    // work for most code.
    if (isAcquire(i->first)) {
      bool found = false;
      for (std::map<Instruction*, bool>::iterator j = flags.begin();
          j != flags.end(); ++j) {
        if (!j->second && isRelease(j->first)) {
          // Balanced acquire/release pair.
          i->second = true;
          j->second = true;
          found = true;
          break;
        }
      }
      if (found)
        continue;
    }

    // Calls must be to precise-pure functions.
    Function *calledFunc = NULL;
    if (CallInst *call = dyn_cast<CallInst>(i->first)) {
      if (!isa<DbgInfoIntrinsic>(call)) {
        calledFunc = call->getCalledFunction();
        if (!calledFunc)
          continue;
      }
    } else if (InvokeInst *invoke = dyn_cast<InvokeInst>(i->first)) {
      calledFunc = invoke->getCalledFunction();
      if (!calledFunc)
        continue;
    }
    if (calledFunc && !isPrecisePure(calledFunc))
        continue;
    // Otherwise, the call itself is precise-pure, but now we need to make
    // sure that the uses are also tainted. Fall through to usage check.

    bool allUsesTainted = true;
    for (Value::use_iterator ui = i->first->use_begin();
          ui != i->first->use_end(); ++ui) {
      Instruction *user = dyn_cast<Instruction>(*ui);
      if (user && (!flags.count(user) || !flags[user])) {
        allUsesTainted = false;
        break;
      }
    }

    if (allUsesTainted) {
      ++changes;
      i->second = true;
    }
  }
  return changes;
}

LineMarker ApproxInfo::markerAtLine(std::string filename, int line) {
  if (!lineMarkers.count(filename)) {
    // Load markers for the file.
    std::map<int, LineMarker> markers;

    std::ifstream srcFile(filename.data());
    int lineno = 1;
    std::string theLine;
    while (srcFile.good()) {
      std::string curLine;
      getline(srcFile, curLine);
      if (curLine.find(PERMIT) != std::string::npos) {
        markers[lineno] = markerPermit;
      } else if (curLine.find(FORBID) != std::string::npos) {
        markers[lineno] = markerForbid;
      }
      ++lineno;
    }
    srcFile.close();

    lineMarkers[filename] = markers;
  }

  std::map<int, LineMarker> &fileMarkers = lineMarkers[filename];
  if (fileMarkers.count(line)) {
    return fileMarkers[line];
  } else {
    return markerNone;
  }
}

LineMarker ApproxInfo::instMarker(Instruction *inst) {
  DebugLoc dl = inst->getDebugLoc();
  DIScope scope(dl.getScope(inst->getContext()));
  if (scope.Verify())
    return markerAtLine(scope.getFilename(), dl.getLine());
  return markerNone;
}

bool ApproxInfo::approxOrLocal(std::set<Instruction*> &insts,
                               Instruction *inst) {
  Function *calledFunc = NULL;

  if (instMarker(inst) == markerPermit) {
    return true;
  } else if (CallInst *call = dyn_cast<CallInst>(inst)) {

    if (isa<DbgInfoIntrinsic>(call)) {
      return true;
    }

    calledFunc = call->getCalledFunction();
    if (!calledFunc)  // Indirect call.
      return false;

  } else if (InvokeInst *invoke = dyn_cast<InvokeInst>(inst)) {
    calledFunc = invoke->getCalledFunction();
    if (!calledFunc)
      return false;
  } else if (isApprox(inst)) {
    return true;
  } else if (isa<BranchInst>(inst)) {
    // TODO: Assuming single-entry/single-exit (all optimizations provide this
    // currently). So all branches are fair game.
    return true;
  } else if (isa<StoreInst>(inst) ||
             isa<ReturnInst>(inst)) {
    return false;  // Never approximate.
  }

  // For call and invoke instructions, ensure the function is precise-pure.
  if (calledFunc) {
    // Special case: out-parameters for memset and memcpy.
    StringRef funcName = calledFunc->getName();
    if (funcName.startswith("llvm.memset.") ||
        funcName.startswith("llvm.memcpy.")) {
      CallInst *call = cast<CallInst>(inst);
      if (isApproxPtr(call->getArgOperand(0)))
        return true;
    }

    // General case: check for precise purity.
    if (!isPrecisePure(calledFunc)) {
      return false;
    }
    if (isApprox(inst)) {
      return true;
    }
    // Otherwise, fall through and check usages for escape.
  }

  for (Value::use_iterator ui = inst->use_begin();
        ui != inst->use_end(); ++ui) {
    Instruction *user = dyn_cast<Instruction>(*ui);
    if (user && insts.count(user) == 0) {
      return false;  // Escapes.
    }
  }
  return true;  // Does not escape.
}

std::set<Instruction*> ApproxInfo::preciseEscapeCheck(
    std::set<Instruction*> insts,
    std::set<Instruction*> *blessed) {
  std::map<Instruction*, bool> flags;

  // Mark all approx and non-escaping instructions.
  for (std::set<Instruction*>::iterator i = insts.begin();
        i != insts.end(); ++i) {
    flags[*i] = approxOrLocal(insts, *i) || (blessed && blessed->count(*i));
  }

  // Iterate to a fixed point.
  while (preciseEscapeCheckHelper(flags, insts)) {}

  // Construct a set of untainted instructions.
  std::set<Instruction*> untainted;
  for (std::map<Instruction*, bool>::iterator i = flags.begin();
      i != flags.end(); ++i) {
    if (!i->second)
      untainted.insert(i->first);
  }
  return untainted;
}

std::set<Instruction*> ApproxInfo::preciseEscapeCheck(
    std::set<BasicBlock*> blocks) {
  std::set<Instruction*> insts;
  for (std::set<BasicBlock*>::iterator bi = blocks.begin();
        bi != blocks.end(); ++bi) {
    for (BasicBlock::iterator ii = (*bi)->begin();
          ii != (*bi)->end(); ++ii) {
      insts.insert(ii);
    }
  }
  return preciseEscapeCheck(insts);
}

bool ApproxInfo::isWhitelistedPure(StringRef s) {
  return funcWhitelist.count(s);
}

// This function finds the file name and line number of each function.
void ApproxInfo::findFunctionLocs(Module &mod) {
  NamedMDNode *namedMD = mod.getNamedMetadata("llvm.dbg.cu");
  for (unsigned i = 0, e = namedMD->getNumOperands(); i != e; ++i) {
    DICompileUnit cu(namedMD->getOperand(i));
    DIArray subps = cu.getSubprograms();
    for (unsigned j = 0, je = subps.getNumElements(); j != je; ++j) {
      DISubprogram subProg(subps.getElement(j));
      std::string fileName = subProg.getFilename().str();
      int lineNumber = (int) subProg.getLineNumber();
      std::pair<std::string, int> functionLoc(fileName, lineNumber);
      functionLocs[subProg.getFunction()] = functionLoc;
    }
  }
}

// Determine whether a function can only affect approximate memory (i.e., no
// precise stores escape).
bool ApproxInfo::isPrecisePure(Function *func) {
  assert(func != NULL);

  // Check for cached result.
  if (functionPurity.count(func)) {
    return functionPurity[func];
  }

  std::string fileName = "";
  int lineNumber = 0;

  if (functionLocs.count(func)) {
    fileName = functionLocs[func].first;
    lineNumber = functionLocs[func].second;
  }
  Description *desc = logAdd("Function", fileName, lineNumber);

  *desc << "checking function " << func->getName().str();
  if (functionLocs.count(func)) {
    *desc << " at " << fileName << ":" << lineNumber;
  }
  *desc << "\n";

  // LLVM's own nominal purity analysis.
  if (func->onlyReadsMemory()) {
    *desc << " - only reads memory\n";
    functionPurity[func] = true;
    return true;
  }

  // Whitelisted pure functions from standard libraries.
  if (func->empty() && isWhitelistedPure(func->getName())) {
    *desc << " - whitelisted\n";
    functionPurity[func] = true;
    return true;
  }

  // Empty functions (those for which we don't have a definition) are
  // conservatively marked non-pure.
  if (func->empty()) {
    *desc << " - definition not available\n";
    functionPurity[func] = false;
    return false;
  }

  // Begin by marking the function as non-pure. This avoids an infinite loop
  // for recursive function calls (but is, of course, conservative).
  functionPurity[func] = false;

  std::set<BasicBlock*> blocks;
  for (Function::iterator bi = func->begin(); bi != func->end(); ++bi) {
    blocks.insert(bi);
  }
  std::set<Instruction*> blockers = preciseEscapeCheck(blocks);

  // Add blocker entries to the description.
  *desc << " - blockers: " << blockers.size() << "\n";
  for (std::set<Instruction*>::iterator i = blockers.begin();
      i != blockers.end(); ++i) {
    std::string blockerEntry = instDesc(*(func->getParent()), *i);
    int blockerLine = extractBlockerLine(blockerEntry);
    desc->blocker(blockerLine, blockerEntry);
  }
  if (blockers.empty()) {
    *desc << " - precise-pure function: " <<
        func->getName().str() << "\n";
  } else {
    *desc << " - precise-impure function: " <<
        func->getName().str() << "\n";
  }

  functionPurity[func] = blockers.empty();
  return blockers.empty();
}

char ApproxInfo::ID = 0;
