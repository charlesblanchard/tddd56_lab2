#!/bin/bash

for i in `seq 1 8`;
do	
	echo
	echo $i
	make NON_BLOCKING=$1 MEASURE=$2 NB_THREADS=$i > /dev/null
	
	for i in `seq 1 100`;
	do
		./stack
	done
done
