#!/usr/bin/env python
import argparse
import inspect
import logging
import numpy as np
import os
import shlex
import subprocess32 as subprocess

from functools import partial

# LLVM instruction categories
controlInsn = ['br','switch','indirectbr','ret']
terminatorInsn = ['invoke','resume','catchswitch','catchret','cleanupret','unreachable']
binaryInsn = ['fadd','fsub','fmul','fdiv','frem','add','sub','mul','udiv','sdiv','urem','srem','shl','lshr','ashr','and','or','xor']
vectorInsn = ['extractelement','insertelement','shufflevector']
aggregateInsn = ['extractvalue','insertvalue']
loadstoreInsn = ['load','store']
getelementptrInsn = ['getelementptr']
memoryInsn = ['alloca','fence','cmpxchg','atomicrmw']
conversionInsn = ['trunc','zext','sext','fptrunc','fpext','fptoui','fptosi','uitofp','sitofp','ptrtoint','inttoptr','bitcast','addrspacecast']
cmpInsn = ['icmp','fcmp']
callInsn = ['call']
phiInsn = ['phi']
otherInsn = ['select','va_arg','landingpad','catchpad','cleanuppad']

# Standard math
stdMathFunc = ["abs","labs","llabs","div","ldiv","lldiv","imaxabs","imaxdiv","fabs","fabsf","fabsl","fmod","fmodf","fmodl","remainder","remainderf","remainderl","remquo","remquof","remquol","fma","fmaf","fmal","fmax","fmaxf","fmaxl","fmin","fminf","fminl","fdim","fdimf","fdiml","exp","expf","expl","exp2","exp2f","exp2l","expm1","expm1f","expm1l","log","logf","logl","log10","log10f","log10l","log2","log2f","log2l","log1p","log1pf","log1pl","pow","powf","powl","sqrt","sqrtf","sqrtl","cbrt","cbrtf","cbrtl","hypot","hypotf","hypotl","sin","sinf","sinl","cos","cosf","cosl","tan","tanf","tanl","asin","asinf","asinl","acos","acosf","acosl","atan","atanf","atanl","atan2","atan2f","atan2l","sinh,""sinhf,""sinhl,""cosh,""coshf,""coshl,""tanh,""tanhf,""tanhl,""asinh,""asinhf,""asinhl,""acosh,""acoshf,""acoshl,""atanh,""atanhf,""atanhl,""ceil","ceilf","ceill","floor","floorf","floorl","trunc","truncf","truncl","round","lround","llround","nearbyint","nearbyintf","nearbyintl","rint","rintf","rintl","lrint","lrintf","lrintl","llrint","llrintf","llrintl","frexp","frexpf","frexpl","ldexp","ldexpf","ldexpl","modf","modff","modfl","scalbn","scalbnf","scalbnl","scalbln","scalblnf","scalblnl","ilogb","ilogbf","ilogbl","logb","logbf","logbl","nextafter","nextafterf","nextafterl","nexttoward","nexttowardf","nexttowardl","copysign","copysignf","copysignl"]

# Destination files
LOG_FILE = 'axe.log'
ACCEPT_STATIC_FILE = 'accept_static.txt'
DFG_FILE_ROOT = 'dfg'

# Commands
MAKE_ORIG = 'make run_orig'

#################################################
# General OS function helpers
#################################################

def shell(command, timeout=600, cwd=None, shell=False):
    """Execute a command (via a shell or directly). Capture the stdout
    and stderr streams as a string.
    """
    subprocess.check_output(
        shlex.split(command),
        cwd=cwd,
        stderr=subprocess.STDOUT,
        shell=shell,
        timeout=timeout
    )

#################################################
# DOT file generation helper functions
#################################################

def insertDotEdge(fp, src, dst, label=''):
    fp.write('\t\"{}\"->\"{}\"\n'.format(src, dst))
    if label!='':
        fp.write('\t[ label=\"{}\" ]\n'.format(label))

def labelDotNode(fp, insn):
    # Derive the node label
    label = str(insn)
    # Derive the node color
    color = 'forestgreen' if insn.isApprox() else 'black'
    # Derive the node shape
    shape = 'box'
    if insn.op=='phi':
        shape = 'diamond'
    elif insn.op=='store':
        shape = 'triangle'
    elif insn.op=='load':
        shape = 'invtriangle'
    elif insn.op=='call':
        shape = 'doubleoctagon'
    # Add the dot edge
    fp.write('\t\"{}\" [ '.format(insn.dst))
    fp.write('label=\"{}\" '.format(label))
    fp.write('color=\"{}\" '.format(color))
    fp.write('shape=\"{}\" '.format(shape))
    fp.write('style=\"rounded\"]\n')

