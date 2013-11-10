#!/bin/sh
outfn=output.txt

./streamcluster 2 5 1 10 10 5 none $outfn 1
cat $outfn
