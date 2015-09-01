#!/usr/bin/env python
import subprocess
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
import eval
from eval import EXT

# FILE NAMES
ACCEPT_CONFIG = 'accept_config.txt'
LOG_FILE = 'tune_precision.log'
ERROR_LOG_FILE = 'error.log'
DYNSTATS_FILE = 'accept_bbstats.txt'
CDF_FILE = 'cdf_stats.txt'

# DIR NAMES
OUTPUT_DIR = 'outputs'

# OUTPUT FILES
PRECISE_OUTPUT = 'orig'+EXT
APPROX_OUTPUT = 'out'+EXT

# PARAMETERS
RESET_CYCLE = 1

# Globals
step_count = 0

#################################################
# Global handling
#################################################
def init_step_count():
    global step_count
    step_count = 0

#################################################
# General OS function helpers
#################################################

def shell(command, cwd=None, shell=False):
    """Execute a command (via a shell or directly). Capture the stdout
    and stderr streams as a string.
    """
    return subprocess.check_output(
        command,
        cwd=cwd,
        stderr=subprocess.STDOUT,
        shell=shell,
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

#################################################
# Configuration file reading/processing
#################################################

def get_bitwidth_from_type(typeStr):
    """Returns the bit-width of a given LLVM type
    """
    if (typeStr=="Half"):
        return 16
    elif(typeStr=="Float"):
        return 32
    elif(typeStr=="Double"):
        return 64
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

def parse_relax_config(f):
    """Parse a relaxation configuration from a file-like object.
    Generates (ident, param) tuples.
    """
    for line in f:
        line = line.strip()
        if line:
            param, ident = line.split(None, 1)
            yield ident, int(param)

def read_config(fname):
    """Reads in a fine error injection descriptor.
    Returns a config object.
    """
    config = []
    with open(fname) as f:
        for ident, param in parse_relax_config(f):
            # If this is in a function indicated in the parameters file,
            # adjust the parameter accordingly.
            if ident.startswith('instruction'):
                _, i_ident = ident.split()
                func, bb, line, opcode, typ = i_ident.split(':')
                himask, lomask = get_masks_from_param(param)

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
                    'rate': get_bitwidth_from_type(typ)/8
                    })

    return config

def read_dyn_stats(fname=DYNSTATS_FILE):
    """Read and the dynamic BB count stats into a list
    """
    bb_info = {}
    with open(fname) as f:
        for l in f:
            line = l.strip()
            bb_idx = line.split('\t')[0]
            bb_num = line.split('\t')[1]
            if (bb_idx.isdigit() and bb_num.isdigit()):
                bb_info[int(bb_idx)] = int(bb_num)
    return bb_info

