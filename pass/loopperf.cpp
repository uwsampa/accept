#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "../llvm/lib/Transforms/Utils/LoopUnrollRuntime.cpp"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <algorithm>

#include "accept.h"

#define ACCEPT_LOG ACCEPT_LOG_(AI)

using namespace llvm;

namespace {
  struct LoopPerfPass : public LoopPass {
    static char ID;
    ACCEPTPass *transformPass;
    ApproxInfo *AI;
    Module *module;
    LoopInfo *LI;

    LoopPerfPass() : LoopPass(ID) {}

    virtual bool doInitialization(Loop *loop, LPPassManager &LPM) {
      transformPass = (ACCEPTPass*)sharedAcceptTransformPass;
      AI = transformPass->AI;
      return false;
    }
    virtual bool runOnLoop(Loop *loop, LPPassManager &LPM) {
      if (transformPass->shouldSkipFunc(*(loop->getHeader()->getParent())))
          return false;
      module = loop->getHeader()->getParent()->getParent();
      LI = &getAnalysis<LoopInfo>();
      return tryToOptimizeLoop(loop);
    }
    virtual bool doFinalization() {
      return false;
    }
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      LoopPass::getAnalysisUsage(AU);
      AU.addRequired<LoopInfo>();
    }

    IntegerType *getNativeIntegerType() {
      DataLayout layout(module->getDataLayout());
      return Type::getIntNTy(module->getContext(),
                              layout.getPointerSizeInBits());
    }

    // Assess whether a loop can be optimized and, if so, log some messages and
    // update the configuration map. If optimization is turned on, the
    // configuration map will be used to actually transform the loop. Returns a
    // boolean indicating whether the code was changed (i.e., the loop
    // perforated).
    bool tryToOptimizeLoop(Loop *loop) {
      Instruction *loopStart = loop->getHeader()->begin();
      std::stringstream ss;
      ss << "loop at " << srcPosDesc(*module, loopStart->getDebugLoc());
      std::string loopName = ss.str();

      Description *desc = AI->logAdd("Loop", loopStart);
      *desc << loopName << "\n";

      Instruction *inst = loop->getHeader()->begin();
      Function *func = inst->getParent()->getParent();
      std::string funcName = func->getName().str();

      *desc << " - within function " << funcName << "\n";

      // Look for ACCEPT_FORBID marker.
      if (AI->instMarker(loop->getHeader()->begin()) == markerForbid) {
        *desc << " - optimization forbidden\n";
        return false;
      }

      // We only consider loops for which there is a header (condition), a
      // latch (increment, in "for"), and a preheader (initialization).
      if (!loop->getHeader() || !loop->getLoopLatch()
          || !loop->getLoopPreheader()) {
        *desc << " - loop not in perforatable form\n";
        return false;
      }

      // Skip array constructor loops manufactured by Clang.
      if (loop->getHeader()->getName().startswith("arrayctor.loop")) {
        *desc << " - array constructor\n";
        return false;
      }

      // Skip empty-body loops (where perforation would do nothing anyway).
      if (loop->getNumBlocks() == 2
          && loop->getHeader() != loop->getLoopLatch()) {
        BasicBlock *latch = loop->getLoopLatch();
        if (&(latch->front()) == &(latch->back())) {
          *desc << " - empty body\n";
          return false;
        }
      }

      // Determine whether this is a for-like or while-like loop. This informs
      // the heuristic that determines which parts of the loop to perforate.
      bool isForLike = false;
      if (loop->getHeader()->getName().startswith("for.cond")) {
        *desc << " - for-like loop\n";
        isForLike = true;
      } else {
        *desc << " - while-like loop\n";
      }

      if (transformPass->relax) {
        int param = transformPass->relaxConfig[loopName];
        if (param) {
          *desc << " - perforating with factor 2^" << param << "\n";
          perforateLoop(loop, param, isForLike);
          return true;
        } else {
          *desc << " - not perforating\n";
          return false;
        }
      }

      // Get the body blocks of the loop: those that will not be fully executed
      // during some iterations of a perforated loop.
      std::set<BasicBlock*> bodyBlocks;
      for (Loop::block_iterator bi = loop->block_begin();
            bi != loop->block_end(); ++bi) {
        if (*bi == loop->getHeader()) {
          // Even in perforated loops, the header gets executed every time. So we
          // don't check it.
          continue;
        } else if (isForLike && *bi == loop->getLoopLatch()) {
          // When perforating for-like loops, we also execute the latch each
          // time.
          continue;
        }
        bodyBlocks.insert(*bi);
      }
      if (bodyBlocks.empty()) {
        *desc << " - empty body\n";
        return false;
      }

      // Check for control flow in the loop body. We don't perforate anything
      // with a break, continue, return, etc.
      for (std::set<BasicBlock*>::iterator i = bodyBlocks.begin();
            i != bodyBlocks.end(); ++i) {
        if (loop->isLoopExiting(*i)) {
          *desc << " - contains loop exit\n";
          *desc << " - cannot perforate loop\n";
          return false;
        }
      }

      // Check whether the body of this loop is elidable (precise-pure).
      std::set<Instruction*> blockers = AI->preciseEscapeCheck(bodyBlocks);

      // Print the blockers to the log.
      *desc << " - blockers: " << blockers.size() << "\n";
      for (std::set<Instruction*>::iterator i = blockers.begin();
            i != blockers.end(); ++i) {
        *desc << *i;
      }

      if (!blockers.size()) {
        *desc << " - can perforate loop\n";
        transformPass->relaxConfig[loopName] = 0;
      } else {
        *desc << " - cannot perforate loop\n";
      }

      return false;
    }

