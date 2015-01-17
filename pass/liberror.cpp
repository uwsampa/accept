#include <iostream>

#include "liberror.h"

using namespace llvm;

namespace {
  void injectAdd(Instruction* inst, int param) {
    std::cerr << "liberror: injectAdd\n" << std::endl;
  }
}

void InjectError::injectInst(Instruction* inst, int param) {
  std::string opcode = inst->getOpcodeName();
  if (opcode == "add") {
    injectAdd(inst, param);
  }
}
