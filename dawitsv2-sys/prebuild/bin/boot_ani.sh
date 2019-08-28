#!/bin/sh 

#cp /share/Updating_bg.bmp /mnt/tmp 
#cp /share/000150_Updating_firmware.bmp /mnt/tmp 

#/bin/splash_poweron /mnt/tmp/Updating_bg.bmp 0 150
#/bin/splash_poweron /mnt/tmp/000150_Updating_firmware.bmp 0 150
#usleep 500000

var=1
var2=96

while true; do 
#  echo "var = $var"
#	/bin/splash_poweron /appfs/pixmaps/Q2/Power/000000_power_on_$var.bmp 0 0
  if [ $var -le 9 ]; then
    /bin/splash_poweron /appfs/pixmaps/Q2/Power/000000_power_on_0000$var.bmp 0 0
    #/bin/splash_poweron ./000000_power_on_0000$var.bmp 0 0
  fi
  
  if [ $var -ge 10 ]; then
    /bin/splash_poweron /appfs/pixmaps/Q2/Power/000000_power_on_000$var.bmp 0 0
    #/bin/splash_poweron ./000000_power_on_000$var.bmp 0 0
  fi
  
	let "var = $var+1"
#	usleep 50000
	if [ $var -eq $var2 ]; then
		break
	fi
done 
