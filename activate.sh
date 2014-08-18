#!/bin/bash

called=$_

[[ $called == $0 ]] && echo "You probably want to source this script: '. ${0}"

ACTIVATE_DIR=`dirname ${BASH_SOURCE}`
ACTIVATE_DIR_NORM="`cd "${ACTIVATE_DIR}";pwd`"

export PATH=${ACTIVATE_DIR_NORM}/bin:${ACTIVATE_DIR_NORM}/build/built/bin:$PATH

unset called
unset ACTIVATE_DIR
unset ACTIVATE_DIR_NORM
