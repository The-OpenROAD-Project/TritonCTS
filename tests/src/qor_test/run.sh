#!/usr/bin/env bash

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -n 64 -s 512 -tech 45 -compute_sink_region_mode -t 0 > log.txt

skew=$(grep "skew" log.txt | sed -n 2p |  awk '{print $3}')
golden_skew=$(grep "skew" golden/log.txt | sed -n 2p |  awk '{print $3}')
upper_limit=$((golden_skew+5))
lower_limit=$((golden_skew-5))

if [ $skew == $golden_skew ]
then
	exit 0
elif [ $skew -gt $lower_limit ] && [ $skew -lt $upper_limit ]
then
	exit 1
else
	exit 2
fi
