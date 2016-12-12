#!/bin/bash

SDCARD=$1
FILE=env.fex

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
    pt_info "Please run as root."
    exit
fi

if [ $# -ne 1 ]; then
    pt_info "Usage:$0 device"
    exit 1
fi

DEV_NAME=`basename $1`
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

cd ../tools/pack/out/ > /dev/null
[ -e ${FILE} ] && dd if=${FILE} of=${SDCARD} bs=1M seek=52
sync
cd -  > /dev/null

pt_info "FINISH: ${FILE} fuse success"