def analyze(config, stats, BITWIDHTMAX=64, csv_fn=CDF_FILE):
    """Analyze final bit-width settings obtained by the autotuner
    and produce aggregate statistics
    """
    baseline_mem = [0]*(BITWIDHTMAX+1)
    precise_mem = [0]*(BITWIDHTMAX+1)
    approx_mem = [0]*(BITWIDHTMAX+1)
    baseline_exe = [0]*(BITWIDHTMAX+1)
    precise_exe = [0]*(BITWIDHTMAX+1)
    approx_exe = [0]*(BITWIDHTMAX+1)
    for conf in config:
        baseline_width = get_bitwidth_from_type(conf['type'])
        precise_width = get_bitwidth_from_type(conf['type'])-conf['himask']
        approx_width = get_bitwidth_from_type(conf['type'])-conf['himask']-conf['lomask']
        execs = stats[conf['bb']]
        if conf['opcode']=='store' or conf['opcode']=='load':
            baseline_mem[baseline_width]+=execs
            precise_mem[precise_width]+=execs
            approx_mem[approx_width]+=execs
        else:
            baseline_exe[baseline_width]+=execs
            precise_exe[precise_width]+=execs
            approx_exe[approx_width]+=execs

    # Now produce the CDF
    mem_total = sum(baseline_mem)
    exe_total = sum(baseline_exe)
    baseline_mem_cdf = [0]*(BITWIDHTMAX+1)
    precise_mem_cdf = [0]*(BITWIDHTMAX+1)
    approx_mem_cdf = [0]*(BITWIDHTMAX+1)
    baseline_exe_cdf = [0]*(BITWIDHTMAX+1)
    precise_exe_cdf = [0]*(BITWIDHTMAX+1)
    approx_exe_cdf = [0]*(BITWIDHTMAX+1)

    # CSV file that gets outputted
    cdf_stats = [["bitw", "baseline_mem", "precise_mem", "approx_mem", "baseline_exe", "precise_exe", "approx_exe"]]
    for i in range(0, (BITWIDHTMAX+1)):
        if i==0:
            if (mem_total>0):
                baseline_mem_cdf[0] = float(baseline_mem[0])/mem_total
                precise_mem_cdf[0] = float(precise_mem[0])/mem_total
                approx_mem_cdf[0] = float(approx_mem[0])/mem_total
            if (exe_total>0):
                baseline_exe_cdf[0] = float(baseline_exe[0])/exe_total
                precise_exe_cdf[0] = float(precise_exe[0])/exe_total
                approx_exe_cdf[0] = float(approx_exe[0])/exe_total
        else:
            if (mem_total>0):
                baseline_mem_cdf[i] = baseline_mem_cdf[i-1]+float(baseline_mem[i])/mem_total
                precise_mem_cdf[i] = precise_mem_cdf[i-1]+float(precise_mem[i])/mem_total
                approx_mem_cdf[i] = approx_mem_cdf[i-1]+float(approx_mem[i])/mem_total
            if (exe_total>0):
                baseline_exe_cdf[i] = baseline_exe_cdf[i-1]+float(baseline_exe[i])/exe_total
                precise_exe_cdf[i] = precise_exe_cdf[i-1]+float(precise_exe[i])/exe_total
                approx_exe_cdf[i] = approx_exe_cdf[i-1]+float(approx_exe[i])/exe_total
        cdf_stats.append([i, baseline_mem_cdf[i], precise_mem_cdf[i], approx_mem_cdf[i],
            baseline_exe_cdf[i], precise_exe_cdf[i], approx_exe_cdf[i]])

    with open(csv_fn, 'w') as fp:
        csv.writer(fp, delimiter='\t').writerows(cdf_stats)


def gen_default_config(instlimit):
    """Reads in the coarse error injection descriptor,
    generates the default config by running make run_orig.
    Returns a config object.
    """
    curdir = os.getcwd()

    logging.info('Generating the fine config file: {}'.format(ACCEPT_CONFIG))
    shell(shlex.split('make run_orig'), cwd=curdir)

    # Load ACCEPT config and adjust parameters.
    config = read_config(ACCEPT_CONFIG)

    # Notify the user that the instruction limit is lower than the configuration length
    if len(config) > instlimit:
        logging.info('Instruction length exceeds the limit set by the user of {}'.format(instlimit))

    return config

def test_curr_config():
    """Tests the current configuration file locally.
    """
    curdir = os.getcwd()

    logging.info('Testing local configuration: {}'.format(ACCEPT_CONFIG))
    shell(shlex.split('make run_opt'), cwd=curdir)

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

#################################################
# Configuration function
#################################################

def print_config(config):
    """Prints out the configuration.
    """
    logging.info("-----------CONFIG DUMP BEGIN-----------")
    for conf in config:
        logging.info(conf)
    logging.info("----------- CONFIG DUMP END -----------")

def eval_compression_factor(config):
    """Evaluate number of bits saved from compression.
    """
    bits, total = 0, 0
    for conf in config:
        bits += conf['himask']+conf['lomask']
        total += get_bitwidth_from_type(conf['type'])
    return float(bits)/total

def test_config(config, dstpath=None):
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
    shell(shlex.split('make clean'), cwd=tmpdir)
    # Dump config
    dump_relax_config(config, tmpdir+'/'+ACCEPT_CONFIG)
    # Full compile and program run
    logging.debug('Lanching compile and run...')
    try:
        shell(shlex.split('make run_opt'), cwd=tmpdir)
    except:
        logging.warning('Make error!')
        print_config(config)

    # Now that we're done with the compilation, evaluate results
    output_fp = os.path.join(tmpdir,APPROX_OUTPUT)
    if os.path.isfile(output_fp):
        error = eval.score(PRECISE_OUTPUT,os.path.join(tmpdir,APPROX_OUTPUT))
        logging.debug('Reported application error: {}'.format(error))
        if(dstpath):
            shutil.copyfile(os.path.join(tmpdir,APPROX_OUTPUT), dstpath)
    else:
        # Program crashed, set the error to an arbitratily high number
        logging.warning('Program crashed!')
        print_config(config)
        error = float('inf')
    # Remove the temporary directory
    shutil.rmtree(tmpdir)
    # Return the error
    return error

