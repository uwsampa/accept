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
import tune_precision as tp

ERROR_LOG_FILE = 'error.log'
ACCEPT_CONFIG = 'accept_config.txt'

# OUTPUT FILES
PRECISE_OUTPUT = 'orig'+EXT
APPROX_OUTPUT = 'out'+EXT

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

def getConfFiles(path, paretoFilter=True):
    ''' Searches the path and returns a list of configuration files that are Pareto-Optimal.
    '''
    logFn = path+'/'+ERROR_LOG_FILE
    if (not os.path.isfile(logFn)):
        logging.error('Log file not found in {}'.format(path))
        exit()

    # Read the log file in CSV format
    csv = []
    with open(logFn) as f:
        lines = f.readlines()
        for l in lines:
            csv.append(l.strip().split('\t'))

    # Error index
    errorIdx = csv[0].index("error")

    # Select the pareto-optimal points
    if paretoFilter:
        paretoCsv = [csv[0]]
        for i, elem in enumerate(csv[1:]):
            paretoOptimal = True
            for j in range(i+1,len(csv)):
                if float(csv[j][errorIdx]) > float(elem[errorIdx]):
                    paretoOptimal = False
            if paretoOptimal:
                paretoCsv.append(elem)
        csv = paretoCsv

    return csv

def runTest(dn, mode, error, conf):
    # Check if source path exist
    if dn and os.path.isdir(dn):
        confs = getConfFiles(dn)
        # Binary search the configs
        confs = confs[1:]
        # Indexing
        increment = int(np.power(2, np.ceil(np.log2(len(confs)/2))))
        idx = increment
        # Best Index
        bestIdx = 0
        while increment>0:
            increment = int(increment/2)
            # Derive configuration file name
            step = int(confs[idx][0])
            SNR = float(confs[idx][1])
            config = dn+"/accept_conf_{0:05d}.txt".format(step)
            # Copy the file over
            shutil.copyfile(config, ACCEPT_CONFIG)
            # Produce output
            print "Evaluating {} - with SNR={}".format(config, SNR)
            shell(shlex.split('make run_opt'), cwd=".")
            # Evaluate quality
            test = eval.qorTest(PRECISE_OUTPUT,APPROX_OUTPUT,mode,error,conf)
            shell(shlex.split('make clean'), cwd=".")
            if (test):
                print "\tpass".format(config, SNR)
                bestIdx = step
                if idx == len(confs)-1:
                    break
                idx += increment
                if idx>=len(confs):
                    idx = len(confs)-1
            else:
                print "\tfail".format(config, SNR)
                idx -= increment

    bestConfig = dn+"/accept_conf_{0:05d}.txt".format(bestIdx)
    return bestConfig



#################################################
# Argument validation
#################################################

def cli():
    parser = argparse.ArgumentParser(
        description='Bit-width tuning using masking'
    )
    parser.add_argument(
        '-config', dest='accept_config_dir', action='store', type=str, required=True,
        default=None, help='accept config dir'
    )
    parser.add_argument(
        '-mode', dest='qor_mode', action='store', type=str, required=False,
        default="stat", help='error mode'
    )
    parser.add_argument(
        '-e', dest='error', action='store', type=float, required=False,
        default=0.1, help='error'
    )
    parser.add_argument(
        '-c', dest='confidence', action='store', type=float, required=False,
        default=0.9, help='confidence interval'
    )
    parser.add_argument(
        '-target', dest='target', action='store', type=str, required=True,
        default=None, help='target kernel'
    )
    args = parser.parse_args()

    # If we want to derive stats only
    bestConfig = runTest(
        args.accept_config_dir,
        args.qor_mode,
        args.error,
        args.confidence
    )
    print "Best Index = {}".format(bestConfig)
    stats = tp.analyzeConfigStats(bestConfig, args.target, fixed=True)

    print stats

if __name__ == '__main__':
    cli()
