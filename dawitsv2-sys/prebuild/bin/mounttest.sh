#!/bin/sh 


error()
{
        echo $@
        echo $@ > /mnt/tmp/log.tartest
		killall -9 $0
		killall -9 tar
}

echo "==================MOUNT TEST START=========================="
echo "START     : `date`"
killall -9 $0
killall -9 tar
sync
echo "Please Wait 3 Sec..."
sleep 3

if [ -z "$1" ]; then
	echo "TAR FILE : no"
	echo "==================MOUNT TEST EXIT=========================="
	exit 1
else
	echo "TAR FILE : $1"
fi

if [ -z "$2" ]; then
	echo "TAR TEST Partition : no"
	echo "==================MOUNT TEST EXIT=========================="
	exit 1
else
	echo "TAR TEST Partition : $1"
fi

mount $2 

no=1
while true; do 
	echo "======================"
	echo "MOUNT TEST : Run $no" 
	echo "======================"
	
	cd $2
	echo "decompress"
	tar xvzf $1 || error "Decompress Error at $no" 
	no=`expr $no + 1`
	cd /
	echo "unmount $2"
	umount $2
	sleep 1
	echo "dosfsck"
	if [ "$2"="/mnt/app" ]; then
		dosfsck /dev/ufda1  || error "ERR Fsck at /dev/ufda1"		
	else
		dosfsck /dev/ufda2  || error "ERR Fsck at /dev/ufda2"		
	fi
	
	sleep 1
	echo "remount $2"
	mount $2 
	sleep 1
	
done 