def report_error_and_savings(base_config, error, recompute=False, error_fn=ERROR_LOG_FILE):
    """Reports the error of the current config,
    and the savings from minimizing Bit-width.
    """
    global step_count
    if recompute:
        error = test_config(config)
    logging.info ("[step, error, savings]: [{}, {}, {}]\n".format(step_count, error, eval_compression_factor(base_config)))
    # Also log to file:
    with open(error_fn, 'a') as f:
        f.write("{}\t{}\t{}\n".format(step_count, error, eval_compression_factor(base_config)))
    # Increment global
    step_count+=1

#################################################
# Parameterisation testing
#################################################

def tune_himask_insn(base_config, idx):
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
        error = test_config(tmp_config)
        # Check the error, and modify mask_val accordingly
        if error==0:
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
        error = test_config(tmp_config)
        if error==0:
            logging.debug ("New best mask!")
            best_mask = mask_val
    # Return the mask value, and type tuple
    return best_mask

def tune_himask(base_config, instlimit, clusterworkers, run_on_grappa):
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
                client.submit(jobid, tune_himask_insn, base_config, idx)
            else:
                insn_himasks[idx] = tune_himask_insn(base_config, idx)

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
    report_error_and_savings(base_config, 0.0)


def tune_lomask(base_config, target_error, passlimit, instlimit, clusterworkers, run_on_grappa):
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
    prev_minerror = 0.0
    # List of instructions that are tuned optimally
    maxed_insn = []
    # Passes
    for tuning_pass in range(0, passlimit):
        logging.info ("Bit tuning pass #{}".format(tuning_pass))
        # Every RESET_CYCLE reset the maxed_insn
        if tuning_pass % RESET_CYCLE == 0:
            maxed_insn = []
        # Now iterate over all instructions
        for idx in range(0, min(instlimit, len(base_config))):
            logging.info ("Tuning lomask on {}".format(base_config[idx]['insn']))
            if (base_config[idx]['himask']+base_config[idx]['lomask']) == get_bitwidth_from_type(base_config[idx]['type']):
                insn_errors[idx] = float('inf')
                logging.info ("Skipping current instruction {} - bitmask max reached".format(idx))
            elif idx in maxed_insn:
                insn_errors[idx] = float('inf')
                logging.info ("Skipping current instruction {} - will degrade quality too much".format(idx))
            else:
                # Generate temporary configuration
                tmp_config = copy.deepcopy(base_config)
                # Derive the output path
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
                    client.submit(jobid, test_config, tmp_config, output_path)
                else:
                    error = test_config(tmp_config, output_path)
                    insn_errors[idx] = error
        if (clusterworkers):
            logging.info('All jobs submitted for pass #{}'.format(tuning_pass))
            client.wait()
            logging.info('All jobs finished for pass #{}'.format(tuning_pass))

        # Post Processing

        # Report all errors
        logging.debug ("Errors: {}".format(insn_errors))
        # Keep track of the instruction that results in the least postive error
        minerror, minidx = float("inf"), -1
        # Keep track of the instructions that results zero error
        zero_error = []
        for idx, conf in enumerate(base_config):
            error = insn_errors[idx]
            # Update min error accordingly
            if error == prev_minerror:
                zero_error.append(idx)
            elif error<minerror:
                minerror = error
                minidx = idx
            else:
                # The error is too large, so let's tell the autotuner
                # not to revisit this instruction during later passes
                maxed_insn.append(idx)
                # Also decrement the masking rate
                base_config[idx]['rate'] = max(tmp_config[idx]['rate']/2, 1)
        # Apply LSB masking to the instruction that are not impacted by it
        logging.debug ("Zero-error instruction list: {}".format(zero_error))
        for idx in zero_error:
            base_config[idx]['lomask'] += tmp_config[idx]['rate']
            logging.info ("Increasing lomask on instruction {} to {} (no additional error)".format(idx, tmp_config[idx]['lomask']))
        # Report savings
        if zero_error:
            report_error_and_savings(base_config, prev_minerror)
        # Apply LSB masking to the instruction that minimizes positive error
        logging.debug ("[minerror, target_error] = [{}, {}]".format(minerror, target_error))
        if minerror <= target_error:
            base_config[minidx]['lomask'] += tmp_config[idx]['rate']
            prev_minerror = minerror
            logging.info ("Increasing lomask on instruction {} to {} (best)".format(minidx, tmp_config[minidx]['lomask']))
            report_error_and_savings(base_config, minerror)
            # Copy file output
            src_path = tmpoutputsdir+'/out_'+str(tuning_pass)+'_'+str(minidx)+EXT
            dst_path = outputsdir+'/out_{0:05d}'.format(step_count)+EXT
            shutil.copyfile(src_path, dst_path)
            create_overwrite_directory(tmpoutputsdir)
        # Empty zero list
        elif not zero_error:
            # Ensure before we finish that all masking rates are now down to 1 (equilibrium reached)
            equilibrium = True
            for conf in tmp_config:
                if tmp_config[idx]['rate'] > 1:
                    equilibrium = False
            if equilibrium:
                break
        logging.info ("Bit tuning pass #{} done!\n".format(tuning_pass))

    if(clusterworkers):
        cw.slurm.stop()

    # Transfer files over
    shutil.move(outputsdir, curdir+'/'+OUTPUT_DIR)


