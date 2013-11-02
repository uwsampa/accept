#include "accept.h"

#include "llvm/Support/InstIterator.h"

using namespace llvm;

bool ACCEPTPass::nullifyApprox(Function &F) {
  bool modified = false;

  *log << "Nullifying precise-pure instrs in function " << F.getName()
    << "\n";

  // Remove calls to precise-pure functions.
  for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
    Function *callee;
    if (CallInst *call = dyn_cast<CallInst>(&*I)) {
      callee = call->getCalledFunction();
    } else if (InvokeInst *invoke = dyn_cast<InvokeInst>(&*I)) {
      callee = invoke->getCalledFunction();
    }

    if (callee && AI->isPrecisePure(callee)) {
      *log << "Removing call to precise-pure function "
        << callee->getName() << "\n";
      I->eraseFromParent();
      modified = true;
    }
  }

  // Step 2: Remove precise-pure BBs.  Note that a Function may include both
  // precise-pure and precise-impure BBs.
  // XXX consult a global counter instead; perforate code according to
  // counter value
  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
    std::set<BasicBlock*> bbSingleton;
    bbSingleton.insert(BB);

    std::set<Instruction*> blockers = AI->preciseEscapeCheck(bbSingleton);
    if (blockers.empty()) {
      *log << "Removing precise-pure basic block " << BB->getName() << "\n";
      BB->eraseFromParent();
      modified = true;
    }
  }

  return modified;
}
