import copy
import os
import shlex
import subprocess
import sys
import numpy as np

import tune_precision as tp
sys.path.insert(0, '/Users/moreau/Documents/research/summer_research/MultivariateNumericalAccuracyPython')
import MathFunctionFitSpaceDivision as mfit

# LLVM instruction categories
# We ignore phi nodes since those are required
# because of the SSA style of the LLVM IR
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

insnList = {
    "control": controlInsn,
    "terminator": terminatorInsn,
    "binary": binaryInsn,
    "vector": vectorInsn,
    "aggregate": vectorInsn,
    "loadstore": loadstoreInsn,
    "getelement": getelementptrInsn,
    "memory": memoryInsn,
    "conversion": conversionInsn,
    "comparison": cmpInsn,
    "call": callInsn,
    "phiInsn": phiInsn,
    "otherInsn": otherInsn
}

insnCounter = {
    "control": 0,
    "terminator": 0,
    "binary": 0,
    "vector": 0,
    "aggregate": 0,
    "loadstore": 0,
    "getelement": 0,
    "memory": 0,
    "conversion": 0,
    "comparison": 0,
    "call": 0,
    "phiInsn": 0,
    "otherInsn": 0
}

opList = {
    "fadd": ['fadd','fsub'],
    "fmul": ['fmul','fdiv','frem'],
    "add": ['add','sub'],
    "mul": ['mul','udiv','sdiv','urem'],
    "bin": ['shl','lshr','ashr','and','or','xor'],
    "load": ['load'],
    "store": ['store'],
    "call": ['call'],
    "ignore": conversionInsn+phiInsn
}

opCounter = {
    "fadd": 0,
    "fmul": 0,
    "add": 0,
    "mul": 0,
    "bin": 0,
    "load": 0,
    "store": 0,
    "call": 0,
    "ignore": 0,
    "other": 0
}

costTable = {
    "fadd": {
        "half": 8.2662711E-12,
        "float": 1.013423E-11,
        "double": 2.9754661E-11
    },
    "fmul": {
        "half": 9.9738855E-12,
        "float": 1.2822082E-11,
        "double": 6.5154459E-11
    }
}

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

def isInt(reg):
    try:
        int(reg)
        return True
    except:
        return False

def isFloat(reg):
    try:
        float(reg)
        return True
    except:
        return False

def isConstant(reg):
    if reg=="null":
        return True
    elif isInt(reg):
        return True
    elif isFloat(reg):
        return True
    else:
        return False

def isApprox(i):
    if i["qual"]=="approx":
        return True
    else:
        return False

def getBBPredecessors(node, branches):
    predecessors = []
    for bb in branches:
        for succ in branches[bb]["tgt"]:
            if succ==branches[node]["src"]:
                predecessors.append(branches[bb]["id"])
    return predecessors

def hasPredecessor(i_key, insns):
    if len(getPredecessors(i_key, insns)):
        return True
    else:
        return False

def hasSuccessor(i_key, insns):
    if len(getSuccessors(i_key, insns)):
        return True
    else:
        return False

def replaceAllUsesOfWith(before, after, insns):
    for insn in insns.itervalues():
        for idx, src in enumerate(insn["src"]):
            if src==before:
                insn["src"][idx] = after

def getPredecessors(i_key, insns):
    predecessors = []
    for src in insns[i_key]["src"]:
        if src in insns:
            predecessors.append(src)
    return predecessors

def getSuccessors(i_key, insns):
    successors = []
    for insn in insns.itervalues():
        for src in insn["src"]:
            if src==i_key:
                successors.append(insn["dst"])
    return successors

def getEntryInstructions(insns):
    entryInsns = []
    for i_key in insns:
        if not hasPredecessor(i_key, insns):
            entryInsns.append(i_key)
    return entryInsns

def getExitInstructions(insns):
    exitInsns = []
    for i_key in insns:
        if not hasSuccessor(i_key, insns):
            exitInsns.append(i_key)
    return exitInsns

