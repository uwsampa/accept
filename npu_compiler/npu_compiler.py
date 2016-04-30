#!/usr/bin/env python

####################################################################################
## File Name: npu_compiler.py
## Author: Thierry Moreau
## Email: moreau@uw.edu
##
## Copyright (c) 2012-2016 University of Washington
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without modification,
## are permitted provided that the following conditions are met:
## -       Redistributions of source code must retain the above copyright notice,
##         this list of conditions and the following disclaimer.
## -       Redistributions in binary form must reproduce the above copyright notice,
##         this list of conditions and the following disclaimer in the documentation
##         and/or other materials provided with the distribution.
## -       Neither the name of the University of Washington nor the names of its
##         contributors may be used to endorse or promote products derived from this
##         software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY OF WASHINGTON AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL THE UNIVERSITY OF WASHINGTON OR CONTRIBUTORS BE LIABLE FOR ANY
## DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
## OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
## NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
## IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
####################################################################################

from __future__ import division
import re
import sys
import os
import argparse
import struct
import logging
from numpy import *
from ctypes import *
from math import *

# Neural Network Dimensions Limits
# Increase if necessary
INPUT_NEURONS_MAX        = 256 # Size limit for input and hidden layers
LOG_INPUT_NEURONS_MAX    = int(log2(INPUT_NEURONS_MAX))
OUTPUT_NEURONS_MAX       = 256 # Size limit for output and hidden layers
LOG_OUTPUT_NEURONS_MAX   = int(log2(OUTPUT_NEURONS_MAX))
WEIGHT_COUNT_MAX         = 2048 # Size limit for weights
LOG_WEIGHT_COUNT_MAX     = int(log2(WEIGHT_COUNT_MAX))

# Test parameter defaults
TEST_SIZE           = 1024
TIMEOUT             = 100000
PERIOD              = 10

# Unroll factor default
UNROLL              = 1

# File Parsing parameters
FILE_OFFSET         = 3         # Data file format (should not be changed)
DATA_OFFSET         = 0         # Skips the first N input/output pairs
INPUT_LINES         = 1         # Number of input lines per input set

# Write CSV file
WRITE_CSV_SCHEDULE  = False

# Directory locations
MIF_DIR             = 'mif/'
CSV_DIR             = 'csv/'

# File Names
NPU_COMPILER_DIR    = os.path.dirname(__file__)
DEFAULT_MIF         = MIF_DIR+'sigmoid.mif'     # Destination for sigmoid LUT mif
INPUT_MIF           = MIF_DIR+'sig_input.mif'   # Destination for sigmoid input test vector mif
OUTPUT_MIF          = MIF_DIR+'sig_output.mif'  # Destination for sigmoid output test vector mif
SIGMOID_INC         = 'sigmoid.inc'             # Destination for sigmoid parameter file
SIGMOID_INC_TMPL    = 'sigmoid.inc.template'    # Sigmoid parameter file template
DESIGN_INC          = 'npu.inc'                 # Destination for NPU design parameter file
LOG_FILENAME        = 'npu_compiler.log'        # Destination for log file

# Default NPU parameters (gets overwritten in args parsing)
DEFAULT_NUM_PEs         = 8
DEFAULT_NUM_PUs         = 2

# Default Input and Weigth fixed point representation parameters (gets overwritten in args parsing)
DEFAULT_IF_WIDTH        = 64
DEFAULT_W_INTEGER       = 8
DEFAULT_W_PRECISION     = 7
DEFAULT_I_INTEGER       = 0
DEFAULT_I_PRECISION     = 7

# Default Sigmoid Parameters (gets overwritten in args parsing)
DEFAULT_LUT_SIZE        = 256
DEFAULT_FUNCT_BOUNDS    = 4
DEFAULT_TEST_BOUNDS     = 16

# Precise Sigmoid (use to evaluate errors)
PRECISE_SIGMOID         = False


############################################
# Functions used to generate instruction mif
############################################
def get_pe_str(string):
    ''' Formats the pe string in the appropriate mif format
    '''
    return '{:b}'.format(int(string)).zfill(LOG_INPUT_NEURONS_MAX)

def get_cycles_str(string):
    ''' Formats the cycle string in the appropriate mif format
    '''
    return '{:b}'.format(int(string)).zfill(LOG_OUTPUT_NEURONS_MAX)

def get_dstsel_str(string):
    ''' Formats the dstsel string in the appropriate mif format
    '''
    if (string == 'NONE'):
        return "000"
    elif (string == 'SIG0'): # Linear
        return "001"
    elif (string == 'SIG1'): # Non-symmetrical
        return "010"
    elif (string == 'SIG2'): # Symmetrical
        return "011"
    elif (string == 'OSIG0'): # Output Linear
        return "101"
    elif (string == 'OSIG1'): # Output Non-symmetrical
        return "110"
    elif (string == 'OSIG2'): # Output Symmetrical
        return "111"
    else:
        print "ERROR: unknown dstsel string!"
        exit(1)

def get_dinsel_str(string):
    ''' Formats the dinsel string in the appropriate mif format
    '''
    if (string == 'DINBUS'):
        return "0"
    elif (string == 'SIG'):
        return "1"
    else:
        print "ERROR: unknown dinsel string!"
        exit(1)

def get_lastl_str(string):
    ''' Formats the lastl string in the appropriate mif format
    '''
    return string


############################################
# Conversion functions
############################################
def float_to_fix(x, int_prec, dec_prec, weight=True):
    '''Converts a float into a fix of precisions p
    '''
    if x>=pow(2, int_prec):
        x = pow(2, int_prec) - pow(2, (-1)*dec_prec)
    if x<=(-1)*(pow(2, int_prec)):
        x = (-1)*pow(2, int_prec)
    x_scaled = x*pow(2,dec_prec)

    x_scaled = floor(x_scaled)
    return x_scaled


