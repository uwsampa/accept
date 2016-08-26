#!/usr/bin/env python
import argparse
import inspect
import logging
import numpy as np
import os
import shlex
import struct
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
ACCEPT_LD_FILE = 'accept_memtrace.txt'
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
# Helpers
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

def getValFromBits(raw, ty):
    if ty=="Float":
        raw = raw[8:16]
        val = struct.unpack('!f', raw.decode('hex'))[0]
        return np.float32(val)
    else:
        assert False, "Unsupported type {}".format(ty)

#################################################
# Supported functions
#################################################

funcMap = {
    'fadd': lambda x, y: np.add(x,y),
    'fsub': lambda x, y: np.subtract(x,y),
    'fmul': lambda x, y: np.multiply(x,y),
    'fdiv': lambda x, y: np.divide(x,y),
    'store': lambda x: x,
    'fpext': lambda x: np.float64(x),
    'fptrunc': lambda x: np.float32(x),
    # Math functions
    'acos': lambda x: np.arccos(x),
    'asin': lambda x: np.arcsin(x),
    'cos': lambda x: np.cos(x),
    'sin': lambda x: np.sin(x),
    'sqrtf': lambda x: np.sqrt(x),
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
    dst = None
    src = None
    dst_ty = None
    src_ty = None
    src_bb_id = None
    callee = None
    addr = None

    # Load/store Index
    load_idx = None
    store_idx = None

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
            # self.dst = tokens[6] # Hack
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
        op = self.callee if self.op=='call' else self.op
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
    # Instruction map: insn = instructions[iid]
    instructions = {}
    constants = {}
    # Entry/exit points of target instructions
    entries = []
    exits = []
    # Load and store value stream
    memTrace = {'load':[], 'store':[]}

    def __init__(self, target=None, fn=ACCEPT_STATIC_FILE):

        # Initialization (load, store index)
        load_idx = 0
        store_idx = 0

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
                # Select only instructions in target function
                if target and instruction.fn==target or not target:
                    # Add the instruction to the instruction map
                    i_id = instruction.i_id
                    self.instructions[i_id] = instruction
                    # Add load instructions to entry point
                    if instruction.op == 'load':
                        self.entries.append(i_id)
                        instruction.load_idx = load_idx
                        load_idx += 1
                    elif instruction.op == 'store':
                        self.exits.append(i_id)
                        instruction.store_idx = store_idx
                        store_idx += 1


        # Constant initialization
        for i_id, insn in self.instructions.iteritems():
            if insn.src:
                for s in insn.src:
                    if isConstant(s) and not s in self.constants:
                        self.constants[s] = Cst(s)

        # Predecessor initialization
        for i_id, insn in self.instructions.iteritems():
            if insn.src:
                for p in insn.src:
                    p_id = self.getIidFromReg(p)
                    if p_id in self.instructions:
                        insn.predecessors.append(self.instructions[p_id])
                    elif p in self.constants:
                        insn.predecessors.append(self.constants[p])
                    else:
                        print "Something went wrong!"
                        exit()

        # Successor initialization
        successors = {}
        for i_id, insn in self.instructions.iteritems():
            if insn.src and insn.dst:
                for s in insn.src:
                    s_id = self.getIidFromReg(s)
                    if not s_id in successors:
                        successors[s_id] = []
                    successors[s_id].append(i_id)
        for i_id, insn in self.instructions.iteritems():
            if i_id in successors:
                for s in successors[i_id]:
                    insn.successors.append(self.instructions[s])

    def getIidFromReg(self, reg):
        """ Returns instruction ID based on register ID """

        for i_id, insn in self.instructions.iteritems():
            if insn.dst == reg:
                return i_id

    def generateDfg(self, fn=DFG_FILE_ROOT):
        """ Generates the DFG of the target kernel """

        # Dump the DOT description of DFG
        with open(fn+'.dot', 'w') as fp:
            fp.write('digraph graphname {\n')
            # Print all instructions
            for i_id, insn in self.instructions.iteritems():
                labelDotNode(fp, insn)
                if insn.src:
                    for src in insn.src:
                        # Insert edge
                        insertDotEdge(fp, src, insn.dst)
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

    def loadMemTrace(self, fn=ACCEPT_LD_FILE, lim=100):
        """ Reads in memory trace file """

        # If the file does not exist, generate it
        if not os.path.isfile(fn):
            try:
                shell(MAKE_ORIG, timeout=600, cwd=os.getcwd())
            except:
                logging.error('Something went wrong executing {}'.format(MAKE_ORIG))
                exit()
        assert(os.path.isfile(fn))

        # Load and store structures
        numEntries = len(self.entries)
        numExits = len(self.entries)

        # Process line by line
        with open(fn) as fp:
            for idx, line in enumerate(fp):
                # Determine the load/store pair index
                pairIdx = idx / (numEntries + numExits)
                # Break early if we reached the quota
                if pairIdx >= lim:
                    break
                # Initialize new load/store value pairs
                if len(self.memTrace['load'])==pairIdx:
                    self.memTrace['load'].append([0]*numEntries)
                if len(self.memTrace['store'])==pairIdx:
                    self.memTrace['store'].append([0]*numExits)
                # Parse the line
                tokens = line.strip().split(',' )
                iid = tokens[0]
                addr = tokens[1]
                val = tokens[2][3:] # Remove '0x'
                # Get the memory instruction
                memInsn = self.instructions[iid]
                # Handle value according to load/store op
                if memInsn.op=="load":
                    # Get the data type
                    ty = memInsn.dst_ty
                    # Determine load index
                    load_idx = memInsn.load_idx
                    # Push load value in queue
                    self.memTrace['load'][pairIdx][load_idx] = getValFromBits(val, ty)
                    logging.debug("#{} ty={}, loadIdx={}, iid={}, addr={}, val={}".format(pairIdx, ty, load_idx, iid, addr, val))
                else:
                    # Get the data type
                    ty = memInsn.src_ty[0]
                    # Determine store index
                    store_idx = memInsn.store_idx
                    # Push store value in queue
                    self.memTrace['store'][pairIdx][store_idx] = getValFromBits(val, ty)
                    logging.debug("#{} ty={}, storeIdx={}, iid={}, addr={}, val={}".format(pairIdx, ty, store_idx, iid, addr, val))

    def test(self):
        """ Evaluates kernel error on memory trace """

        # Kernel outputs
        outputs = [self.evaluate(x) for x in self.memTrace['load']]

        # Evaluate SNR
        goldenData = np.array(self.memTrace['store'])
        relaxedData = np.array(outputs)
        num = ((goldenData) ** 2).sum(axis=None)
        den = ((goldenData - relaxedData) ** 2).sum(axis=None)
        snr = 10 * np.log10( num/den )
        return snr

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

    # Process DDDG
    dddg = DDDG(args.target)
    dddg.generateDfg()

    # Load memory trace
    dddg.loadMemTrace()

    # Produce SNR
    snr = dddg.test()
    logging.info("SNR = {}".format(snr))


if __name__ == '__main__':
    cli()