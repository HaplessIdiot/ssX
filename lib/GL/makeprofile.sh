#!/bin/sh

RUNDIR=$1

[ -f ${RUNDIR}/glx_lowpc ] || { echo "no file ${RUNDIR}/glx_lowpc" ; exit 1 } 
[ -f ${RUNDIR}/gmon.out ] || { echo "no file ${RUNDIR}/gmon.out" ; exit 1 }

rm -f glx_lowpc gmon.out glxsyms profile
ln -s ${RUNDIR}/glx_lowpc . || exit 1
ln -s ${RUNDIR}/gmon.out . || exit 1

ld -o glxsyms -noinhibit-exec --whole-archive -Ttext=`cat glx_lowpc` libGL.a 2> /dev/null && \
gprof glxsyms < gmon.out > profile
