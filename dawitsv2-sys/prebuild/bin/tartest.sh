#!/bin/sh 

error()
{
        echo $@
        echo $@ > /mnt/tmp/log.tartest
		killall -9 $0
		killall -9 tar
}

echo "==================TAR TEST START=========================="
echo "START     : `date`"
killall -9 $0
killall -9 tar
sync
echo "Please Wait 3 Sec..."
sleep 3

if [ -z "$1" ]; then
	echo "TAR FILE : no"
	echo "==================TAR TEST EXIT=========================="
	exit 1
else
	echo "TAR FILE : $1"
fi

if [ -z "$2" ]; then
	echo "TAR TEST DIR : no"
	echo "==================TAR TEST EXIT=========================="
	exit 1
else
	echo "TAR TEST DIR : $1"
fi

no=1
while true; do 
	echo "====================================="
	echo "Tar Test : Run $no"
	echo "====================================="
		rm -rf $2
		mkdir -p $2

        tar xvzf $1 -C $2 || error "Decompress failed at $no" 
        no=`expr $no + 1`
        echo "Removing TAR Test directory" 
        sleep 1
        rm -rf $2
	sleep 1
done 
