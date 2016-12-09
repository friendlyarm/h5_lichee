#!/bin/bash

function pt_error()
{
    echo -e "\033[1;31mERROR: $*\033[0m"
}

function pt_warn()
{
    echo -e "\033[1;31mWARN: $*\033[0m"
}

function pt_info()
{
    echo -e "\033[1;32mINFO: $*\033[0m"
}

pt_info "This script is only for ANDROID."
cd ..
(cd brandy && ./build.sh -p sun50iw2p1 && cd -)
echo -e "1\n0\n" | ./build.sh config

cd ../android
source ./build/envsetup.sh
lunch cheetah_fvd_p1-eng
extract-bsp
if [ "x${1}" = "xboot.img" ]; then
    make bootimage
else 
    make -j8
fi
pack
