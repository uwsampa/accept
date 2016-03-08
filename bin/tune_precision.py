#!/usr/bin/env python
from __future__ import division
import subprocess32 as subprocess
import tempfile
import os
import shutil
import shlex
import math
import copy
import argparse
import logging
import cw.client
import threading
import collections
import csv
import re
import time
import numpy as np
import matplotlib.pyplot as plt
import eval
from eval import EXT

# FILE NAMES
ACCEPT_CONFIG = 'accept_config.txt'
LOG_FILE = 'tune_precision.log'
ERROR_LOG_FILE = 'error.log'
BB_DYNSTATS_FILE = 'accept_bbstats.txt'
FP_DYNSTATS_FILE = 'accept_fpstats.txt'
CDF_FILE = 'cdf_stats'

# DIR NAMES
OUTPUT_DIR = 'outputs'

# OUTPUT FILES
PRECISE_OUTPUT = 'orig'+EXT
APPROX_OUTPUT = 'out'+EXT

# PARAMETERS
RESET_CYCLE = 1

# Error codes
CRASH = -1
TIMEOUT = -2
NOOUTPUT = -3

# Globals
step_count = 0

# LLVM instruction categories
# We ignore phi nodes since those are required
# because of the SSA style of the LLVM IR
controlInsn = ['br','switch','indirectbr','ret']
terminatorInsn = ['invoke','resume','catchswitch','catchret','cleanupret','unreachable']
fpInsn = ['fadd','fsub','fmul','fdiv','frem']
binaryInsn = ['add','sub','mul','udiv','sdiv','urem','srem']
bitbinInsn = ['shl','lshr','ashr','and','or','xor']
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

# C standard library function calls (non-exhaustive)
cMathFunc = ["abs","labs","llabs","div","ldiv","lldiv","imaxabs","imaxdiv","fabs","fabsf","fabsl","fmod","fmodf","fmodl","remainder","remainderf","remainderl","remquo","remquof","remquol","fma","fmaf","fmal","fmax","fmaxf","fmaxl","fmin","fminf","fminl","fdim","fdimf","fdiml","exp","expf","expl","exp2","exp2f","exp2l","expm1","expm1f","expm1l","log","logf","logl","log10","log10f","log10l","log2","log2f","log2l","log1p","log1pf","log1pl","pow","powf","powl","sqrt","sqrtf","sqrtl","cbrt","cbrtf","cbrtl","hypot","hypotf","hypotl","sin","sinf","sinl","cos","cosf","cosl","tan","tanf","tanl","asin","asinf","asinl","acos","acosf","acosl","atan","atanf","atanl","atan2","atan2f","atan2l","sinh,""sinhf,""sinhl,""cosh,""coshf,""coshl,""tanh,""tanhf,""tanhl,""asinh,""asinhf,""asinhl,""acosh,""acoshf,""acoshl,""atanh,""atanhf,""atanhl,""ceil","ceilf","ceill","floor","floorf","floorl","trunc","truncf","truncl","round","lround","llround","nearbyint","nearbyintf","nearbyintl","rint","rintf","rintl","lrint","lrintf","lrintl","llrint","llrintf","llrintl","frexp","frexpf","frexpl","ldexp","ldexpf","ldexpl","modf","modff","modfl","scalbn","scalbnf","scalbnl","scalbln","scalblnf","scalblnl","ilogb","ilogbf","ilogbl","logb","logbf","logbl","nextafter","nextafterf","nextafterl","nexttoward","nexttowardf","nexttowardl","copysign","copysignf","copysignl"]
cStdFunc = ['__memcpy_chk','__memset_chk','calloc','free','printf']


# LLVM instruction category to be ignored
ignoreList = [
    "phi",              # phi instructions are used by LLVM for dependence analysis
    "getelementptr",    # this is implicitly performed with load instructions
    "vector",           # same here
    "compare",          # this is implicitly performed with conditional jumps
    "other",            # don't care about these (usually very low)
    "terminator",       # rarely ecountered
    "aggregate",        # rarely ecountered
    "conversion"        # although it is work, it's considered an artifact
]

# List of LLVM instruction lists
llvmInsnList = [
    { "cat": "control", "iList": controlInsn},
    { "cat": "terminator", "iList": terminatorInsn},
    { "cat": "fp_arith", "iList": fpInsn},
    { "cat": "int_arith", "iList": binaryInsn+bitbinInsn},
    { "cat": "vector", "iList": vectorInsn},
    { "cat": "aggregate", "iList": aggregateInsn},
    { "cat": "loadstore", "iList": loadstoreInsn},
    { "cat": "getelementptr", "iList": getelementptrInsn},
    { "cat": "memory", "iList": memoryInsn},
    { "cat": "conversion", "iList": conversionInsn},
    { "cat": "compare", "iList": cmpInsn},
    { "cat": "call", "iList": callInsn},
    { "cat": "phi", "iList": phiInsn},
    { "cat": "other", "iList": otherInsn}
]

# List of standard C functions
stdCList = [
    { "cat": "cmath", "iList": cMathFunc},
    { "cat": "cstd", "iList": cStdFunc}
]

# Dictionary for convenient list access
llvmInsnListDict = {
    "control" : controlInsn,
    "terminator" : terminatorInsn,
    "fp_arith" : fpInsn,
    "int_arith" : binaryInsn+bitbinInsn,
    "vector" : vectorInsn,
    "aggregate" : aggregateInsn,
    "loadstore" : loadstoreInsn,
    "getelementptr" : getelementptrInsn,
    "memory" : memoryInsn,
    "conversion" : conversionInsn,
    "compare" : cmpInsn,
    "call" : callInsn,
    "phi" : phiInsn,
    "other" : otherInsn,
    "cmath": cMathFunc,
    "cstd": cStdFunc
}

# BB IR Category Dictionary
bbIrCatDict = {
    "total": 0,
    "control": 0,
    "terminator": 0,
    "fp_arith": 0,
    "int_arith": 0,
    "vector": 0,
    "aggregate": 0,
    "loadstore": 0,
    "getelementptr": 0,
    "memory": 0,
    "conversion": 0,
    "compare": 0,
    "call": 0,
    "phi": 0,
    "other": 0,
    "cmath": 0,
    "cstd": 0
}

#################################################
# Global handling
#################################################
def init_step_count():
    global step_count
    step_count = 0

#################################################
# General OS function helpers
#################################################

def shell(command, timeout=600, cwd=None, shell=False):
    """Execute a command (via a shell or directly). Capture the stdout
    and stderr streams as a string.
    """
    return subprocess.check_output(
        command,
        cwd=cwd,
        stderr=subprocess.STDOUT,
        shell=shell,
        timeout=timeout
    )

