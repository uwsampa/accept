#include "accept.h"

#include <sstream>

using namespace llvm;

const char *FUNC_BARRIER = "pthread_barrier_wait";
const char *FUNC_PARSEC_BARRIER = "_Z19parsec_barrier_waitP16parsec_barrier_t";
bool isBarrier(Instruction *inst) {
  return isCallOf(inst, FUNC_BARRIER) || isCallOf(inst, FUNC_PARSEC_BARRIER);
}

// Given an acquire call or a barrier call, find all the instructions between
// it and a corresponding release call or the next barrier. The instructions
// in the critical section are collected into the set supplied. Returns the
// release/next barrier instruction if one is found or NULL otherwise.
Instruction *ACCEPTPass::findCritSec(Instruction *acq,
                                     std::set<Instruction*> &cs) {
  bool acquired = false;
  BasicBlock *bb = acq->getParent();

  bool isLock;
  if (isAcquire(acq)) {
    isLock = true;
  } else if (isBarrier(acq)) {
    isLock = false;
  } else {
    errs() << "not a critical section entry!\n";
    return NULL;
  }

  // Next, follow jumps until we find the release call.
  while (true) {
    // Look for acquire and release.
    for (BasicBlock::iterator i = bb->begin(); i != bb->end(); ++i) {
      if (acq == i) {
        acquired = true;
      } else if (acquired) {
        if (isLock && isRelease(i)) {
          // TODO check same lock
          return i;
        } else if (!isLock && isBarrier(i)) {
          return i;
        } else if (isAcquire(i)) {
          *log << "nested locks\n";
          return NULL;
        } else if (bb->getTerminator() != i) {
          cs.insert(i);
        }
      }
    }

    // Just to check my logic: the lock must be acquired after we look through
    // the first block (and subsequent blocks).
    if (!acquired) {
      errs() << "not acquired! (this should never happen)\n";
      return NULL;
    }

    // Follow the jump.
    TerminatorInst *term = bb->getTerminator();
    if (!term) {
      *log << "found not-well-formed block\n";
      return NULL;
    } else if (term->getNumSuccessors() != 1) {
      // This is conservative. As long as the control flow reconverges
      // (diamond), we're cool, but we don't handle that yet.
      *log << "control flow divergence at "
           << srcPosDesc(*module, term->getDebugLoc())
           << "\n";
      return NULL;
    } else {
      // Conservative for the same reason.
      bb = term->getSuccessor(0);
      if (!bb->getUniquePredecessor()) {
        *log << "control flow convergence at "
             << srcPosDesc(*module, bb->front().getDebugLoc())
             << "\n";
        return NULL;
      }
    }
  }
}


std::string ACCEPTPass::siteName(std::string kind, Instruction *at) {
  std::stringstream ss;
  ss << kind << " at "
     << srcPosDesc(*module, at->getDebugLoc());
  return ss.str();
}


// Find the critical section beginning with an acquire (or barrier), check for
// approximateness, and return the release (or next barrier). If the critical
// section cannot be identified or is not approximate, return null.
Instruction *ACCEPTPass::findApproxCritSec(Instruction *acq) {
  // Find all the instructions between this acquire and the next release.
  std::set<Instruction*> critSec;
  Instruction *rel = findCritSec(acq, critSec);
  if (!rel) {
    return NULL;
  }

  // Check for precise side effects.
  std::set<Instruction*> blessed;
  blessed.insert(rel);
  critSec.insert(rel);
  std::set<Instruction*> blockers = AI->preciseEscapeCheck(critSec, &blessed);
  *log << "blockers: " << blockers.size() << "\n";
  for (std::set<Instruction*>::iterator i = blockers.begin();
        i != blockers.end(); ++i) {
    *log << " * " << instDesc(*module, *i) << "\n";
  }
  if (blockers.size()) {
    return NULL;
  }

  return rel;
}


bool ACCEPTPass::optimizeAcquire(Instruction *acq, int id) {
  // Generate a name for this opportunity site.
  std::string optName = siteName("lock acquire", acq);
  *log << "---\n" << optName << "\n";

  Instruction *rel = findApproxCritSec(acq);
  if (!rel)
    return false;

  // Success.
  *log << "can elide lock " << id << "\n";
  if (relax) {
    int param = relaxConfig[id];
    if (param) {
      // Remove the acquire and release calls.
      *log << "eliding lock\n";
      acq->eraseFromParent();
      rel->eraseFromParent();
      return true;
    }
  } else {
    relaxConfig[id] = 0;
    configDesc[id] = optName;
  }
  return false;
}



bool ACCEPTPass::optimizeBarrier(Instruction *bar1, int id) {
  std::string optName = siteName("barrier", bar1);
  *log << "---\n" << optName << "\n";

  if (!findApproxCritSec(bar1))
    return false;

  // Success.
  *log << "can elide barrier " << id << "\n";
  if (relax) {
    int param = relaxConfig[id];
    if (param) {
      // Remove the first barrier.
      *log << "eliding barrier wait\n";
      bar1->eraseFromParent();
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
      if (isAcquire(bi) || isBarrier(bi)) {
        bool optimized;
        if (isAcquire(bi))
          optimized = optimizeAcquire(bi, opportunityId);
        else
          optimized = optimizeBarrier(bi, opportunityId);
        changed |= optimized;
        ++opportunityId;
        if (optimized)
          // Stop iterating over this block, since it changed (and there's
          // almost certainly not another critical section in here anyway).
          break;
      }
    }
  }
  return changed;
}