    // This class keeps track of some important instructions for a store
    // corresponding to a compound assignment.
    class CmpdAssignInfo {
      public:
        CmpdAssignInfo() : opInst(NULL), varLoad(NULL) {}
        CmpdAssignInfo(BinaryOperator *myOp, LoadInst *myLoad,
            std::vector<Value *> &myIncOperands, std::vector<Value *> &myDecOperands) :
          opInst(myOp), varLoad(myLoad), incOperands(myIncOperands),
              decOperands(myDecOperands) {}
        BinaryOperator *opInst;
        LoadInst *varLoad;
        std::vector<Value *> incOperands;
        std::vector<Value *> decOperands;
    };

    // Search the loop for the most recent StoreInst that precedes the LoadInst
    // and has the same pointer operand.
    StoreInst *findMostRecentStoreInst(Instruction *inst, Loop *loop) {
      if (LoadInst *LI = dyn_cast_or_null<LoadInst>(inst)) {
        BasicBlock *bodyBlock = inst->getParent();

        BasicBlock::InstListType::reverse_iterator revInstItr =
            bodyBlock->getInstList().rend();
        for (BasicBlock::InstListType::reverse_iterator
            i = bodyBlock->getInstList().rbegin();
            i != bodyBlock->getInstList().rend(); i++) {
          if (&*i == inst) {
            revInstItr = ++i;
            break;
          }
        }
        for (BasicBlock::InstListType::reverse_iterator
            i = revInstItr; i != bodyBlock->getInstList().rend(); i++) {
          if (StoreInst *SI = dyn_cast_or_null<StoreInst>(&*i)) {
            if (SI->getPointerOperand() == LI->getPointerOperand()) {
              return SI;
            }
          }
        }
        typedef std::vector<BasicBlock *>::reverse_iterator reverse_block_iterator;
        std::vector<BasicBlock *> loopBlocks = loop->getBlocks();
        reverse_block_iterator revBlkItr =
            ++std::find(loopBlocks.rbegin(), loopBlocks.rend(), bodyBlock);
        for (reverse_block_iterator i = revBlkItr; i != loopBlocks.rend(); i++) {
          for (BasicBlock::InstListType::reverse_iterator
              j = (*i)->getInstList().rbegin();
              j != (*i)->getInstList().rend(); j++) {
            if (StoreInst *SI = dyn_cast_or_null<StoreInst>(&*j)) {
              if (SI->getPointerOperand() == LI->getPointerOperand()) {
                return SI;
              }
            }
          }
        }
      }

      return NULL;
    }

    // This function slices the pointer operand of a store instruction to
    // determine which other instructions need to be preserved.
    void slicePointerOperand(StoreInst *SI, Loop *loop,
        std::set<Instruction *> &insts,
        std::set<Instruction *> &preserved) {
      if (Instruction *inst = dyn_cast_or_null<Instruction>(SI->getPointerOperand())) {
        if (insts.count(inst)) {
          preserved.insert(inst);
          sliceOperandHelper(inst, loop, insts, preserved);
        }
      }
    }

    // This function is a helper function for slicing operands.
    void sliceOperandHelper(Instruction *inst, Loop *loop,
        std::set<Instruction *> &insts,
        std::set<Instruction *> &preserved) {
      BasicBlock *bodyBlock = inst->getParent();

      if (LoadInst *LI = dyn_cast_or_null<LoadInst>(inst)) {
        if (!dyn_cast<AllocaInst>(LI->getPointerOperand())) {
          if (Instruction *pointerOperand =
              dyn_cast_or_null<Instruction>(LI->getPointerOperand())) {
            preserved.insert(pointerOperand);
            sliceOperandHelper(pointerOperand, loop, insts, preserved);
          }
        } else if (StoreInst *SI = findMostRecentStoreInst(inst, loop)) {
          preserved.insert(SI);
          sliceOperandHelper(SI, loop, insts, preserved);
        }
      } else {
        for (Instruction::op_iterator i = inst->op_begin(); i != inst->op_end(); i++) {
          if (Instruction *tmp = dyn_cast_or_null<Instruction>(&*i)) {
            if (insts.count(tmp)) {
              preserved.insert(tmp);
              sliceOperandHelper(tmp, loop, insts, preserved);
            }
          }
        }
      }
    }