def getInsnFromBB(bbid, insns):
    # Excludes phi nodes!
    insnList = []
    for i_key in insns:
        if insns[i_key]["bbid"]==bbid:
            insnList.append(i_key)
    return insnList

def getInsnfromReg(reg, insns):
    for i_key in insns:
        if insns[i_key]["dst"]==reg:
            return i_key
    return ""

def getSrcFromPhi(i_key, insns):
    assert len(insns[i_key]["src"])==1
    return insns[i_key]["src"][0]

def cleanupPhiNodes(dddg):
    # Delete list
    delList = []
    # Look for phi nodes
    for dst in dddg:
        insn = dddg[dst]
        if insn["op"]=="phi":
            src = getSrcFromPhi(dst, dddg)
            replaceAllUsesOfWith(dst, src, dddg)
            delList.append(dst)
    # Remove all phi nodes
    for d in delList:
        del dddg[d]

def insertDotEdge(fp, src, dst, label=""):
    fp.write("\t\"{}\"->\"{}\"\n".format(src, dst))
    if label!="":
        fp.write("\t[ label=\"{}\" ]\n".format(label))

def getLabel(insn):
    label="("+insn["bbid"]+") "
    if insn["dst"]:
        label += insn["dst"] + " = "
    label += insn["op"] + " "
    if insn["op"] == "call":
        label += insn["fn"] + "("
    if insn["op"] == "phi":
        src = ["[" + x[0] + ", " + x[1] + "]" for x in zip(insn["src"], insn["srcBB"])]
        label += ", ".join(src)
    else:
        label += ", ".join(insn["src"])
    if insn["op"] == "call":
        label += ")"
    if "addr" in insn:
        label += insn["addr"]
    return label

def labelDotNode(fp, i_key, insns):
    insn = insns[i_key]
    # print "{}, {}".format(i_key, insn)
    # Derive the node label
    label = getLabel(insn)
    # Derive the node color
    color = "forestgreen" if isApprox(insn) else "black"
    # Derive the node shape
    shape = "box"
    if insn["op"]=="phi":
        shape = "diamond"
    elif insn["op"]=="store":
        shape = "triangle"
    elif insn["op"]=="load":
        shape = "invtriangle"
    elif insn["op"]=="call":
        shape = "doubleoctagon"
    # Add the dot edge
    fp.write("\t\"{}\" [ ".format(i_key))
    fp.write("label=\"{}\" ".format(label))
    fp.write("color=\"{}\" ".format(color))
    fp.write("shape=\"{}\" ".format(shape))
    fp.write("style=\"rounded\"]\n")

def trimStep(insns):
    trimList = []
    for i_key, insn in insns.iteritems():
        if not hasApproxPredecessor(i_key, insns) and not hasApproxSuccessor(i_key, insns):
            trimList.append(i_key)
            trimmed = True
        elif insn["op"] != "store" and insn["op"] != "ret":
            if len(getSuccessors(i_key, insns))==0:
                trimList.append(i_key)
                trimmed = True
    if len(trimList):
        for i_key in trimList:
            del insns[i_key]
        return True
    else:
        return False

def trim(insns):
    # Trim the graph
    epoch = 0
    while(trimStep(insns)):
        epoch+=1
    print "Trimmed the graph in {} steps".format(epoch)

def hasApproxPredecessor(i_key, insns):
    visited = []
    predecessors = [i_key]

    while len(predecessors):
        new_predecessors = []
        for p_key in predecessors:
            # Visit the node
            visited.append(p_key)
            # Check if it is approx
            if isApprox(insns[p_key]):
                return True
            # Prepare the new predecessors list
            for pp_key in getPredecessors(p_key, insns):
                if pp_key not in visited and pp_key not in new_predecessors:
                    new_predecessors.append(pp_key)
        predecessors = new_predecessors
    return False

