#!/bin/bash

declare -A errs
declare -A opts


function usage() {
        echo "Assemble *.asm or *.a65 test files"
        echo "  $0 [options] [frs_scripts]"
        echo "Options:"
        echo "       -v                      verbose log"
        echo "       -o                      server log on console"
        echo "       -d <breakpoint>         run xa with gdb and set given breakpoint. Can be "
        echo "                               used multiple times"
        echo "       -k                      keep all files, even non-log and non-data files in run directory"
        echo "       -c                      clean up only non-log and non-data files from run directory,"
        echo "                               keep rest (default)"
        echo "       -C                      always clean up complete run directory"
        echo "       -R <run directory>      use given run directory instead of tmp folder (note:"
        echo "                               will not be rmdir'd on -C"
        echo "       -q                      will suppress any output except whether test was successful"
        echo "       -h                      show this help"
}

function contains() {
        local j
        for j in "${@:2}"; do test "$j" == "$1"  && return 0; done;
        return 1;
}


function hexdiff() {
        diffres=0
        if ! cmp -b "$1" "$2"; then
                tmp1=`mktemp`
                tmp2=`mktemp`

                hexdump -C "$1" > $tmp1
                hexdump -C "$2" > $tmp2

                diff -u $tmp1 $tmp2
                diffres=$?

                rm $tmp1 $tmp2
        fi;
        return $diffres
}

VERBOSE=""
DEBUG=""
CLEAN=1
QUIET=0
LOGFILE=""

TMPDIR=`mktemp -d`
OWNDIR=1

while test $# -gt 0; do
  case $1 in
  -h)
        usage
        exit 0;
        ;;
  -o)
        LOGFILE="-"
        shift;
        ;;
  -v)
        VERBOSE="-v"
        shift;
        ;;
  -d)
        if test $# -lt 2; then
                echo "Option -d needs the break point name for gdb as parameter"
                exit -1;
        fi;
        DEBUG="$DEBUG $2"
        shift 2;
        ;;
  -k)
        CLEAN=0
        shift;
        ;;
  -c)
        CLEAN=1
        shift;
        ;;
  -C)
        CLEAN=2
        shift;
        ;;
  -q)
        QUIET=1
        shift;
        ;;
  -R)
        if test $# -lt 2; then
                echo "Option -R needs the directory path as parameter"
                exit -1;
        fi;
        TMPDIR="$2"
        OWNDIR=0
        shift 2;
        ;;
  -?)
        echo "Unknown option $1"
        usage
        exit 1;
        ;;
  *)
        break;
        ;;
  esac;
done;

#######################
# test for executables

ERR=0
if test ! -e $XA; then
       echo "$XA does not exist! Maybe forgot to compile?"
        ERR=1
fi
if [ $ERR -ge 1 ]; then
        echo "Aborting!"
        exit 1;
fi