# Taken from Python Central
def copy_directory(src, dest):
    """Copies a directory recursively, and does
    error handling.
    """
    try:
        shutil.copytree(src, dest)
    # Directories are the same
    except shutil.Error as e:
        print('Directory not copied. Error: %s' % e)
    # Any error saying that the directory doesn't exist
    except OSError as e:
        print('Directory not copied. Error: %s' % e)

def create_overwrite_directory(dirpath):
    """If the path exists, deletes the dir.
    Creates a new directory.
    """
    if os.path.exists(dirpath):
        shutil.rmtree(dirpath)
    os.makedirs(dirpath)

def safe_copy(src, dst, lim=1000):
    sleepCounter = 0
    time.sleep(5)
    while not os.path.isfile(src) and sleepCounter<lim:
        time.sleep(1)
        sleepCounter += 1
    shutil.copyfile(src, dst)

#################################################
# LLVM Type Analysis Helper Functions
#################################################

def get_bitwidth_from_type(typeStr):
    """Returns the bit-width of a given LLVM type
    """
    if (typeStr=="Half"):
        return 10
    elif(typeStr=="Float"):
        return 23
    elif(typeStr=="Double"):
        return 52
    elif (typeStr=="Int1"):
        return 1
    elif (typeStr=="Int8"):
        return 8
    elif (typeStr=="Int16"):
        return 16
    elif (typeStr=="Int32"):
        return 32
    elif (typeStr=="Int64"):
        return 64
    else:
        logging.error('Unrecognized type: {}'.format(typeStr))
        exit()

def is_float_type(typeStr):
    """Returns the true if type is a Float
    """
    if (typeStr=="Half"):
        return True
    elif(typeStr=="Float"):
        return True
    elif(typeStr=="Double"):
        return True
    else:
        return False



#################################################
# LLVM LL IR parsing
#################################################

def checkIRCat(l, opList, strict):
    """ Performs simple regular expression matching
    of an LLVM IR line against a list of instructions
    """
    for s in opList:
        if strict:
            if re.search(r"\A" + re.escape(s) + r"\b", l):
                return 1
        else:
            if '@'+s in l:
                return 1
    return 0

def determineIRCat(l, dictionary, strict=True):
    """ Determines the instruction category of an LLVM
    IR line and returns a category vector
    """
    wordList = l.split()
    matchVector = [0]*len(dictionary)
    for w in wordList:
        for idx, iList in enumerate(dictionary):
            found = checkIRCat(w, iList["iList"], strict)
            matchVector[idx] += found
            if found:
                return matchVector
    # Extra checks
    if (sum(matchVector)==0 and strict):
        print "Error - no match!"
        print l
        print matchVector
        exit()
    if (sum(matchVector)>1):
        print "Error - multiple matches!"
        print l
        print matchVector
        exit()
    return matchVector

def read_static_stats(llFile):
    """ Processes an LLVM IR file (.ll) and produces
    a static profile of the instructions
    """

    bbInfo = {}
    locMap = {}

    with open(llFile) as f:
        content = f.readlines()
        prevInsnIdx = 0

        for idx,l in enumerate(content):
            matchObj = re.match(r'(.*)(!bb\d+i\d+)(.*)', l, re.M|re.I)
            if matchObj:
                instrId = matchObj.group(2)
                (bbId, inId) = [int(x) for x in instrId[3:].split('i')]

                # Add the BB category info dictionary
                if bbId not in bbInfo:
                    bbInfo[bbId] = dict(bbIrCatDict)

                # Switch statements are over multiple lines
                if l.strip()[0]==']':
                    for i in range(idx, prevInsnIdx, -1):
                        if content[i].strip()[-1]=='[':
                            l = ''.join(content[i:idx+1])
                            break

                # Apply instruction filters
                skip = False
                if "@llvm.dbg" in l:
                    skip = True

                if not skip:

                    insnCatVector = determineIRCat(l, llvmInsnList)
                    insnCat = llvmInsnList[insnCatVector.index(1)]["cat"]
                    if insnCat in bbInfo[bbId]:
                        if insnCat=="call":
                            # Special case, check for cMath functions we care about
                            stdcCatVector = determineIRCat(l, stdCList, strict=False)
                            if 1 not in stdcCatVector:
                                bbInfo[bbId][insnCat] += 1
                            else:
                                stdcCat = stdCList[stdcCatVector.index(1)]["cat"]
                                bbInfo[bbId][stdcCat] += 1
                        else:
                            bbInfo[bbId][insnCat] += 1
                        bbInfo[bbId]["total"] += 1

                prevInsnIdx = idx

            # Check for metadata lines
            # Very primitive parsing...
            line = l.strip()
            if line.startswith("!"):
                tuples = line.split("!{")[1][:-1].split(", ")
                allMetadataStr = True
                for t in tuples:
                    if not t.startswith("metadata !"):
                        allMetadataStr = False
                if allMetadataStr and len(tuples)==2:
                    key = tuples[0][len("metadata !")+1:-1]
                    val = tuples[1][len("metadata !")+1:-1]
                    if key.startswith("bb"):
                        locMap[key] = val

    return bbInfo, locMap


#################################################
# Bit mask setting to conf file param conversion
#################################################

def get_param_from_masks(himask, lomask):
    """Returns parameter from width settings
    """
    return (9<<16) + (himask<<8) + lomask

def get_masks_from_param(param):
    """Returns mask width settings from parameter
    """
    if param==0:
        return 0,0
    param -= (9<<16)
    himask = (param >> 8) & 0xFF;
    lomask = (param & 0xFF);
    return himask, lomask


#################################################
# Configuration file reading/processing
#################################################