def hasApproxSuccessor(i_key, insns):
    visited = []
    successors = [i_key]

    while len(successors):
        new_successors = []
        for s_key in successors:
            # Visit the node
            visited.append(s_key)
            # Check if it is approx
            if isApprox(insns[s_key]):
                return True
            # Prepare the new successors list
            for ss_key in getSuccessors(s_key, insns):
                if ss_key not in visited and ss_key not in new_successors:
                    new_successors.append(ss_key)
        successors = new_successors
    return False

def processBBExecs(fn):
    # Process bbstats file line by line
    bbExecs = {}
    if (os.path.isfile(fn)):
        with open(fn) as fp:
            for line in fp:
                # Tokenize
                tokens = line.strip().split("\t")
                if tokens[0]!="BB":
                    bbExecs["bb"+tokens[0]] = int(tokens[1])
    return bbExecs

def processBranchOutcomes(fn, insn, branches):

    phiPrevDict = {}
    branchOutcomes = {}
    # outgoingStat = {}
    # incomingStat = {}

    print "Processing branch outcomes"
    if (os.path.isfile(fn)):
        with open(fn) as fp:
            for line in fp:
                # Tokenize
                tokens = line.strip().split(",")
                srcBB = tokens[0]
                outcome = int(tokens[1])
                numOutcomes = int(tokens[2])
                dstBB =  branches[srcBB]["tgt"][outcome]
                # if not srcBB in outgoingStat:
                #     outgoingStat[srcBB] = {}
                # if not dstBB in outgoingStat[srcBB]:
                #     outgoingStat[srcBB][dstBB] = 0
                # outgoingStat[srcBB][dstBB] += numOutcomes
                # if not dstBB in incomingStat:
                #     incomingStat[dstBB] = {}
                # if not srcBB in incomingStat[dstBB]:
                #     incomingStat[dstBB][srcBB] = 0
                # incomingStat[dstBB][srcBB] += numOutcomes

                # Log branch outcomes
                if not srcBB in branchOutcomes:
                    branchOutcomes[srcBB] = []
                branchOutcomes[srcBB].append([dstBB, numOutcomes])

    # print "Incoming Branch Statistics"
    # print incomingStat
    # print "Outgoing Branch Statistics"
    # print outgoingStat

    return branchOutcomes


def processBranches(fn):
    branches = {}
    # Process static dump line by line
    if (os.path.isfile(fn)):
        with open(fn) as fp:
            for line in fp:
                # Tokenize
                tokens = line.strip().split(", ")
                fn = tokens[0]
                op = tokens[1]
                # Check for op type
                if op == "br" and fn=="conv2d":
                    iid = tokens[2]
                    src = tokens[3].split("->")[0]
                    dst = tokens[3].split("->")[1]
                    if src in branches:
                        branches[src]["tgt"].append(dst)
                    else:
                        params = {}
                        params["fn"] = fn
                        params["op"] = op
                        params["id"] = iid
                        params["src"] = src
                        params["tgt"] = [dst]
                        branches[src] = params

    return branches