    // Find the most recent add/fadd/sub/fsub instruction.
    BinaryOperator *findAddInst(Instruction *inst, Loop *loop, Value *pointerOperand,
        std::set<Instruction *> insts, LoadInst **varLoad,
        std::vector<Value *> &addends, std::vector<Value *> &subtrahends) {
      if (insts.count(inst)) {
        if (StoreInst *storeInst = dyn_cast_or_null<StoreInst>(inst)) {
          if (LoadInst *loadInst = dyn_cast_or_null<LoadInst>(storeInst->getValueOperand())) {
            return findAddInst(loadInst, loop, pointerOperand, insts, varLoad,
                addends, subtrahends);
          } else if (BinaryOperator *opInst =
              dyn_cast_or_null<BinaryOperator>(storeInst->getValueOperand())) {
            if ((opInst->getOpcode() == Instruction::Add) ||
                (opInst->getOpcode() == Instruction::FAdd)) {
              Value *op1 = opInst->getOperand(0);
              Value *op2 = opInst->getOperand(1);
              if (loadsAccumVariable(op1, loop, pointerOperand, insts,
                  varLoad, addends, subtrahends)) {
                addends.push_back(op2);
                return opInst;
              } else if (loadsAccumVariable(op2, loop, pointerOperand, insts,
                  varLoad, addends, subtrahends)) {
                addends.push_back(op1);
                return opInst;
              }
            } else if ((opInst->getOpcode() == Instruction::Sub) ||
                (opInst->getOpcode() == Instruction::FSub)) {
              Value *op1 = opInst->getOperand(0);
              Value *op2 = opInst->getOperand(1);
              if (loadsAccumVariable(op1, loop, pointerOperand, insts, varLoad,
                  addends, subtrahends)) {
                subtrahends.push_back(op2);
                return opInst;
              }
            }
          }
        } else if (LoadInst *loadInst = dyn_cast_or_null<LoadInst>(inst)) {
          if (StoreInst *storeInst = findMostRecentStoreInst(loadInst, loop)) {
            return findAddInst(storeInst, loop, pointerOperand, insts, varLoad,
                addends, subtrahends);
          }
        }
      }

      return NULL;
    }

    // Determine whether an operand involves the accumulation variable.
    bool loadsAccumVariable(Value *operand, Loop *loop, Value *pointerOperand,
        std::set<Instruction *> insts, LoadInst **varLoad,
        std::vector<Value *> &addends, std::vector<Value *> &subtrahends) {
      if (Instruction *inst = dyn_cast_or_null<Instruction>(operand)) {
        if (insts.count(inst)) {
          if (LoadInst *loadInst = dyn_cast_or_null<LoadInst>(inst)) {
            if (loadInst->getPointerOperand() == pointerOperand) {
              *varLoad = loadInst;
              return true;
            } else if (StoreInst *storeInst = findMostRecentStoreInst(loadInst, loop)) {
              return loadsAccumVariable(storeInst, loop, pointerOperand, insts,
                  varLoad, addends, subtrahends);
            }
          } else if (StoreInst *storeInst = dyn_cast_or_null<StoreInst>(inst)) {
            if (LoadInst *loadInst = dyn_cast_or_null<LoadInst>(storeInst->getValueOperand())) {
              return loadsAccumVariable(loadInst, loop, pointerOperand, insts,
                  varLoad, addends, subtrahends);
            }
          } else if (BinaryOperator *opInst = dyn_cast_or_null<BinaryOperator>(operand)) {
            if ((opInst->getOpcode() == Instruction::Add) ||
                (opInst->getOpcode() == Instruction::FAdd)) {
              Value *op1 = opInst->getOperand(0);
              Value *op2 = opInst->getOperand(1);
              if (loadsAccumVariable(op1, loop, pointerOperand, insts, varLoad,
                  addends, subtrahends)) {
                addends.push_back(op2);
                return true;
              } else if (loadsAccumVariable(op2, loop, pointerOperand, insts,
                  varLoad, addends, subtrahends)) {
                addends.push_back(op1);
                return true;
              }
            } else if ((opInst->getOpcode() == Instruction::Sub) ||
                (opInst->getOpcode() == Instruction::FSub)) {
              Value *op1 = opInst->getOperand(0);
              Value *op2 = opInst->getOperand(1);
              if (loadsAccumVariable(op1, loop, pointerOperand, insts, varLoad,
                  addends, subtrahends)) {
                subtrahends.push_back(op2);
                return true;
              }
            }
          }
        }
      }

      return false;
    }

    // Find the most recent mul/fmul/udiv/sdiv/fdiv instruction.
    BinaryOperator *findMulInst(Instruction *inst, Loop *loop, Value *pointerOperand,
        std::set<Instruction *> insts, LoadInst **varLoad,
        std::vector<Value *> &factors, std::vector<Value *> &divisors) {
      if (insts.count(inst)) {
        if (StoreInst *storeInst = dyn_cast_or_null<StoreInst>(inst)) {
          if (LoadInst *loadInst = dyn_cast_or_null<LoadInst>(storeInst->getValueOperand())) {
            return findMulInst(loadInst, loop, pointerOperand, insts, varLoad,
                factors, divisors);
          } else if (BinaryOperator *opInst =
                dyn_cast_or_null<BinaryOperator>(storeInst->getValueOperand())) {
            if ((opInst->getOpcode() == Instruction::Mul) ||
                (opInst->getOpcode() == Instruction::FMul)) {
              Value *op1 = opInst->getOperand(0);
              Value *op2 = opInst->getOperand(1);
              if (loadsScaledVariable(op1, loop, pointerOperand, insts,
                  varLoad, factors, divisors)) {
                factors.push_back(op2);
                return opInst;
              } else if (loadsScaledVariable(op2, loop, pointerOperand, insts,
                  varLoad, factors, divisors)) {
                factors.push_back(op1);
                return opInst;
              }
            } else if ((opInst->getOpcode() == Instruction::UDiv) ||
                (opInst->getOpcode() == Instruction::SDiv) ||
                (opInst->getOpcode() == Instruction::FDiv)) {
              Value *op1 = opInst->getOperand(0);
              Value *op2 = opInst->getOperand(1);
              if (loadsScaledVariable(op1, loop, pointerOperand, insts, varLoad,
                  factors, divisors)) {
                divisors.push_back(op2);
                return opInst;
              }
            }
          }
        } else if (LoadInst *loadInst = dyn_cast_or_null<LoadInst>(inst)) {
          if (StoreInst *storeInst = findMostRecentStoreInst(loadInst, loop)) {
            return findMulInst(storeInst, loop, pointerOperand, insts, varLoad,
                factors, divisors);
          }
        }
      }

      return NULL;
    }

