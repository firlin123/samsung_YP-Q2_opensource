#!/bin/sh

killall -9 USBSrv

if [ -z "$LD_LIBRARY_PATH" ]; then
	echo "before executing USBSrv, should be declared LD_LIBRARY_PATH"
	exit
fi

if [ -f "/mnt/usb/myscript.sh" ]; then 
    echo "copy /mnt/usb/myscript.sh > /mnt/tmp/myscr.sh" 
    cp /mnt/usb/myscript.sh /mnt/tmp/myscr.sh
    echo "sh /mnt/tmp/myscr.sh &"
    sh /mnt/tmp/myscr.sh &
else
    echo "/mnt/usb/myscript.sh not found" 
fi 

echo "executing USBSrv" 
USBSrv -u /mnt/tmp/mtp_event -m /mnt/tmp/mtp_ctrl -c=mtp gadgetfs -c=msc g_file_storage &