def processInsn(fn, filter):
    # List of unknown opcodes
    unknown = []
    # List of instructions
    insns = {}
    # Process static dump line by line
    if (os.path.isfile(fn)):
        with open(fn) as fp:
            for line in fp:
                # Skip instruction
                skip = False
                # Tokenize
                tokens = line.strip().split(", ")
                # Get the function name
                fn_name = tokens[0]
                # Get the op type
                opcode = tokens[1]
                # Process token
                params = {}
                params["fn"] = fn_name
                params["op"] = opcode
                params["id"] = tokens[2]
                params["bbid"] = params["id"].split("i")[0]
                params["qual"] = tokens[3]
                # If return instruction
                if opcode == "ret":
                    params["ty"] = tokens[4]
                    params["dst"] = None
                    params["src"] = [tokens[5]]
                # If call instruction
                elif opcode in callInsn:
                    params["fn"] = tokens[4]
                    params["dstty"] = tokens[5]
                    params["dst"] = None
                    params["srcty"] = []
                    params["src"] = []
                    idx = 6
                    if params["dstty"]!="void":
                        params["dst"] = tokens[idx]
                        idx+=1
                    for i in range(int(tokens[idx])):
                        token_idx = idx+1+i*2
                        params["srcty"].append(tokens[token_idx+0])
                        params["src"].append(tokens[token_idx+1])
                # If phi node
                elif opcode in phiInsn:
                    params["ty"] = tokens[4]
                    params["dst"] = tokens[5]
                    params["src"] = []
                    params["srcBB"] = []
                    for i in range(int(tokens[6])):
                        params["src"].append(tokens[7+2*i+0])
                        params["srcBB"].append(tokens[7+2*i+1])
                # If load instruction
                elif opcode == "load":
                    params["ty"] = tokens[4]
                    params["dst"] = tokens[5]
                    params["src"] = []
                    params["addr"] = tokens[6]
                    params["addrIdx"] = 0
                # If store instruction
                elif opcode == "store":
                    params["ty"] = tokens[4]
                    params["src"] = [tokens[5]]
                    params["dst"] = tokens[6] # HACK
                    params["addr"] = tokens[6]
                # If cast instruction
                elif opcode in conversionInsn:
                    params["dstty"] = tokens[4]
                    params["srcty"] = tokens[5]
                    params["dst"] = tokens[6]
                    params["src"] = [tokens[7]]
                # If binary or comparison instruction
                elif opcode in binaryInsn or opcode in cmpInsn:
                    params["ty"] = tokens[4]
                    params["dst"] = tokens[5]
                    params["src"] = [tokens[6], tokens[7]]
                # Skip branches
                elif opcode=="br":
                    # skip
                    skip = True
                else:
                    if opcode not in unknown:
                        unknown.append(opcode)
                        print "Don't know how to handle {} opcode!".format(opcode)
                    skip = True

                if filter!=fn_name:
                    skip = True

                if not skip and params["dst"]:
                    insns[params["dst"]] = params
    else:
        print "file not found!"

    # Cleanup instructions
    trim(insns)

    return insns

def CFG(branches, fn):
    # Dumpt DOT description of CFG
    with open(fn, "w") as fp:
        fp.write("digraph graphname {\n")
        for iid in branches:
            br = branches[iid]
            for idx, tgt in enumerate(br["tgt"]):
                if "outcome" in br:
                    if br["outcome"][idx]>0:
                        label = ""
                        if br["outcome"][idx]!=br["total"]:
                            label = str(br["outcome"][idx])+"/"+str(br["total"])
                        insertDotEdge(fp, br["src"], tgt, label)
                else:
                    insertDotEdge(fp, br["src"], tgt)
        fp.write("}")

def DFG(insns, fn):
    # Dump the DOT description of DFG
    with open(fn, "w") as fp:
        fp.write("digraph graphname {\n")
        # Print all instructions
        for i_key, insn in insns.iteritems():
            for s_key in getSuccessors(i_key, insns):
                # Insert edge
                insertDotEdge(fp, i_key, s_key)
                labelDotNode(fp, i_key, insns)
            # Corner case: store instructions don't have successors
            if insn["op"]=="store":
                labelDotNode(fp, i_key, insns)
            # Other corner case: terminators
            elif insn["op"]=="ret":
                labelDotNode(fp, i_key, insns)

        fp.write("}")

