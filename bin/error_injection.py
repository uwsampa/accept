#!/usr/bin/env python
from __future__ import division
import subprocess32 as subprocess
import tempfile
import os
import shutil
import shlex
import copy
import argparse
import logging
import cw.client
import threading
import collections
import math
import numpy as np
import eval
from eval import EXT

# FILE NAMES
ACCEPT_CONFIG = 'accept_config.txt'
LOG_FILE = 'tune_precision.log'

# OUTPUT FILES
PRECISE_OUTPUT = 'orig'+EXT
APPROX_OUTPUT = 'out'+EXT

# Error codes
ERROR = 0


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

#################################################
# Single Experiment
#################################################

def test(iter, timeout):

    # Get the current working directory
    curdir = os.getcwd()
    # Get the last level directory name (the one we're in)
    dirname = os.path.basename(os.path.normpath(curdir))
    # Create a temporary directory
    tmpdir = tempfile.mkdtemp()+'/'+dirname
    logging.debug('New directory created: {}'.format(tmpdir))
    # Transfer files over
    copy_directory(curdir, tmpdir)
    try:
        shell(shlex.split('make run_opt'), cwd=tmpdir, timeout=timeout)
    except subprocess.TimeoutExpired:
        logging.error('Timed out!')
        return ERROR
    except:
        logging.error('Make error!')
        return ERROR
    # Check file output
    output_fn = os.path.join(tmpdir,APPROX_OUTPUT)
    if os.path.isfile(output_fn):
        snr = eval.score(PRECISE_OUTPUT,output_fn)
        logging.debug('Reported application snr: {}'.format(snr))
    else:
        # Something went wrong - no output!
        logging.warning('Missing output!')
        print_config(config)
        return ERROR

    # Remove the temporary directory
    shutil.rmtree(tmpdir)

    # If nan - set to 0
    if math.isnan(snr):
        snr = 0

    # If SNR is negative - set to 0
    if snr < 0:
        snr = 0

    return float(snr)

#################################################
# Experiments
#################################################

def run_experiment(runs, clusterworkers, timeout=600, confFn=ACCEPT_CONFIG):

    # Map job IDs to instruction index (clusterworkers)
    jobs = {}
    jobs_lock = threading.Lock()

    # Clean up current directory
    try:
        shell(shlex.split('make clean'), timeout=timeout)
    except:
        logging.error('Something went wrong during cleanup.')
        exit()

     # Map runs to SNR
    snr = collections.defaultdict(list)

    # Completion callback (clusterworkers)
    def completion(jobid, output):
        with jobs_lock:
            idx = jobs.pop(jobid)
        logging.info ("Error injection test #{} done - snr = {}".format(idx, output))
        snr[idx] = output

    # Generate the default config
    try:
        shell(shlex.split('make run_orig'), timeout=timeout)
    except:
        logging.error('Something went wrong during initial compilation.')
        exit()

    # Read in default config
    config = []
    with open(confFn) as f:
        for ident, param in parse_relax_config(f):
            config.append({
                'insn': ident,
                'relax': param
                })

    # Edit the default config
    with open(confFn, 'w') as f:
        for conf in config:
            f.write('1 ' + conf['insn'] + '\n')

    # Clusterworkers initilization
    if clusterworkers:
        import cw.client
        # Kill the master/workers in case previous run failed
        logging.info ("Stopping master/workers that are still running")
        cw.slurm.stop()
        # Start the workers & master
        logging.info ("Starting {} worker(s)".format(clusterworkers))
        cw.slurm.start(
            nworkers=clusterworkers
        )
        client = cw.client.ClientThread(completion, cw.slurm.master_host())
        client.start()

    # Run experiments
    for idx in range(runs):
        logging.info ("Error injection test #{}".format(idx))
        if (clusterworkers>0):
            jobid = cw.randid()
            with jobs_lock:
                jobs[jobid] = idx
            client.submit(jobid, test, idx, timeout)
        else:
            snr[idx] = test(idx, timeout)

    if (clusterworkers):
        logging.info('All jobs submitted for error injection experiment')
        client.wait()
        logging.info('All jobs finished for error injection experiment')
        cw.slurm.stop()

    # Process result
    experiments = np.array([snr[key] for key in snr])
    logging.debug("Raw experimental results {}".format(experiments))
    logging.info("Mean:    {}".format(np.mean(experiments)))
    logging.info("Std Dev: {}".format(np.std(experiments)))
    logging.info("Min:     {}".format(np.min(experiments)))
    logging.info("Max:     {}".format(np.max(experiments)))

#################################################
# Argument validation
#################################################

def cli():
    parser = argparse.ArgumentParser(
        description='Error Injection Experiment'
    )
    parser.add_argument(
        '-runs', dest='runs', action='store', type=int, required=False,
        default=100, help='number of experimental runs'
    )
    parser.add_argument(
        '-d', dest='debug', action='store_true', required=False,
        default=False, help='print out debug messages'
    )
    parser.add_argument(
        '-c', dest='clusterworkers', action='store', type=int, required=False,
        default=0, help='number of machines to allocate on the cluster'
    )
    parser.add_argument(
        '-log', dest='logpath', action='store', type=str, required=False,
        default=LOG_FILE, help='path to log file'
    )
    args = parser.parse_args()

    # Take care of log formatting
    logFormatter = logging.Formatter("%(asctime)s [%(threadName)-12.12s] [%(levelname)-5.5s]  %(message)s", datefmt='%m/%d/%Y %I:%M:%S %p')
    rootLogger = logging.getLogger()

    # Print to console
    consoleHandler = logging.StreamHandler()
    consoleHandler.setFormatter(logFormatter)
    rootLogger.addHandler(consoleHandler)

    if(args.debug):
        rootLogger.setLevel(logging.DEBUG)
    else:
        rootLogger.setLevel(logging.INFO)

    run_experiment(args.runs, args.clusterworkers)


if __name__ == '__main__':
    cli()
