#!/bin/sh 

echo "temporary.. "
exit 0

echo "" > missing
make 
cp -v .libs/libSDL_ttf-2.0.so.0 ../../target/A16/lib/