def analyzeInstructionMix(config, extraChecks=True):
    """ Analyzes the instruction mix of an approximate program
    """

    # Generate the LLVM IR file of the orig program
    curdir = os.getcwd()
    llFn = curdir.split('/')[-1] + ".orig.ll"
    logging.debug('Generating IR file (.ll): {}'.format(llFn))
    try:
        shell(shlex.split('make '+llFn), cwd=curdir)
    except:
        logging.error('Something went wrong generating the orig ll.')
        exit()

    # Obtain static instruction mix from LLVM IR
    staticStats, locMap = read_static_stats(llFn)

    # Obtain BB dynamic information
    if not os.path.isfile(BB_DYNSTATS_FILE):
        try:
            shell(shlex.split('make run_orig'), cwd=curdir)
            shell(shlex.split('make run_opt'), cwd=curdir)
        except:
            logging.error('Something went wrong generating default config.')
            exit()
    assert (os.path.isfile(BB_DYNSTATS_FILE)),"BB_DYNSTATS_FILE not found!"
    dynamicBbStats = read_dyn_bb_stats(BB_DYNSTATS_FILE)

    # Combine static and dynamic profiles to produce instruction breakdown
    totalStats = dict(bbIrCatDict)
    for bbIdx in dynamicBbStats:
        execCount = dynamicBbStats[bbIdx]
        if execCount>1: # Only account for non-main BBs
            for cat in staticStats[bbIdx]:
                totalStats[cat] += staticStats[bbIdx][cat]*execCount
                # some debug code commented out for convenience
                # if cat=="call" and staticStats[bbIdx][cat]>0:
                #     print "BB {} executes {} times".format(bbIdx, execCount)
                #     print staticStats[bbIdx]

    # Get the approximate instruction breakdown from the config file
    approxStats = dict(bbIrCatDict)
    for approxInsn in config:
        for l in llvmInsnList+stdCList:
            if approxInsn['opcode'] in l['iList']:
                approxStats[l['cat']] += dynamicBbStats[approxInsn['bb']]
        # Print the approximation setting
        iid = "bb" + str(approxInsn["bb"]) + "i" + str(approxInsn["line"])
        logging.debug("{}:{}:{}:{}:wi={}:lo={}".format(
            locMap[iid],approxInsn["opcode"], approxInsn["type"], iid,
            get_bitwidth_from_type(approxInsn["type"])-approxInsn["lomask"]-approxInsn["himask"],
            approxInsn["lomask"]
        ))

    # Print out dynamic instruction mix statistics
    csvHeader = []
    csvRow = []
    for cat in sorted(totalStats):
        if cat!="total" and cat not in ignoreList:
            ignorePerc = sum([totalStats[i] for i in ignoreList])
            perc = totalStats[cat]/(totalStats["total"]-ignorePerc)
            csvHeader.append(cat)
            csvRow.append(str(perc))
            # Set the threshold at 0.1%
            if perc > 0.001:
                logging.info('Dynamic instruction mix breakdown: {} = {:.1%}'.format(cat, perc))
    logging.debug('CSV Header: {}'.format(('\t').join(csvHeader)))
    logging.debug('CSV Row: {}'.format(('\t').join(csvRow)))

    # Print out the approximate dynaic instruction mix statistics
    csvHeader = []
    csvRow = []
    for cat in sorted(approxStats):
        perc = 0.0
        if approxStats[cat] > 0:
            perc = approxStats[cat]/totalStats[cat]
            logging.info('Approx to precise instruction percentage: {} = {:.1%}'.format(cat, perc))
        if cat!="total" and cat not in ignoreList:
            csvHeader.append(cat)
            csvRow.append(str(perc))
    logging.debug('CSV Header: {}'.format(('\t').join(csvHeader)))
    logging.debug('CSV Row: {}'.format(('\t').join(csvRow)))

    # Finally report the number of knobs
    logging.info('There are {} static safe to approximate instructions'.format(len(config)))

    # Check for non-approximable instructions that should be approximate!
    if extraChecks:
        for bbIdx in staticStats:
            for cat in staticStats[bbIdx]:
                if cat=="loadstore" or cat=="fp_arith" or cat=="cmath":
                    staticPreciseCount = staticStats[bbIdx][cat]
                    staticApproxCount = 0
                    for approxInsn in config:
                        if bbIdx == approxInsn['bb'] and approxInsn['opcode'] in llvmInsnListDict[cat]:
                            staticApproxCount+=1
                    if staticStats[bbIdx][cat] > 0 and dynamicBbStats[bbIdx] > 1:
                        if staticApproxCount < staticPreciseCount:
                            logging.debug("bb {} executes {} times".format(bbIdx, dynamicBbStats[bbIdx]))
                            logging.debug("\t{} has {} approx insn vs. {} precise insn".format(cat, staticApproxCount, staticPreciseCount))

    # Obtain FP dynamic information
    if os.path.isfile(FP_DYNSTATS_FILE):
        dynamicFpStats = read_dyn_fp_stats(FP_DYNSTATS_FILE)
        csvHeader = ["bbId", "expRange", "mantissa", "execs"]
        csvRow = []
        for approxInsn in config:
            fpId = 'bb'+str(approxInsn['bb'])+'i'+str(approxInsn['line'])
            if fpId in dynamicFpStats:
                expRange = dynamicFpStats[fpId][1]-dynamicFpStats[fpId][0]
                mantissa = get_bitwidth_from_type(approxInsn['type']) - approxInsn['lomask']
                execCount = dynamicBbStats[approxInsn['bb']]
                logging.info("{}: [expRange, mantissa, execs] = [{}, {}, {}]".format(fpId, expRange, mantissa, execCount))
                csvRow.append([fpId, str(expRange), str(mantissa), str(execCount)])
        print('{}'.format(('\t').join(csvHeader)))
        for row in csvRow:
            print('{}'.format(('\t').join(row)))

        expRange = [int(x[1]) for x in csvRow]
        mantissa = [int(x[2]) for x in csvRow]
        execCount = [int(x[3])*1000.0/10223616 for x in csvRow]

        plt.scatter(expRange, mantissa, s=execCount, alpha=0.5)
        plt.xlabel('Exponent Range')
        plt.ylabel('Mantissa Width')
        plt.savefig("fp.pdf", bbox_inches='tight')

        # Analyze Math Functions
        for approxInsn in config:
            if approxInsn['opcode'] in llvmInsnListDict["cmath"]:
                fpId = 'bb'+str(approxInsn['bb'])+'i'+str(approxInsn['line'])
                if fpId in dynamicFpStats:
                    expRange = dynamicFpStats[fpId][1]-dynamicFpStats[fpId][0]
                else:
                    expRange = -1
                mantissa = get_bitwidth_from_type(approxInsn['type']) - approxInsn['lomask']
                execCount = dynamicBbStats[approxInsn['bb']]
                logging.info("Math function {}: [expRange, mantissa, execs] = [{}, {}, {}]".format(approxInsn['opcode'], expRange, mantissa, execCount))


#################################################
# Configuration file reading/processing
#################################################

def parse_relax_config(f):
    """Parse a relaxation configuration from a file-like object.
    Generates (ident, param) tuples.
    """
    for line in f:
        line = line.strip()
        if line:
            param, ident = line.split(None, 1)
            yield ident, int(param)