    // Determine whether an operand involves the scaled variable.
    bool loadsScaledVariable(Value *operand, Loop *loop, Value *pointerOperand,
        std::set<Instruction *> insts, LoadInst **varLoad,
        std::vector<Value *> &factors, std::vector<Value *> &divisors) {
      if (Instruction *inst = dyn_cast_or_null<Instruction>(operand)) {
        if (insts.count(inst)) {
          if (LoadInst *loadInst = dyn_cast_or_null<LoadInst>(inst)) {
            if (loadInst->getPointerOperand() == pointerOperand) {
              *varLoad = loadInst;
              return true;
            } else if (StoreInst *storeInst = findMostRecentStoreInst(loadInst, loop)) {
              return loadsScaledVariable(storeInst, loop, pointerOperand, insts,
                  varLoad, factors, divisors);
            }
          } else if (StoreInst *storeInst = dyn_cast_or_null<StoreInst>(inst)) {
            if (LoadInst *loadInst = dyn_cast_or_null<LoadInst>(storeInst->getValueOperand())) {
              return loadsScaledVariable(loadInst, loop, pointerOperand, insts,
                  varLoad, factors, divisors);
            }
          } else if (BinaryOperator *opInst = dyn_cast_or_null<BinaryOperator>(operand)) {
            if ((opInst->getOpcode() == Instruction::Mul) ||
                (opInst->getOpcode() == Instruction::FMul)) {
              Value *op1 = opInst->getOperand(0);
              Value *op2 = opInst->getOperand(1);
              if (loadsScaledVariable(op1, loop, pointerOperand, insts, varLoad,
                  factors, divisors)) {
                factors.push_back(op2);
                return true;
              } else if (loadsScaledVariable(op2, loop, pointerOperand, insts,
                  varLoad, factors, divisors)) {
                factors.push_back(op1);
                return true;
              }
            } else if ((opInst->getOpcode() == Instruction::UDiv) ||
                (opInst->getOpcode() == Instruction::SDiv) ||
                (opInst->getOpcode() == Instruction::FDiv)) {
              Value *op1 = opInst->getOperand(0);
              Value *op2 = opInst->getOperand(1);
              if (loadsScaledVariable(op1, loop, pointerOperand, insts, varLoad,
                  factors, divisors)) {
                divisors.push_back(op2);
                return true;
              }
            }
          }
        }
      }

      return false;
    }

