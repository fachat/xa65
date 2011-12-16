#!/bin/bash
#
#    xa65 - 6502 cross assembler and utility suite
#    mkrom.sh - assemble several 'romable' files into one binary
#    Copyright (C) 1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

XA="../xa"
FILE=../file65

start=32768
ende=65536
romfile=rom65

next=$[ start + 2 ];
pars="-A $next"

umask 077

tmp1=/tmp/mkrom65.sh.$$.a
tmp2=/tmp/mkrom65.sh.$$.b
tmp3=/tmp/mkrom65.sh.$$.c

echo -e "#include <stdio.h>\nvoid main(int argc, char *argv[]) { printf(\"%c%c\",atoi(argv[1])&255,(atoi(argv[1])/256)&255);}"  \
	> $tmp3;
cc $tmp3 -o $tmp2

while [ $# -ne 0 ]; do 
  case $1 in
  -A)	# get start address
	start=$[ $2 ];
	shift
	;;
  -E)	# get end address
	ende=$[ $2 ];
	shift
	;;
  -R)   # get ROM filename
	romfile=$2;
	shift
	;;
  -O)	# xa options
	XA="$XA $2";
	shift
	;;
  -S)   # segment addresses - in xa option format
	pars="$pars $2";
	shift
	;;
  *)
	break;
	;;
  esac;
  shift
done

#get file list 
list="$*"


echo -n > $romfile

for i in $list; do 
  #echo "next=$next, start=$start, pars=$pars"
  #echo "cmd= ${XA} -R $pars -o $tmp1 $i"
  $XA -R $pars -o $tmp1 $i
  pars=`$FILE -a 2 -P $tmp1`;
  next=`$FILE -A 0 $tmp1`;

  $tmp2 $next >> $romfile
  cat $tmp1 >> $romfile;

done;

$tmp2 65535 >> $romfile

rm -f $tmp1 $tmp2 $tmp3