def DDDG(insns, branchOutcomes, limit=10000):

    # Get exit instructions
    exitInsns = getExitInstructions(insns)

    # Replay the program based on CFG outcomes
    DDDG = {}       # DDDG
    currBB = "bb0"  # Programs always start at bb0
    nextBB = ""     # Next basic block to visit
    prevBB = ""     # Previous basic block visited
    cntr = 0        # Counter to limit the size of the DDDG
    done = False    # Indicates that we have reached an exit
    bbIdx = 0       # Basic block index used in the traversal
    bbIdxMap = {}   # Map of the last index for a given BB

    # Replay the program based on control flow trace
    while currBB!="" and cntr<=limit:

        # Multiple possible branch outcomes
        if len(branches[currBB]["tgt"])>1:
            if len(branchOutcomes[currBB])>0:
                nextBB = branchOutcomes[currBB][0][0]
                branchOutcomes[currBB][0][1]-=1
                if branchOutcomes[currBB][0][1]==0:
                    branchOutcomes[currBB].pop(0)
            else:
                nextBB = ""
        # Single branch outcomes
        elif len(branches[currBB]["tgt"])==1:
            nextBB = branches[currBB]["tgt"][0]
        # No branch outcome (reached the exit)
        else:
            nextBB = ""

        print "{}->{}, prev={}".format(currBB, nextBB, prevBB)

        # Get all instructions from the current basic block
        for i_key in getInsnFromBB(currBB, insns):
            insn = copy.deepcopy(insns[i_key])
            # Modify the id and registers
            insn["id"]+="_"+str(bbIdx)
            if insn["dst"]:
                insn["dst"]+="_"+str(bbIdx)
            # If the instruction is a phi node, discard non-valid sources
            if insn["op"]=="phi":
                # Set the srcBB and src register to match dynamic execution
                for srcReg, srcBB in zip(insns[i_key]["src"], insns[i_key]["srcBB"]):
                    if srcBB==prevBB:
                        insn["srcBB"]=[prevBB]
                        # Check if we are dealing with a constant
                        if isConstant(srcReg):
                            # No need to derive producer instruction
                            insn["src"]=[srcReg]
                        else:
                            # Determine bbid of producer instruction
                            srcRegBB = insns[getInsnfromReg(srcReg, insns)]["bbid"]
                            insn["src"]=[srcReg+"_"+str(bbIdxMap[srcRegBB])]
            else:
                for idx, srcReg in enumerate(insn["src"]):
                    insn["src"][idx]+="_"+str(bbIdx)
            # Special case: if the op is a load, index the access
            if insn["op"]=="load":
                insns[i_key]["addrIdx"] += 1
                insn["addrIdx"] = insns[i_key]["addrIdx"]
            print "\t {}".format(getLabel(insn))
            DDDG[insn["dst"]] = insn
            cntr += 1

        # Check that we have reached a exit instruction
        for i_key in exitInsns:
            if insns[i_key]["bbid"] == currBB:
                done = True

        bbIdxMap[currBB] = bbIdx
        bbIdx += 1
        prevBB = currBB
        currBB = nextBB

        if done:
            break

    # Eliminate phi nodes
    cleanupPhiNodes(DDDG)

    return DDDG

def getBitWidth(iid, configMap, signed=True):
    # Update energy cost
    bitWidth = tp.get_precision_from_type(configMap[iid]["type"])
    bitWidth -= (configMap[iid]["himask"]+configMap[iid]["lomask"])
    # Add sign and implicit bits
    if tp.is_float_type(configMap[iid]["type"]):
        bitWidth += 1
        if signed:
            bitWidth += 1
    return bitWidth

def inversek2j_0(x, y):
    l1 = 0.5
    l2 = 0.5
    denom = x * x + y * y
    theta2 = np.arccos((denom - 0.5)*2)
    return np.array(theta2)

def inversek2j_1(x, y):
    l1 = 0.5
    l2 = 0.5
    denom = x * x + y * y
    theta2 = np.arccos((denom - 0.5)*2)
    num = y * (l1 + l2 * np.cos(theta2)) - x * l2 * np.sin(theta2);
    theta1 = np.arcsin( num / denom )
    return np.array(theta1)