def read_config(fname, adaptiverate, stats_fn=None):
    """Reads in a fine error injection descriptor.
    Returns a config object.
    """
    # Filter out the instructions that don't execute dynamically
    if stats_fn:
        # Read dynamic statistics
        bb_info = read_dyn_bb_stats(stats_fn)

    # Configuration object
    config = []
    with open(fname) as f:
        for ident, param in parse_relax_config(f):
            # If this is in a function indicated in the parameters file,
            # adjust the parameter accordingly.
            if ident.startswith('instruction'):
                _, i_ident = ident.split()
                func, bb, line, opcode, typ = i_ident.split(':')
                himask, lomask = get_masks_from_param(param)

                # Derive the masking rate
                maskingRate = get_bitwidth_from_type(typ)/8 if adaptiverate else 1

                # Filter the instruction if it doesn't execute more than once
                # (This excludes code that is in the main file)
                if not stats_fn or bb_info[int(bb)]>1:
                    # Add the config entry for the instruction
                    config.append({
                        'insn': ident,
                        'relax': param,
                        'himask': himask,
                        'lomask': lomask,
                        'bb': int(bb),
                        'line': int(line),
                        'opcode': opcode,
                        'type': typ,
                        'rate': int(maskingRate)
                        })

    return config

def gen_default_config(instlimit, adaptiverate, timeout):
    """Reads in the coarse error injection descriptor,
    generates the default config by running make run_orig.
    Returns a config object.
    """
    curdir = os.getcwd()

    # Generate the default configuration file, generate dynamic stats file
    logging.info('Generating the fine config file: {}'.format(ACCEPT_CONFIG))
    try:
        shell(shlex.split('make run_orig'), cwd=curdir, timeout=timeout)
        shell(shlex.split('make run_opt'), cwd=curdir, timeout=timeout)
    except:
        logging.error('Something went wrong generating default config.')
        exit()

    # Load ACCEPT config file
    config = read_config(ACCEPT_CONFIG, adaptiverate, BB_DYNSTATS_FILE)

    # Notify the user that the instruction limit is lower than the configuration length
    if len(config) > instlimit:
        logging.info('Instruction length exceeds the limit set by the user of {}'.format(instlimit))

    return config

def dump_relax_config(config, fname):
    """Write a relaxation configuration to a file-like object. The
    configuration should be a sequence of tuples.
    """
    logging.debug("-----------FILE DUMP BEGIN-----------")
    with open(fname, 'w') as f:
        for conf in config:
            mode = get_param_from_masks(conf['himask'], conf['lomask'])
            f.write(str(mode)+ ' ' + conf['insn'] + '\n')
            logging.debug(str(mode)+ ' ' + conf['insn'])
    logging.debug("----------- FILE DUMP END -----------")

def print_config(config):
    """Prints out the configuration.
    """
    logging.debug("-----------CONFIG DUMP BEGIN-----------")
    for conf in config:
        logging.debug(conf)
    logging.debug("----------- CONFIG DUMP END -----------")

def eval_compression_factor(config):
    """Evaluate number of bits saved from compression.
    """
    bits, total = 0, 0
    for conf in config:
        bits += conf['himask']+conf['lomask']
        total += get_bitwidth_from_type(conf['type'])
    return float(bits)/total


#################################################
# Configuration function testing
#################################################

def test_config(config, timeout, statspath=None, dstpath=None):
    """Creates a temporary directory to run ACCEPT with
    the passed in config object for precision relaxation.
    """
    # Get the current working directory
    curdir = os.getcwd()
    # Get the last level directory name (the one we're in)
    dirname = os.path.basename(os.path.normpath(curdir))
    # Create a temporary directory
    tmpdir = tempfile.mkdtemp()+'/'+dirname
    logging.debug('New directory created: {}'.format(tmpdir))
    # Transfer files over
    copy_directory(curdir, tmpdir)
    # Cleanup
    try:
        shell(shlex.split('make clean'), cwd=tmpdir, timeout=timeout)
    except:
        logging.error('Something went wrong generating during cleanup.')
    # Dump config
    dump_relax_config(config, tmpdir+'/'+ACCEPT_CONFIG)
    # Full compile and program run
    logging.debug('Lanching compile and run...')
    try:
        shell(shlex.split('make run_opt'), cwd=tmpdir, timeout=timeout)
    except subprocess.TimeoutExpired:
        logging.warning('Timed out!')
        print_config(config)
        return float(TIMEOUT)
    except:
        logging.warning('Make error!')
        print_config(config)
        return float(CRASH)

    # Now that we're done with the compilation, evaluate results
    output_fp = os.path.join(tmpdir,APPROX_OUTPUT)
    trace_fp = os.path.join(tmpdir,BB_DYNSTATS_FILE)
    if os.path.isfile(output_fp):
        error = eval.score(PRECISE_OUTPUT,os.path.join(tmpdir,APPROX_OUTPUT))
        logging.debug('Reported application error: {}'.format(error))
        if(dstpath):
            shutil.copyfile(output_fp, dstpath)
        if(statspath):
            shutil.copyfile(trace_fp, statspath)
    else:
        # Something went wrong - no output!
        logging.warning('Missing output!')
        print_config(config)
        return float(NOOUTPUT)

    # Remove the temporary directory
    shutil.rmtree(tmpdir)
    # Return the error
    return float(error)


#################################################
# Dynamic statistics file reading/processing
#################################################

def read_dyn_bb_stats(stats_fn):
    """Reads in the dynamic BB statistics file that ACCEPT spits out
    """
    bb_info = {}
    with open(stats_fn) as f:
        for l in f:
            line = l.strip()
            bb_idx = line.split('\t')[0]
            bb_num = line.split('\t')[1]
            if (bb_idx.isdigit() and bb_num.isdigit()):
                bb_info[int(bb_idx)] = int(bb_num)
    return bb_info

def read_dyn_fp_stats(stats_fn):
    """Reads in the dynamic FP statistics file that ACCEPT spits out
    """
    fp_info = {}
    with open(stats_fn) as f:
        for l in f:
            line = l.strip()
            elems = line.split('\t')
            fp_id = elems[0]
            if fp_id != 'id':
                fp_min = int(elems[1])
                fp_max = int(elems[2])
                fp_info[fp_id] = [fp_min, fp_max]
    return fp_info

