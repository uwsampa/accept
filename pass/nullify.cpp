#include "accept.h"

#include "llvm/Support/InstIterator.h"

using namespace llvm;

bool ACCEPTPass::nullifyApprox(Function &F) {
  bool modified = false;
  int relaxParam = 0;

  std::string optName = "nullify at " +
    srcPosDesc(*module, inst_begin(&F)->getDebugLoc());
  *log << "---\n" << optName << "\n";
  if (relax)
    relaxParam = relaxConfig[optName];
  else
    relaxConfig[optName] = 0;

  *log << "Nullifying precise-pure instrs in function " << F.getName()
    << "\n";

  // Mark calls to precise-pure functions for removal.
  std::set<CallInst *> callsToRemove;
  for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
    Function *callee;
    CallInst *call;

    if (call = dyn_cast<CallInst>(&*I)) {
      callee = call->getCalledFunction();
    } else if (InvokeInst *invoke = dyn_cast<InvokeInst>(&*I)) {
      callee = invoke->getCalledFunction();
    } else {
      // This is not a call or invoke instruction.
      continue;
    }

    assert(call != NULL);
    assert(callee != NULL);

    if (shouldSkipFunc(*callee))
      continue;

    if (AI->isPrecisePure(callee)) {
      *log << "can remove call to precise-pure function "
        << callee->getName() << "\n";
      if (relax && relaxParam) {
        callsToRemove.insert((CallInst *)&*I);
      }
    }
  }

  // Actually remove all the Instructions marked for removal.
  for (std::set<CallInst *>::iterator I = callsToRemove.begin(),
      E = callsToRemove.end(); I != E; ++I) {
    CallInst *call = *I;
    assert(call != NULL);

    *log << "removing call to precise-pure function " << *call << "\n";
    call->replaceAllUsesWith(UndefValue::get(call->getType()));
    call->eraseFromParent();
    modified = true;
  }

  /*
  // Step 2: Remove precise-pure BBs.  Note that a Function may include both
  // precise-pure and precise-impure BBs.
  // XXX consult a global counter instead; perforate code according to
  // counter value
  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
    std::set<BasicBlock*> bbSingleton;
    bbSingleton.insert(BB);

    std::set<Instruction*> blockers = AI->preciseEscapeCheck(bbSingleton);
    if (blockers.empty()) {
      if (relax && relaxParam) {
        *log << "Removing precise-pure basic block " << BB->getName() << "\n";
        BB->eraseFromParent();
        modified = true;
      }
    }
  }
  */

  return modified;
}
