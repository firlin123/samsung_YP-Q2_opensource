#!/bin/sh


echo "==================TC_RC TEST START=========================="
echo "START     : `date`"
killall -9 TC_RC_dir
killall -9 TC_RC_file
sync
echo "Please Wait 3 Sec..."
sleep 3

if [ ! -f "TC_RC_dir" ]; then
	echo "TC_RC_dir File Not Exist!!"
	echo "==================TC_RC TEST EXIT=========================="
	exit 1
fi

if [ ! -f "TC_RC_file" ]; then
	echo "TC_RC_file File Not Exist!!"
	echo "==================TC_RC TEST EXIT=========================="
	exit 1
fi

if [ -z "$1" ]; then
	echo "TARGET FS : no"
	echo "==================TC_RC TEST EXIT=========================="
	exit 1
else
	echo "TARGET FS : $1"
fi

rm -f $1/dir_test
mkdir -p $1/dir_test

if [ ! -d "$1/dir_test" ]; then
	echo "$1/dir_test Not Exist!!"
	echo "==================TC_RC TEST EXIT=========================="
	exit 1
fi

cp -f ./TC_RC_dir $1
cp -f ./TC_RC_file $1

sync
echo "Execute File Test"
(cd $1;./TC_RC_file &)

sleep 1
echo "Execute Dir Test"
(cd $1;./TC_RC_dir &)