def process_dyn_bb_stats(config, stats_fn, cdf_fn=None, BITWIDHTMAX=64):
    """Processes the dynamic BB count file to produce stats
    """
    bb_info = read_dyn_bb_stats(stats_fn)

    # Program versions
    categories = ['baseline', 'precise', 'approx']
    # Op categories
    op_categories = ['mem', 'exe', 'math', 'all']
    # Op types
    op_types = ['int', 'fp', 'all']

    # Initialize statistics
    histograms = {}
    for cat in categories:
        cat_hist = {}
        for opcat in op_categories:
            op_hist = {}
            for typ in op_types:
                hist = [0]*(BITWIDHTMAX+1)
                op_hist[typ] = hist
            cat_hist[opcat] = op_hist
        histograms[cat] = cat_hist

    # Populate histogram
    for conf in config:
        # Compute precision requirements
        widths = {}
        widths['baseline'] = get_bitwidth_from_type(conf['type'])
        widths['precise'] = widths['baseline']-conf['himask']
        widths['approx'] = widths['precise']-conf['lomask']
        # Derive dynamic execution count
        execs = bb_info[conf['bb']]
        # Derive op category
        if conf['opcode']=='store' or conf['opcode']=='load':
            opcat = 'mem'
        elif (conf['opcode'] in cMathFunc):
            opcat = 'math'
        else:
            opcat = 'exe'
        # Derive op type
        typ = 'fp' if is_float_type(conf['type']) else 'int'
        # Update stats
        for cat in categories:
            for oc in [opcat, 'all']:
                for t in [typ, 'all']:
                    histograms[cat][oc][t][widths[cat]]+=execs

    # Compute dynamic savings from baseline to approx
    savings = {}
    for opcat in op_categories:
        op_sav = {}
        for typ in op_types:
            baseline_count = 0
            approx_count = 0
            for bitw in range(0, BITWIDHTMAX+1):
                baseline_count += bitw*histograms['baseline'][opcat][typ][bitw]
                approx_count += bitw*histograms['approx'][opcat][typ][bitw]
            sav = float(baseline_count - approx_count)/baseline_count if baseline_count>0 else 0.0
            op_sav[typ] = sav
        savings[opcat] = op_sav

    if (cdf_fn):

        # Now produce the CDF
        cdfs = {}
        for cat in categories:
            cat_cdf = {}
            for opcat in op_categories:
                op_cdf = {}
                for typ in op_types:
                    cdf = [0]*(BITWIDHTMAX+1)
                    op_cdf[typ] = cdf
                cat_cdf[opcat] = op_cdf
            cdfs[cat] = cat_cdf

        # CSV file that gets outputted
        cdf_stats = {}
        for opcat in op_types:
            cdf_stats[opcat] = [["bitw", "baseline_mem", "precise_mem", "approx_mem", "baseline_exe", "precise_exe", "approx_exe"]]
        for i in range(0, (BITWIDHTMAX+1)):
            for cat in categories:
                for opcat in op_categories:
                    for typ in op_types:
                        total = sum(histograms[cat][opcat][typ])
                        prev = 0 if i==0 else cdfs[cat][opcat][typ][i-1]
                        if total>0:
                            cdfs[cat][opcat][typ][i] = prev+float(histograms[cat][opcat][typ][i])/total
            for typ in op_types:
                cdf_stats[typ].append([i,
                    cdfs['baseline']['mem'][typ][i], cdfs['precise']['mem'][typ][i], cdfs['approx']['mem'][typ][i],
                    cdfs['baseline']['exe'][typ][i], cdfs['precise']['exe'][typ][i], cdfs['approx']['exe'][typ][i]])

        for typ in op_types:
            dest = cdf_fn+'_'+typ+'.csv'
            with open(dest, 'w') as fp:
                csv.writer(fp, delimiter='\t').writerows(cdf_stats[typ])

        # Formatting
        palette = [ '#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf']
        cat_format = {
            "mem": "mem",
            "exe": "alu",
            "all": "mem+alu"
        }
        typ_format = {
            "int": "(int only)",
            "fp": "(fp only)",
            "all": ""
        }

        # Now plot the data!
        f, axarr = plt.subplots(len(op_categories), len(op_types), sharex='col', sharey='row')
        for i,opcat in enumerate(op_categories):
            for j,typ in enumerate(op_types):
                plots=[None] * len(categories)
                legend=[None] * len(categories)
                for k,cat in enumerate(categories):
                    x = np.array(range(0, (BITWIDHTMAX+1)))
                    y = np.array(cdfs[cat][opcat][typ])
                    axarr[i, j].set_title(cat_format[opcat]+' '+typ_format[typ])
                    x1,x2,y1,y2 = plt.axis()
                    axarr[i][j].axis((x1,BITWIDHTMAX,y1,y2))

                    plots[k], = axarr[i, j].plot(x, y, c=palette[k%len(palette)])
                    legend[k] = cat

                axarr[2][j].set_xlabel("Bit-width")
            axarr[i][0].set_ylabel("Percentage")

        # Plot legend
        f.legend(plots,
               legend,
               loc='upper center',
               ncol=3,
               fontsize=8)

        # Plot
        plt.savefig(cdf_fn+'.pdf', bbox_inches='tight')

    return savings

def report_error_and_savings(base_config, timeout, error=0, stats_fn=None, cdf_fn=None, accept_fn=None, error_fn=ERROR_LOG_FILE):
    """Reports the error of the current config,
    and the savings from minimizing Bit-width.
    """
    global step_count

    if step_count==0:
        with open(error_fn, 'a') as f:
            f.write("step\terror\tmem\tmem_int\tmem_fp\texe\texe_int\texe_fp\tmath\n")

    # Re-run approximate program (pass error as 0)
    if error==0:
        stats_fn = "tmp_stats.txt"
        error = test_config(base_config, timeout, stats_fn)
        if error==CRASH or error==TIMEOUT or error==NOOUTPUT:
            logging.error("Configuration is faulty - exiting program")
            exit()

    # Collect dynamic statistics
    if (stats_fn):
        savings = process_dyn_bb_stats(base_config, stats_fn, cdf_fn)

    # Dump the ACCEPT configuration file.
    if (accept_fn):
        dump_relax_config(base_config, accept_fn)

    # Also log to file:
    logging.info ("[step, error, static savings]: [{}, {}, {}]".format(step_count, error, eval_compression_factor(base_config)))
    logging.debug ("Dynamic savings at step {}: {}".format(step_count, savings))
    with open(error_fn, 'a') as f:
        f.write("{}\t{}\t".format(step_count, error))
        f.write("{}\t{}\t{}\t".format(savings["mem"]["all"], savings["mem"]["int"], savings["mem"]["fp"]))
        f.write("{}\t{}\t{}\t{}\n".format(savings["exe"]["all"], savings["exe"]["int"], savings["exe"]["fp"], savings["math"]["fp"]))

    # Increment global
    step_count+=1


#################################################
# Parameterisation testing
#################################################