#################################################
# Main Function
#################################################

def tune_width(accept_config_fn, target_error, passlimit, instlimit, skip, clusterworkers, run_on_grappa):
    """Performs instruction masking tuning
    """
    # Generate default configuration
    if (accept_config_fn):
        config = read_config(accept_config_fn)
        stats = read_dyn_stats()
        analyze(config, stats)
        exit()
    else:
        config = gen_default_config(instlimit)

    # Initialize globals
    init_step_count()

    # Let's tune the high mask bits (0 performance degradation)
    if skip!='hi':
        tune_himask(config, instlimit, clusterworkers, run_on_grappa)

    # Now let's tune the low mask bits (performance degradation allowed)
    if skip!='lo':
        tune_lomask(config, target_error, passlimit, instlimit, clusterworkers, run_on_grappa)

    # Do some post-processing
    stats = read_dyn_stats()
    analyze(config, stats)

    # Dump back to the fine (ACCEPT) configuration file.
    dump_relax_config(config, ACCEPT_CONFIG)

    # Generate the BB stats
    test_curr_config()

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
        default=0.1, help='target application error'
    )
    parser.add_argument(
        '-pl', dest='passlimit', action='store', type=int, required=False,
        default=1000, help='limits the number of tuning passes (for quick testing)'
    )
    parser.add_argument(
        '-il', dest='instlimit', action='store', type=int, required=False,
        default=1000, help='limits the number of instructions that get tuned (for quick testing)'
    )
    parser.add_argument(
        '-skip', dest='skip', action='store', type=str, required=False,
        default=None, help='skip a particular phase'
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

    # Tuning
    tune_width(
        args.accept_config_fn,
        args.target_error,
        args.passlimit,
        args.instlimit,
        args.skip,
        args.clusterworkers,
        args.grappa)

    # Close the log handlers
    handlers = rootLogger.handlers[:]
    for handler in rootLogger.handlers[:]:
        handler.close()
        rootLogger.removeHandler(handler)

    # Finally, transfer all files in the outputs dir
    if (os.path.isdir(OUTPUT_DIR)):
        shutil.move(ACCEPT_CONFIG, OUTPUT_DIR+'/'+ACCEPT_CONFIG)
        shutil.move(ERROR_LOG_FILE, OUTPUT_DIR+'/'+ERROR_LOG_FILE)
        shutil.move(DYNSTATS_FILE, OUTPUT_DIR+'/'+DYNSTATS_FILE)
        shutil.move(CDF_FILE, OUTPUT_DIR+'/'+CDF_FILE)
        shutil.move(args.logpath, OUTPUT_DIR+'/'+args.logpath)

    # Cleanup
    curdir = os.getcwd()
    dirname = os.path.basename(os.path.normpath(curdir))
    tmpoutputsdir = curdir+'/../'+dirname+'_tmpoutputs'
    shutil.rmtree(tmpoutputsdir)

if __name__ == '__main__':
    cli()
