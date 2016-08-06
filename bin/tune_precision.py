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
# import matplotlib.pyplot as plt
import eval
from eval import EXT

# FILE NAMES
ACCEPT_CONFIG = 'accept_config.txt'
ACCEPT_CONFIG_ROOT = 'accept_conf_'
LOG_FILE = 'tune_precision.log'
ERROR_LOG_FILE = 'error.log'
ERROR_FILE = 'error_stats.txt'
BB_DYNSTATS_FILE = 'accept_bbstats.txt'
FP_DYNSTATS_FILE = 'accept_fpstats.txt'
STATIC_INFO_FILE = 'accept_static.txt'
CDF_FILE = 'cdf_stats'
CSV_FILE = 'run.csv'
EXPONENT_FILE = '../../../../exponentStats.txt'

# DIR NAMES
OUTPUT_DIR = 'outputs'

# Globally defined
outputsdir = os.getcwd()+'/../'+os.path.basename(os.path.normpath(os.getcwd()))+'_outputs'
tmpoutputsdir = os.getcwd()+'/../'+os.path.basename(os.path.normpath(os.getcwd()))+'tmp_outputs'

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

# RTL-derived energy numbers
ALUEnergy = {
    "add": {
        "par": [0, 0.0264, 0.0418, 0.0802, 0.0912, 0.1702, 0.1806, 0.1922, 0.2, 0.342, 0.354, 0.364, 0.374, 0.384, 0.396, 0.408, 0.416, 0.568, 0.578, 0.588, 0.598, 0.608, 0.62, 0.63, 0.642, 0.652, 0.662, 0.674, 0.682, 0.694, 0.706, 0.718, 0.726, 1.06, 1.068, 1.084, 1.096, 1.108, 1.118, 1.134, 1.146, 1.158, 1.168, 1.182, 1.194, 1.206, 1.214, 1.23, 1.24, 1.25, 1.26, 1.27, 1.282, 1.29, 1.298, 1.308, 1.318, 1.33, 1.338, 1.35, 1.358, 1.368, 1.378, 1.39, 1.4],
        64: [0, 0.69, 0.702, 0.714, 0.726, 0.736, 0.746, 0.76, 0.77, 0.78, 0.79, 0.804, 0.814, 0.824, 0.834, 0.85, 0.86, 0.872, 0.88, 0.896, 0.906, 0.916, 0.926, 0.94, 0.952, 0.962, 0.97, 0.984, 0.994, 1.006, 1.016, 1.034, 1.046, 1.06, 1.068, 1.084, 1.096, 1.108, 1.118, 1.134, 1.146, 1.158, 1.168, 1.182, 1.194, 1.206, 1.214, 1.23, 1.24, 1.25, 1.26, 1.27, 1.282, 1.29, 1.298, 1.308, 1.318, 1.33, 1.338, 1.35, 1.358, 1.368, 1.378, 1.39, 1.4],
        32: [0, 0.378, 0.39, 0.402, 0.416, 0.428, 0.438, 0.454, 0.464, 0.476, 0.488, 0.502, 0.514, 0.526, 0.536, 0.548, 0.558, 0.568, 0.578, 0.588, 0.598, 0.608, 0.62, 0.63, 0.642, 0.652, 0.662, 0.674, 0.682, 0.694, 0.706, 0.718, 0.726, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452, 1.452],
        16: [0, 0.25, 0.262, 0.274, 0.288, 0.298, 0.31, 0.322, 0.332, 0.342, 0.354, 0.364, 0.374, 0.384, 0.396, 0.408, 0.416, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 0.832, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.248, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664, 1.664],
        8: [0, 0.122, 0.1346, 0.1468, 0.1596, 0.1702, 0.1806, 0.1922, 0.2, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.2, 1.4, 1.4, 1.4, 1.4, 1.4, 1.4, 1.4, 1.4, 1.6, 1.6, 1.6, 1.6, 1.6, 1.6, 1.6, 1.6],
        4: [0, 0.0554, 0.068, 0.0802, 0.0912, 0.1824, 0.1824, 0.1824, 0.1824, 0.2736, 0.2736, 0.2736, 0.2736, 0.3648, 0.3648, 0.3648, 0.3648, 0.456, 0.456, 0.456, 0.456, 0.5472, 0.5472, 0.5472, 0.5472, 0.6384, 0.6384, 0.6384, 0.6384, 0.7296, 0.7296, 0.7296, 0.7296, 0.8208, 0.8208, 0.8208, 0.8208, 0.912, 0.912, 0.912, 0.912, 1.0032, 1.0032, 1.0032, 1.0032, 1.0944, 1.0944, 1.0944, 1.0944, 1.1856, 1.1856, 1.1856, 1.1856, 1.2768, 1.2768, 1.2768, 1.2768, 1.368, 1.368, 1.368, 1.368, 1.4592, 1.4592, 1.4592, 1.4592],
        2: [0, 0.0312, 0.0418, 0.0836, 0.0836, 0.1254, 0.1254, 0.1672, 0.1672, 0.209, 0.209, 0.2508, 0.2508, 0.2926, 0.2926, 0.3344, 0.3344, 0.3762, 0.3762, 0.418, 0.418, 0.4598, 0.4598, 0.5016, 0.5016, 0.5434, 0.5434, 0.5852, 0.5852, 0.627, 0.627, 0.6688, 0.6688, 0.7106, 0.7106, 0.7524, 0.7524, 0.7942, 0.7942, 0.836, 0.836, 0.8778, 0.8778, 0.9196, 0.9196, 0.9614, 0.9614, 1.0032, 1.0032, 1.045, 1.045, 1.0868, 1.0868, 1.1286, 1.1286, 1.1704, 1.1704, 1.2122, 1.2122, 1.254, 1.254, 1.2958, 1.2958, 1.3376, 1.3376],
        1: [0, 0.0264, 0.0528, 0.0792, 0.1056, 0.132, 0.1584, 0.1848, 0.2112, 0.2376, 0.264, 0.2904, 0.3168, 0.3432, 0.3696, 0.396, 0.4224, 0.4488, 0.4752, 0.5016, 0.528, 0.5544, 0.5808, 0.6072, 0.6336, 0.66, 0.6864, 0.7128, 0.7392, 0.7656, 0.792, 0.8184, 0.8448, 0.8712, 0.8976, 0.924, 0.9504, 0.9768, 1.0032, 1.0296, 1.056, 1.0824, 1.1088, 1.1352, 1.1616, 1.188, 1.2144, 1.2408, 1.2672, 1.2936, 1.32, 1.3464, 1.3728, 1.3992, 1.4256, 1.452, 1.4784, 1.5048, 1.5312, 1.5576, 1.584, 1.6104, 1.6368, 1.6632, 1.6896]
    },
    # Power-gated model: assumes power-of-two power gating ability (idealized)
    "mul_pg": {
        "par": [0, 0.0408, 0.0552, 0.1526, 0.1926, 0.402, 0.49, 0.558, 0.63, 1.266, 1.474, 1.546, 1.726, 1.812, 1.966, 2.04, 2.18, 4.9, 5.44, 5.66, 6.1, 6.24, 6.74, 6.9, 7.32, 7.46, 7.84, 8.04, 8.36, 8.5, 8.8, 8.96, 9.22, 23.8, 24.8, 25.4, 26.4, 27.0, 27.8, 28.4, 29.4, 30.0, 31.0, 31.6, 32.6, 33.2, 34.2, 34.6, 35.4, 35.8, 36.8, 37.2, 38.0, 38.4, 39.2, 39.6, 40.2, 40.6, 41.2, 41.6, 42.2, 42.2, 43.0, 43.2, 43.8],
        64: [0, 2.9, 4.52, 4.76, 5.54, 5.62, 6.4, 6.34, 7.32, 7.44, 8.44, 8.4, 9.62, 9.7, 10.7, 10.88, 12.08, 12.2, 13.42, 13.48, 14.7, 14.8, 16.12, 16.36, 17.52, 17.78, 18.84, 19.18, 20.2, 20.6, 21.8, 22.2, 23.2, 23.8, 24.8, 25.4, 26.4, 27.0, 27.8, 28.4, 29.4, 30.0, 31.0, 31.6, 32.6, 33.2, 34.2, 34.6, 35.4, 35.8, 36.8, 37.2, 38.0, 38.4, 39.2, 39.6, 40.2, 40.6, 41.2, 41.6, 42.2, 42.2, 43.0, 43.2, 43.8],
        32: [0, 1.042, 1.624, 1.738, 1.946, 2.02, 2.28, 2.34, 2.68, 2.76, 3.14, 3.18, 3.6, 3.74, 4.08, 4.24, 4.72, 4.9, 5.44, 5.66, 6.1, 6.24, 6.74, 6.9, 7.32, 7.46, 7.84, 8.04, 8.36, 8.5, 8.8, 8.96, 9.22, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36],
        16: [0, 0.396, 0.592, 0.642, 0.766, 0.814, 0.958, 1.01, 1.188, 1.266, 1.474, 1.546, 1.726, 1.812, 1.966, 2.04, 2.18, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0],
        8: [0, 0.224, 0.248, 0.284, 0.332, 0.402, 0.49, 0.558, 0.63, 2.056, 2.056, 2.056, 2.056, 2.056, 2.056, 2.056, 2.056, 5.238, 5.238, 5.238, 5.238, 5.238, 5.238, 5.238, 5.238, 6.984, 6.984, 6.984, 6.984, 6.984, 6.984, 6.984, 6.984, 17.3, 17.3, 17.3, 17.3, 17.3, 17.3, 17.3, 17.3, 20.76, 20.76, 20.76, 20.76, 20.76, 20.76, 20.76, 20.76, 24.22, 24.22, 24.22, 24.22, 24.22, 24.22, 24.22, 24.22, 27.68, 27.68, 27.68, 27.68, 27.68, 27.68, 27.68, 27.68],
        4: [0, 0.1038, 0.1222, 0.1526, 0.1926, 0.684, 0.684, 0.684, 0.684, 1.668, 1.668, 1.668, 1.668, 2.224, 2.224, 2.224, 2.224, 5.29, 5.29, 5.29, 5.29, 6.348, 6.348, 6.348, 6.348, 7.406, 7.406, 7.406, 7.406, 8.464, 8.464, 8.464, 8.464, 18.72, 18.72, 18.72, 18.72, 20.8, 20.8, 20.8, 20.8, 22.88, 22.88, 22.88, 22.88, 24.96, 24.96, 24.96, 24.96, 27.04, 27.04, 27.04, 27.04, 29.12, 29.12, 29.12, 29.12, 31.2, 31.2, 31.2, 31.2, 33.28, 33.28, 33.28, 33.28],
        2: [0, 0.0408, 0.0552, 0.2616, 0.2616, 0.5586, 0.5586, 0.7448, 0.7448, 1.85, 1.85, 2.22, 2.22, 2.59, 2.59, 2.96, 2.96, 7.038, 7.038, 7.82, 7.82, 8.602, 8.602, 9.384, 9.384, 10.166, 10.166, 10.948, 10.948, 11.73, 11.73, 12.512, 12.512, 25.398, 25.398, 26.892, 26.892, 28.386, 28.386, 29.88, 29.88, 31.374, 31.374, 32.868, 32.868, 34.362, 34.362, 35.856, 35.856, 37.35, 37.35, 38.844, 38.844, 40.338, 40.338, 41.832, 41.832, 43.326, 43.326, 44.82, 44.82, 46.314, 46.314, 47.808, 47.808],
        1: [0, 0.0428, 0.0856, 0.1848, 0.2464, 0.663, 0.7956, 0.9282, 1.0608, 2.484, 2.76, 3.036, 3.312, 3.588, 3.864, 4.14, 4.416, 10.132, 10.728, 11.324, 11.92, 12.516, 13.112, 13.708, 14.304, 14.9, 15.496, 16.092, 16.688, 17.284, 17.88, 18.476, 19.072, 41.052, 42.296, 43.54, 44.784, 46.028, 47.272, 48.516, 49.76, 51.004, 52.248, 53.492, 54.736, 55.98, 57.224, 58.468, 59.712, 60.956, 62.2, 63.444, 64.688, 65.932, 67.176, 68.42, 69.664, 70.908, 72.152, 73.396, 74.64, 75.884, 77.128, 78.372, 79.616]
    },
    # Reference multiplier width: 64bits
    "mul_64": {
        64: [0, 2.9, 4.52, 4.76, 5.54, 5.62, 6.4, 6.34, 7.32, 7.44, 8.44, 8.4, 9.62, 9.7, 10.7, 10.88, 12.08, 12.2, 13.42, 13.48, 14.7, 14.8, 16.12, 16.36, 17.52, 17.78, 18.84, 19.18, 20.2, 20.6, 21.8, 22.2, 23.2, 23.8, 24.8, 25.4, 26.4, 27.0, 27.8, 28.4, 29.4, 30.0, 31.0, 31.6, 32.6, 33.2, 34.2, 34.6, 35.4, 35.8, 36.8, 37.2, 38.0, 38.4, 39.2, 39.6, 40.2, 40.6, 41.2, 41.6, 42.2, 42.2, 43.0, 43.2, 43.8],
        32: [0, 1.042, 1.624, 1.738, 1.946, 2.02, 2.28, 2.34, 2.68, 2.76, 3.14, 3.18, 3.6, 3.74, 4.08, 4.24, 4.72, 4.9, 5.44, 5.66, 6.1, 6.24, 6.74, 6.9, 7.32, 7.46, 7.84, 8.04, 8.36, 8.5, 8.8, 8.96, 14.18, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36, 28.36],
        16: [0, 0.396, 0.592, 0.642, 0.766, 0.814, 0.958, 1.01, 1.188, 1.266, 1.474, 1.546, 1.726, 1.812, 1.966, 2.04, 6.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0, 24.0],
        8: [0, 0.224, 0.248, 0.284, 0.332, 0.402, 0.49, 0.558, 3.46, 6.92, 6.92, 6.92, 6.92, 6.92, 6.92, 6.92, 6.92, 10.38, 10.38, 10.38, 10.38, 10.38, 10.38, 10.38, 10.38, 13.84, 13.84, 13.84, 13.84, 13.84, 13.84, 13.84, 13.84, 17.3, 17.3, 17.3, 17.3, 17.3, 17.3, 17.3, 17.3, 20.76, 20.76, 20.76, 20.76, 20.76, 20.76, 20.76, 20.76, 24.22, 24.22, 24.22, 24.22, 24.22, 24.22, 24.22, 24.22, 27.68, 27.68, 27.68, 27.68, 27.68, 27.68, 27.68, 27.68],
        4: [0, 0.1038, 0.1222, 0.1526, 2.08, 4.16, 4.16, 4.16, 4.16, 6.24, 6.24, 6.24, 6.24, 8.32, 8.32, 8.32, 8.32, 10.4, 10.4, 10.4, 10.4, 12.48, 12.48, 12.48, 12.48, 14.56, 14.56, 14.56, 14.56, 16.64, 16.64, 16.64, 16.64, 18.72, 18.72, 18.72, 18.72, 20.8, 20.8, 20.8, 20.8, 22.88, 22.88, 22.88, 22.88, 24.96, 24.96, 24.96, 24.96, 27.04, 27.04, 27.04, 27.04, 29.12, 29.12, 29.12, 29.12, 31.2, 31.2, 31.2, 31.2, 33.28, 33.28, 33.28, 33.28],
        2: [0, 0.0408, 1.494, 2.988, 2.988, 4.482, 4.482, 5.976, 5.976, 7.47, 7.47, 8.964, 8.964, 10.458, 10.458, 11.952, 11.952, 13.446, 13.446, 14.94, 14.94, 16.434, 16.434, 17.928, 17.928, 19.422, 19.422, 20.916, 20.916, 22.41, 22.41, 23.904, 23.904, 25.398, 25.398, 26.892, 26.892, 28.386, 28.386, 29.88, 29.88, 31.374, 31.374, 32.868, 32.868, 34.362, 34.362, 35.856, 35.856, 37.35, 37.35, 38.844, 38.844, 40.338, 40.338, 41.832, 41.832, 43.326, 43.326, 44.82, 44.82, 46.314, 46.314, 47.808, 47.808],
        1: [0, 1.244, 2.488, 3.732, 4.976, 6.22, 7.464, 8.708, 9.952, 11.196, 12.44, 13.684, 14.928, 16.172, 17.416, 18.66, 19.904, 21.148, 22.392, 23.636, 24.88, 26.124, 27.368, 28.612, 29.856, 31.1, 32.344, 33.588, 34.832, 36.076, 37.32, 38.564, 39.808, 41.052, 42.296, 43.54, 44.784, 46.028, 47.272, 48.516, 49.76, 51.004, 52.248, 53.492, 54.736, 55.98, 57.224, 58.468, 59.712, 60.956, 62.2, 63.444, 64.688, 65.932, 67.176, 68.42, 69.664, 70.908, 72.152, 73.396, 74.64, 75.884, 77.128, 78.372, 79.616]
    },
     # Reference multiplier width: 32bits
    "mul_32": {
        32: [0, 1.042, 1.624, 1.738, 1.946, 2.02, 2.28, 2.34, 2.68, 2.76, 3.14, 3.18, 3.6, 3.74, 4.08, 4.24, 4.72, 4.9, 5.44, 5.66, 6.1, 6.24, 6.74, 6.9, 7.32, 7.46, 7.84, 8.04, 8.36, 8.5, 8.8, 8.96, 9.22, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88, 36.88],
        16: [0, 0.396, 0.592, 0.642, 0.766, 0.814, 0.958, 1.01, 1.188, 1.266, 1.474, 1.546, 1.726, 1.812, 1.966, 2.04, 3.44, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 6.88, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 20.64, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52, 27.52],
        8: [0, 0.224, 0.248, 0.284, 0.332, 0.402, 0.49, 0.558, 1.746, 3.492, 3.492, 3.492, 3.492, 3.492, 3.492, 3.492, 3.492, 5.238, 5.238, 5.238, 5.238, 5.238, 5.238, 5.238, 5.238, 6.984, 6.984, 6.984, 6.984, 6.984, 6.984, 6.984, 6.984, 17.46, 17.46, 17.46, 17.46, 17.46, 17.46, 17.46, 17.46, 20.952, 20.952, 20.952, 20.952, 20.952, 20.952, 20.952, 20.952, 24.444, 24.444, 24.444, 24.444, 24.444, 24.444, 24.444, 24.444, 27.936, 27.936, 27.936, 27.936, 27.936, 27.936, 27.936, 27.936],
        4: [0, 0.1038, 0.1222, 0.1526, 1.058, 2.116, 2.116, 2.116, 2.116, 3.174, 3.174, 3.174, 3.174, 4.232, 4.232, 4.232, 4.232, 5.29, 5.29, 5.29, 5.29, 6.348, 6.348, 6.348, 6.348, 7.406, 7.406, 7.406, 7.406, 8.464, 8.464, 8.464, 8.464, 19.044, 19.044, 19.044, 19.044, 21.16, 21.16, 21.16, 21.16, 23.276, 23.276, 23.276, 23.276, 25.392, 25.392, 25.392, 25.392, 27.508, 27.508, 27.508, 27.508, 29.624, 29.624, 29.624, 29.624, 31.74, 31.74, 31.74, 31.74, 33.856, 33.856, 33.856, 33.856],
        2: [0, 0.0408, 0.782, 1.564, 1.564, 2.346, 2.346, 3.128, 3.128, 3.91, 3.91, 4.692, 4.692, 5.474, 5.474, 6.256, 6.256, 7.038, 7.038, 7.82, 7.82, 8.602, 8.602, 9.384, 9.384, 10.166, 10.166, 10.948, 10.948, 11.73, 11.73, 12.512, 12.512, 26.588, 26.588, 28.152, 28.152, 29.716, 29.716, 31.28, 31.28, 32.844, 32.844, 34.408, 34.408, 35.972, 35.972, 37.536, 37.536, 39.1, 39.1, 40.664, 40.664, 42.228, 42.228, 43.792, 43.792, 45.356, 45.356, 46.92, 46.92, 48.484, 48.484, 50.048, 50.048],
        1: [0, 0.596, 1.192, 1.788, 2.384, 2.98, 3.576, 4.172, 4.768, 5.364, 5.96, 6.556, 7.152, 7.748, 8.344, 8.94, 9.536, 10.132, 10.728, 11.324, 11.92, 12.516, 13.112, 13.708, 14.304, 14.9, 15.496, 16.092, 16.688, 17.284, 17.88, 18.476, 19.072, 39.336, 40.528, 41.72, 42.912, 44.104, 45.296, 46.488, 47.68, 48.872, 50.064, 51.256, 52.448, 53.64, 54.832, 56.024, 57.216, 58.408, 59.6, 60.792, 61.984, 63.176, 64.368, 65.56, 66.752, 67.944, 69.136, 70.328, 71.52, 72.712, 73.904, 75.096, 76.288]
    },
     # Reference multiplier width: 16bits
    "mul_16": {
        16: [0, 0.396, 0.592, 0.642, 0.766, 0.814, 0.958, 1.01, 1.188, 1.266, 1.474, 1.546, 1.726, 1.812, 1.966, 2.04, 2.18, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 8.72, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 19.62, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88, 34.88],
        8: [0, 0.224, 0.248, 0.284, 0.332, 0.402, 0.49, 0.558, 1.028, 2.056, 2.056, 2.056, 2.056, 2.056, 2.056, 2.056, 2.056, 6.168, 6.168, 6.168, 6.168, 6.168, 6.168, 6.168, 6.168, 8.224, 8.224, 8.224, 8.224, 8.224, 8.224, 8.224, 8.224, 15.42, 15.42, 15.42, 15.42, 15.42, 15.42, 15.42, 15.42, 18.504, 18.504, 18.504, 18.504, 18.504, 18.504, 18.504, 18.504, 28.784, 28.784, 28.784, 28.784, 28.784, 28.784, 28.784, 28.784, 32.896, 32.896, 32.896, 32.896, 32.896, 32.896, 32.896, 32.896],
        4: [0, 0.1038, 0.1222, 0.1526, 0.556, 1.112, 1.112, 1.112, 1.112, 1.668, 1.668, 1.668, 1.668, 2.224, 2.224, 2.224, 2.224, 5.56, 5.56, 5.56, 5.56, 6.672, 6.672, 6.672, 6.672, 7.784, 7.784, 7.784, 7.784, 8.896, 8.896, 8.896, 8.896, 15.012, 15.012, 15.012, 15.012, 16.68, 16.68, 16.68, 16.68, 18.348, 18.348, 18.348, 18.348, 20.016, 20.016, 20.016, 20.016, 28.912, 28.912, 28.912, 28.912, 31.136, 31.136, 31.136, 31.136, 33.36, 33.36, 33.36, 33.36, 35.584, 35.584, 35.584, 35.584],
        2: [0, 0.0408, 0.37, 0.74, 0.74, 1.11, 1.11, 1.48, 1.48, 1.85, 1.85, 2.22, 2.22, 2.59, 2.59, 2.96, 2.96, 6.66, 6.66, 7.4, 7.4, 8.14, 8.14, 8.88, 8.88, 9.62, 9.62, 10.36, 10.36, 11.1, 11.1, 11.84, 11.84, 18.87, 18.87, 19.98, 19.98, 21.09, 21.09, 22.2, 22.2, 23.31, 23.31, 24.42, 24.42, 25.53, 25.53, 26.64, 26.64, 37.0, 37.0, 38.48, 38.48, 39.96, 39.96, 41.44, 41.44, 42.92, 42.92, 44.4, 44.4, 45.88, 45.88, 47.36, 47.36],
        1: [0, 0.276, 0.552, 0.828, 1.104, 1.38, 1.656, 1.932, 2.208, 2.484, 2.76, 3.036, 3.312, 3.588, 3.864, 4.14, 4.416, 9.384, 9.936, 10.488, 11.04, 11.592, 12.144, 12.696, 13.248, 13.8, 14.352, 14.904, 15.456, 16.008, 16.56, 17.112, 17.664, 27.324, 28.152, 28.98, 29.808, 30.636, 31.464, 32.292, 33.12, 33.948, 34.776, 35.604, 36.432, 37.26, 38.088, 38.916, 39.744, 54.096, 55.2, 56.304, 57.408, 58.512, 59.616, 60.72, 61.824, 62.928, 64.032, 65.136, 66.24, 67.344, 68.448, 69.552, 70.656]
    },
     # Reference multiplier width: 8bits
    "mul_8": {
        8: [0, 0.224, 0.248, 0.284, 0.332, 0.402, 0.49, 0.558, 0.63, 2.52, 2.52, 2.52, 2.52, 2.52, 2.52, 2.52, 2.52, 5.67, 5.67, 5.67, 5.67, 5.67, 5.67, 5.67, 5.67, 10.08, 10.08, 10.08, 10.08, 10.08, 10.08, 10.08, 10.08, 15.75, 15.75, 15.75, 15.75, 15.75, 15.75, 15.75, 15.75, 22.68, 22.68, 22.68, 22.68, 22.68, 22.68, 22.68, 22.68, 30.87, 30.87, 30.87, 30.87, 30.87, 30.87, 30.87, 30.87, 40.32, 40.32, 40.32, 40.32, 40.32, 40.32, 40.32, 40.32],
        4: [0, 0.1038, 0.1222, 0.1526, 0.342, 0.684, 0.684, 0.684, 0.684, 2.052, 2.052, 2.052, 2.052, 2.736, 2.736, 2.736, 2.736, 5.13, 5.13, 5.13, 5.13, 6.156, 6.156, 6.156, 6.156, 9.576, 9.576, 9.576, 9.576, 10.944, 10.944, 10.944, 10.944, 15.39, 15.39, 15.39, 15.39, 17.1, 17.1, 17.1, 17.1, 22.572, 22.572, 22.572, 22.572, 24.624, 24.624, 24.624, 24.624, 31.122, 31.122, 31.122, 31.122, 33.516, 33.516, 33.516, 33.516, 41.04, 41.04, 41.04, 41.04, 43.776, 43.776, 43.776, 43.776],
        2: [0, 0.0408, 0.1862, 0.3724, 0.3724, 0.5586, 0.5586, 0.7448, 0.7448, 1.862, 1.862, 2.2344, 2.2344, 2.6068, 2.6068, 2.9792, 2.9792, 5.0274, 5.0274, 5.586, 5.586, 6.1446, 6.1446, 6.7032, 6.7032, 9.6824, 9.6824, 10.4272, 10.4272, 11.172, 11.172, 11.9168, 11.9168, 15.827, 15.827, 16.758, 16.758, 17.689, 17.689, 18.62, 18.62, 23.4612, 23.4612, 24.5784, 24.5784, 25.6956, 25.6956, 26.8128, 26.8128, 32.585, 32.585, 33.8884, 33.8884, 35.1918, 35.1918, 36.4952, 36.4952, 43.1984, 43.1984, 44.688, 44.688, 46.1776, 46.1776, 47.6672, 47.6672],
        1: [0, 0.1326, 0.2652, 0.3978, 0.5304, 0.663, 0.7956, 0.9282, 1.0608, 2.3868, 2.652, 2.9172, 3.1824, 3.4476, 3.7128, 3.978, 4.2432, 6.7626, 7.1604, 7.5582, 7.956, 8.3538, 8.7516, 9.1494, 9.5472, 13.26, 13.7904, 14.3208, 14.8512, 15.3816, 15.912, 16.4424, 16.9728, 21.879, 22.542, 23.205, 23.868, 24.531, 25.194, 25.857, 26.52, 32.6196, 33.4152, 34.2108, 35.0064, 35.802, 36.5976, 37.3932, 38.1888, 45.4818, 46.41, 47.3382, 48.2664, 49.1946, 50.1228, 51.051, 51.9792, 60.4656, 61.5264, 62.5872, 63.648, 64.7088, 65.7696, 66.8304, 67.8912]
    }
}


