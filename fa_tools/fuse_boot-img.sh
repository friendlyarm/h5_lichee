#!/bin/bash

SDCARD=$1
FILE=boot.fex
POSITION=68                 # default dragonboard

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

if [ $UID -ne 0 ]
    then
    pt_error "Please run as root."
    exit
fi

if [ $# -ne 2 ]; then
    pt_error "Usage:$0 device system"
    exit 1
fi

DEV_NAME=`basename $1`
SYSTEM=${2}
BLOCK_CNT=`cat /sys/block/${DEV_NAME}/size`
if [ $? -ne 0 ]; then
    pt_error "Can't find device ${DEV_NAME}"
    exit 1
fi

if [ ${BLOCK_CNT} -le 0 ]; then
    pt_error "NO media found in card reader."
    exit 1
fi

if [ ${BLOCK_CNT} -gt 64000000 ]; then
    pt_error "Block device size (${BLOCK_CNT}) is too large"
    exit 1
fi

if [ "x${SYSTEM}" = x"android" ]; then
    POSITION=68
elif [ "x${SYSTEM}" = x"dragonboard" ]; then
    POSITION=68
else
    pt_error "This script is only for ANDROID/DRAGON."
    exit 1
fi
cd ../tools/pack/out/ > /dev/null
[ -e ${FILE} ] && dd if=${FILE} of=${SDCARD} bs=1M seek=${POSITION}
sync
cd -  > /dev/null

pt_info "FINISH: boot.img fuse success"
