#!/bin/bash

LINUX_PLAT="LINUX platform"
ANDROID_PLAT="ANDROID platform"
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
    eval $@ || exit $?
}

function usage()
{
    echo -e "\033[1;32mUsage: `basename $0` -b board[${H5_BOARD}] -p platform[${H5_PLATFORM}] -t target[${H5_TARGET}]\033[0m"
    exit 1
}

function parse_arg()
{
    if [ $# -lt 4 ]; then
        usage
    fi
    OLD_IFS=${IFS}
    IFS="|"
    while getopts "b:p:t:" opt
    do
        case $opt in
            b )
                BOARD_NAME=$OPTARG
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
                    usage
                fi
                ;;
            p )
                PLATFORM=$OPTARG
                for TMP in ${H5_PLATFORM}
                do
                    if [ ${PLATFORM} = ${TMP} ];then
                        FOUND=1
                        break
                    else
                        FOUND=0
                    fi
                done
                if [ ${FOUND} -eq 0 ]; then
                    pt_error "unsupported platform"
                    usage
                fi
                ;;
            t )
                TARGET=$OPTARG
                for TMP in ${H5_TARGET}
                do
                    if [ ${TARGET} = ${TMP} ];then
                        FOUND=1
                        break
                    else
                        FOUND=0
                    fi
                done
                if [ ${FOUND} -eq 0 ]; then
                    pt_error "unsupported target"
                    usage
                fi
                ;;
            ? )
                usage;;
            esac
    done
    IFS=${OLD_IFS}
}

function pack_lichee_4_linux()
{
    cd ${PRJ_ROOT_DIR}
    execute_cmd "./build.sh pack" 
}

function build_uboot_4_linux()
{
    cd ${PRJ_ROOT_DIR}
    execute_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
    pack_lichee_4_linux
    pt_info "build and pack u-boot for ${LINUX_PLAT} success"    
}

function  build_kernel_4_linux()
{
    cd ${PRJ_ROOT_DIR}
    execute_cmd "echo -e \"1\n2\n1\n\" | ./build.sh config"
    pack_lichee_4_linux
    pt_info "build and pack linux kernel for ${LINUX_PLAT} success"
}

function  build_lichee_4_linux()
{
    cd ${PRJ_ROOT_DIR}
    execute_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
    execute_cmd "echo -e \"1\n2\n1\n\" | ./build.sh config"
    pack_lichee_4_linux
    pt_info "build and pack lichee for ${LINUX_PLAT} success"
}

function build_clean_4_linux()
{
    cd ${PRJ_ROOT_DIR}
    execute_cmd "./build.sh -p sun50iw2p1 -k linux-3.10 -b cheetah-p1 -m clean"
    pt_info "clean lichee for ${LINUX_PLAT} success"
}

function pack_lichee_4_android()
{
    cd ${PRJ_ROOT_DIR}
    if [ -d ../android ]; then
        (cd ../android && source ./build/envsetup.sh && lunch cheetah_fvd_p1-eng && pack)
    else
        pt_error "Android directory not found"
    fi
}

function build_uboot_4_android()
{
    cd ${PRJ_ROOT_DIR}
    execute_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
    pack_lichee_4_android
    pt_info "build and pack u-boot for ${ANDROID_PLAT} success"    
}

function build_lichee_4_android()
{
    cd ${PRJ_ROOT_DIR}
    execute_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
    execute_cmd "echo -e \"1\n0\n\" | ./build.sh config"
    pt_info "build lichee for ${ANDROID_PLAT} success. Please build and pack in Android directory"
}

cd ..
PRJ_ROOT_DIR=`pwd`
H5_BOARD="nanopi-neo2|nanopi-m1-plus2|nanopi-neo-plus2"
H5_PLATFORM="linux|android"
H5_TARGET="all|u-boot|kernel|pack|clean"
BOARD_NAME=none
PLATFORM=linux
TARGET=all
SYS_CONFIG_DIR=${PRJ_ROOT_DIR}/tools/pack/chips/sun50iw2p1/configs/cheetah-p1

parse_arg $@

pt_info "preparing sys_config.fex"
cd ${PRJ_ROOT_DIR}
cp -rvf ${SYS_CONFIG_DIR}/board/sys_config_${BOARD_NAME}.fex ${SYS_CONFIG_DIR}/sys_config.fex
touch ./linux-3.10/.scmversion

if [ "x${PLATFORM}" = "xlinux" ]; then
    if [ "x${TARGET}" = "xpack" ]; then
        pack_lichee_4_linux
        pt_info "pack lichee for ${LINUX_PLAT} success"   
    elif [ "x${TARGET}" = "xu-boot" ]; then
        build_uboot_4_linux
    elif [ "x${TARGET}" = "xkernel" ]; then
        build_kernel_4_linux
    elif [ "x${TARGET}" = "xall" ]; then
        build_lichee_4_linux
    elif [ "x${TARGET}" = "xclean" ]; then
        build_clean_4_linux
    else
        pt_error "unsupported target"
        usage
    fi
elif [ "x${PLATFORM}" = "xandroid" ]; then
    if [ "x${TARGET}" = "xpack" ]; then
        pack_lichee_4_android
        pt_info "pack lichee for ${ANDROID_PLAT} success" 
    elif [ "x${TARGET}" = "xu-boot" ]; then
        build_uboot_4_android
    else
        build_lichee_4_android
    fi
fi