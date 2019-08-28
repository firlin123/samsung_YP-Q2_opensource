#!/bin/sh

SI4703ID="1242"
readID=$(cat /proc/fm)

if [ "$SI4703ID" == "$readID" ] 
then
	echo '<POST> diagnose FM tuner .... passed'
else
	echo '<POST> diagnose FM tuner .... failed'
	/bin/splash_poweron /share/post_fm_error.bmp 0 0

	sleep 10
	#while true; do
        #        continue
        #done
fi

exit 0
