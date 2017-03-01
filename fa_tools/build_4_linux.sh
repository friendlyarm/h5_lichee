#!/bin/bash
#set -x
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

function execute_cmd() 
{
    pt_info "==> Executing: '${@}'"
    eval $@ || exit $?
}

function parse_arg()
{
    if [ $# -lt 2 ]; then
        pt_warn "Usage:`basename $0` -b board_name"
        exit 1
    fi
    while getopts "b:" opt
    do
        case $opt in
            b )
                BOARD_NAME=$OPTARG;;
            ? )
                pt_warn "Usage:`basename $0` -b board_name"
                exit 1;;
            esac
    done

    for TMP in ${H5_BOARD}
    do
        if [ ${BOARD_NAME} = ${TMP} ];then
            FOUND=1
            break
        else
            FOUND=0
        fi
    done
    if [ ${FOUND} -eq 0 ]; then
        pt_error "unsupported board"
        exit 1
    fi
}

cd ..
PRJ_ROOT_DIR=`pwd`
H5_BOARD="nanopi-neo2 nanopi-m1-plus2"
BOARD_NAME=none
SYS_CONFIG_DIR=${PRJ_ROOT_DIR}/tools/pack/chips/sun50iw2p1/configs/cheetah-p1

parse_arg $@

# prepare sys_config.fex
pt_info "This script is only for Linux platform"
cd ${PRJ_ROOT_DIR}
pt_info "preparing sys_config.fex"
cp -rvf ${SYS_CONFIG_DIR}/board/sys_config_${BOARD_NAME}.fex ${SYS_CONFIG_DIR}/sys_config.fex

# build u-boot & Linux
cd ${PRJ_ROOT_DIR}
execute_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
execute_cmd "touch ./linux-3.10/.scmversion"
execute_cmd "echo -e \"1\n2\n1\n\" | ./build.sh config && ./build.sh pack"

cd ${PRJ_ROOT_DIR}/fa_tools
pt_info "build lichee for Linux success"