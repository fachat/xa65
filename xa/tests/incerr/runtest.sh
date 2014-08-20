#!/bin/bash

# run a test script

#THISDIR=`dirname $0`
THISDIR=`pwd`

#echo "0=$0"
#echo "THISDIR=$THISDIR"

declare -A opts
opts=([test.a65]="-w")

#ASMFLAGS=-v
ASMFLAGS=

# exclude filter from *.asm if no explicit file is given
EXCLUDE=

# test files used
TESTFILES="test.6502"

# files to compare afterwards, against <file>-<script>
COMPAREFILES=

XA=$THISDIR/../../xa

##########################
# actual code
. ../func.sh