def tune_himask_insn(base_config, idx, init_snr, timeout):
    """Tunes the most significant bit masking of
    an instruction given its index without affecting
    application error.
    """
    # Generate temporary configuration
    tmp_config = copy.deepcopy(base_config)

    # Initialize the mask and best mask variables
    bitwidth = get_bitwidth_from_type(base_config[idx]['type'])
    mask_val = bitwidth>>1
    best_mask = 0
    # Now to the autotune part - do a log exploration
    for i in range(0, int(math.log(bitwidth, 2))):
        logging.info ("Increasing himask on instruction {} to {}".format(idx, mask_val))
        # Set the mask in the temporary config
        tmp_config[idx]['himask'] = mask_val
        # Test the config
        error = test_config(tmp_config, timeout)
        # Check the error, and modify mask_val accordingly
        if (init_snr==0 and error==0) or (init_snr>0 and abs(init_snr-error)<0.001):
            logging.debug ("New best mask!")
            best_mask = mask_val
            mask_val += bitwidth>>(i+2)
        else:
            mask_val -= bitwidth>>(i+2)
    # Corner case - e.g.: bitmask=31, test 32
    if best_mask==bitwidth-1:
        mask_val = bitwidth
        logging.info ("Increasing himask on instruction {} to {}".format(idx, mask_val))
        # Set the mask in the temporary config
        tmp_config[idx]['himask'] = bitwidth
        # Test the config
        error = test_config(tmp_config, timeout)
        if (init_snr==0 and error==0) or (init_snr>0 and abs(init_snr-error)<0.001):
            logging.debug ("New best mask!")
            best_mask = mask_val
    logging.info ("Himask on instruction {} set to {}".format(idx, mask_val))
    # Return the mask value, and type tuple
    return best_mask

def tune_himask(base_config, init_snr, instlimit, timeout, clusterworkers, run_on_grappa):
    """Tunes the most significant bit masking at an instruction
    granularity without affecting application error.
    """
    logging.info ("##########################")
    logging.info ("Tuning high-order bit mask")
    logging.info ("##########################")

    # Map job IDs to instruction index
    jobs = {}
    jobs_lock = threading.Lock()

    # Map instructions to errors
    insn_himasks = collections.defaultdict(list)

    def completion(jobid, output):
        with jobs_lock:
            idx = jobs.pop(jobid)
        logging.info ("Bit tuning on instruction {} done!".format(idx))
        insn_himasks[idx] = output

    if (clusterworkers):
        # Select partition
        partition = "grappa" if run_on_grappa else "sampa"
        # Kill the master/workers in case previous run failed
        logging.info ("Stopping master/workers that are still running")
        cw.slurm.stop()
        # Start the workers & master
        logging.info ("Starting {} worker(s)".format(clusterworkers))
        cw.slurm.start(
            nworkers=clusterworkers,
            master_options=['--partition='+partition, '-s'],
            worker_options=['--partition='+partition, '-s']
        )
        client = cw.client.ClientThread(completion, cw.slurm.master_host())
        client.start()

    for idx in range(0, min(instlimit, len(base_config))):
        logging.info ("Tuning instruction: {}".format(base_config[idx]['insn']))
        if (is_float_type(base_config[idx]['type'])):
            logging.info ("Skipping tuning because instruction type is a Float")
            insn_himasks[idx] = 0
        else:
            if (clusterworkers>0):
                jobid = cw.randid()
                with jobs_lock:
                    jobs[jobid] = idx
                client.submit(jobid, tune_himask_insn, base_config, idx, init_snr, timeout)
            else:
                insn_himasks[idx] = tune_himask_insn(base_config, idx, init_snr, timeout)

    if (clusterworkers):
        logging.info('All jobs submitted for himaks tuning')
        client.wait()
        logging.info('All jobs finished for himaks tuning')
        cw.slurm.stop()

    # Post processing
    logging.debug ("Himasks: {}".format(insn_himasks))
    for idx in range(0, min(instlimit, len(base_config))):
        base_config[idx]['himask'] = insn_himasks[idx]
        logging.info ("Himask of instruction {} tuned to {}".format(idx, insn_himasks[idx]))

    report_error_and_savings(base_config, timeout)