# scripts to run
if [ "x$*" = "x" ]; then
	shopt -s extglob
        SCRIPTS=$THISDIR/*${FILTER}*.a[s6][m5]
        SCRIPTS=`basename -a $SCRIPTS`;

        TESTSCRIPTS=""

        if test "x$EXCLUDE" != "x"; then
                exarr=( $EXCLUDE )
                scrarr=( $SCRIPTS )
                for scr in "${scrarr[@]}"; do
                        if ! contains "${scr}" "${exarr[@]}"; then
                                TESTSCRIPTS="$TESTSCRIPTS $scr";
                        fi
                done;
        else
                TESTSCRIPTS="$SCRIPTS"
        fi;
else
        TESTSCRIPTS="";

        for i in "$@"; do
               if test -f "$i".asm ; then
                        TESTSCRIPTS="$TESTSCRIPTS $i.asm";
               else
               		if test -f "$i".a65 ; then
                        	TESTSCRIPTS="$TESTSCRIPTS $i.a65";
			else
                       	 	TESTSCRIPTS="$TESTSCRIPTS $i";
			fi
               fi
        done;
fi;

#echo "TESTSCRIPTS=$TESTSCRIPTS"

########################
# tmp names
#

DEBUGFILE="$TMPDIR"/gdb.ex

########################
# stdout

# remember stdout for summary output
exec 5>&1
exec 6>&2

# redirect log when quiet
if test $QUIET -ge 1 ; then
       exec 1>$TMPDIR/stdout.log
       exec 2>$TMPDIR/stderr.log
fi

########################
# prepare files
#

for i in $TESTSCRIPTS; do
        cp "$THISDIR/$i" "$TMPDIR"
done;

########################
# run scripts
#

for script in $TESTSCRIPTS; do

        echo "Run script $script"

	AFLAGS="${ASMFLAGS} ${opts[$script]}"

	ALOG=
        if test "x$LOGFILE" = "x"; then
		ALOG="-P $script.log"
	else
		ALOG="-P $LOGFILE"
	fi
	ALOG="$ALOG -e $script.err"

        # overwrite test files in each iteration, just in case
        for i in $TESTFILES; do
                if [ -f ${THISDIR}/${i}.gz ]; then
                        gunzip -c ${THISDIR}/${i}.gz >  ${TMPDIR}/${i}
                else
                        cp ${THISDIR}/${i} ${TMPDIR}/${i}
                fi;
        done;

        if test "x$DEBUG" = "x"; then
                ############################################
                # simply do assemble

                echo "Run assembler:" $XA $AFLAGS $script 
		(cd $TMPDIR; $XA -o a.out $ALOG $AFLAGS $script)
                RESULT=$?

		# check if we actually expected an error
		#echo "errs=${errs[$script]}"
		if [ "x${errs[$script]}" != "x" ]; then
			if [ "$RESULT" = ${errs[$script]} ]; then
				echo "override error because was expected"
				RESULT=0
			fi
		fi

                if test $RESULT -eq 0; then
                        echo "$script: Ok" >&5
                else
                        echo "$script: errors: $RESULT" >&5
                fi

        else
                echo > $DEBUGFILE;
                for i in $DEBUG; do
                        echo "break $i" >> $DEBUGFILE
                done;
               	gdb -x $DEBUGFILE -ex "run -o $TMPDIR/a.out $ALOG $AFLAGS $script" $XA
        fi;

        #echo "Killing server (pid $SERVERPID)"
        #kill -TERM $SERVERPID

	if [ -f "${THISDIR}/$script.err" ]; then
	        echo "Comparing file ${script}.err"
        	hexdiff ${THISDIR}/$script.err ${TMPDIR}/$script.err
	fi

        if test "x$COMPAREFILES" != "x"; then
                testname=`basename $script .asm`
		if [ "$script" = "$testname" ]; then
                	testname=`basename $script .a65`
		fi
                for i in $COMPAREFILES; do
                        NAME="${THISDIR}/${i}-${testname}"
                        if test -f ${NAME}; then
                                echo "Comparing file ${i} with ${NAME}"
                                hexdiff ${NAME} $TMPDIR/${i}
                                if test $? -ne 0; then
                                        echo "$script: File ${i} differs!" >&5
                                fi;
                        fi
                        if test -f ${NAME}.gz; then
                                 echo "Comparing file ${i} with ${NAME}.gz"
                                 gunzip -c ${NAME}.gz > ${TMPDIR}/shouldbe_${i}
                                 hexdiff ${TMPDIR}/shouldbe_${i} ${TMPDIR}/${i}
                                 if test $? -ne 0; then
                                        echo "$script: File ${i} differs!" >&5
                                 fi;
                                 rm -f ${TMPDIR}/shouldbe_${i}
                        fi
                done;
        fi

        if test $CLEAN -ge 1; then
                rm -f $DEBUGFILE;
                rm -f $TMPDIR/$script;
                rm -f $TMPDIR/a.out;
        fi
done;

if test $CLEAN -ge 2; then
        echo "Cleaning up directory $TMPDIR"

        for script in $TESTSCRIPTS; do
                rm -f $TMPDIR/$script.log
                rm -f $TMPDIR/$script.err
        done;

        # gzipped test files are unzipped
        for i in $TESTFILES; do
                rm -f $TMPDIR/$i;
        done;

        rm -f $TMPDIR/stdout.log
        rm -f $TMPDIR/stderr.log

        # only remove work dir if we own it (see option -R)
        if test $OWNDIR -ge 1; then
                rmdir $TMPDIR
        fi;
else
        echo "Find debug info in $TMPDIR" >&5
fi;



