#!/bin/bash

while getopts i:s:b:w opt; do

  case "$opt" in
    i) ftype=1; pe=0; ibase=2  ;;
    s) ftype=2; pe=0; ibase=16 ;;
    b) ftype=3; pe=0; ibase=16 ;;
    w) break;;
    *) echo "bad args"; exit 1 ;;
  esac

  file=$OPTARG

  nlines=$(wc -l $file | cut -d' ' -f 1)
 
  hex="$(\
    echo "obase=10; ibase=$ibase; $(awk '{print toupper($0)}' $file)" |\
    bc |\
    awk '{printf("%016X\n", $0)}' |\
    awk 'BEGIN{FS=""} {print $15$16$13$14$11$12$9$10$7$8$5$6$3$4$1$2}'
  )"

  printf "%02X" $ftype  | xxd -ps -r
  printf "%02X" $pe     | xxd -ps -r 
  printf "%04X" $nlines | xxd -ps -r | dd conv=swab status=none
  echo "$hex"           | xxd -ps -r

done

shift $(($OPTIND-1))
pe_cnt=0;
for file in $@; do
 
  ftype=4
  pe=$pe_cnt
  pe_cnt=$(($pe_cnt + 1))
  ibase=16

  nlines=$(wc -l $file | cut -d' ' -f 1)
 
  hex="$(\
    echo "obase=10; ibase=$ibase; $(awk '{print toupper($0)}' $file)" |\
    bc |\
    awk '{printf("%016X\n", $0)}' |\
    awk 'BEGIN{FS=""} {print $15$16$13$14$11$12$9$10$7$8$5$6$3$4$1$2}'
  )"

  printf "%02X" $ftype  | xxd -ps -r
  printf "%02X" $pe     | xxd -ps -r
  printf "%04X" $nlines | xxd -ps -r | dd conv=swab status=none
  echo "$hex"           | xxd -ps -r


done