def CNDF(InputX):

    if InputX < 0.0:
        InputX = -InputX
        sign = 1
    else:
        sign = 0

    xInput = InputX

    expValues = np.exp(-0.5 * InputX * InputX)
    xNPrimeofX = expValues
    xNPrimeofX = xNPrimeofX * 0.39894228040143270286

    xK2 = 0.2316419 * xInput
    xK2 = 1.0 + xK2
    xK2 = 1.0 / xK2
    xK2_2 = xK2 * xK2
    xK2_3 = xK2_2 * xK2
    xK2_4 = xK2_3 * xK2
    xK2_5 = xK2_4 * xK2

    xLocal_1 = xK2 * 0.319381530
    xLocal_2 = xK2_2 * -0.356563782
    xLocal_3 = xK2_3 * 1.781477937
    xLocal_2 = xLocal_2 + xLocal_3
    xLocal_3 = xK2_4 * -1.821255978
    xLocal_2 = xLocal_2 + xLocal_3
    xLocal_3 = xK2_5 * 1.330274429
    xLocal_2 = xLocal_2 + xLocal_3

    xLocal_1 = xLocal_2 + xLocal_1
    xLocal   = xLocal_1 * xNPrimeofX
    xLocal   = 1.0 - xLocal

    OutputX  = xLocal

    if sign:
        OutputX = 1.0 - OutputX

    return OutputX


def functionInversek2j_0(x):
    return inversek2j_0(x[0], x[1])
def functionInversek2j_1(x):
    return inversek2j_1(x[0], x[1])
def functionCNDF(x):
    vCNDF = np.vectorize(CNDF)
    return np.array(vCNDF(x[0]))
def functionAcos(x):
    return np.arccos(x[0])
def functionAsin(x):
    return np.arcsin(x[0])
def functionCos(x):
    return np.cos(x[0])
def functionSin(x):
    return np.sin(x[0])
def functionExp(x):
    return np.exp(x[0])
def functionLog(x):
    return np.log(x[0])
def functionSqrt(x):
    return np.sqrt(x[0])
def functionRec(x):
    return np.reciprocal(x[0])

functionList = {
    "CNDF": {
        "func": functionCNDF
    },
    "inversek2j": {
        "func": [functionInversek2j_0, functionInversek2j_1],
        "min": [0,0],
        "max": [0.86, 0.86]
    },
    "acos": {
        "func": functionAcos,
        "min": -1,
        "max": 1
    },
    "asin": {
        "func": functionAsin,
        "min": -1,
        "max": 1
    },
    "cos": {
        "func": functionCos
    },
    "cosf": {
        "func": functionCos
    },
    "sin": {
        "func": functionSin
    },
    "sinf": {
        "func": functionSin
    },
    "log": {
        "func": functionLog,
        "min": 1E-9
    },
    "exp": {
        "func": functionExp
    },
    "sqrtf": {
        "func": functionSqrt,
        "min": 0
    },
    "rec": {
        "func": functionRec,
        "min": 1E-9
    }
}

