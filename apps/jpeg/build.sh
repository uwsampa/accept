#!/bin/sh
here=`dirname $0`
args="--runargs '' jpeg dct.c encoder.c huffman.c jpeg.c quant.c rgbimage.c marker.c"
. $here/../build.sh