def tune_lomask(base_config, target_error, target_snr, init_snr, passlimit, instlimit, timeout, clusterworkers, run_on_grappa, save_output = False):
    """Tunes the least significant bits masking to meet the
    specified error requirements, given a passlimit.
    The tuning algorithm performs multiple passes over every
    instructions. For each pass, it masks the LSB of each instuction
    DST register value. At the end of each pass, it masks off the
    instructions that don't affect error at all, and masks the instruction
    that affects error the least. This process is repeated until the target
    error is violated, or the passlimit is reached.
    """
    logging.info ("#########################")
    logging.info ("Tuning low-order bit mask")
    logging.info ("#########################")

    # Map job IDs to instruction index
    jobs = {}
    jobs_lock = threading.Lock()

    # Map instructions to errors
    insn_errors = collections.defaultdict(list)

    # Are we measuring SNR or error?
    snr_mode = True if target_snr > 0 else False

    def completion(jobid, output):
        with jobs_lock:
            idx = jobs.pop(jobid)
        logging.info ("Bit tuning on instruction {} done!".format(idx))
        insn_errors[idx] = output

    if (clusterworkers):
        # Select partition
        partition = "grappa" if run_on_grappa else "sampa"
        # Kill the master/workers in case previous run failed
        logging.info ("Stopping master/workers that are still running")
        cw.slurm.stop()
        # Start the workers & master
        logging.info ("Starting {} worker(s)".format(clusterworkers))
        cw.slurm.start(
            nworkers=clusterworkers,
            master_options=['--partition='+partition, '-s'],
            worker_options=['--partition='+partition, '-s']
        )
        client = cw.client.ClientThread(completion, cw.slurm.master_host())
        client.start()

    # Get the current working directory
    curdir = os.getcwd()
    # Get the last level directory name (the one we're in)
    dirname = os.path.basename(os.path.normpath(curdir))
    # Create a temporary output directory
    outputsdir = curdir+'/../'+dirname+'_outputs'
    tmpoutputsdir = curdir+'/../'+dirname+'_tmpoutputs'
    create_overwrite_directory(outputsdir)
    create_overwrite_directory(tmpoutputsdir)
    logging.debug('Output directory created: {}'.format(outputsdir))
    logging.debug('Tmp output directory created: {}'.format(tmpoutputsdir))

    # Previous min error (to keep track of instructions that don't impact error)
    prev_besterror = init_snr if snr_mode else 0.0
    max_error = 0.0 if snr_mode else float('inf') # worste case error/snr
    # List of instructions that are tuned optimally
    maxed_insn = []
    # List of instructions that cause a timeout
    timeout_insn = []
    # Passes
    for tuning_pass in range(0, passlimit):
        logging.info ("Bit tuning pass #{}".format(tuning_pass))
        # Cleanup tmp results dir
        create_overwrite_directory(tmpoutputsdir)
        # Every RESET_CYCLE reset the maxed_insn
        if tuning_pass % RESET_CYCLE == 0:
            maxed_insn = []
        # Now iterate over all instructions
        for idx in range(0, min(instlimit, len(base_config))):
            logging.info ("Tuning lomask on {}".format(base_config[idx]['insn']))

            # Initial adjustments
            if (base_config[idx]['rate'] > get_bitwidth_from_type(base_config[idx]['type'])-(base_config[idx]['himask']+base_config[idx]['lomask'])):
                base_config[idx]['rate'] = get_bitwidth_from_type(base_config[idx]['type'])-(base_config[idx]['himask']+base_config[idx]['lomask'])
                logging.debug("Updated the mask increment of instruction {} to {}".format(idx, base_config[idx]['rate']))

            # Check if we've reached the bitwidth limit
            if (base_config[idx]['himask']+base_config[idx]['lomask']) == get_bitwidth_from_type(base_config[idx]['type']):
                insn_errors[idx] = max_error
                logging.info ("Skipping current instruction {} - bitmask max reached".format(idx))
            # Check if we've maxed that instruction out (already reached the max error)
            elif idx in maxed_insn:
                insn_errors[idx] = max_error
                logging.info ("Skipping current instruction {} - will degrade quality too much".format(idx))
            # Check if decreasing numerical accuracy of instruction causes a timeout
            elif idx in timeout_insn:
                insn_errors[idx] = max_error
                logging.info ("Skipping current instruction {} - has caused a timeout".format(idx))
            else:
                # Generate temporary configuration
                tmp_config = copy.deepcopy(base_config)
                # Derive the destination file paths
                stats_path = tmpoutputsdir+'/stats_'+str(tuning_pass)+'_'+str(idx)+'.txt'
                if save_output:
                    output_path = tmpoutputsdir+'/'+'out_'+str(tuning_pass)+'_'+str(idx)+EXT
                    logging.debug ("File output path of instruction {}: {}".format(tmp_config[idx]['lomask'], output_path))
                # Increment the LSB mask value
                tmp_config[idx]['lomask'] += tmp_config[idx]['rate']
                logging.info ("Testing lomask of value {} on instruction {}".format(tmp_config[idx]['lomask'], idx))
                # Test the config
                if (clusterworkers):
                    jobid = cw.randid()
                    with jobs_lock:
                        jobs[jobid] = idx
                    if save_output:
                        client.submit(jobid, test_config, tmp_config, timeout, stats_path, output_path)
                    else:
                        client.submit(jobid, test_config, tmp_config, timeout, stats_path)
                else:
                    if save_output:
                        error = test_config(tmp_config, timeout, stats_path, output_path)
                    else:
                        error = test_config(tmp_config, timeout, stats_path)
                    insn_errors[idx] = error
        if (clusterworkers):
            logging.info('All jobs submitted for pass #{}'.format(tuning_pass))
            client.wait()
            logging.info('All jobs finished for pass #{}'.format(tuning_pass))

        # Report all raw errors
        logging.debug ("Errors: {}".format(insn_errors))
        # Keep track of the instruction that results in the least postive error, or the max SNR
        besterror = 0.0 if snr_mode else max_error
        bestidx = -1

        # Keep track of the instructions that results in zero additional error
        # and record the instruction that results in minimal error
        zero_error = []
        for idx in range(0, min(instlimit, len(base_config))):
            error = insn_errors[idx]
            # Check if something went wrong
            if error==CRASH or error==TIMEOUT or error==NOOUTPUT:
                error=max_error
            # If the config cause a timeout, do not revisit the instruction
            if error==TIMEOUT:
                timeout_insn.append(idx)
            # If relaxing the instruction does not add to the error, add to zero error list
            if error == prev_besterror:
                zero_error.append(idx)
            # Update min error accordingly
            elif (not snr_mode and error<besterror) or (snr_mode and error>besterror):
                    besterror = error
                    bestidx = idx

        # Apply LSB masking to the instruction that are not impacted by it
        logging.debug ("Zero-error instruction list: {}".format(zero_error))
        for idx in zero_error:
            base_config[idx]['lomask'] += base_config[idx]['rate']
            logging.info ("Increasing lomask on instruction {} to {} (no additional error)".format(idx, base_config[idx]['lomask']))
        # Report best error achieved
        if snr_mode:
            logging.debug ("[besterror, target_snr] = [{}, {}]".format(besterror, target_snr))
        else:
            logging.debug ("[besterror, target_error] = [{}, {}]".format(besterror, target_error))
        # Apply LSB masking to the instruction that minimizes positive error
        if (not snr_mode and besterror <= target_error) or (snr_mode and besterror >= target_snr):
            base_config[bestidx]['lomask'] += base_config[idx]['rate']
            prev_besterror = besterror
            logging.info ("Increasing lomask on instruction {} to {} (best)".format(bestidx, base_config[bestidx]['lomask']))
            if save_output:
                # Copy file output
                src_path = tmpoutputsdir+'/out_'+str(tuning_pass)+'_'+str(bestidx)+EXT
                dst_path = outputsdir+'/out_{0:05d}'.format(step_count)+EXT
                shutil.copyfile(src_path, dst_path)
            # Report Error and Savings stats
            stats_path = tmpoutputsdir+'/stats_'+str(tuning_pass)+'_'+str(bestidx)+'.txt'
            cdf_path = outputsdir+'/cdf_{0:05d}'.format(step_count)
            accept_path = outputsdir+'/accept_conf_{0:05d}'.format(step_count)+'.txt'
            # report_error_and_savings(base_config, timeout, besterror, stats_path, cdf_path, accept_path)
            report_error_and_savings(base_config, timeout, besterror, stats_path, None, accept_path)

        # Update the masking rates for the instructions which error degradations
        # exceed the threshold. Also set equilibrium to True if all rates have
        # converged to 1.
        equilibrium = True
        for idx in range(0, min(instlimit, len(base_config))):
            # The error is too large so let's reduce the masking rate
            if (not snr_mode and insn_errors[idx] > target_error) or (snr_mode and insn_errors[idx] < target_snr):
                if (base_config[idx]['rate']>1):
                    # This means we haven't reached equilibrium
                    equilibrium = False
                    # Let's half reduce the masking rate
                    base_config[idx]['rate'] = max(int(base_config[idx]['rate']/2), 1)
                    logging.debug("Updated the mask increment of instruction {} to {}".format(idx, base_config[idx]['rate']))
                else:
                    # The rate is already set to 1 so let's tell the autotuner
                    # not to revisit this instruction during later passes
                    maxed_insn.append(idx)

        logging.info ("Bit tuning pass #{} done!\n".format(tuning_pass))

        # Termination conditions:
        # 1 - min error exceeds target_error / max snr is below target snr
        # 2 - empty zero_error
        # 3 - reached equilibrium
        if ((not snr_mode and besterror>target_error) or (snr_mode and besterror<target_snr)) and (not zero_error) and equilibrium:
            break

    if(clusterworkers):
        cw.slurm.stop()

    # Transfer files over
    shutil.move(outputsdir, curdir+'/'+OUTPUT_DIR)


