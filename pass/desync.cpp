#include "accept.h"

#include <sstream>

using namespace llvm;

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


bool ACCEPTPass::findCritSec(Instruction *acq, std::set<Instruction*> &cs) {
  bool acquired = false;

  // Start with the remainder of the containing block.
  BasicBlock *bb = acq->getParent();
  for (BasicBlock::iterator i = bb->begin(); i != bb->end(); ++i) {
    if (acq == i) {
      acquired = true;
    } else if (acquired) {
      if (isRelease(i)) {
        return true;
      } else if (isAcquire(i)) {
        *log << "nested locks\n";
        return false;
      } else if (bb->getTerminator() != i) {
        cs.insert(i);
      }
    }
  }

  // Next, follow jumps until we find the release call.
  while (true) {
    // Follow the jump.
    TerminatorInst *term = bb->getTerminator();
    if (!term) {
      *log << "found not-well-formed block\n";
      return false;
    } else if (term->getNumSuccessors() != 1) {
      // This is conservative. As long as the control flow reconverges
      // (diamond), we're cool, but we don't handle that yet.
      *log << "control flow divergence at "
           << srcPosDesc(*module, term->getDebugLoc())
           << "\n";
      return false;
    } else {
      // Conservative for the same reason.
      bb = term->getSuccessor(0);
      if (!bb->getUniquePredecessor()) {
        *log << "control flow convergence at "
             << srcPosDesc(*module, bb->front().getDebugLoc())
             << "\n";
        return false;
      }
    }

    // Look for release.
    for (BasicBlock::iterator i = bb->begin(); i != bb->end(); ++i) {
      if (isRelease(i)) {
        return true;
      } else if (isAcquire(i)) {
        *log << "nested locks\n";
        return false;
      } else if (bb->getTerminator() != i) {
        cs.insert(i);
      }
    }
  }
}


bool ACCEPTPass::optimizeAcquire(Instruction *acq, int id) {
  // Generate a name for this opportunity site.
  std::stringstream ss;
  ss << "lock acquire at "
      << srcPosDesc(*module, acq->getDebugLoc());
  std::string optName = ss.str();
  *log << "---\n" << optName << "\n";

  // Find all the instructions between this acquire and the next release.
  std::set<Instruction*> critSec;
  if (!findCritSec(acq, critSec)) {
    return false;
  }

  // TODO check same lock
  // TODO check for approximateness

  // Success.
  *log << "can elide lock " << id << "\n";
  if (relax) {
    int param = relaxConfig[id];
    if (param) {
      *log << "eliding lock\n";
      // TODO remove it
      return true;
    }
  } else {
    relaxConfig[id] = 0;
    configDesc[id] = optName;
  }
  return false;
}


bool ACCEPTPass::optimizeSync(Function &F) {
  bool changed = false;
  for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {
    for (BasicBlock::iterator bi = fi->begin(); bi != fi->end(); ++bi) {
      if (isAcquire(bi)) {
        changed |= optimizeAcquire(bi, opportunityId);
        ++opportunityId;
      }
    }
  }
  return changed;
}