#################################################
# Helper for constant handling
#################################################

def isFloat(reg):
    try:
        float(reg)
        return True
    except:
        return False

def isConstant(reg):
    if reg=="null":
        return True
    elif isFloat(reg):
        return True
    else:
        return False

#################################################
# Supported functions
#################################################

funcMap = {
    'fadd': lambda x, y: x+y,
    'fsub': lambda x, y: x-y,
    'fmul': lambda x, y: x*y,
    'fdiv': lambda x, y: x/y,
    'store': lambda x: x,
    'sqrtf': lambda x: np.sqrt(x)
}

#################################################
# Class definitions
#################################################

class Cst:
    """ Constant class """
    val = None

    def __str__(self):
        return str(self.val)

    def __init__(self, cst):
        self.val = cst

    def evaluate(self, *x):
        return float(self.val)

class Insn:
    """ Instruction class """
    fn = None
    op = None
    i_id = None
    bb_id = None
    qual = None
    ty = None
    dst = None
    src = None
    dst_ty = None
    src_ty = None
    src_bb_id = None
    callee = None
    addr = None

    # Load Index - used for evaluation of load instructions
    load_idx = None

    def __str__(self):
        label='('+self.bb_id+') '
        if self.dst:
            label += self.dst + ' = '
        label += self.op + ' '
        if self.op == 'call':
            label += self.callee + '('
        if self.op == 'phi':
            src = ['[' + x[0] + ', ' + x[1] + ']' for x in zip(self.src, self.src_bb_id)]
            label += ', '.join(src)
        elif self.src:
            label += ', '.join(self.src)
        if self.op == 'call':
            label += ')'
        if self.addr:
            label += self.addr
        return label

    def __init__(self, tokens):
        # Initialize function to identity
        self.func = lambda x: x
        # Process token
        self.fn = tokens[0]
        self.op = tokens[1]
        self.i_id = tokens[2]
        self.bb_id = tokens[2].split('i')[0]
        self.qual = tokens[3]
        # If return instruction
        if self.op == 'ret':
            self.src_ty = [tokens[4]]
            self.src = [tokens[5]]
        # If call instruction
        elif self.op in callInsn:
            self.callee = tokens[4]
            self.dst_ty = tokens[5]
            self.src_ty = []
            self.src = []
            idx = 6
            if self.dst_ty!='void':
                self.dst = tokens[idx]
                idx+=1
            for i in range(int(tokens[idx])):
                token_idx = idx+1+i*2
                self.src_ty.append(tokens[token_idx+0])
                self.src.append(tokens[token_idx+1])
        # If phi node
        elif self.op in phiInsn:
            self.dst_ty = tokens[4]
            self.dst = tokens[5]
            self.src_ty = []
            self.src = []
            self.src_bb_id = []
            for i in range(int(tokens[6])):
                self.src_ty.append(tokens[4])
                self.src.append(tokens[7+2*i+0])
                self.src_bb_id.append(tokens[7+2*i+1])
        # If load instruction
        elif self.op == 'load':
            self.dst_ty = tokens[4]
            self.dst = tokens[5]
            self.addr = tokens[6]
        # If store instruction
        elif self.op == 'store':
            self.src_ty = [tokens[4]]
            self.src = [tokens[5]]
            self.dst = tokens[6] # Hack
            self.addr = tokens[6]
        # If coversion instruction
        elif self.op in conversionInsn:
            self.dst_ty = tokens[4]
            self.src_ty = [tokens[5]]
            self.dst = tokens[6]
            self.src = [tokens[7]]
        # If binary or comparison instruction
        elif self.op in binaryInsn or self.op in cmpInsn:
            self.dst_ty = tokens[4]
            self.dst = tokens[5]
            self.src_ty = [tokens[4]]
            self.src = [tokens[6], tokens[7]]
        else:
            logging.debug('Instruction unknown: {}'.format(tokens))

        # Initialize predecessor and successor
        self.predecessors = []
        self.successors = []

    def isApprox(self):
        if self.qual=="approx":
            return True
        else:
            return False

    def addPredecessor(self, p):
        if not p in self.predecessors:
            self.predecessors.append(p)

    def addSuccessor(self, p):
        if not p in self.successors:
            self.successors.append(p)

    def evaluate(self, *x):
        # Determine op function
        op = self.op
        # Pre-process arguments
        arg = [ p.evaluate(*x) for p in self.predecessors]
        # Load - retrieve input i
        if op=='load':
            return x[self.load_idx]
        elif op in binaryInsn:
            return funcMap[op](arg[0], arg[1])
        else:
            return funcMap[op](arg[0])

