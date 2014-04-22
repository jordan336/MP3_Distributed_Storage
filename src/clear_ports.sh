#!/bin/bash
# Clears all ports needed for snapshot

for i in 15457 15458 15459 15460 15461 15462
do
	fuser $i/udp -k
done

for x in 25457 25458 25459 25460 25461 25462
do
	fuser $x/udp -k
done

for x in 35457 35458 35459 35460 35461 35462
do
	fuser $x/udp -k
done

