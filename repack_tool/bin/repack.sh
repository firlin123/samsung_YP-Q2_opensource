#!/bin/bash
GZIP=-9
DIR="q2update_unpacked"
Q2U="Q2Update.dat"
Q2U_O="Q2Update.original.dat"
DCU="decompressed_upd.tar"
AFS="appfs.cramfs"
AFS_D="appfs"

if [ -f "$DIR/$AFS" ]; then
	echo "Removing old $DIR/$AFS..."
	rm $DIR/$AFS
fi

echo "Repacking $DIR/$AFS..."
fakeroot -i $DIR/.fakeroot mkcramfs $DIR/$AFS_D $DIR/$AFS
echo "Checking $DIR/$AFS..."
if [ ! -f "$DIR/$AFS" ]; then
	echo "File $DIR/$AFS does not exists."
	exit 1;
else
	cramfsck $DIR/$AFS
	if [ -f "$DIR/$DCU" ]; then
		echo "Removing old $DIR/$DCU..."
		rm -rf $DIR/$DCU
	fi
	echo "Repacking $DIR/$DCU..."
	cd $DIR && tar -cf $DCU --exclude="$AFS_D" --exclude=".fakeroot" * && cd ..
	if [ ! -f "$DIR/$DCU" ]; then
		echo "Repack $DIR/$DCU failed"
		exit 1;
	else
		if [ -f "$Q2U" ]; then
			mv $Q2U $Q2U_O
		fi
		echo "Encryping $Q2U..."
		! ./bin/Q2Crypt -enc $DIR/$DCU $Q2U > /dev/null
	fi
fi