#################################################
# Main Function
#################################################

def tune_width(accept_config_fn, target_error, target_snr, adaptiverate, passlimit, instlimit, skip, timeout, statsOnly, clusterworkers, run_on_grappa):
    """Performs instruction masking tuning
    """
    # Generate default configuration
    if (accept_config_fn):
        config = read_config(accept_config_fn, adaptiverate)
        print_config(config)
    else:
        config = gen_default_config(instlimit, adaptiverate, timeout)

    # If we're only interested in instruction mix stats
    if statsOnly:
            analyzeInstructionMix(config)
            exit()

    # If in SNR mode, measure initial SNR
    if (target_snr>0):
        init_snr = test_config(config, timeout)
        logging.info ("Initial SNR = {}\n".format(init_snr))
    else:
        init_snr = 0

    # Initialize globals
    init_step_count()

    # Let's tune the high mask bits (0 performance degradation)
    if skip!='hi':
        tune_himask(config, init_snr, instlimit, timeout, clusterworkers, run_on_grappa)

    # Now let's tune the low mask bits (performance degradation allowed)
    if skip!='lo':
        tune_lomask(
            config,
            target_error,
            target_snr,
            init_snr,
            passlimit,
            instlimit,
            timeout,
            clusterworkers,
            run_on_grappa)


#################################################
# Argument validation
#################################################

def cli():
    parser = argparse.ArgumentParser(
        description='Bit-width tuning using masking'
    )
    parser.add_argument(
        '-conf', dest='accept_config_fn', action='store', type=str, required=False,
        default=None, help='accept_config_file'
    )
    parser.add_argument(
        '-t', dest='target_error', action='store', type=float, required=False,
        default=0.1, help='target relative application error'
    )
    parser.add_argument(
        '-snr', dest='target_snr', action='store', type=float, required=False,
        default=0, help='target signal to noise ratio (if set, error is ignored)'
    )
    parser.add_argument(
        '-pl', dest='passlimit', action='store', type=int, required=False,
        default=10000, help='limits the number of tuning passes (for quick testing)'
    )
    parser.add_argument(
        '-il', dest='instlimit', action='store', type=int, required=False,
        default=1000, help='limits the number of instructions that get tuned (for quick testing)'
    )
    parser.add_argument(
        '-adaptiverate', dest='adaptiverate', action='store_true', required=False,
        default=False, help='enables adaptive masking rate (speeds up autotuner)'
    )
    parser.add_argument(
        '-skip', dest='skip', action='store', type=str, required=False,
        default=None, help='skip a particular phase'
    )
    parser.add_argument(
        '-timeout', dest='timeout', action='store', type=int, required=False,
        default=600, help='timeout of an experiment in minutes'
    )
    parser.add_argument(
        '-seaborn', dest='seaborn', action='store_true', required=False,
        default=False, help='use seaborn formatting'
    )
    parser.add_argument(
        '-stats', dest='statsOnly', action='store_true', required=False,
        default=False, help='produce instruction breakdown'
    )
    parser.add_argument(
        '-c', dest='clusterworkers', action='store', type=int, required=False,
        default=0, help='max number of machines to allocate on the cluster'
    )
    parser.add_argument(
        '-grappa', dest='grappa', action='store_true', required=False,
        default=False, help='run on grappa partition'
    )
    parser.add_argument(
        '-d', dest='debug', action='store_true', required=False,
        default=False, help='print out debug messages'
    )
    parser.add_argument(
        '-log', dest='logpath', action='store', type=str, required=False,
        default=LOG_FILE, help='path to log file'
    )
    args = parser.parse_args()

    # Take care of log formatting
    logFormatter = logging.Formatter("%(asctime)s [%(threadName)-12.12s] [%(levelname)-5.5s]  %(message)s", datefmt='%m/%d/%Y %I:%M:%S %p')
    rootLogger = logging.getLogger()

    open(args.logpath, 'a').close()
    fileHandler = logging.FileHandler(args.logpath)
    fileHandler.setFormatter(logFormatter)
    rootLogger.addHandler(fileHandler)

    consoleHandler = logging.StreamHandler()
    consoleHandler.setFormatter(logFormatter)
    rootLogger.addHandler(consoleHandler)

    if(args.debug):
        rootLogger.setLevel(logging.DEBUG)
    else:
        rootLogger.setLevel(logging.INFO)

    if(args.seaborn):
        import seaborn

    # Tuning
    tune_width(
        args.accept_config_fn,
        args.target_error,
        args.target_snr,
        args.adaptiverate,
        args.passlimit,
        args.instlimit,
        args.skip,
        args.timeout,
        args.statsOnly,
        args.clusterworkers,
        args.grappa)

    # Close the log handlers
    handlers = rootLogger.handlers[:]
    for handler in rootLogger.handlers[:]:
        handler.close()
        rootLogger.removeHandler(handler)

    # Finally, transfer all files in the outputs dir
    if (os.path.isdir(OUTPUT_DIR)):
        if os.path.exists(ACCEPT_CONFIG):
            shutil.move(ACCEPT_CONFIG, OUTPUT_DIR+'/'+ACCEPT_CONFIG)
        if os.path.exists(ERROR_LOG_FILE):
            shutil.move(ERROR_LOG_FILE, OUTPUT_DIR+'/'+ERROR_LOG_FILE)
        shutil.move(args.logpath, OUTPUT_DIR+'/'+args.logpath)

    # Cleanup
    curdir = os.getcwd()
    dirname = os.path.basename(os.path.normpath(curdir))
    tmpoutputsdir = curdir+'/../'+dirname+'_tmpoutputs'
    shutil.rmtree(tmpoutputsdir)

if __name__ == '__main__':
    cli()
