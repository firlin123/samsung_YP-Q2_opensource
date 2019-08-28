#!/bin/sh

echo "[FS] Scan Log Block Start..."

count=1
while true; do 
	if [ "$count" -gt 15 ]; then   # exit condition
		break
	else
		echo "0" > /proc/ftl/scan_logblock
		usleep 30000
#		sleep 1
		count=`expr $count + 1`  # increase counter
	fi
done

echo "[FS] Scan Log Block End..."
