#!/bin/sh

startRTC=$(cat /proc/rtc)
echo "$startRTC"

sleep 2

secondRTC=$(cat /proc/rtc)
echo "$secondRTC"

if [ "$startRTC" == "$secondRTC" ]; then
  /bin/splash_poweron /share/post_rtc.bmp 0 0
  sleep 5
  killall -9 AppMain
  killall -9 USBSrv
  echo "<POST> diagnose RTC .... failed"
  exit 0
fi

echo "<POST> diagnose RTC .... passed"
    
exit 0

