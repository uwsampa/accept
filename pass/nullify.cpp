#include "accept.h"

#include "llvm/Support/InstIterator.h"

using namespace llvm;

bool ACCEPTPass::nullifyApprox(Function &F) {
  bool modified = false;

  if (shouldSkipFunc(F)) {
    *log << "skipping function " << F.getName() << "\n";
    return false;
  }


  // Mark calls to precise-pure functions for removal.
  std::set<CallInst *> callsToRemove;
  for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
    Function *callee;
    CallInst *call;

    if ((call = dyn_cast<CallInst>(&*I))) {
      callee = call->getCalledFunction();
    } else if (InvokeInst *invoke = dyn_cast<InvokeInst>(&*I)) {
      callee = invoke->getCalledFunction();
    }

    if (!call || !callee)
      continue;

    if (shouldSkipFunc(*callee))
      continue;

    if (AI->isPrecisePure(callee)) {
      // We should be able to remove the call to this precise-pure function
      std::string optName = "nullable call at " +
        srcPosDesc(*module, I->getDebugLoc());
      *log << "---\n" << optName << "\n"
        << "can remove call to precise-pure function " << callee->getName()
        << "\n";
      if (relax) {
        if (relaxConfig[optName])
          callsToRemove.insert((CallInst *)&*I);
      } else {
        *log << "but not removing because relax=0\n";
        relaxConfig[optName] = 0;
      }
    }
  }

  // Actually remove all the Instructions marked for removal.
  for (std::set<CallInst *>::iterator I = callsToRemove.begin(),
      E = callsToRemove.end(); I != E; ++I) {
    CallInst *call = *I;
    assert(call != NULL);

    *log << "removing call to precise-pure function "
      << call->getCalledFunction()->getName() << "\n";
    call->replaceAllUsesWith(UndefValue::get(call->getType()));
    call->eraseFromParent();
    modified = true;
  }

  // Step 2: Remove precise-pure BBs.  Note that a Function may include both
  // precise-pure and precise-impure BBs.
  // XXX consult a global counter instead; perforate code according to
  // counter value
  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
    std::set<BasicBlock*> bbSingleton;
    bbSingleton.insert(BB);
    const Instruction &front = BB->front();
    std::string pos = srcPosDesc(*module, front.getDebugLoc());

    std::set<Instruction*> blockers = AI->preciseEscapeCheck(bbSingleton);
    if (blockers.empty()) {
      // Remove this precise-pure BB
      std::string optName = "nullable BB at " + pos;
      *log << "---\n" << optName << "\n"
        << "can remove precise-pure BB at " << pos << "\n";
      if (relax) {
        if (relaxConfig[optName]) {
          *log << "removing precise-pure BB at " << pos << "\n";
          while (BB->begin() != BB->end()
              && &BB->front() != BB->getTerminator()) {
            Instruction *I = BB->begin();
            I->replaceAllUsesWith(UndefValue::get(I->getType()));
            I->eraseFromParent();
          }
          modified = true;
        }
      } else {
        *log << "but not removing because relax=0\n";
        relaxConfig[optName] = 0;
      }
    }
  }

  return modified;
}
