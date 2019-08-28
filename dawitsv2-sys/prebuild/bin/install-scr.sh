#!/bin/sh 

cp /share/Updating_bg.bmp /mnt/tmp 
cp /share/000150_Updating_firmware.bmp /mnt/tmp 

/bin/splash_poweron /mnt/tmp/Updating_bg.bmp 0 150
/bin/splash_poweron /mnt/tmp/000150_Updating_firmware.bmp 0 150
usleep 500000

while true; do 
    /bin/splash_poweron /mnt/tmp/Updating_bg.bmp 0 150
    usleep 500000
    /bin/splash_poweron /mnt/tmp/000150_Updating_firmware.bmp 0 150
    usleep 500000
done 