def deriveCosts(insns, conf, coarseTarget):

    # v = [0, 0.5, -0.5]

    # print functionCNDF([v])
    # print functionAcos([v])

    # exit()

    opStats = dict(opCounter)

    # Cost Model
    energy = 0
    sram = 0
    sram_opt = 0
    sramAccesses = 0

    # Math function approximation requirements
    fineFunc = []
    # Coarse-grained approximation requirements
    coarseFunc = {
        "iid": [],
        "input_range": [],
        "input_res": [],
        "output": []
    }

    configMap = {}
    for c in conf:
        iid = "bb"+str(c["bb"])+"i"+str(c["line"])
        configMap[iid] = c

    for i_key in insns:
        insn = insns[i_key]
        opcode = insn["op"]
        iid = insn["id"]

        # Estimate the costs
        if opcode in ['fmul','fdiv','frem','mul','udiv','sdiv','urem']:
            energy += ALUEnergy["mul_8"][8][getBitWidth(iid, configMap)]
        elif opcode in ['fadd','fsub','add','sub']:
            energy += ALUEnergy["add"][8][getBitWidth(iid, configMap)]

        # Test for math functions
        if opcode=="call" or opcode=="fdiv":
            # Obtain callee
            callee = "rec" if opcode=="fdiv" else insn["fn"]
            # Derive output resolution
            # FIXME: this might not necessarily be true for reciprocal
            output_bw = getBitWidth(iid, configMap)
            # Get input variables to func
            # FIXME: only works on 1-input argument functions
            if opcode=="fdiv":
                # Obtain the denominator
                pred = insn["src"][1]
            else:
                assert len(insn["src"])==1
                pred = insn["src"][0]
            while insns[pred]["op"] in opList["ignore"]:
                pred_i_key = pred
                pred = insns[pred_i_key]["src"][0]
            # Derive Predecessor instruction id
            pred_iid = insns[pred]["id"]
            # Get input variable precision
            input_bw = getBitWidth(pred_iid, configMap, signed=False)
            # Get exponent bounds
            expMax = configMap[pred_iid]["maxexp"]-tp.get_exponent_offset_from_type(configMap[pred_iid]["type"])
            expMin = configMap[pred_iid]["minexp"]-tp.get_exponent_offset_from_type(configMap[pred_iid]["type"])
            # Derive input range
            input_res = pow(2, expMax+1-input_bw)
            rangeMin = max([pow(2, expMin), input_res])
            rangeMax = pow(2, expMax+1)
            # Derive output error
            if opcode=="fdiv":
                outRangeMax = float(1)/rangeMin
                output_error = outRangeMax/(pow(2, output_bw+1))
            else:
                output_error = float(rangeMax)/(pow(2, output_bw+1))
            # Function
            func = {
                "iid": iid,
                "target": callee,
                "input_bw": input_bw,
                "output_bw": output_bw,
                "input_res": input_res,
                "maxexp": expMax,
                "minexp": expMin,
                "input_range": [rangeMin, rangeMax],
                "output_error": output_error
            }
            print func
            fineFunc.append(func)

        # Coarse grained requirements
        if opcode=="load":
            input_bw = getBitWidth(iid, configMap, signed=False)
            expMax = configMap[iid]["maxexp"]-tp.get_exponent_offset_from_type(configMap[iid]["type"])
            expMin = configMap[iid]["minexp"]-tp.get_exponent_offset_from_type(configMap[iid]["type"])
            # Derive input range
            input_res = pow(2, expMax+1-input_bw)
            rangeMin = max([pow(2, expMin), input_res])
            rangeMax = pow(2, expMax+1)
            # Function input params
            coarseFunc["iid"].append(iid)
            coarseFunc["input_range"].append([rangeMin, rangeMax])
            coarseFunc["input_res"].append(input_res)
        if opcode=="store":
            output_bw = getBitWidth(iid, configMap)
            expMax = configMap[iid]["maxexp"]-tp.get_exponent_offset_from_type(configMap[iid]["type"])
            # Derive range
            rangeMax = pow(2, expMax+1)
            # Function output params
            coarseFunc["output"].append({
                "iid": iid,
                "error": float(rangeMax)/(pow(2, output_bw+1))
            })

        # Derive categories
        found = False
        for cat, iList in opList.iteritems():
            if opcode in iList:
                opStats[cat] += 1
                found = True
        if not found:
            print "Opcode not accounted for: {}".format(opcode)
            opStats["other"] += 1

    # Report opcode breakdown
    print "Opcode breakdown: {}".format(opStats)

    # # Energy w/o PWP
    # print "Default Energy: {}pJ".format(energy)

    # for func in fineFunc:
    #     error = func["output_error"]
    #     input_res = func["input_res"]
    #     input_range = func["input_range"]
    #     target = functionList[func["target"]]
    #     # Massage the input range
    #     if "min" in target:
    #         input_range[0] = target["min"] if target["min"]>input_range[0] else input_range[0]
    #     if "max" in target:
    #         input_range[1] = target["max"] if target["max"]<input_range[1] else input_range[1]
    #     print "Approximating {}, input range {}, input resolution {}, error {}".format(func["target"], input_range, input_res, error)
    #     app = mfit.Polynomial (
    #         [input_range],
    #         [input_res],
    #         error,
    #         [1,1],
    #         2,
    #         "out.csv",
    #         target["func"]
    #     )
    #     # Process results
    #     res = {
    #         "error": app.best_error,
    #         "cost": app.best_cost,
    #         "degree": app.best_degree,
    #         "fractional": list(app.best_number_of_fractional_bits),
    #         "integer": list(app.best_number_of_integer_bits),
    #         "pieces": len(app.coeffs_list),
    #         "min_pieces": app.best_pieces
    #     }
    #     print res
    #     if res["error"]>error:
    #         print "ERROR NOT MET!"

    #     # Evaluate bitwidths
    #     bitWidth = 0
    #     # # TODO: add 1 to range
    #     for i in range(res["degree"]):
    #         bitWidth += res["integer"][i]+res["fractional"][i]
    #     # HACK: before we have the last coefficient
    #     bitWidth += res["integer"][i]+res["fractional"][i]
    #     # Update costs
    #     energy += res["cost"]
    #     sram += (res["pieces"])*bitWidth
    #     sram_opt += (res["min_pieces"])*bitWidth
    #     sramAccesses += res["degree"]+1


    print coarseFunc

    for idx, outputConf in enumerate(coarseFunc["output"]):
        input_range = coarseFunc["input_range"]
        input_res = coarseFunc["input_res"]
        error = outputConf["error"]

        target = functionList[coarseTarget]
        # Massage the input range
        for j, elem in enumerate(input_range):
            if "min" in target:
                input_range[j][0] = target["min"][j] if target["min"][j]>input_range[j][0] else input_range[j][0]
            if "max" in target:
                input_range[j][1] = target["max"][j] if target["max"][j]<input_range[j][1] else input_range[j][1]

        print "Approximating {}, input range {}, input resolution {}, error {}".format(coarseTarget, input_range, input_res, error)

        app = mfit.Polynomial (
            input_range,
            input_res,
            error,
            [1,1],
            2,
            "out.csv",
            target["func"][idx]
        )
        # Process results
        res = {
            "error": app.best_error,
            "cost": app.best_cost,
            "degree": app.best_degree,
            "fractional": list(app.best_number_of_fractional_bits),
            "integer": list(app.best_number_of_integer_bits),
            "pieces": len(app.coeffs_list),
            "min_pieces": app.best_pieces
        }
        print res

    print "Final Costs:"
    print "\t Energy = {}pJ".format(energy)
    print "\t SRAM = {} bits".format(sram)
    print "\t SRAM (opt) = {} bits".format(sram_opt)
    print "\t SRAM Reads = {}".format(sramAccesses)

