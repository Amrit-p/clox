#!/usr/bin/env bash
program="compiled/fu.out"
comman_cmd="$program examples/native-functions.p"
if [ "$1" == "-val" ]; then
    make -B DEBUG=1 && valgrind --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        --log-file=valgrind-out.txt \
        $comman_cmd
elif [ "$1" == "-gf" ]; then
    make -B DEBUG=1 && gf2 $program
elif [ "$1" == "-gdb" ]; then
    make -B DEBUG=1 && gdb $program
elif [ "$1" == "-trace" ]; then
    make -B DEBUG=1 DEBUG_TRACE_EXECUTION=1 && $comman_cmd
else
    make -B LOG=1 && $comman_cmd
fi