    // Transform a loop to skip iterations.
    // The loop should already be validated as perforatable, but checks will be
    // performed nonetheless to ensure safety.
    void perforateLoop(Loop *loop, int logfactor, bool isForLike) {
      // Check whether this loop is perforatable.
      // First, check for required blocks.
      if (!loop->getHeader() || !loop->getLoopLatch()
          || !loop->getLoopPreheader() || !loop->getExitBlock()) {
        errs() << "malformed loop\n";
        return;
      }

      // Next, make sure the header (condition block) ends with a body/exit
      // conditional branch.
      BranchInst *condBranch = dyn_cast_or_null<BranchInst>(
          loop->getHeader()->getTerminator()
      );
      if (!condBranch || condBranch->getNumSuccessors() != 2) {
        errs() << "malformed loop condition\n";
        return;
      }
      BasicBlock *bodyBlock;
      if (condBranch->getSuccessor(0) == loop->getExitBlock()) {
        bodyBlock = condBranch->getSuccessor(1);
      } else if (condBranch->getSuccessor(1) == loop->getExitBlock()) {
        bodyBlock = condBranch->getSuccessor(0);
      } else {
        errs() << "loop condition does no exit\n";
        return;
      }

      // Get the shortcut for the destination. In for-like loop perforation, we
      // shortcut to the latch (increment block). In while-like perforation, we
      // jump to the header (condition block).
      BasicBlock *shortcutDest;
      if (isForLike) {
        shortcutDest = loop->getLoopLatch();
      } else {
        shortcutDest = loop->getHeader();
      }

      // Collect the set of instructions in the loop body.
      std::set<Instruction *> insts;
      std::set<StoreInst *> storeInsts;
      std::set<Instruction *> preserved;
      std::set<BasicBlock *> withPreservedInsts;

      Loop::block_iterator bodyBlockItr =
          std::find(loop->block_begin(), loop->block_end(), bodyBlock);
      for (Loop::block_iterator bi = bodyBlockItr; bi != loop->block_end(); bi++) {
        if (isForLike && (*bi == loop->getLoopLatch())) {
          continue;
        }

        for (BasicBlock::iterator i = (*bi)->begin(); i != (*bi)->end(); i++) {
          insts.insert(i);
          // Collect the set of store instructions.
          if (StoreInst *SI = dyn_cast_or_null<StoreInst>(&*i)) {
            storeInsts.insert(SI);
          }
        }
      }

      // Map the set of store instructions that correspond to accumulations
      // or scalings in the loop body.
      std::map<StoreInst *, CmpdAssignInfo> cmpdAssignStores, accumStores, scaleStores;
      for (std::set<StoreInst *>::iterator
          i = storeInsts.begin(); i != storeInsts.end(); i++) {
        LoadInst *varLoad;
        std::vector<Value *> incOperands, decOperands;
        if (BinaryOperator *opInst =
            findAddInst(*i, loop, (*i)->getPointerOperand(), insts, &varLoad,
            incOperands, decOperands)) {
          preserved.insert(*i);
          preserved.insert(opInst);
          accumStores[*i] = CmpdAssignInfo(opInst, varLoad, incOperands, decOperands);
          cmpdAssignStores[*i] = CmpdAssignInfo(opInst, varLoad, incOperands, decOperands);
        } else if (BinaryOperator *opInst =
            findMulInst(*i, loop, (*i)->getPointerOperand(), insts, &varLoad,
            incOperands, decOperands)) {
          preserved.insert(*i);
          preserved.insert(opInst);
          scaleStores[*i] = CmpdAssignInfo(opInst, varLoad, incOperands, decOperands);
          cmpdAssignStores[*i] = CmpdAssignInfo(opInst, varLoad, incOperands, decOperands);
        }
      }

      // Collect the set of store instructions to preserve that do not
      // correspond to accumulations or scalings.
      std::set<StoreInst *> preservedStores;
      for (std::set<StoreInst *>::iterator i = storeInsts.begin(); i != storeInsts.end(); i++) {
        if (AI->storeEscapes(*i, insts) && !cmpdAssignStores.count(*i)) {
          preservedStores.insert(*i);
        }
      }

      // Slice the pointer operand of each of the store instructions corresponding to
      // a compound assignment (accumulation or scaling).
      for (std::map<StoreInst *, CmpdAssignInfo>::iterator
          i = cmpdAssignStores.begin(); i != cmpdAssignStores.end(); i++) {
        StoreInst *SI = i->first;
        preserved.insert(SI);
        slicePointerOperand(SI, loop, insts, preserved);
      }

      // Slice the pointer operand of each of the preserved store instructions
      // that do not correspond to accumulations.
      for (std::set<StoreInst *>::iterator i = preservedStores.begin();
          i != preservedStores.end(); i++) {
        preserved.insert(*i);
        slicePointerOperand(*i, loop, insts, preserved);
      }

      // Slice the condition of each of the branch instructions.
      for (std::set<Instruction *>::iterator i = insts.begin(); i != insts.end(); i++) {
        if (BranchInst *BI = dyn_cast_or_null<BranchInst>(*i)) {
          preserved.insert(BI);
          sliceOperandHelper(BI, loop, insts, preserved);
        }
      }

      // Keep track of the body blocks that contain instructions
      // that have been marked for preservation.
      for (std::set<Instruction *>::iterator i = preserved.begin();
          i != preserved.end(); i++) {
        withPreservedInsts.insert((*i)->getParent());
      }

      // Keep track of the preheader block and latch block of the original loop.
      BasicBlock *preheaderBlock = loop->getLoopPreheader();
      BasicBlock *latchBlock = loop->getLoopLatch();

      std::vector<BasicBlock *> blockClones;
      LoopBlocksDFS LoopBlocks(loop);
      LoopBlocks.perform(LI);
      ValueToValueMapTy VMap, LVMap;

      // Clone the blocks of the loop. The block clones will be inserted
      // between the preheader block and latch block of the original loop.
      CloneLoopBlocks(loop, true, preheaderBlock, latchBlock, blockClones,
          LoopBlocks, VMap, LVMap, LI);

      // Replace the operands of all cloned instructions, as well as the
      // incoming blocks of all PHI nodes, with their cloned counterparts.
      for (ValueToValueMapTy::iterator i = VMap.begin(); i != VMap.end(); i++) {
        if (Instruction *origInst = (Instruction *) dyn_cast_or_null<Instruction>(i->first)) {
          if (Instruction *clonedInst = dyn_cast_or_null<Instruction>(&*(i->second))) {
            for (unsigned j = 0; j != clonedInst->getNumOperands(); j++) {
              if (VMap.count(origInst->getOperand(j))) {
                clonedInst->setOperand(j, VMap[origInst->getOperand(j)]);
              }
            }
            if (PHINode *origPN = dyn_cast_or_null<PHINode>(origInst)) {
              if (PHINode *clonedPN = dyn_cast_or_null<PHINode>(clonedInst)) {
                for (unsigned j = 0; j != clonedPN->getNumOperands(); j++) {
                  if (VMap.count(origPN->getIncomingBlock(j))) {
                    clonedPN->setIncomingBlock(j,
                        (BasicBlock *) &*(VMap[origPN->getIncomingBlock(j)]));
                  }
                }
              }
            }
          }
        }
      }

      // Identify the clones of the header block and latch block of the
      // original loop. Also mark the instruction clones that will be removed.
      std::set<Instruction *> removed;
      BasicBlock *clonedHeader = NULL;
      BasicBlock *clonedLatch = NULL;

      for (ValueToValueMapTy::iterator i = VMap.begin(); i != VMap.end(); i++) {
        if (Instruction *origInst = (Instruction *) dyn_cast_or_null<Instruction>(i->first)) {
          if (Instruction *clonedInst = dyn_cast_or_null<Instruction>(&*(i->second))) {
            if (!preserved.count(origInst)) {
              if (origInst->getParent() == loop->getHeader()) {
                if (!clonedHeader) {
                  clonedHeader = clonedInst->getParent();
                }
              } else if (origInst->getParent() == loop->getLoopLatch()) {
                if (!clonedLatch) {
                  clonedLatch = clonedInst->getParent();
                }
              } else if (withPreservedInsts.count(origInst->getParent())) {
                removed.insert(clonedInst);
              }
            }
          }
        }
      }

      // Identify the clone of the first body block.
      BranchInst *clonedCondBranch = dyn_cast_or_null<BranchInst>(
          clonedHeader->getTerminator()
      );
      BasicBlock *clonedBodyBlock;
      if (clonedCondBranch->getSuccessor(0) == loop->getExitBlock()) {
        clonedBodyBlock = clonedCondBranch->getSuccessor(1);
      } else if (clonedCondBranch->getSuccessor(1) == loop->getExitBlock()) {
        clonedBodyBlock = clonedCondBranch->getSuccessor(0);
      }

      // Identify the clones of the predecessors of the shortcut
      // destination block.
      std::set<BasicBlock *> clonedShortcutPreds;
      for (std::vector<BasicBlock *>::iterator i = blockClones.begin();
          i != blockClones.end(); i++) {
        BasicBlock *curBlock = *i;
        if (isForLike) {
          if (curBlock->getTerminator()->getSuccessor(0) == clonedLatch) {
            clonedShortcutPreds.insert(curBlock);
          }
        } else {
          if (curBlock->getTerminator()->getSuccessor(0) == latchBlock) {
            clonedShortcutPreds.insert(curBlock);
          }
        }
      }

      // Delete the header block clone.
      std::vector<BasicBlock *>::iterator findItr =
          std::find(blockClones.begin(), blockClones.end(), clonedHeader);
      blockClones.erase(findItr);
      for (BasicBlock::iterator i = clonedHeader->begin(); i != clonedHeader->end(); i++) {
        i->dropAllReferences();
      }
      clonedHeader->removeFromParent();
      LI->removeBlock(clonedHeader);

      // Delete the latch block clone only if the loop is for-like.
      if (isForLike) {
        findItr = std::find(blockClones.begin(), blockClones.end(), clonedLatch);
        blockClones.erase(findItr);
        for (BasicBlock::iterator i = clonedLatch->begin(); i != clonedLatch->end(); i++) {
          i->dropAllReferences();
        }
        clonedLatch->removeFromParent();
        LI->removeBlock(clonedLatch);
      }

      // Set the operands of some crucial preserved instructions to null values,
      // in order to eliminate references to instructions that will be removed.
      for (std::set<StoreInst *>::iterator i = preservedStores.begin();
          i != preservedStores.end(); i++) {
        if (StoreInst *clonedStore = dyn_cast_or_null<StoreInst>(&*(VMap[*i]))) {
          clonedStore->setOperand(0, Constant::getNullValue((*i)->getValueOperand()->getType()));
        }
      }

      for (std::map<StoreInst *, CmpdAssignInfo>::iterator
          i = cmpdAssignStores.begin(); i != cmpdAssignStores.end(); i++) {
        StoreInst *origStore = i->first;
        if (StoreInst *clonedStore = dyn_cast_or_null<StoreInst>(&*(VMap[origStore]))) {
          clonedStore->setOperand(0, Constant::getNullValue(origStore->getValueOperand()->getType()));
        }

        BinaryOperator *origOp = (i->second).opInst;
        if (BinaryOperator *clonedOp = dyn_cast_or_null<BinaryOperator>(&*(VMap[origOp]))) {
          clonedOp->setOperand(0, Constant::getNullValue(origOp->getOperand(0)->getType()));
          clonedOp->setOperand(1, Constant::getNullValue(origOp->getOperand(1)->getType()));
        }
      }

      // Drop all the references of each of the instructions that will be removed,
      // then remove the instructions one by one.
      for (std::set<Instruction *>::iterator i = removed.begin(); i != removed.end(); i++) {
        (*i)->dropAllReferences();
      }
      for (std::set<Instruction *>::iterator i = removed.begin(); i != removed.end(); i++) {
        (*i)->removeFromParent();
      }

      // Change the preheader block to point to the condition block again.
      preheaderBlock->getTerminator()->setSuccessor(0, condBranch->getParent());

      IRBuilder<> builder(module->getContext());
      Value *result;

      // Allocate stack space for the counter.
      // LLVM "alloca" instructions go in the function's entry block. Otherwise,
      // they have to adjust the frame size dynamically (and, in my experience,
      // can actually segfault!). And we only want one of these per static loop
      // anyway.
      builder.SetInsertPoint(
          loop->getLoopPreheader()->getParent()->getEntryBlock().begin()
      );

      IntegerType *nativeInt = getNativeIntegerType();
      AllocaInst *counterAlloca = builder.CreateAlloca(
          nativeInt,
          0,
          "accept_counter"
      );

      // Initialize the counter in the preheader.
      builder.SetInsertPoint(loop->getLoopPreheader()->getTerminator());
      builder.CreateStore(
          ConstantInt::get(nativeInt, 0, false),
          counterAlloca
      );

      // Increment the counter in the latch.
      builder.SetInsertPoint(loop->getLoopLatch()->getTerminator());
      result = builder.CreateLoad(
          counterAlloca,
          "accept_tmp"
      );
      result = builder.CreateAdd(
          result,
          ConstantInt::get(nativeInt, 1, false),
          "accept_inc"
      );
      builder.CreateStore(
          result,
          counterAlloca
      );

      // Check the counter before the loop's body.
      BasicBlock *checkBlock = BasicBlock::Create(
          module->getContext(),
          "accept_cond",
          bodyBlock->getParent(),
          bodyBlock
      );
      builder.SetInsertPoint(checkBlock);
      result = builder.CreateLoad(
          counterAlloca,
          "accept_tmp"
      );
      // Check whether the low n bits of the counter are zero.
      result = builder.CreateTrunc(
          result,
          Type::getIntNTy(module->getContext(), logfactor),
          "accept_trunc"
      );
      result = builder.CreateIsNull(
          result,
          "accept_cmp"
      );
      result = builder.CreateCondBr(
          result,
          bodyBlock,
          clonedBodyBlock
      );

      // Change the condition block to point to our new condition
      // instead of the body.
      condBranch->setSuccessor(0, checkBlock);

      // Add condition block to the loop structure.
      loop->addBasicBlockToLoop(checkBlock, LI->getBase());

      // Change the clone of each predecessor of the shortcut for the destination
      // to point to the shortcut. In for-like loop perforation, we shortcut to
      // the latch (increment block). In while-like perforation, we jump to the
      // header (condition block).
      for (std::set<BasicBlock *>::iterator i = clonedShortcutPreds.begin();
          i != clonedShortcutPreds.end(); i++) {
        (*i)->getTerminator()->setSuccessor(0, shortcutDest);
      }

      // Add all cloned body blocks to the loop structure.
      for (std::vector<BasicBlock *>::iterator
          i = blockClones.begin(); i != blockClones.end(); i++) {
        if (const Loop *curLoop = (*LI)[*i]) {
          errs() << "Block " << (*i)->getName() << " is already in a loop:\n"
            << *curLoop << "...but we want it to be in:\n" << *loop;
          LI->removeBlock(*i);
        }
        loop->addBasicBlockToLoop(*i, LI->getBase());
      }

      // Insert instructions to store and load values saved from evaluated iterations.
      for (std::set<StoreInst *>::iterator i = preservedStores.begin();
          i != preservedStores.end(); i++) {
        StoreInst *SI = *i;

        // Insert an AllocaInst for the saved value.
        builder.SetInsertPoint(
            loop->getLoopPreheader()->getParent()->getEntryBlock().begin()
        );

        AllocaInst *valueAlloca = builder.CreateAlloca(
            SI->getValueOperand()->getType(),
            0,
            "accept_value"
        );

        // Insert a StoreInst to store the saved value.
        builder.SetInsertPoint(SI);
        builder.CreateStore(
            SI->getValueOperand(),
            valueAlloca
        );

        // Insert a LoadInst to load the saved value.
        Instruction *clonedStore = (Instruction *) &*VMap[SI];
        builder.SetInsertPoint(clonedStore);
        result = builder.CreateLoad(
            valueAlloca,
            "accept_load"
        );

        // Store the saved value to the output variable.
        clonedStore->setOperand(0, result);
      }

      // Insert instructions for the accumulation optimization.
      for (std::map<StoreInst *, CmpdAssignInfo>::iterator i = accumStores.begin();
          i != accumStores.end(); i++) {
        StoreInst *storeInst = i->first;
        BinaryOperator *opInst = (i->second).opInst;
        LoadInst *varLoad = (i->second).varLoad;
        std::vector<Value *> addends = (i->second).incOperands;
        std::vector<Value *> subtrahends = (i->second).decOperands;

        // Insert an AllocaInst for the accumulation increment/decrement value.
        builder.SetInsertPoint(
            loop->getLoopPreheader()->getParent()->getEntryBlock().begin()
        );

        AllocaInst *accumAlloca = builder.CreateAlloca(
            storeInst->getValueOperand()->getType(),
            0,
            "accept_accum"
        );

        // If there is only one operand, simply insert a StoreInst to store the
        // accumulation increment/decrement value. Otherwise, insert a sub/fsub
        // instruction to find the net increment/decrement to be stored.
        builder.SetInsertPoint(storeInst);
        Value *incValue;

        if (addends.size() == 1 && !subtrahends.size()) {
          incValue = *(addends.begin());
        } else if (!addends.size() && subtrahends.size() == 1) {
          incValue = *(subtrahends.begin());
        } else {
          Value *operand1, *operand2;
          if ((opInst->getOpcode() == Instruction::Add) ||
              (opInst->getOpcode() == Instruction::FAdd)) {
            operand1 = storeInst->getValueOperand();
            operand2 = varLoad;
          } else if ((opInst->getOpcode() == Instruction::Sub) ||
              (opInst->getOpcode() == Instruction::FSub)) {
            operand1 = varLoad;
            operand2 = storeInst->getValueOperand();
          }

          if (storeInst->getValueOperand()->getType()->isIntOrIntVectorTy()) {
            result = builder.CreateSub(
                operand1,
                operand2,
                "accept_accumDiff"
            );
          } else {
            result = builder.CreateFSub(
                operand1,
                operand2,
                "accept_accumFDiff"
            );
          }

          incValue = result;
        }

        builder.CreateStore(
            incValue,
            accumAlloca
        );

        // Insert a LoadInst to load the accumulation variable.
        Instruction *clonedOp = (Instruction *) &*VMap[opInst];
        builder.SetInsertPoint(clonedOp);

        Value *accumVar;
        if (VMap.count(storeInst->getPointerOperand())) {
          accumVar = VMap[storeInst->getPointerOperand()];
        } else {
          accumVar = storeInst->getPointerOperand();
        }
        result = builder.CreateLoad(
            accumVar,
            "accept_accumVar"
        );

        // Insert a LoadInst to load the accumulation increment/decrement value.
        Value *result2 = builder.CreateLoad(
            accumAlloca,
            "accept_accumInc"
        );

        // Change the operands of the cloned BinaryOperator to be the two LoadInsts.
        clonedOp->setOperand(0, result);
        clonedOp->setOperand(1, result2);

        // Change the value operand of the cloned accumulation StoreInst to be
        // the cloned BinaryOperator.
        Instruction *clonedStore = (Instruction *) &*VMap[storeInst];
        clonedStore->setOperand(0, clonedOp);
      }

      // Insert instructions for the scaling optimization.
      for (std::map<StoreInst *, CmpdAssignInfo>::iterator i = scaleStores.begin();
          i != scaleStores.end(); i++) {
        StoreInst *storeInst = i->first;
        BinaryOperator *opInst = (i->second).opInst;
        LoadInst *varLoad = (i->second).varLoad;
        std::vector<Value *> factors = (i->second).incOperands;
        std::vector<Value *> divisors = (i->second).decOperands;

        // Insert an AllocaInst for the scale factor value.
        builder.SetInsertPoint(
            loop->getLoopPreheader()->getParent()->getEntryBlock().begin()
        );

        AllocaInst *accumAlloca = builder.CreateAlloca(
            storeInst->getValueOperand()->getType(),
            0,
            "accept_scale"
        );

        // If there is only one operand, simply insert a StoreInst to store the
        // scale factor value. Otherwise, insert a sdiv/fdiv instruction
        // to find the net scale factor to be stored.
        builder.SetInsertPoint(storeInst);
        Value *factorValue;

        if (factors.size() == 1 && !divisors.size()) {
          factorValue = *(factors.begin());
        } else if (!factors.size() && divisors.size() == 1) {
          factorValue = *(divisors.begin());
        } else {
          Value *operand1, *operand2;
          if ((opInst->getOpcode() == Instruction::Mul) ||
              (opInst->getOpcode() == Instruction::FMul)) {
            operand1 = storeInst->getValueOperand();
            operand2 = varLoad;
          } else if ((opInst->getOpcode() == Instruction::UDiv) ||
              (opInst->getOpcode() == Instruction::SDiv) ||
              (opInst->getOpcode() == Instruction::FDiv)) {
            operand1 = varLoad;
            operand2 = storeInst->getValueOperand();
          }

          if (storeInst->getValueOperand()->getType()->isIntOrIntVectorTy()) {
            result = builder.CreateSDiv(
                operand1,
                operand2,
                "accept_scaleQuo"
            );
          } else {
            result = builder.CreateFDiv(
                operand1,
                operand2,
                "accept_scaleFQuo"
            );
          }

          factorValue = result;
        }

        builder.CreateStore(
            factorValue,
            accumAlloca
        );

        // Insert a LoadInst to load the scaled variable.
        Instruction *clonedOp = (Instruction *) &*VMap[opInst];
        builder.SetInsertPoint(clonedOp);

        Value *scaleVar;
        if (VMap.count(storeInst->getPointerOperand())) {
          scaleVar = VMap[storeInst->getPointerOperand()];
        } else {
          scaleVar = storeInst->getPointerOperand();
        }
        result = builder.CreateLoad(
            scaleVar,
            "accept_scaleVar"
        );

        // Insert a LoadInst to load the accumulation increment/decrement value.
        Value *result2 = builder.CreateLoad(
            accumAlloca,
            "accept_scaleFactor"
        );

        // Change the operands of the cloned BinaryOperator to be the two LoadInsts.
        clonedOp->setOperand(0, result);
        clonedOp->setOperand(1, result2);

        // Change the value operand of the cloned scaling StoreInst to be
        // the cloned BinaryOperator.
        Instruction *clonedStore = (Instruction *) &*VMap[storeInst];
        clonedStore->setOperand(0, clonedOp);
      }
    }
  };
}

char LoopPerfPass::ID = 0;
LoopPass *llvm::createLoopPerfPass() { return new LoopPerfPass(); }