if __name__ == '__main__':
    TARGET = "inversek2j"

    # Read config
    # default_conf = tp.read_config("accept_conf_orig.txt", fixed=True, target_func=TARGET, bb_stats_fn="accept_bbstats.txt", fp_stats_fn="accept_fpstats.txt")
    opt_conf = tp.read_config("accept_conf_opt.txt", fixed=True, target_func=TARGET, bb_stats_fn="accept_bbstats.txt", fp_stats_fn="accept_fpstats.txt")
    # Extract instructions
    insns = processInsn("accept_static.txt", TARGET)
    # deriveCosts(insns, opt_conf, TARGET)
    # Extract branches
    branches = processBranches("accept_static.txt")
    # Produce DFG and CFG graph based on static information
    DFG(insns, "dfg.dot")
    subprocess.call(shlex.split("dot dfg.dot -Tpng -odfg.png"))
    CFG(branches, "cfg.dot")
    subprocess.call(shlex.split("dot cfg.dot -Tpng -ocfg.png"))

    # # Produce DDDG
    # branchOutcomes = processBranchOutcomes('accept_dyntrace.txt', insns, branches)
    # dddg = DDDG(insns, branchOutcomes)
    # DFG(dddg, "dddg.dot")
    # subprocess.call(shlex.split('dot dddg.dot -Tpng -odddg.png'))