# Simple energy cost map:
energyMap = {
    'fadd': "add",
    'fsub': "add",
    'fmul': "mul",
    'fdiv': "mul",
    'frem': "mul",
    'add': "add",
    'sub': "add",
    'mul': "mul",
    'udiv': "mul",
    'sdiv': "mul",
    'urem': "mul",
    'srem': "mul",
    'shl': "add",
    'lshr': "add",
    'ashr': "add",
    'and': "add",
    'or': "add",
    'xor': "add"
}
fpInsn = ['fadd','fsub','fmul','fdiv','frem']
binaryInsn = ['add','sub','mul','udiv','sdiv','urem','srem']
bitbinInsn = ['shl','lshr','ashr','and','or','xor']

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
    { "cat": "int_arith", "iList": binaryInsn},
    { "cat": "bit_arith", "iList": bitbinInsn},
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
    "int_arith" : binaryInsn,
    "bit_arith" : bitbinInsn,
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
    "bit_arith": 0,
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

def get_precision_from_type(typeStr):
    """Returns the precision bit-width of a given LLVM type
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

def get_float_exp_offset(typeStr):
    """Returns the exponent offset of a float
    """
    if (typeStr=="Half"):
        return 15
    elif(typeStr=="Float"):
        return 127
    elif(typeStr=="Double"):
        return 1023
    else:
        return 0



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
        exit()
    if (sum(matchVector)>1):
        print "Error - multiple matches!"
        print l
        exit()
    return matchVector

def determineIRCat2(op, dictionary):
    matchVector = [0]*len(dictionary)
    for idx, cat in enumerate(dictionary):
        if op in cat["iList"]:
            matchVector[idx] += 1
            return matchVector
    return matchVector

def read_static_stats(llFile):
    """ Processes an LLVM IR file (.ll) and produces
    a static profile of the instructions
    """

    bbInfo = {} # bbInfo[bbId][insnCat] = count
    locMap = {} # locMap[bbId] = location

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

def read_static_stats_2(staticFile, target_func=None):

    bbInfo = {} # bbInfo[bbId][insnCat] = count

    # Process static dump line by line
    if (os.path.isfile(staticFile)):
        with open(staticFile) as fp:
            for line in fp:
                # Tokenize
                tokens = line.strip().split(", ")
                # Get the function name
                fn_name = tokens[0]
                # Get the op type
                opcode = tokens[1]
                # Get the iid
                iid = tokens[2]
                bbid = int(iid.split("i")[0][2:])

                # Restrict to instrution in the target function
                # If specified
                if target_func and fn_name==target_func or not target_func:

                    # Add the instruction dictionary for BB
                    if bbid not in bbInfo:
                        bbInfo[bbid] = dict(bbIrCatDict)

                    insnCatVector = determineIRCat2(opcode, llvmInsnList)
                    insnCat = llvmInsnList[insnCatVector.index(1)]["cat"]
                    if insnCat in bbInfo[bbid]:
                        if insnCat=="call":
                            # Special case, check for cMath functions we care about
                            callee = tokens[4]
                            stdcCatVector = determineIRCat2(callee, stdCList)
                            if 1 not in stdcCatVector:
                                bbInfo[bbid][insnCat] += 1
                            else:
                                stdcCat = stdCList[stdcCatVector.index(1)]["cat"]
                                bbInfo[bbid][stdcCat] += 1
                        else:
                            bbInfo[bbid][insnCat] += 1

                    bbInfo[bbid]["total"] += 1

    return bbInfo


#################################################
# Bit mask setting to conf file param conversion
#################################################

def get_param_from_masks(himask, lomask, maxexp=0):
    """Returns parameter from width settings
    """
    return (maxexp<<16) + (himask<<8) + lomask

def get_masks_from_param(param):
    """Returns mask width settings from parameter
    """
    if param==0:
        return 0,0,0
    maxexp = (param>>16) & 0x0000FFFF;
    param &= 0x0000FFFF
    himask = (param >> 8) & 0xFF;
    lomask = (param & 0xFF);
    return himask, lomask, maxexp


#################################################
# Configuration file reading/processing
#################################################

def analyzeConfigStats(config_fn, target_func, fixed, extraChecks=True, reportEnergy = False, plotFPScatterplot=False, aluWidthMin=8, aluWidthMax=64):
    """ Analyzes the instruction mix of an approximate program
    """

    # Statistics
    stats = []

    # Generate the LLVM IR file of the orig program
    curdir = os.getcwd()
    bench = curdir.split('/')[-1]

    # Generate config file if necessary, produce output
    try:
        # Check if config file is provided
        if config_fn:
            shutil.copyfile(config_fn, ACCEPT_CONFIG)
        else:
            config_fn = ACCEPT_CONFIG
            shell(shlex.split('make run_orig'), cwd=curdir)
    except:
        logging.error('Something went wrong generating default config.')
        exit()
    shell(shlex.split('make run_opt'), cwd=curdir)

    assert (os.path.isfile(config_fn)),"config file not found!"
    assert (os.path.isfile(BB_DYNSTATS_FILE)),"dynamic bb stats file not found!"
    assert (os.path.isfile(STATIC_INFO_FILE)),"static dump file file not found!"

    # Retrieve Configuration
    config = read_config(config_fn, fixed, target_func)
    # Retrieve Synamic Stats
    dynamicBbStats = read_dyn_bb_stats(BB_DYNSTATS_FILE)
    # Obtain static instruction mix from static dump
    staticStats = read_static_stats_2(STATIC_INFO_FILE, target_func)
    locMap = {}

    # Measure error
    if os.path.isfile(APPROX_OUTPUT):
        error = eval.score(PRECISE_OUTPUT,APPROX_OUTPUT)
    else:
        # Something went wrong - no output!
        logging.error("Something went wrong - missing output!")
        exit()
    stats.append(["snr", error])
    logging.info('Application SNR = {}.'.format(error))

    # Combine static and dynamic profiles to produce instruction breakdown
    totalStats = dict(bbIrCatDict)
    execCountMax = 0
    for bbIdx in dynamicBbStats:
        execCount = dynamicBbStats[bbIdx]
        execCountMax = execCount if execCount > execCountMax else execCountMax
        # if execCount>1: # Only account for non-main BBs
        if bbIdx in staticStats:
            for cat in staticStats[bbIdx]:
                totalStats[cat] += staticStats[bbIdx][cat]*execCount

    # Get the instruction breakdown from the config file
    approxStats = dict(bbIrCatDict)
    for approxInsn in config:
        for l in llvmInsnList+stdCList:
            if approxInsn['opcode'] in l['iList']:
                approxStats[l['cat']] += dynamicBbStats[approxInsn['bb']]

    # Print out instruction mix stats
    categories = ["control", "bit_arith", "int_arith", "fp_arith", "loadstore", "call", "cmath"]
    csvHeader = []
    csvRow = []
    total = 0
    for cat in categories:
        total += totalStats[cat]
    for cat in categories:
        approxCount = approxStats[cat]
        preciseCount = totalStats[cat]-approxStats[cat]
        approxPerc = approxCount/total
        precisePerc = preciseCount/total
        csvHeader.append(cat+"_a")
        csvRow.append(str(approxPerc))
        csvHeader.append(cat+"_p")
        csvRow.append(str(precisePerc))
    logging.debug('CSV Header: {}'.format(('\t').join(csvHeader)))
    logging.debug('CSV Row: {}'.format(('\t').join(csvRow)))

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
    dynFp = True if os.path.isfile(FP_DYNSTATS_FILE) else False
    dynamicFpStats = read_dyn_fp_stats(FP_DYNSTATS_FILE)
    if dynFp and len(dynamicFpStats)>1:
        dynamicFpStats = read_dyn_fp_stats(FP_DYNSTATS_FILE)
        csvHeader = ["bbId", "expRange", "mantissa", "execs"]
        csvRow = []
        for approxInsn in config:
            fpId = 'bb'+str(approxInsn['bb'])+'i'+str(approxInsn['line'])
            if fpId in dynamicFpStats:
                expRange = dynamicFpStats[fpId][1]-dynamicFpStats[fpId][0]
                mantissa = get_precision_from_type(approxInsn['type']) - approxInsn['lomask']
                execCount = dynamicBbStats[approxInsn['bb']]
                logging.debug("{}: [expRange, mantissa, execs] = [{}, {}, {}]".format(fpId, expRange, mantissa, execCount))
                csvRow.append([fpId, str(expRange), str(mantissa), str(execCount)])
        logging.debug('{}'.format(('\t').join(csvHeader)))
        for row in csvRow:
            logging.debug('{}'.format(('\t').join(row)))

        expRange = [int(x[1]) for x in csvRow]
        mantissa = [int(x[2]) for x in csvRow]
        execCount = [int(x[3])*1000.0/execCountMax for x in csvRow]

        # if plotFPScatterplot:
        #     plt.scatter(expRange, mantissa, s=execCount, alpha=0.5)
        #     plt.xlabel('Exponent Range')
        #     plt.ylabel('Mantissa Width')
        #     plt.savefig("fp.pdf", bbox_inches='tight')

        logging.info("Math Function Analysis")
        # Analyze Math Functions
        for idx, currInsn in enumerate(config):
            if currInsn['opcode'] in llvmInsnListDict["cmath"]:
                prevInsn = config[idx-1]
                currId = 'bb'+str(currInsn['bb'])+'i'+str(currInsn['line'])
                prevId = 'bb'+str(prevInsn['bb'])+'i'+str(prevInsn['line'])
                if currId in dynamicFpStats:
                    # Derive Output Range
                    outExpMin = dynamicFpStats[currId][0]-get_float_exp_offset(currInsn['type'])
                    outExpMax = dynamicFpStats[currId][1]-get_float_exp_offset(currInsn['type'])
                    outSign = dynamicFpStats[currId][2]
                    outMinVal = pow(2, outExpMin)
                    outMaxVal = pow(2, outExpMax+1)
                    if outSign==1:
                        outMinVal = 0-outMaxVal
                    # Derive absolute error threshold
                    mantissa = get_precision_from_type(currInsn['type']) - currInsn['lomask']
                    fixPrec = mantissa + 1
                    error = pow(2, outExpMax-fixPrec)/2
                    # Derive Input Range
                    # TODO: prev instruction might not be dependent instruction!
                    inExpMin = dynamicFpStats[prevId][0]-get_float_exp_offset(prevInsn['type'])
                    inExpMax = dynamicFpStats[prevId][1]-get_float_exp_offset(prevInsn['type'])
                    inSign = dynamicFpStats[prevId][2]
                    inMinVal = pow(2, inExpMin)
                    inMaxVal = pow(2, inExpMax+1)
                    if inSign==1:
                        inMinVal = 0-inMaxVal

                    logging.info("  Function {} with prev {}: [inMin, inMax, outMin, outMax, error] = [{}\t{}\t{}\t{}\t{}]".format(currInsn['opcode'], prevInsn['opcode'], \
                        inMinVal, inMaxVal, outMinVal, outMaxVal, error))

    # Keep track of the exponent min and max
    expMax = -1000
    expMin = 1000
    expRangeMax = 0
    expStats = []
    # Print the approximation setting
    logging.info("Approximate instruction statistics:")
    for approxInsn in config:
        iid = "bb" + str(approxInsn["bb"]) + "i" + str(approxInsn["line"])
        execs = dynamicBbStats[approxInsn['bb']]
        location = locMap[iid] if iid in locMap else approxInsn["opcode"]
        if execs>1:
            if is_float_type(approxInsn['type']) and iid in dynamicFpStats:
                minExp = dynamicFpStats[iid][0]
                maxExp = dynamicFpStats[iid][1]
                rangeExp = maxExp-minExp
                expMin = minExp if minExp<expMin else expMin
                expMax = maxExp if maxExp>expMax else expMax
                expRangeMax = rangeExp if rangeExp>expRangeMax else expRangeMax
                width = get_precision_from_type(approxInsn["type"])-approxInsn["lomask"]
                expStats.append([bench, iid, approxInsn["opcode"], approxInsn["type"], str(minExp), str(maxExp), str(rangeExp)])
                if fixed:
                    width += 1 #implicit mantissa bit
                logging.info("\t{}:{}:{}:{}:wi={}:lo={}:exp=[{},{}]:expRange:{}:execs={}".format(
                    location, approxInsn["opcode"], approxInsn["type"], iid,
                    width,
                    approxInsn["lomask"],
                    minExp, maxExp, rangeExp, execs
                ))
            else:
                logging.info("\t{}:{}:{}:{}:wi={}:hi={}:lo={}:execs={}".format(
                    location, approxInsn["opcode"], approxInsn["type"], iid,
                    get_precision_from_type(approxInsn["type"])-approxInsn["lomask"]-approxInsn["himask"],
                    approxInsn["himask"], approxInsn["lomask"], execs
                ))
    logging.info('Exponent min and max and range max: [{}, {}, {}]'.format(expMin, expMax, expRangeMax))
    # Dump the exponent range stats
    with open(EXPONENT_FILE, 'a') as fp:
        for insn in expStats:
            fp.write(", ".join(insn)+"\n")


    if reportEnergy:
        # Compute bit savings and energy costs and memory bandwidth savings!
        energy_ref = {}
        energy_apr = {}
        energy_ideal = 0
        memory_ref = 0
        memory_apr = {}
        # Iterate through different ALU reference widths
        alu_range = [int(math.pow(2, x)) for x in range(int(math.log(aluWidthMin, 2)), int(math.log(aluWidthMax, 2)+1))]
        for aluWidth in alu_range:
            slice_range = [int(math.pow(2, x)) for x in range(0, int(math.log(aluWidth, 2)+1))]
            energy_ref[aluWidth] = 0
            energy_apr[aluWidth] = {}
            # Iterate through different slice widths
            for sliceWidth in slice_range:
                energy_apr[aluWidth][sliceWidth] = 0
        # Iterate through different word granularities
        word_range = [int(math.pow(2, x)) for x in range(0, int(math.log(aluWidthMax, 2)+1))]
        for wordWidth in word_range:
            memory_apr[wordWidth] = 0

        # memory_ref = 0 # Float
        # memory_apx_0 = 0 # Float with lo offset (log)
        # memory_apx_1 = 0 # Float with no offset
        # memory_apx_2 = 0 # Fixed with lo offset (log)
        # memory_apx_3 = 0 # Fixed with no offset (stored in instruction stream)
        bit_savings = 0
        total_execs = 0
        for approxInsn in config:

            # Derive instruction id for referencing
            iid = "bb" + str(approxInsn["bb"]) + "i" + str(approxInsn["line"])

            # Obtain the number of executions
            execs = dynamicBbStats[approxInsn["bb"]]

            # Derive reference and approximate bit-precisions
            ref_precision = get_precision_from_type(approxInsn["type"])
            apr_precision = get_precision_from_type(approxInsn["type"])-approxInsn["lomask"]-approxInsn["himask"]
            # Adjust if fixed to float conversion for the mantissa bit and sign bit
            if fixed and is_float_type(approxInsn['type']):
                ref_precision += 1
                apr_precision += 1
            # Set cost to 1 if 0
            apr_precision = apr_precision if apr_precision else 1

            # Derive Bit Savings
            bit_savings += (ref_precision-apr_precision)/ref_precision*execs
            total_execs += execs

            # Derive Energy Cost
            if (approxInsn["opcode"] in energyMap):
                # Derive reference and approximate energy costs
                for aluWidth in energy_ref:
                    # HACK: append the ALU width to mul
                    op_cat = energyMap[approxInsn["opcode"]]
                    if op_cat=="mul":
                        op_cat = "mul_"+str(aluWidth)
                    # Reference energy cost
                    energy_ref[aluWidth] += ALUEnergy[op_cat][aluWidth][ref_precision]*execs
                    # Approximate energy cost
                    for sliceWidth in energy_apr[aluWidth]:
                        energy_apr[aluWidth][sliceWidth] += ALUEnergy[op_cat][sliceWidth][apr_precision]*execs
                # Derive ideal energy costs
                if op_cat=="add":
                    adder_width = int(math.pow(2, math.ceil(math.log(apr_precision, 2))))
                    energy_ideal += ALUEnergy["add"][adder_width][apr_precision]*execs
                elif op_cat=="mul":
                    energy_ideal += ALUEnergy["mul_pg"]["par"][apr_precision]*execs

            # Derive bandwidth utilization
            if (approxInsn["opcode"] in loadstoreInsn):
                memory_ref += get_bitwidth_from_type(approxInsn["type"])*execs
                for wordWidth in memory_apr:
                    reduced_bitwidth = get_precision_from_type(approxInsn["type"])-approxInsn["lomask"]-approxInsn["himask"]
                    if is_float_type(approxInsn['type']) and fixed: # Compensate for Mantissa
                        reduced_bitwidth += 1
                    memory_apr[wordWidth] += int(math.ceil(reduced_bitwidth/wordWidth)*wordWidth)*execs

            # Report on bit savings
            stats.append(["bit_savings", 100.0*bit_savings/total_execs])
            logging.info('Aggregate bit-savings: {0:.2f}%'.format(100.0*bit_savings/total_execs))

            # Report on energy savings
            # For all reference ALU widths
            for aluWidth in sorted(energy_ref):
                # Derive reference energy cost
                ref_cost = energy_ref[aluWidth]
                stats.append(["ref_"+str(aluWidth), ref_cost])
                # For all slice widths
                for sliceWidth in sorted(energy_apr[aluWidth]):
                    # Derive approximate energy cost
                    apr_cost = energy_apr[aluWidth][sliceWidth]
                    stats.append(["energy_"+str(aluWidth)+"_"+str(sliceWidth), apr_cost])
                    logging.info('Energy for [ALU, slice] = [{}, {}] is [precise, approx, savings] = [{}J, {}J, {}x]'.format(aluWidth, sliceWidth, ref_cost, apr_cost, ref_cost/apr_cost))
                stats.append(["energy_ideal", energy_ideal])
                logging.info('Energy for ideal ALU vs. ALU_{} is [precise, approx, savings] = [{}J, {}J, {}x]'.format(aluWidth, ref_cost, energy_ideal, ref_cost/energy_ideal))

        # Report on bandwidth savings
        stats.append(["memory_ref", memory_ref])
        for wordWidth in sorted(memory_apr):
            stats.append(["memory_"+str(wordWidth), memory_apr[wordWidth]])
            logging.info('Memory (fixed) at slice {} is [precise, approx, savings] = [{}b, {}b, {}x]'.format(wordWidth, memory_ref, memory_apr[wordWidth], memory_ref/memory_apr[wordWidth]))

    # Finally report the number of knobs
    logging.info('There are {} static safe to approximate instructions'.format(len(config)))

    # Return the stats
    return stats


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

def read_config(fname, fixed, target_func=None, bb_stats_fn=None, fp_stats_fn=None):
    """Reads in a fine error injection descriptor.
    Returns a config object.
    """
    # Filter out the instructions that don't execute dynamically
    if bb_stats_fn:
        # Read dynamic statistics
        bb_info = read_dyn_bb_stats(bb_stats_fn)

    # Read in fp exponent ranges
    if fp_stats_fn:
        fp_info = read_dyn_fp_stats(fp_stats_fn)

    # Configuration object
    config = []
    with open(fname) as f:
        for ident, param in parse_relax_config(f):
            # If this is in a function indicated in the parameters file,
            # adjust the parameter accordingly.
            if ident.startswith('instruction'):
                _, i_ident = ident.split()
                func, bb, line, opcode, typ = i_ident.split(':')
                himask, lomask, maxexp = get_masks_from_param(param)

                if (target_func and target_func==func) or not target_func:

                    # Derive the min exponent
                    maxexp = 0
                    iid = "bb" + bb + "i" + line
                    if fixed and fp_stats_fn and iid in fp_info:
                        maxexp = fp_info[iid][1]

                    # Filter the instruction if it doesn't execute more than once
                    # (This excludes code that is in the main file)
                    if not bb_stats_fn or bb_info[int(bb)]>1:
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
                            'maxexp': maxexp
                            })

    return config

def gen_default_config(instlimit, fixed, target_func, timeout):
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
    config = read_config(ACCEPT_CONFIG, fixed, target_func, BB_DYNSTATS_FILE, FP_DYNSTATS_FILE)

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
            mode = get_param_from_masks(conf['himask'], conf['lomask'], conf['maxexp'])
            f.write(str(mode)+ ' ' + conf['insn'] + '\n')
            logging.debug(str(mode)+ ' ' + conf['insn'] + ":" + str(conf['maxexp']))
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
        total += get_precision_from_type(conf['type'])
    return float(bits)/total


#################################################
# Configuration function testing
#################################################

def test_config(config, timeout, bbpath=None, fppath=None, dstpath=None):
    """Creates a temporary directory to run ACCEPT with
    the passed in config object for precision relaxation.
    """
    # Get the current working directory
    curdir = os.getcwd()
    # Get the last level directory name (the one we're in)
    dirname = os.path.basename(os.path.normpath(curdir))
    # Create a temporary directory
    # FIXME: only works on cluster (avoids full /tmp issue)
    tmpdir = tempfile.mkdtemp(dir='/scratch')+'/'+dirname
    # tmpdir = tempfile.mkdtemp()+'/'+dirname
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
        shutil.rmtree(tmpdir)
        return float(TIMEOUT)
    except:
        logging.warning('Make error!')
        print_config(config)
        shutil.rmtree(tmpdir)
        return float(CRASH)

    # Now that we're done with the compilation, evaluate results
    output_fn = os.path.join(tmpdir,APPROX_OUTPUT)
    bb_stats_fn = os.path.join(tmpdir,BB_DYNSTATS_FILE)
    fp_stats_fn = os.path.join(tmpdir,FP_DYNSTATS_FILE)
    if os.path.isfile(output_fn):
        error = eval.score(PRECISE_OUTPUT,os.path.join(tmpdir,APPROX_OUTPUT))
        logging.debug('Reported application error: {}'.format(error))
        if(dstpath):
            shutil.copyfile(output_fn, dstpath)
        if(bbpath):
            shutil.copyfile(bb_stats_fn, bbpath)
        if(fppath):
            shutil.copyfile(fp_stats_fn, fppath)
    else:
        # Something went wrong - no output!
        logging.warning('Missing output!')
        print_config(config)
        shutil.rmtree(tmpdir)
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
                fp_sign = int(elems[3])
                fp_info[fp_id] = [fp_min, fp_max, fp_sign]
    return fp_info

def process_dyn_bb_stats(config, stats_fn, BITWIDHTMAX=64):
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
        widths['baseline'] = get_precision_from_type(conf['type'])
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

    # # Now produce the CDF
    # cdfs = {}
    # for cat in categories:
    #     cat_cdf = {}
    #     for opcat in op_categories:
    #         op_cdf = {}
    #         for typ in op_types:
    #             cdf = [0]*(BITWIDHTMAX+1)
    #             op_cdf[typ] = cdf
    #         cat_cdf[opcat] = op_cdf
    #     cdfs[cat] = cat_cdf

    # # CSV file that gets outputted
    # cdf_stats = {}
    # for opcat in op_types:
    #     cdf_stats[opcat] = [["bitw", "baseline_mem", "precise_mem", "approx_mem", "baseline_exe", "precise_exe", "approx_exe"]]
    # for i in range(0, (BITWIDHTMAX+1)):
    #     for cat in categories:
    #         for opcat in op_categories:
    #             for typ in op_types:
    #                 total = sum(histograms[cat][opcat][typ])
    #                 prev = 0 if i==0 else cdfs[cat][opcat][typ][i-1]
    #                 if total>0:
    #                     cdfs[cat][opcat][typ][i] = prev+float(histograms[cat][opcat][typ][i])/total
    #     for typ in op_types:
    #         cdf_stats[typ].append([i,
    #             cdfs['baseline']['mem'][typ][i], cdfs['precise']['mem'][typ][i], cdfs['approx']['mem'][typ][i],
    #             cdfs['baseline']['exe'][typ][i], cdfs['precise']['exe'][typ][i], cdfs['approx']['exe'][typ][i]])

    # for typ in op_types:
    #     dest = cdf_fn+'_'+typ+'.csv'
    #     with open(dest, 'w') as fp:
    #         csv.writer(fp, delimiter='\t').writerows(cdf_stats[typ])

    # # Formatting
    # palette = [ '#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf']
    # cat_format = {
    #     "mem": "mem",
    #     "exe": "alu",
    #     "all": "mem+alu"
    # }
    # typ_format = {
    #     "int": "(int only)",
    #     "fp": "(fp only)",
    #     "all": ""
    # }

    # # Now plot the data!
    # f, axarr = plt.subplots(len(op_categories), len(op_types), sharex='col', sharey='row')
    # for i,opcat in enumerate(op_categories):
    #     for j,typ in enumerate(op_types):
    #         plots=[None] * len(categories)
    #         legend=[None] * len(categories)
    #         for k,cat in enumerate(categories):
    #             x = np.array(range(0, (BITWIDHTMAX+1)))
    #             y = np.array(cdfs[cat][opcat][typ])
    #             axarr[i, j].set_title(cat_format[opcat]+' '+typ_format[typ])
    #             x1,x2,y1,y2 = plt.axis()
    #             axarr[i][j].axis((x1,BITWIDHTMAX,y1,y2))

    #             plots[k], = axarr[i, j].plot(x, y, c=palette[k%len(palette)])
    #             legend[k] = cat

    #         axarr[2][j].set_xlabel("Bit-width")
    #     axarr[i][0].set_ylabel("Percentage")

    # # Plot legend
    # f.legend(plots,
    #        legend,
    #        loc='upper center',
    #        ncol=3,
    #        fontsize=8)

    # # Plot
    # plt.savefig(cdf_fn+'.pdf', bbox_inches='tight')

    return savings

def report_error_and_savings(base_config, timeout, error=0, bb_stats_fn=None, error_fn=ERROR_LOG_FILE):
    """Reports the error of the current config,
    and the savings from minimizing Bit-width.
    """
    global step_count

    if step_count==0:
        with open(error_fn, 'a') as f:
            f.write("step\terror\tmem\tmem_int\tmem_fp\texe\texe_int\texe_fp\tmath\n")

    # Collect dynamic statistics
    if not bb_stats_fn or error==0:
        bb_stats_fn = "tmp_bb_stats.txt"
        error = test_config(base_config, timeout, bb_stats_fn)
        if error==CRASH or error==TIMEOUT or error==NOOUTPUT:
            logging.info("Configuration is faulty, error set to 0 SNR")
            return
    savings = process_dyn_bb_stats(base_config, bb_stats_fn)

    # Dump the ACCEPT configuration file.
    dump_relax_config(base_config, outputsdir+'/'+ACCEPT_CONFIG_ROOT+'{0:05d}'.format(step_count)+'.txt')

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

def tune_himask_insn(base_config, idx, init_snr, timeout, double_to_single=True, snr_diff_threshold=1.0):
    """Tunes the most significant bit masking of
    an instruction given its index without affecting
    application error.
    """
    # Generate temporary configuration
    tmp_config = copy.deepcopy(base_config)

    # If int type, tune hi-mask, if float, tune lo-mask
    if is_float_type(base_config[idx]['type']):
        mask = 'lomask'
    else:
        mask = 'himask'

    # Initialize the mask and best mask variables
    bitwidth = get_precision_from_type(base_config[idx]['type'])
    mask_val = bitwidth>>1
    best_mask = 0
    # Now to the autotune part - do a log exploration
    for i in range(0, int(math.log(bitwidth, 2))):
        logging.info ("Increasing {} on instruction {} to {}".format(mask, idx, mask_val))
        # Set the mask in the temporary config
        tmp_config[idx][mask] = mask_val
        # Test the config
        error = test_config(tmp_config, timeout)
        # Check the error, and modify mask_val accordingly
        if (init_snr==0 and error==0) or (init_snr>0 and abs(init_snr-error)<snr_diff_threshold):
            logging.debug ("New best mask!")
            best_mask = mask_val
            mask_val += bitwidth>>(i+2)
        else:
            mask_val -= bitwidth>>(i+2)
    # Corner case - e.g.: bitmask=31, test 32
    if best_mask==bitwidth-1:
        mask_val = bitwidth
        logging.info ("Increasing {} on instruction {} to {}".format(mask, idx, mask_val))
        # Set the mask in the temporary config
        tmp_config[idx][mask] = bitwidth
        # Test the config
        error = test_config(tmp_config, timeout)
        if (init_snr==0 and error==0) or (init_snr>0 and abs(init_snr-error)<0.001):
            logging.debug ("New best mask!")
            best_mask = mask_val
    logging.info ("Instruction {} {} set to {}".format(idx, mask, mask_val))

    # Fast tuning for double precision FP
    if double_to_single and base_config[idx]['type']=='Double':
        double_to_single_mask = get_precision_from_type("Double")-get_precision_from_type("Float")
        best_mask = double_to_single_mask if double_to_single_mask>best_mask else best_mask

    # Return the mask value
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
    insn_masks = collections.defaultdict(list)

    def completion(jobid, output):
        with jobs_lock:
            idx = jobs.pop(jobid)
        logging.info ("Bit tuning on instruction {} done!".format(idx))
        insn_masks[idx] = output

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
        if (clusterworkers>0):
            jobid = cw.randid()
            with jobs_lock:
                jobs[jobid] = idx
            client.submit(jobid, tune_himask_insn, base_config, idx, init_snr, timeout)
        else:
            insn_masks[idx] = tune_himask_insn(base_config, idx, init_snr, timeout)

    if (clusterworkers):
        logging.info('All jobs submitted for himaks tuning')
        client.wait()
        logging.info('All jobs finished for himaks tuning')
        cw.slurm.stop()

    # Post processing
    logging.debug ("Himasks: {}".format(insn_masks))
    for idx in range(0, min(instlimit, len(base_config))):
        if (is_float_type(base_config[idx]['type'])):
            base_config[idx]['lomask'] = insn_masks[idx]
            logging.info ("Lomask of instruction {} tuned to {}".format(idx, insn_masks[idx]))
        else:
            base_config[idx]['himask'] = insn_masks[idx]
            logging.info ("Himask of instruction {} tuned to {}".format(idx, insn_masks[idx]))

    report_error_and_savings(base_config, timeout)

def tune_lomask(base_config, target_error, target_snr, init_snr, fixed, passlimit, instlimit, timeout, clusterworkers, run_on_grappa, save_output = False, sloppy=False, snr_diff_threshold=1.0):
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

            # Check if we've reached the bitwidth limit
            if (base_config[idx]['himask']+base_config[idx]['lomask']) == get_precision_from_type(base_config[idx]['type']):
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
                bb_stats_path = tmpoutputsdir+'/bb_stats_'+str(tuning_pass)+'_'+str(idx)+'.txt'
                fp_stats_path = tmpoutputsdir+'/fp_stats_'+str(tuning_pass)+'_'+str(idx)+'.txt'
                if save_output:
                    output_path = tmpoutputsdir+'/'+'out_'+str(tuning_pass)+'_'+str(idx)+EXT
                    logging.debug ("File output path of instruction {}: {}".format(tmp_config[idx]['lomask'], output_path))
                # Increment the LSB mask value
                tmp_config[idx]['lomask'] += 1
                logging.info ("Testing lomask of value {} on instruction {}".format(tmp_config[idx]['lomask'], idx))
                # Test the config
                if (clusterworkers):
                    jobid = cw.randid()
                    with jobs_lock:
                        jobs[jobid] = idx
                    if save_output:
                        client.submit(jobid, test_config, tmp_config, timeout, bb_stats_path, fp_stats_path, output_path)
                    else:
                        client.submit(jobid, test_config, tmp_config, timeout, bb_stats_path, fp_stats_path)
                else:
                    if save_output:
                        error = test_config(tmp_config, timeout, bb_stats_path, fp_stats_path, output_path)
                    else:
                        error = test_config(tmp_config, timeout, bb_stats_path, fp_stats_path)
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

        # Sloppy mode (SNR only) - all instructions which errors are close-enough (as defined
        # by snr_diff_threshold) to the best error are added to the zero_error list
        for idx in range(0, min(instlimit, len(base_config))):
            if snr_mode and sloppy:
                if abs(besterror-prev_besterror)<snr_diff_threshold:
                 zero_error.append(idx)


        # Apply LSB masking to the instruction that are not impacted by it
        logging.debug ("Zero-error instruction list: {}".format(zero_error))
        for idx in zero_error:
            base_config[idx]['lomask'] += 1
            logging.info ("Increasing lomask on instruction {} to {} (no additional error)".format(idx, base_config[idx]['lomask']))
        # Report best error achieved
        if snr_mode:
            logging.debug ("[besterror, target_snr] = [{}, {}]".format(besterror, target_snr))
        else:
            logging.debug ("[besterror, target_error] = [{}, {}]".format(besterror, target_error))
        # Apply LSB masking to the instruction that minimizes positive error
        if (not snr_mode and besterror <= target_error) or (snr_mode and besterror >= target_snr):
            base_config[bestidx]['lomask'] += 1
            prev_besterror = besterror
            logging.info ("Increasing lomask on instruction {} to {} (best)".format(bestidx, base_config[bestidx]['lomask']))
            if save_output:
                # Copy file output
                src_path = tmpoutputsdir+'/out_'+str(tuning_pass)+'_'+str(bestidx)+EXT
                dst_path = outputsdir+'/out_{0:05d}'.format(step_count)+EXT
                shutil.copyfile(src_path, dst_path)
            # Report Error and Savings stats
            bb_stats_path = tmpoutputsdir+'/bb_stats_'+str(tuning_pass)+'_'+str(bestidx)+'.txt'
            report_error_and_savings(base_config, timeout, besterror, bb_stats_path)
            # Update expmin if fixed-point emulation is enabled
            if fixed:
                fp_stats_path = tmpoutputsdir+'/fp_stats_'+str(tuning_pass)+'_'+str(bestidx)+'.txt'
                fp_stats = read_dyn_fp_stats(fp_stats_path)
                for conf in base_config:
                    iid = "bb" + str(conf["bb"]) + "i" + str(conf["line"])
                    if iid in fp_stats:
                        conf["maxexp"] = fp_stats[iid][1]

        logging.info ("Bit tuning pass #{} done!\n".format(tuning_pass))

        # Termination conditions:
        # 1 - min error exceeds target_error / max snr is below target snr
        # 2 - empty zero_error
        if ((not snr_mode and besterror>target_error) or (snr_mode and besterror<target_snr)) and (not zero_error):
            report_error_and_savings(base_config, timeout)
            break

    if(clusterworkers):
        cw.slurm.stop()


#################################################
# Post Processing
#################################################

def postProcess(fn, dn, snr, target_func, fixed):

    # Results
    results = []

    # Check if source path was provided
    if dn and os.path.isdir(dn):
        if snr==0:
            configList = [[x, getConfFile(dn, x)] for x in range(10, 121, 10)+[200]]
        else:
            configList = [[snr, getConfFile(dn, snr)]]
    else:
        configList = [[snr, fn]]

    for c in configList:
        conf_snr = c[0]
        conf_fn = c[1]
        res = analyzeConfigStats(conf_fn, target_func, fixed)
        results.append([["target", conf_snr]] + res)

    # Dump CSV data
    csvData = [[x[0] for x in results[0]]]
    for r in results:
        csvData.append([x[1] for x in r])

    # with open(CSV_FILE, 'w') as fp:
    #    csv.writer(fp, delimiter='\t').writerows(csvData)


def getConfFile(path, snr):
    ''' Searches the path and returns configuration file that meets the provided SNR target.
    If the SNR target is 0, returns the list of files that meet dB target from 10dB to 120dB.
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
    paretoCsv = [csv[0]]
    for i, elem in enumerate(csv[1:]):
        paretoOptimal = True
        for j in range(i+1,len(csv)):
            if float(csv[j][errorIdx]) > float(elem[errorIdx]):
                paretoOptimal = False
        if paretoOptimal:
            paretoCsv.append(elem)
    csv = paretoCsv

    # Optimal index given SNR target
    idx = 0
    for i, elem in enumerate(csv[1:]):
        if float(elem[errorIdx]) >= snr:
            idx = int(elem[0])

    confFile = ACCEPT_CONFIG_ROOT+'{0:05d}'.format(idx)+'.txt'
    if idx==0:
        logging.info("No configuration with SNR = {} was found".format(snr))
    else:
        logging.info("Found {}".format(confFile))
    return path+'/'+confFile


#################################################
# Main Function(s)
#################################################

def runExperiments(runs, instlimit, target_func, timeout, clusterworkers, run_on_grappa):

    logging.info ("###################################")
    logging.info ("Running error injection experiments")
    logging.info ("###################################")

    # Extract the configuration file
    config = gen_default_config(instlimit, False, target_func, timeout)
    print_config(config)

    # Map job IDs to instruction index
    jobs = {}
    jobs_lock = threading.Lock()

    # Map instructions to errors
    error_runs = collections.defaultdict(list)

    # Modify the configuration file to enable approximations
    for c in config:
        c["lomask"] = 1
    print_config(config)

    def completion(jobid, output):
        with jobs_lock:
            run = jobs.pop(jobid)
        logging.info ("Experiment {} done!".format(run))
        error_runs[run] = output

    # Launch clusterworkers if needed
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

    # Now run the benchmark!
    logging.info('Running {} error injection experiments'.format(runs))
    for run in range(runs):
        if (clusterworkers):
            jobid = cw.randid()
            with jobs_lock:
                jobs[jobid] = run
            client.submit(jobid, test_config, config, timeout)
        else:
            error = test_config(config, timeout)
            error_runs[run] = error

    # Wait for workers to be done
    if (clusterworkers):
        logging.info('All jobs submitted for pass #{}'.format(tuning_pass))
        client.wait()
        logging.info('All jobs finished for pass #{}'.format(tuning_pass))

    # Process errors
    errors = error_runs.values()
    logging.info('min: {}, mean: {}, max: {}'.format(min(errors), np.median(np.array(errors)), max(errors)))
    with open(ERROR_FILE, 'w') as fp:
        fp.write('min, mean, max\n')
        fp.write('{}, {}, {}\n'.format(min(errors), np.median(np.array(errors)), max(errors)))
        fp.write('{}\n'.format(', '.join([str(x) for x in errors])))


def tune_width(accept_config_fn, target_error, target_snr, target_func, fixed, passlimit, instlimit, skip, timeout, clusterworkers, run_on_grappa):
    """Performs instruction masking tuning
    """
    # Generate default configuration
    if (accept_config_fn):
        config = read_config(accept_config_fn, fixed, target_func)
        print_config(config)
    else:
        config = gen_default_config(instlimit, fixed, target_func, timeout)
        print_config(config)

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
            fixed,
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
        '-runs', dest='accept_config_dir', action='store', type=str, required=False,
        default='outputs', help='directory containing experimental results'
    )
    parser.add_argument(
        '-t', dest='target_error', action='store', type=float, required=False,
        default=0.1, help='target relative application error'
    )
    parser.add_argument(
        '-target', dest='target_func', action='store', type=str, required=False,
        default=None, help='target function to approximate'
    )
    parser.add_argument(
        '-snr', dest='target_snr', action='store', type=float, required=False,
        default=0, help='target signal to noise ratio (if set, error is ignored)'
    )
    parser.add_argument(
        '-fixed', dest='fixed', action='store_true', required=False,
        default=False, help='if set, emulates fixed-point behavior'
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
        '-skip', dest='skip', action='store', type=str, required=False,
        default=None, help='skip a particular phase'
    )
    parser.add_argument(
        '-timeout', dest='timeout', action='store', type=int, required=False,
        default=600, help='timeout of an experiment in minutes'
    )
    parser.add_argument(
        '-stats', dest='statsOnly', action='store_true', required=False,
        default=False, help='produce instruction breakdown'
    )
    parser.add_argument(
        '-run', dest='runs', action='store', type=int, required=False,
        default=0, help='run n times'
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

    # Print to console
    consoleHandler = logging.StreamHandler()
    consoleHandler.setFormatter(logFormatter)
    rootLogger.addHandler(consoleHandler)

    if(args.debug):
        rootLogger.setLevel(logging.DEBUG)
    else:
        rootLogger.setLevel(logging.INFO)

    # If we want to derive stats only
    if args.statsOnly:
        postProcess(
            args.accept_config_fn,
            args.accept_config_dir,
            args.target_snr,
            args.target_func,
            args.fixed
        )

        # Close the log handlers
        handlers = rootLogger.handlers[:]
        for handler in rootLogger.handlers[:]:
            handler.close()
            rootLogger.removeHandler(handler)

    # Run n times (for voltage overscaling experiments for instance):
    elif args.runs>0:
        runExperiments(
            args.runs,
            args.instlimit,
            args.target_func,
            args.timeout,
            args.clusterworkers,
            args.grappa
        )

    # Precision autotuning
    else:
        # Open log handler
        open(args.logpath, 'a').close()
        fileHandler = logging.FileHandler(args.logpath)
        fileHandler.setFormatter(logFormatter)
        rootLogger.addHandler(fileHandler)

        # Create a temporary output directory
        create_overwrite_directory(outputsdir)
        logging.debug('Output directory created: {}'.format(outputsdir))

        # Tuning
        tune_width(
            args.accept_config_fn,
            args.target_error,
            args.target_snr,
            args.target_func,
            args.fixed,
            args.passlimit,
            args.instlimit,
            args.skip,
            args.timeout,
            args.clusterworkers,
            args.grappa)

        # Close the log handlers
        handlers = rootLogger.handlers[:]
        for handler in rootLogger.handlers[:]:
            handler.close()
            rootLogger.removeHandler(handler)

        # Finally, transfer all files in the outputs dir
        shutil.move(outputsdir, os.getcwd()+'/'+OUTPUT_DIR)
        if (os.path.isdir(OUTPUT_DIR)):
            if os.path.exists(ACCEPT_CONFIG):
                shutil.move(ACCEPT_CONFIG, OUTPUT_DIR+'/'+ACCEPT_CONFIG)
            if os.path.exists(ERROR_LOG_FILE):
                shutil.move(ERROR_LOG_FILE, OUTPUT_DIR+'/'+ERROR_LOG_FILE)
            shutil.move(args.logpath, OUTPUT_DIR+'/'+args.logpath)

        # Cleanup
        if (os.path.isdir(tmpoutputsdir)):
            shutil.rmtree(tmpoutputsdir)

if __name__ == '__main__':
    cli()
