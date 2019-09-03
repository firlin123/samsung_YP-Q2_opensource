# Kernel, rootfs and some libs source code for the Samsung YP-Q2

123

# build and install

* **install ubuntu trusty (14.04) i386 on virtual machine** 
* **open terminal**
* **login as root:** 
``` 
$ sudo -i
```

* **download install needed packages:**
```
# apt-get update
# apt-get install -y git libcurl4-openssl-dev fakeroot gcc cramfsprogs
# cd ~ && wget https://moonbutt.science/firlin4pda/samsung_YP-Q2_opensource/raw/branch/master/git/git_1.9.1-1ubuntu0.10_i386.deb
# dpkg -i git_1.9.1-1ubuntu0.10_i386.deb
```
* **clone repositories:**
```
# git clone https://moonbutt.science/firlin4pda/samsung_YP-Q2_opensource.git src
# git clone https://github.com/firlin123/arm-none-linux-gnueabi.git toolchain
```
* **set up toolchain:**
```
# export PATH=$PWD/toolchain/bin:$PATH
```
* **build:**
```
# cd ~/src/dawitsv2-sys
# make
```
* **download original firmware (Q2V1.23.zip)**
* **repack firmware:**
```
# cp /home/*/Downloads/Q2V1.23.zip ~/src/repack_tool/
# cd ~/src/repack_tool/
# unzip Q2V1.23.zip
# sh unpack.sh
# cp ../dawitsv2-sys/build/zImage ../dawitsv2-sys/build/rootfs.sqfs q2update_unpacked/
# cp scripts/* q2update_unpacked/appfs/bin/
# sh repack.sh
# nautilus .
```
* **install:**

     **connect player to the virtual machine and copy Q2Update.dat and myscript.sh to the root directory of the player**


To install original firmware copy Q2Update.original.dat to the player and rename it to Q2Update.dat
