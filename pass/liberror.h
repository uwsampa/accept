#include "llvm/Instruction.h"

class InjectError {
public:
  static void injectInst(llvm::Instruction* inst, int param);
};

