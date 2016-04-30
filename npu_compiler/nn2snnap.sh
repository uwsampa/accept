#!/bin/bash

nn=$1

pe=16
pu=4

./npu_compiler.py -nn $nn -pe $pe -pu $pu &>/dev/null

./mif_binary.sh \
  -i mif/meminit_insn.mif \
  -s mif/sigmoid.mif \
  -b mif/meminit_offset.mif \
  -w mif/meminit_w??.mif