def float_to_hex(fl):
    '''Produces binary representation of float.
    '''
    return ''.join('%2.2x' % ord(c) for c in struct.pack('>f', fl))

def weight_tohex(n, w):
    """Generates hex representation for a float.
    """
    # This could be better written, but let's leave it at that for now
    if w <= 4:
        n_str = "%s"%("0%x"%(int(n)&0xf))[-1:]
    elif w <= 8:
        n_str = "%s"%("00%x"%(int(n)&0xff))[-2:]
    elif w <= 12:
        n_str = "%s"%("000%x"%(int(n)&0xfff))[-3:]
    elif w <= 16:
        n_str = "%s"%("0000%x"%(int(n)&0xffff))[-4:]
    elif w <= 20:
        n_str = "%s"%("00000%x"%(int(n)&0xfffff))[-5:]
    elif w <= 24:
        n_str = "%s"%("000000%x"%(int(n)&0xffffff))[-6:]
    elif w <= 28:
        n_str = "%s"%("0000000%x"%(int(n)&0xfffffff))[-7:]
    elif w <= 32:
        n_str = "%s"%("00000000%x"%(int(n)&0xffffffff))[-8:]
    else:
        print('Error:\tData width is too wide!')
        exit()
    return n_str

def getZeros(w):
    """Returns 0 in hex of width w
    """
    return "".zfill(w//4)

# Instruction subfields
insnfields = ['pe', 'cycles', 'dstsel', 'lastl', 'dinsel']

# Maps the instruction subfield to the corresponding formatting function
insnfieldformat = {
    'pe'        : get_pe_str,
    'cycles'    : get_cycles_str,
    'dstsel'    : get_dstsel_str,
    'lastl'     : get_lastl_str,
    'dinsel'    : get_dinsel_str
    }

# Maps .nn sigmoid function code to csv sigmoid function code
translate_sigmoid = {
    0: {'hidden': 'SIG0', 'output': 'OSIG0'},
    3: {'hidden': 'SIG1', 'output': 'OSIG1'},
    5: {'hidden': 'SIG2', 'output': 'OSIG2'}
}

############################################
# ANN Class
############################################
class ANN:
    ''' A multi-layer perceptron class
    '''

    def __init__(self, nn_file, benchmark, num_pus, num_pes, unroll_factor, ifWidth, iFracBits, iIntBits, wFracBits, wIntBits, sigSize, sigFBounds, sigTBounds, sigNoSymm):
        ''' Reads the ANN description from the .nn file
        '''
        # ANN description
        self.layers  = []           # A list describing the ANN's topology
        self.inputs  = 0            # Number of inputs
        self.outputs = 0            # Number of outputs
        self.sigmoid_inv = 0        # Number of sigmoid invocations
        self.weights = []           # The weights of the neural network
        self.neurons = []           # The activation function at a neuron
        self.param_size = 0         # Parameterization size for the benchmark
        # Derive benchmark name
        self.bench = benchmark
        # Achitectural parameters
        self.num_pus = num_pus
        self.num_pes = num_pes
        # Unroll factor
        self.unroll_factor = unroll_factor
        # Bitwidths
        self.ifWidth = ifWidth # Interface bitwidth (e.g. ACP has 64 bits)
        self.iFracBits = iFracBits  # Input Fractional Bits
        self.iIntBits = iIntBits    # Input Integer Bits
        self.wFracBits = wFracBits  # Weight Fractional Bits
        self.wIntBits = wIntBits    # Weight Integer Bits
        # Derived bitwidths
        self.iWidth = iFracBits+iIntBits+1
        self.iToFix = int(pow(2,iFracBits))
        self.wWidth = wFracBits+wIntBits+1
        self.wToFix = int(pow(2,wFracBits))
        # Sigmoid parameters
        self.sigSteepness   = None       # Activation Function Steepness
        self.sigSize        = sigSize    # Sigmoid LUT size
        self.sigFBounds     = sigFBounds # Sigmoid Function Bounds
        self.sigTBounds     = sigTBounds # Sigmoid Test Bounds
        self.sigNoSymm      = sigNoSymm  # Symmetrical LUT

        # Interface width check
        if self.ifWidth % self.iWidth != 0:
            print "Error : Interface width {} has to be a multiple of input width {}.".format(self.interfaceBits, self.iWidth)
            exit()

        # Read the nn file
        with open(nn_file, 'r') as f:

            # Read in the topology
            for line in f:
                match = re.match (r'^\s*layer_sizes=((\d+.+)*)\s*$', line)
                if match:
                    self.layers = [int(x) for x in match.group(1).split()]

            # Make sure that the neural network is not too big
            num_weights = 0
            for idx, layer in enumerate(self.layers):
                if idx < len(self.layers)-1:
                    num_weights += self.layers[idx]*self.layers[idx+1]
                # Check input neuron
                if idx==0:
                    if layer > INPUT_NEURONS_MAX:
                        print "Error : Neural network input layer is too wide."
                        print "        Increase INPUT_NEURONS_MAX or reduce the input layer of the neural network."
                        exit()
                elif idx==len(self.layers)-1:
                    if layer > OUTPUT_NEURONS_MAX:
                        print "Error : Neural network output layer is too wide."
                        print "        Increase OUTPUT_NEURONS_MAX or reduce the output layer of the neural network."
                        exit()
                else:
                    if layer > INPUT_NEURONS_MAX:
                        print "Error : Neural network hidden layer is too wide."
                        print "        Increase INPUT_NEURONS_MAX or reduce the hidden layer of the neural network."
                        exit()
                    if layer > OUTPUT_NEURONS_MAX:
                        print "Error : Neural network hidden layer is too wide."
                        print "        Increase OUTPUT_NEURONS_MAX or reduce the hidden layer of the neural network."
                        exit()
            if num_weights > WEIGHT_COUNT_MAX:
                print "Error : Number of neural network weights is too large."
                print "        Increase WEIGHT_COUNT_MAX or reduce the size of the neural network."
                exit()

            # Derive the input and output sizes
            self.inputs = self.layers[0]-1
            self.outputs = self.layers[len(self.layers)-1]-1

            # If the number of inputs or outputs is odd, the unroll factor has to be at least 2!
            # if self.unroll_factor==1:
            #     if self.inputs%2==1 or self.outputs%2==1:
            #         self.unroll_factor = 2
            #         print "Info: The unrolling factor was set to 2 because the input or output size is odd"

            # Compute the total number of neurons
            self.sigmoid_inv = sum(self.layers) - self.layers[0] - (len(self.layers)-1)
            num_neurons = sum(self.layers) - self.layers[len(self.layers)-1]

            # Read in the raw weights + neuron activation functions
            raw_weights = [[] for i in range(num_neurons)]
            raw_neuron = []
            f.seek(0)
            for line in f:
                matchObj = re.match (r'connections \(connected_to_neuron, weight\)=(.*)', line)
                if matchObj:
                    tmp_weights = matchObj.group(1)
                    tmp_weights = re.sub(r'\(', '', tmp_weights)
                    tmp_weights = re.sub(r'\)', ';', tmp_weights)
                    tmp_weights = re.split(r'; ', tmp_weights)
                    for weight in tmp_weights:
                        if weight!= '':
                            weight = re.split(r', ', weight)
                            n = int(weight[0])
                            w = float_to_fix(float(weight[1]), self.wIntBits, self.wFracBits)
                            raw_weights[n].append([n,w])
                matchObj = re.match (r'neurons \(num_inputs, activation_function, activation_steepness\)=(.*)', line)
                if matchObj:
                    tmp_neurons = matchObj.group(1)
                    tmp_neurons = re.sub(r'\(', '', tmp_neurons)
                    tmp_neurons = re.sub(r'\)', ';', tmp_neurons)
                    tmp_neurons = re.split(r'; ', tmp_neurons)
                    for neuron in tmp_neurons:
                        if neuron!= '':
                            neuron = re.split(r', ', neuron)
                            activation = int(neuron[1])
                            steepness = float(neuron[2])
                            raw_neuron.append([activation,steepness])

            # Process the raw weights into a list of weight matrices,
            # extract the neuron type for each layer
            n_index = 0
            for l in range(len(self.layers)-1):
                w_matrix = zeros(shape=(self.layers[l],  self.layers[l+1]-1))
                for i in range(self.layers[l]):
                    w_vector = raw_weights[n_index+i]
                    for j, w in enumerate(w_vector):
                        w_matrix[i][j] = w[1]
                n_index = n_index + self.layers[l]
                self.neurons.append(raw_neuron[n_index])
                self.weights.append(w_matrix)
                # Set the activation steepness
                steepness = raw_neuron[n_index][1]
                if self.sigSteepness == None:
                    self.sigSteepness = steepness
                elif self.sigSteepness != steepness:
                    print "Error: Steepness mismatch! Please use consistent neuron activation steepness!"
                    exit()


    def evaluate(self, dat_file, test_size, data_offset, input_lines):
        ''' Evaluates the HW ANN on a input dataset
        '''
        # Input and output vectors in hex
        self.tb_inputs = []
        self.tb_outputs = []
        # Test size
        self.test_size = test_size

        # Activation functions
        activation_function = {
            0: self.linear,
            3: self.nonsymmetric,
            5: self.symmetric
        }


        print "Info: Evaluating hardware neural network on a set of %dx%d inputs..." % (self.test_size, self.unroll_factor)

        # Output values - used to compute RMSE
        precise_data = []
        approx_data = []

        with open(dat_file, 'r') as f:
            lines = f.readlines()

            # File start and end values
            f_start = FILE_OFFSET+(input_lines+1)*data_offset
            f_end = f_start+(input_lines+1)*self.unroll_factor*self.test_size

            # Check the file length
            if len(lines) < f_end:
                f_end = len(lines)
                self.test_size = (f_end-f_start)/((input_lines+1)*self.unroll_factor)

            for i in range(f_start, f_end, (input_lines+1)):
                # Process inputs
                inputs = []
                for j in range(input_lines):
                    inputs += [float(x) for x in lines[i+j].split()]
                # Push the inputs into tb_inputs in hex form
                self.tb_inputs.append(inputs)
                # Convert inputs to fixed-point
                inputs = [float_to_fix(x, self.iIntBits, self.iFracBits, False) for x in inputs]
                mat_A = array([inputs])
                # Evaluate the neural network
                for layer, w in enumerate(self.weights):
                    # Layer input vector
                    mat_A = append(mat_A, [[self.iToFix]], 1)
                    # Weight Matrix
                    mat_B = w
                    logging.debug('A\n' + str(mat_A))
                    logging.debug('B\n' + str(mat_B))
                    # Get the steepness and activation function
                    steepness = self.neurons[layer][1]
                    sigmoid = vectorize(activation_function[self.neurons[layer][0]])
                    mat_A = dot(mat_A, mat_B)
                    logging.debug('AxB\n' + str(mat_A))
                    mat_A = sigmoid(mat_A,steepness)
                    logging.debug('SIG(AXB)\n' + str(mat_A))
                approx_outputs = mat_A[0].tolist()
                approx_outputs = [float(x) / (self.iToFix) for x in approx_outputs]
                # Log the approximate output values for RMSE computation
                approx_data.append(approx_outputs)
                # Push the outputs into tb_outputs in hex form
                self.tb_outputs.append(approx_outputs)
                logging.debug('Approx Outputs\n'+str(approx_outputs))

                # Process outputs
                precise_outputs = [float(x) for x in lines[i+input_lines].split()]
                # Log the precise output values for RMSE computation
                precise_data.append(precise_outputs)
                logging.debug('Precise Outputs\n'+str(precise_outputs))

        # Used to compute RMSE
        mse = ((array(precise_data) - array(approx_data)) ** 2).mean()
        print('Info:\tMSE on HW neural network is: %f' % mse)

        classification_errors = 0
        for p,a in zip(precise_data, approx_data):
            if p[0] < 0.5 and a[0] >= 0.5:
                classification_errors += 1
            elif p[0] >= 0.5 and a[0] < 0.5:
                classification_errors += 1
        print('Info:\tClassification error is %f' % (float(classification_errors)/len(precise_data)))


    def generate_fixed_test_vectors(self):
        ''' Generates test vectors + verilog test bench code
        '''

        # Flatten the input, output lists
        input_list = [float_to_fix(x, self.iIntBits, self.iFracBits, False) for input_set in self.tb_inputs for x in input_set]
        output_list = [float_to_fix(x, self.iIntBits, self.iFracBits, False) for output_set in self.tb_outputs for x in output_set]

        # Derive the interface width to input widht ratio
        ifRatio = self.ifWidth//self.iWidth

        # Generate input test vectors
        input_mif_string = ''
        input_mif_line = ''
        for i in range (0, len(input_list)):
            input_mif_line = weight_tohex(input_list[i], self.iWidth)+input_mif_line
            if i % ifRatio == ifRatio-1:
                input_mif_string += input_mif_line+'\n'
                input_mif_line = ''
        while i % ifRatio != ifRatio-1:
            input_mif_line = weight_tohex(0, self.iWidth)+input_mif_line
            i+=1
        input_mif_string += input_mif_line

        # Generate output test vectors
        output_mif_string = ''
        output_mif_line = ''
        for i in range (0, len(output_list)):
            output_mif_line = weight_tohex(output_list[i], self.iWidth)+output_mif_line
            if i % ifRatio == ifRatio-1:
                output_mif_string += output_mif_line+'\n'
                output_mif_line = ''
        while i % ifRatio != ifRatio-1:
            output_mif_line = weight_tohex(0, self.iWidth)+output_mif_line
            i+=1
        output_mif_string += output_mif_line

        # If the directory does not exist, make it
        if(os.path.exists(MIF_DIR)==False):
            os.mkdir(MIF_DIR)

        # Write the input Vectors
        mif_file = MIF_DIR+self.bench+'_input_vectors.mif'
        print('Info: Writing input vector of length %d to %s' % (input_mif_string.count('\n'), mif_file))
        f = open(mif_file, 'w')
        f.write(input_mif_string)
        f.close()

        # Write the output Vectors
        mif_file = MIF_DIR+self.bench+'_output_vectors.mif'
        print('Info: Writing output vector of length %d to %s' % (output_mif_string.count('\n'), mif_file))
        f = open(mif_file, 'w')
        f.write(output_mif_string)
        f.close()

    def generate_schedule(self):
        '''Generates the schedule for the NPU
        '''
        instructions = []

        # Insert no-ops if there are less inputs than outputs
        # FIXME: might not work for all topologies
        if(self.layers[0]<self.layers[len(self.layers)-1]):
            for noop in range(self.layers[len(self.layers)-1]-self.layers[0]):
                # Add a no-op
                instructions.append({
                    'pe': '0',
                    'cycles': '0',
                    'dstsel': 'NONE',
                    'lastl': '0',
                    'dinsel': 'DINBUS'
                })


        for l in range(len(self.layers)-1):
            pe_count = self.layers[l]-1
            cycles = self.layers[l+1]-1
            lastl = 0

            if l==0:
                dinsel = 'DINBUS'
            else:
                dinsel = 'SIG'

            for u in range(self.unroll_factor):
                for pe in range(pe_count):

                    dstsel = 'NONE'
                    if pe == pe_count-1:
                        if l==len(self.layers)-2:
                            dstsel = translate_sigmoid[self.neurons[l][0]]['output']
                        else:
                            dstsel = translate_sigmoid[self.neurons[l][0]]['hidden']

                    instructions.append({
                        'pe': str(pe),
                        'cycles': str(cycles),
                        'dstsel': dstsel,
                        'lastl': '0',
                        'dinsel': dinsel
                    })

        # Determine the lastl bit by traversing the instruction list backwards
        pe_checklist = [True for i in range(self.num_pes)]
        for insn in reversed(instructions):
            if pe_checklist[int(insn['pe'])%self.num_pes]==True:
                pe_checklist[int(insn['pe'])%self.num_pes]=False
                insn['lastl'] = '1'

        if (WRITE_CSV_SCHEDULE):
            # Print instructions to output csv file
            csv_string = ',\t'.join(insnfields)+'\n'
            for insn in instructions:
                instruction = []
                for field in insnfields:
                    instruction.append(insn[field])
                csv_string += ',\t'.join(instruction)+'\n'

             # If the directory does not exist, make it
            if(os.path.exists(CSV_DIR)==False):
                os.mkdir(CSV_DIR)

            csv_file = CSV_DIR+self.bench+'.csv'
            print('Info: Writing instruction schedule to %s' % csv_file)
            f = open(csv_file, 'w')
            f.write(csv_string)
            f.close()

        # Generate instruction schedule mif
        insn_string = ''
        insn_len = 0
        for insn in instructions:
            instruction = []
            for field in insnfields:
                instruction.append(insnfieldformat[field](insn[field]))
            insn_string += ''.join(instruction)+'\n'
            insn_len += 1

        # Make sure it fits!
        if insn_len > INPUT_NEURONS_MAX:
            print('Error: Instruction stream length exceeds total input count')
            print('       Please increase INPUT_NEURONS_MAX or reduce the size of the neural network.')
            exit()

        # If the directory does not exist, make it
        if(os.path.exists(MIF_DIR)==False):
            os.mkdir(MIF_DIR)

        # Write microcode to disk
        mif_file = MIF_DIR+self.bench+'_insn.mif'
        # Note: 5 is derived from IM_SRC_WIDTH+IM_LST_WIDTH+IM_DST_WIDTH (see params.inc)
        INSN_WIDTH = 5+LOG_OUTPUT_NEURONS_MAX+LOG_INPUT_NEURONS_MAX
        print('Info: Writing raw instruction schedule to %s [size = %dx%db]' % (mif_file, insn_len, INSN_WIDTH))
        f = open(mif_file, 'w')
        f.write(insn_string)
        # Pad with zeros
        for z in range(INPUT_NEURONS_MAX-len(instructions)):
            f.write(getZeros(self.wWidth)+'\n')
        f.close()

        # Update the parameterization size
        self.param_size += insn_len*INSN_WIDTH


    def generate_weights(self):
        """Generates weight memory initialization files from a .nn file.
        """

        # Map the neuron weights to the MIF files
        w_mif = [[] for i in range(self.num_pes)]
        o_mif = []
        num_weights = 0
        n_index = 0
        # For each layer
        for l in range(len(self.layers)-1):
            w_layer = self.weights[l]
            # For time a layer is unrolled
            for u in range(self.unroll_factor):
                # For each neuron
                for n, w in enumerate(w_layer):
                    if n == self.layers[l]-1:
                        # Offset
                        o_mif.extend(w)
                        # Offset padding
                        o_mif.extend([0]*(len(w_mif[0])-len(o_mif)))
                    else:
                        # Weight
                        w_mif[n%self.num_pes].extend(w)
                        num_weights += len(w)

            n_index = n_index + self.layers[l]

        # If directory does not exist, create directory
        if not os.path.exists(MIF_DIR):
            os.mkdir(MIF_DIR)

        # Write offset mif file
        # Make sure that the offset size does not exceed hardware limits
        if len(o_mif) > WEIGHT_COUNT_MAX//self.num_pes:
            print('Error: Offset exceed memory bounds')
            print('       Please increase WEIGHT_COUNT_MAX or reduce the size of the neural network.')
            exit()
        mif_file = MIF_DIR+self.bench+'_offset.mif'
        print ('Info: Writing offsets to %s [size = %dx%db]' % (mif_file, len(o_mif), self.wWidth))
        f = open(mif_file, 'w')
        for w in o_mif:
            s = weight_tohex(w, self.wWidth)+'\n'
            f.write(s)

        for z in range(WEIGHT_COUNT_MAX//self.num_pes-len(o_mif)):
            f.write(getZeros(self.wWidth)+'\n')
        f.close()
        # Update the parameterization size
        self.param_size += len(o_mif)*self.wWidth

        # Write weights mif files
        for i in range(len(w_mif)):
            if len(w_mif[i]) > WEIGHT_COUNT_MAX/self.num_pes:
                print('Error: Offset exceed memory bounds')
                print('       Please increase WEIGHT_COUNT_MAX or reduce the size of the neural network.')
                exit()
            mif_file = "%s%s_w%02d.mif" % (MIF_DIR,self.bench,i)
            print ('Info: Writing weights to %s [size = %dx%db]' % (mif_file, len(w_mif[i]), self.wWidth))
            f = open(mif_file, 'w')
            for w in w_mif[i]:
                s = weight_tohex(w, self.wWidth)+'\n'
                f.write(s)
            for z in range(WEIGHT_COUNT_MAX//self.num_pes-len(w_mif[i])):
                f.write(getZeros(self.wWidth)+'\n')
            f.close()
            # Update the parameterization size
            self.param_size += len(w_mif[i])*self.wWidth

    def report_param_size(self):
        print "Info: Parameterization size is: %d bits" % self.param_size


    ############################################
    # Sigmoid Implementation
    ############################################

    def sig_approx(self, LUT, cut_off, d_in, steepness=1):
        '''Sigmoid Approximation Implementation
           Takes in a fixed-point x value, returns a fixed-point y value
        '''
        lshift = int(-log2(steepness))
        d_in = d_in/(1<<lshift)
        index = int(d_in*self.sigSize//self.sigFBounds)
        if index < cut_off:
            return d_in
        elif index>=len(LUT):
            return 1
        else:
            return LUT[index]/(1 << self.iFracBits)

    def generate_sigmoid(self, mif=DEFAULT_MIF, error=False, plot=False):
        '''Generate an Approximate Sigmoid
        '''
        # Parameter derivation
        iPrec = self.iFracBits+self.wFracBits
        oPrec = self.iFracBits
        input_granularity = 1/(1 << iPrec)
        lut_granularity = self.sigFBounds/self.sigSize

        # Now generating the LookUp Table (LUT) bounds and the LUT values
        LUT = []
        x = lut_granularity/2
        for i in range (0,self.sigSize):
            LUT.append(int(tanh(x)*(1<<oPrec)))
            x = x + lut_granularity

        # Lut containing only positive values
        pos_LUT = LUT[0:len(LUT)]

        if (self.sigNoSymm is False):
            x = 0-self.sigFBounds+lut_granularity/2
            for i in range (0,self.sigSize):
                val = int(tanh(x)*(1<<oPrec))
                hex_val = ((abs(val) ^ 0xff) + 1) & 0xff
                LUT.append(hex_val)
                x = x + lut_granularity

        # Dump the RAM initialization file
        f = open(mif, 'w')
        for i in range(len(LUT)):
            f.write('%02x\n' % LUT[i])

        # Determine the cut-off LUT index from which
        # error is better using an x=y approximation vs.
        # a LUT-based approximation
        cut_off = 0;
        for idx,x in enumerate(arange(0, self.sigFBounds, lut_granularity)):
                ref = tanh(x)
                apx = self.sig_approx(pos_LUT, 0, x)
                e_lut = abs(ref-apx)
                e_lin = abs(ref-x)
                if e_lut < e_lin:
                    cut_off = idx
                    break
        self.cut_off = cut_off
        print "Cut-off is: " + str(cut_off)

        # Produce the Sigmoid test inputs
        f_in = open(INPUT_MIF, 'w')
        f_out = open(OUTPUT_MIF, 'w')
        for i in arange(0, self.sigFBounds, lut_granularity):
            o = self.sig_approx(pos_LUT, cut_off, i, self.sigSteepness)
            d_in = int(i * (1 << iPrec))
            d_out = int(o * (1 << oPrec))
            f_in.write('%08x' % d_in)
            f_out.write('%08x' % d_out)
            if i<(self.sigFBounds/lut_granularity)-1:
                f_in.write('\n')
                f_out.write('\n')

        if (error):
            # Evaluate the RMSE and worst case error of sigmoid implementation
            errors = []
            MSE = 0
            for x in arange(0, self.sigTBounds, input_granularity):
                ref = tanh(self.sigSteepness*x)
                apx = self.sig_approx(pos_LUT, cut_off, x, self.sigSteepness)
                error = abs(ref-apx)
                MSE += error*error
                errors.append(error*100.0)
            MSE /= (self.sigTBounds/input_granularity)
            RMSE = sqrt(MSE)
            WCE = max(errors)
            print "RMSE = {0}".format(RMSE)
            print "Worst Case Error = {0}%".format(WCE)

        # Derive Parameters
        SIG_INPUT_WIDTH = self.iWidth+self.wWidth+LOG_INPUT_NEURONS_MAX
        SIG_OUTPUT_WIDTH = self.iWidth
        # Verilog module parameters
        parameters = {"Sigmoid Bus Widths": {}, "Simulation Params": {}, "Values": {}}
        # Sigmoid Bus Width
        parameters["Sigmoid Bus Widths"]["SIG_LUT_DEPTH"] = self.sigSize*(1 if self.sigNoSymm else 2)
        parameters["Sigmoid Bus Widths"]["LOG_SIG_LUT_DEPTH"] = int(log2(parameters["Sigmoid Bus Widths"]["SIG_LUT_DEPTH"]))
        parameters["Sigmoid Bus Widths"]["SIG_IN_PREC"] = self.iFracBits+self.wFracBits
        parameters["Sigmoid Bus Widths"]["SIG_OUT_PREC"] = self.iFracBits
        parameters["Sigmoid Bus Widths"]["LUT_PRECISION"] = int(log2(self.sigSize)-log2(self.sigFBounds))
        parameters["Sigmoid Bus Widths"]["PRECISION_SHIFT"] = parameters["Sigmoid Bus Widths"]["SIG_IN_PREC"]-parameters["Sigmoid Bus Widths"]["LUT_PRECISION"]
        parameters["Sigmoid Bus Widths"]["INDEX_WIDTH"] = SIG_INPUT_WIDTH-parameters["Sigmoid Bus Widths"]["PRECISION_SHIFT"]
        parameters["Sigmoid Bus Widths"]["SIG_LSHIFT"] = int(-log2(self.sigSteepness))
        # Simulation Parameters
        parameters["Simulation Params"]["SIGTB_UUT_DELAY"] = 1
        parameters["Simulation Params"]["SIGTB_TEST_SIZE"] = int(self.sigFBounds/lut_granularity)
        parameters["Simulation Params"]["SIGTB_LOG_TEST_SIZE"] = int(log2(parameters["Simulation Params"]["SIGTB_TEST_SIZE"]))
        # Values
        parameters["Values"]["PCUTOFF"] = "{0}\'d{1}".format(parameters["Sigmoid Bus Widths"]["INDEX_WIDTH"], cut_off)
        parameters["Values"]["NCUTOFF"] = "-{0}\'d{1}".format(parameters["Sigmoid Bus Widths"]["INDEX_WIDTH"], cut_off)
        parameters["Values"]["PMAX"] = "{0}\'d{1}".format(parameters["Sigmoid Bus Widths"]["INDEX_WIDTH"], self.sigSize-1)
        parameters["Values"]["NMAX"] = "-{0}\'d{1}".format(parameters["Sigmoid Bus Widths"]["INDEX_WIDTH"], self.sigSize-1)
        parameters["Values"]["MAXVAL"] = "%d\'h%0x" % (SIG_OUTPUT_WIDTH,pow(2,SIG_OUTPUT_WIDTH-1)-1)
        parameters["Values"]["MINVAL"] = "%d\'h%0x" % (SIG_OUTPUT_WIDTH,pow(2,SIG_OUTPUT_WIDTH-1))
        parameters["Values"]["ONE"] = "%d\'h0%0x" % (SIG_OUTPUT_WIDTH+1,pow(2,SIG_OUTPUT_WIDTH-1))

        if(parameters["Sigmoid Bus Widths"]["PRECISION_SHIFT"]<0):
            print ("ERROR: LUT size is too large given the numerical precision of input.")
            print ("\tReduce the LUT size or increase numerical percision.")
            exit()

        # Produce the parameter files
        f_inc = open(SIGMOID_INC, 'w')
        with open(os.path.join(NPU_COMPILER_DIR,SIGMOID_INC_TMPL), 'r') as f_tmpl:
            lines = f_tmpl.readlines()
            for l in lines:
                f_inc.write(l)
        f_inc.write("\n\n//////////////////////////////////////\n")
        f_inc.write(    "// Automatically derived parameters //\n")
        f_inc.write(    "//////////////////////////////////////\n")
        for category in parameters:
            f_inc.write("\n// {0}\n".format(category))
            for param in parameters[category]:
                param_str = "localparam signed " if category=="Values" else "localparam "
                # Compute number of spaces required
                num_spaces = 32 - len(param_str + param)
                f_inc.write("{0}{1}{2} = {3};\n".format(param_str, param, " "*num_spaces, parameters[category][param]))


    ############################################
    # Sigmoid functions
    ############################################
    def linear(self, x, s):
        '''Linear Sigmoid
        '''
        logging.debug('LINEAR(%d):' % x)
        x = trunc(x*s)
        x = x//(self.wToFix)
        logging.debug('\t OUTPUT %d:' % x)
        return x

    def sigmoid(self, x, s, ideal=PRECISE_SIGMOID):
        '''Hyperbolic Tangent function (with input scaling by steepness s)
        '''

        if ideal==True:
            # scale x
            shfl = int(floor(log2(1/s)))
            x = int(x)>>shfl

            logging.debug("{}".format(float(x)/pow(2, (self.iFracBits+self.wFracBits))))

            y = tanh(float(x)/pow(2, (self.iFracBits+self.wFracBits))) * pow(2, self.iFracBits)
            return trunc(y)

        # scale x
        shfl = int(floor(log2(1/s)))
        x = int(x)>>shfl
        # negative flag
        is_neg = 1 if x<0 else 0
        # compute LUT index based on x
        index = trunc(int(x) >> int(self.iFracBits+self.wFracBits+log(float(self.sigFBounds)/self.sigSize,2)))
        logging.debug ('\tindex(x) = %d' % index)
        # return value
        y = 0
        if (index > 0-self.cut_off and index < self.cut_off):
            logging.debug ('\tY=X')
            y = x>>self.wFracBits
        elif (not is_neg and index >= self.sigSize):
            logging.debug ('\tY=1')
            y = self.iToFix-1
        elif (is_neg and index <= 0-self.sigSize):
            logging.debug ('\tY=-1')
            y = 0-self.iToFix
        else:
            index = index+0.5 if is_neg else index+0.5
            index = float((index)*float(self.sigFBounds)/self.sigSize)
            logging.debug ('\tsig(%f)' % index)
            y = tanh(index)*(self.iToFix)
        logging.debug ('\tOUTPUT %d' % y)
        return trunc(y)

    def symmetric(self, x, s):
        '''Symmetric Sigmoid (with input scaling by steepness s)
        '''
        logging.debug('SYMM_SIG(%d):' % x)
        return self.sigmoid(x, s)

    def nonsymmetric(self, x, s):
        '''Non-symmetric Sigmoid (with input scaling by steepness s)
        '''
        logging.debug('SIGMOID(%d):' % x)
        out_val = self.sigmoid(x, s)
        out_val = trunc(out_val >> 1)
        out_val += self.iToFix/2
        return out_val

    def dumpParameters(self):
        # Dump the test parameters into an include file
        inc_file = DESIGN_INC

        # Derive Parameters
        acc_width = self.iWidth+self.wWidth+int(log2(INPUT_NEURONS_MAX))

        with open(inc_file, 'w') as f:
            f.write("localparam NUM_PU                  = {};\n".format(self.num_pus))
            f.write("localparam LOG_NUM_PU              = {};\n".format(int(log2(self.num_pus))))
            f.write("localparam NUM_PE                  = {};\n".format(self.num_pes))
            f.write("localparam PER                     = {};\n".format(PERIOD))
            f.write("localparam NPU_TEST_SIZE           = {};\n".format(self.test_size*self.iWidth//self.ifWidth))
            f.write("localparam TIMEOUT                 = {};\n".format(self.test_size*TIMEOUT))
            f.write("localparam INPUT_SIZE              = {};\n".format(self.inputs))
            f.write("localparam OUTPUT_SIZE             = {};\n".format(self.outputs))
            f.write("localparam HIDDEN_NEURONS          = {};\n".format(self.sigmoid_inv))
            # Bus Width
            f.write("localparam IF_WIDTH                = {};\n".format(self.ifWidth))
            f.write("localparam IF_RATIO                = {};\n".format(self.ifWidth//self.iWidth))
            f.write("localparam LOG_IF_RATIO            = {};\n".format(int(log2(self.ifWidth//self.iWidth))))
            f.write("localparam ACC_WIDTH               = {};\n".format(acc_width))
            f.write("localparam DATA_WIDTH              = {};\n".format(self.iWidth))
            f.write("localparam WEIGHT_WIDTH            = {};\n".format(self.wWidth))
            f.write("localparam DATA_DEC_WIDTH          = {};\n".format(self.iFracBits))
            f.write("localparam DATA_INT_WIDTH          = {};\n".format(self.iIntBits))
            f.write("localparam WGHT_DEC_WIDTH          = {};\n".format(self.wFracBits))
            f.write("localparam WGHT_INT_WIDTH          = {};\n".format(self.wIntBits))
            # Neural Network Dimensions
            f.write("localparam INPUT_NEURONS_MAX       = {};\n".format(INPUT_NEURONS_MAX))
            f.write("localparam LOG_INPUT_NEURONS_MAX   = {};\n".format(LOG_INPUT_NEURONS_MAX))
            f.write("localparam OUTPUT_NEURONS_MAX      = {};\n".format(OUTPUT_NEURONS_MAX))
            f.write("localparam LOG_OUTPUT_NEURONS_MAX  = {};\n".format(LOG_OUTPUT_NEURONS_MAX))
            f.write("localparam WEIGHT_COUNT_MAX        = {};\n".format(WEIGHT_COUNT_MAX))
            f.write("localparam LOG_WEIGHT_COUNT_MAX    = {};\n".format(LOG_WEIGHT_COUNT_MAX))

            f.close()


def cli():
    parser = argparse.ArgumentParser(
        description='Process the neural network description files, generate mif for neuron weights, NPU schedules and NPU test vectors'
    )
    parser.add_argument(
        '-nn', dest='nn_file', action='store', type=str, required=True,
        help='neural network configuration file (.nn)'
    )
    parser.add_argument(
        '-dat', dest='data_file', action='store', type=str, required=False,
        default=None, help='neural network training data (.data)'
    )
    parser.add_argument(
        '-i', dest='input_size', action='store', type=int, required=False,
        default=TEST_SIZE, help='number of neural network invocations'
    )
    parser.add_argument(
        '-pu', dest='num_pu', action='store', type=int, required=False,
        default=DEFAULT_NUM_PUs, help='number of processing units'
    )
    parser.add_argument(
        '-pe', dest='num_pe', action='store', type=int, required=False,
        default=DEFAULT_NUM_PEs, help='number of processing elements per processing unit'
    )
    parser.add_argument(
        '-u', dest='unroll_factor', action='store', type=int, required=False,
        default=UNROLL, help='unroll factor for ANN computation'
    )
    parser.add_argument(
        '-data_offset', dest='data_offset', action='store', type=int, required=False,
        default=DATA_OFFSET, help='number of lines to skip in the dataset'
    )
    parser.add_argument(
        '-input_lines', dest='input_lines', action='store', type=int, required=False,
        default=INPUT_LINES, help='number of lines per input set in data file'
    )
    parser.add_argument(
        '-ifWidth', dest='ifWidth', action='store', type=int, required=False,
        default=DEFAULT_IF_WIDTH, help='interface width'
    )
    parser.add_argument(
        '-iFracBits', dest='iFracBits', action='store', type=int, required=False,
        default=DEFAULT_I_PRECISION, help='input fractional bits'
    )
    parser.add_argument(
        '-iIntBits', dest='iIntBits', action='store', type=int, required=False,
        default=DEFAULT_I_INTEGER, help='input integer bits'
    )
    parser.add_argument(
        '-wFracBits', dest='wFracBits', action='store', type=int, required=False,
        default=DEFAULT_W_PRECISION, help='weight fractional bits'
    )
    parser.add_argument(
        '-wIntBits', dest='wIntBits', action='store', type=int, required=False,
        default=DEFAULT_W_INTEGER, help='weight integer bits'
    )
    parser.add_argument(
        '-sigSize', dest='sigSize', action='store', type=int, required=False,
        default=DEFAULT_LUT_SIZE, help='sigmoid LUT size'
    )
    parser.add_argument(
        '-fBounds', dest='fBounds', action='store', type=int, required=False,
        default=DEFAULT_FUNCT_BOUNDS, help='bounds of the sigmoid LUT'
    )
    parser.add_argument(
        '-tBounds', dest='tBounds', action='store', type=int, required=False,
        default=DEFAULT_TEST_BOUNDS, help='test bounds of the sigmoid LUT'
    )
    parser.add_argument(
        '-log', dest='log_fn', action='store', type=str, required=False,
        default=LOG_FILENAME, help='number of lines per input set in data file'
    )
    parser.add_argument(
        '-noSymm', dest='noSymm', action='store_true', required=False,
        default=False, help='LUT does not cover the negative range'
    )
    parser.add_argument(
        '-mif', dest='mif', action='store', type=str, required=False,
        default=DEFAULT_MIF, help='path to the mif file'
    )
    args = parser.parse_args()

    # Logger
    logging.basicConfig(filename=args.log_fn,level=logging.DEBUG)

    # Derive the benchmark name
    # benchName = os.path.splitext(os.path.basename(args.nn_file))[0]
    benchName = "meminit"

    # Read the ANN description
    ann = ANN (
        nn_file = args.nn_file,
        benchmark = benchName,
        num_pus = args.num_pu,
        num_pes = args.num_pe,
        unroll_factor = args.unroll_factor,
        ifWidth = args.ifWidth,
        iFracBits = args.iFracBits,
        iIntBits = args.iIntBits,
        wFracBits = args.wFracBits,
        wIntBits = args.wIntBits,
        sigSize = args.sigSize,
        sigFBounds = args.fBounds,
        sigTBounds = args.tBounds,
        sigNoSymm = args.noSymm
    )

    # Generate the instruction schedule
    ann.generate_schedule()

    # Generate the weights
    ann.generate_weights()

    # Report param size
    ann.report_param_size()

    # Generate the Sigmoid RTL
    ann.generate_sigmoid()

    if args.data_file:
        # Evaluate ANN on inputs
        ann.evaluate(args.data_file, args.input_size, args.data_offset, args.input_lines)
        # Generate test vectors + stimulus file
        ann.generate_fixed_test_vectors()
        # Dump parameters
        ann.dumpParameters()


if __name__ == '__main__':
    cli()
