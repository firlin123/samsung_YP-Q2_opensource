#!/bin/bash
DIR="q2update_unpacked"
Q2U="Q2Update.dat"
DCU="decompressed_upd.tar"
if [ -d "$DIR" ]; then
	echo "Removing old $DIR..."
	rm -rf $DIR
	echo "Creating new $DIR..."
else
	echo "Creating $DIR..."
fi
mkdir $DIR
if [ ! -f "$Q2U" ]; then
	echo "File $Q2U does not exists."
	echo "Removing $DIR..."
	rmdir $DIR
	exit 1;
else
	echo "Decryping $Q2U..."
	./bin/Q2Crypt -dec $Q2U $DIR/$DCU > /dev/null
	if [ ! -f "$DIR/$DCU" ]; then
		echo "Decryping $Q2U failed"
		echo "Removing $DIR..."
		rmdir $DIR
		exit 1;
	else
		echo "Unpacking $DCU..."
		tar xf $DIR/$DCU -C $DIR/
		echo "Unpacking appfs..."
		fakeroot -s $DIR/.fakeroot cramfsck -x $DIR/appfs $DIR/appfs.cramfs
	fi
fi
