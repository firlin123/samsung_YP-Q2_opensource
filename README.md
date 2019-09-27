# Kernel, rootfs and some libs source code for the Samsung YP-Q2

123

# build and install

* **install ubuntu trusty (14.04) i386 on virtual machine** 
* **open terminal**
* **download install needed packages:**

```
sudo apt-get update
sudo apt-get install -y git libcurl4-openssl-dev fakeroot gcc cramfsprogs automake1.4
cd ~ && wget https://moonbutt.science/firlin4pda/samsung_YP-Q2_opensource/raw/branch/master/git/git_1.9.1-1ubuntu0.10_i386.deb
sudo dpkg -i git_1.9.1-1ubuntu0.10_i386.deb
```
* **clone repositories:**

```
git clone https://moonbutt.science/firlin4pda/samsung_YP-Q2_opensource.git src
git clone https://github.com/firlin123/arm-none-linux-gnueabi.git toolchain
```
* **set up toolchain:**

```
export PATH=~/toolchain/bin:$PATH
```
* **build kernel and rootfs:**

```
cd ~/src/dawitsv2-sys
make
```
* **build libraries:**

```
cd ~/src/dawitsv2-mid
make
```
* **download original firmware (Q2V1.23.zip)**
* **repack firmware:**

```
cp ~/Downloads/Q2V1.23.zip ~/src/repack_tool/
cd ~/src/repack_tool/
unzip Q2V1.23.zip
sh unpack.sh
cp ../dawitsv2-sys/build/zImage ../dawitsv2-sys/build/rootfs.sqfs q2update_unpacked/
cp scripts/* q2update_unpacked/appfs/bin/
sudo cp ../dawitsv2-mid/target/appfs/lib/* q2update_unpacked/appfs/lib/
sh repack.sh
```
* **install:**

     **connect player to the virtual machine and copy Q2Update.dat and myscript.sh to the root directory of the player**


To install original firmware copy Q2Update.original.dat to the player and rename it to Q2Update.dat


