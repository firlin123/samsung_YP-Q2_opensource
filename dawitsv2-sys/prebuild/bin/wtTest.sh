#!/bin/sh

error()
{
	echo $@
#	echo $@ > /mnt/tmp/log.rfstest
#	./killtest.sh
#	killall -9 sh
	exit 1
}             

#export PATH=$PATH:/z-proj/bin:/mnt/app:/mnt/usb

# don't print the other debug message
#echo 0 > /proc/sys/kernel/printk

PART1_SIZE=65536
PART2_SIZE=7865952

PART1_COUNT=100
PART2_COUNT=5

umount /mnt/app
sleep 3
./wtSectorTest -d /dev/ufda1 -r 0 -s "$PART1_SIZE" -t "$PART1_COUNT" || error "RFS test died at $no"

umount /mnt/app
sleep 3
./wtSectorTest -d /dev/ufda2 -r 0 -s "$PART2_SIZE" -t "$PART2_COUNT" || error "RFS test died at $no"

mount -t vfat /dev/ufda1 /mnt/app
sleep 3
count=1
while true; do 
	# exit condition
	if [ "$count" -gt "$PART1_COUNT" ]; then
		break;
	else
		echo "=================================="
		echo "Rfs Test : Run $count"
		echo "=================================="
	fi
	
	# run test code
	./wtSectorTest -d /mnt/app/test.img -r 0 -s "$PART1_SIZE" -t 1 || error "RFS test died at $no"

	# increase counter
	count=`expr $count + 1`

	echo "Removing" 
	rm -f /mnt/app/test.img

	sleep 3
done 

mount -t vfat /dev/ufda2 /mnt/usb
sleep 3
count=1
while true; do 
	# exit condition
	if [ "$count" -gt "$PART2_COUNT" ]; then
		break;
	else
		echo "=================================="
		echo "Rfs Test : Run $count"
		echo "=================================="
	fi
	
	# run test code
	./wtSectorTest -d /mnt/usb/test.img -r 0 -s "$PART2_SIZE" -t 1 || error "RFS test died at $no"

	# increase counter
	count=`expr $count + 1`

	echo "Removing" 
	rm -f /mnt/usb/test.img

	sleep 3
done 

echo "Test Ended"
