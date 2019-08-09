#!/usr/bin/env bash

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -n 64 -s 512 -tech 45 -compute_sink_region_mode -t 0 > log.txt

if cmp -s "sol_0.txt" "golden/sol_0.txt";
then
	exit 0
else
	exit 2
fi