class DDDG:
    """ DDDG class """
    # Instruction map
    instructions = {}
    constants = {}
    # Entry/exit points of target instructions
    entries = []
    exits = []

    def __init__(self, target=None, fn=ACCEPT_STATIC_FILE):

        # Initialization (load index)
        load_idx = 0

        # If the file does not exist, generate it
        if not os.path.isfile(fn):
            try:
                shell(MAKE_ORIG, timeout=600, cwd=os.getcwd())
            except:
                logging.error('Something went wrong executing {}'.format(MAKE_ORIG))
                exit()
        assert(os.path.isfile(fn))

        # Read static dump line by line
        with open(fn) as fp:
            for line in fp:
                # Tokenize
                tokens = line.strip().split(', ')
                # Parse instruction
                instruction = Insn(tokens)
                if instruction.dst:
                    if target and instruction.fn==target or not target:
                        # Add the instruction to the instruction map
                        i_id = instruction.dst
                        self.instructions[i_id] = instruction
                        # Add load instructions to entry point
                        if instruction.op == 'load':
                            self.entries.append(i_id)
                            instruction.load_idx = load_idx
                            load_idx += 1
                        elif instruction.op == 'store':
                            self.exits.append(i_id)

        # Constant initialization
        for dst, insn in self.instructions.iteritems():
            if insn.src:
                for s in insn.src:
                    if isConstant(s) and not s in self.constants:
                        self.constants[s] = Cst(s)

        # Predecessor initialization
        for dst, insn in self.instructions.iteritems():
            if insn.src:
                for p in insn.src:
                    if p in self.instructions:
                        insn.predecessors.append(self.instructions[p])
                    elif p in self.constants:
                        insn.predecessors.append(self.constants[p])
                    else:
                        print "Something went wrong!"
                        exit()

        # Successor initialization
        successors = {}
        for dst, insn in self.instructions.iteritems():
            if insn.src and insn.dst:
                for s in insn.src:
                    if not s in successors:
                        successors[s] = []
                    successors[s].append(insn.dst)
        for dst, insn in self.instructions.iteritems():
            if dst in successors:
                for s in successors[dst]:
                    insn.successors.append(self.instructions[s])

    def generateDfg(self, fn=DFG_FILE_ROOT):
        """ Generates the DFG of the target kernel """

        # Dump the DOT description of DFG
        with open(fn+'.dot', 'w') as fp:
            fp.write('digraph graphname {\n')
            # Print all instructions
            for dst, insn in self.instructions.iteritems():
                labelDotNode(fp, insn)
                if insn.src:
                    for src in insn.src:
                        # Insert edge
                        insertDotEdge(fp, src, dst)
                # Corner case: load instruction don't have predecessors
                if insn.op=='load':
                    labelDotNode(fp, insn)
            fp.write('}')

        # Generate the png
        try:
            shell('dot {}.dot -Tpng -o{}.png'.format(fn, fn), timeout=600, cwd=os.getcwd())
        except:
            logging.error('Something went compiling {}.dot with graphviz'.format(fn))
            exit()

    def evaluate(self, x):
        """ Evaluate the current DDDG on the input vector """

        # Derive number of inputs:
        assert len(x)==len(self.entries), "Input vector length mismatch!"

        # Produce output vector
        y = []
        for exit in self.exits:
            y.append(self.instructions[exit].evaluate(*x))

        return y


#################################################
# Argument validation
#################################################

def cli():
    parser = argparse.ArgumentParser(
        description='Program Analysis & Precision Tuning'
    )
    parser.add_argument(
        '-conf', dest='accept_config_fn', action='store', type=str, required=False,
        default=None, help='accept_config_file'
    )
    parser.add_argument(
        '-d', dest='debug', action='store_true', required=False,
        default=False, help='print out debug messages'
    )
    parser.add_argument(
        '-target', dest='target', action='store', type=str, required=True,
        default=None, help='target function to approximate'
    )
    parser.add_argument(
        '-log', dest='logpath', action='store', type=str, required=False,
        default=LOG_FILE, help='path to log file'
    )
    args = parser.parse_args()


    # Initialize logging
    logFormatter = logging.Formatter('%(asctime)s [%(threadName)-12.12s] [%(levelname)-5.5s]  %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
    rootLogger = logging.getLogger()
    consoleHandler = logging.StreamHandler()
    consoleHandler.setFormatter(logFormatter)
    rootLogger.addHandler(consoleHandler)
    if(args.debug):
        rootLogger.setLevel(logging.DEBUG)
    else:
        rootLogger.setLevel(logging.INFO)

    # # Process DDDG
    dddg = DDDG(args.target)
    dddg.generateDfg()
    print dddg.evaluate([1, 4])

if __name__ == '__main__':
    cli()