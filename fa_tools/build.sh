#!/bin/bash -u

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

function run_cmd() 
{
    if [ "x${DEBUG}" = "xyes" ]; then
        pt_info "                =>cmd: ${@}"
    fi
    
    eval $@
    local ret=$?
    if [ ${ret} -ne 0 ]; then
        pt_error "$@ fail"
        exit  ${ret}
    fi
}

function print_func()
{
    if [ "x${DEBUG}" = "xyes" ]; then
        pt_info "   =>func: $@"
    fi
}

function DBG_INFO()
{
    if [ "x${DEBUG}" = "xyes" ]; then
        if [ "x${1}" = "xfilename" ]; then
            pt_warn "=>file: ${2}"
        else
            pt_info "        =>func: $@"
        fi
    fi
}

function usage()
{
    echo -e "\033[1;32mUsage: `basename $0` -b board[${H5_BOARD}] -p platform[${H5_PLATFORM}] -t target[${H5_TARGET}]\033[0m"
    exit 1
}

function parse_arg()
{
    if [ $# -ne 6 ]; then
        usage
    fi

    local found=0
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
                        found=1
                        break
                    else
                        found=0
                    fi
                done
                if [ ${found} -eq 0 ]; then
                    pt_error "unsupported board"
                    usage
                fi
                ;;
            p )
                PLATFORM=$OPTARG
                for TMP in ${H5_PLATFORM}
                do
                    if [ ${PLATFORM} = ${TMP} ];then
                        found=1
                        break
                    else
                        found=0
                    fi
                done
                if [ ${found} -eq 0 ]; then
                    pt_error "unsupported platform"
                    usage
                fi
                ;;
            t )
                TARGET=$OPTARG
                for TMP in ${H5_TARGET}
                do
                    if [ ${TARGET} = ${TMP} ];then
                        found=1
                        break
                    else
                        found=0
                    fi
                done
                if [ ${found} -eq 0 ]; then
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
    DBG_INFO "$FUNCNAME" $@

    cd ${PRJ_ROOT_DIR}
    run_cmd "./build.sh pack" 

    find ./${AW_KER}/output/lib/modules/ -name \*.ko \
    	| xargs ./brandy/toolchain/gcc-aarch64/bin/aarch64-linux-gnu-strip --strip-unneeded
    (cd ./${AW_KER}/output/lib/modules/ && tar czf ${FA_OUT}/3.10.65.tar.gz 3.10.65)
    run_cmd "cp ./${AW_KER}/output/boot.img ${FA_OUT}/"
    run_cmd "cp ./${AW_KER}/output/rootfs.cpio.gz ${FA_OUT}/"
    run_cmd "cp ./tools/pack/out/boot0_sdcard.fex ${FA_OUT}/"
    run_cmd "cp ./tools/pack/out/boot_package.fex ${FA_OUT}/"

    tree ${FA_OUT}
}

function build_uboot_4_linux()
{
    cd ${PRJ_ROOT_DIR}
    run_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
    pack_lichee_4_linux
    pt_info "Build and pack u-boot for ${LINUX_PLAT_MSG} success"    
}

function  build_kernel_4_linux()
{
    cd ${PRJ_ROOT_DIR}
    run_cmd "echo -e \"1\n2\n1\n\" | ./build.sh config"
    pack_lichee_4_linux
    pt_info "Build and pack linux kernel for ${LINUX_PLAT_MSG} success"
}

function  build_lichee_4_linux()
{
    cd ${PRJ_ROOT_DIR}
    run_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
    run_cmd "echo -e \"1\n2\n1\n\" | ./build.sh config"
    pack_lichee_4_linux
    pt_info "Build and pack lichee for ${LINUX_PLAT_MSG} success"
}

function build_clean_4_linux()
{
    cd ${PRJ_ROOT_DIR}
    run_cmd "./build.sh -p sun50iw2p1 -k linux-3.10 -b cheetah-p1 -m clean"
    pt_info "Clean lichee for ${LINUX_PLAT_MSG} success"
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
    run_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
    pack_lichee_4_android
    pt_info "Build and pack u-boot for ${ANDROID_PLAT_MSG} success"    
}

function build_lichee_4_android()
{
    cd ${PRJ_ROOT_DIR}
    run_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
    run_cmd "echo -e \"1\n0\n\" | ./build.sh config"
    pt_info "Build lichee for ${ANDROID_PLAT_MSG} success. Please build and pack in Android directory"
}

function prepare_toolchain()
{
    local src_dir=${PRJ_ROOT_DIR}/fa_tools/toolchain_h5_linux-3.10
    local target_dir=${PRJ_ROOT_DIR}/brandy/toolchain

    if [ ! -e ${target_dir}/gcc-linaro-aarch64.tar.xz ] || [ ! -e ${target_dir}/gcc-linaro-arm-4.6.3.tar.xz ]; then
        [ -d ${src_dir} ] || git clone https://github.com/friendlyarm/toolchain_h5_linux-3.10 ${src_dir} --depth 1 -b master
        run_cmd "cp ${src_dir}/gcc-linaro-arm-4.6.3.tar.xz ${target_dir}/"
        run_cmd "cat ${src_dir}/gcc-linaro-aarch64.tar.xz* >${target_dir}/gcc-linaro-aarch64.tar.xz"
    fi
}

DEBUG="no"
cd ..
PRJ_ROOT_DIR=`pwd`
H5_BOARD="nanopi-neo2|nanopi-m1-plus2|nanopi-neo-plus2"
H5_PLATFORM="linux|android"
H5_TARGET="all|u-boot|kernel|pack|clean"
BOARD_NAME=none
PLATFORM=linux
TARGET=all
SYS_CONFIG_DIR=${PRJ_ROOT_DIR}/tools/pack/chips/sun50iw2p1/configs/cheetah-p1
AW_KER=linux-3.10

LINUX_PLAT_MSG="Linux platform"
ANDROID_PLAT_MSG="Android platform"

# friendlyelec attribute
FA_OUT=${PRJ_ROOT_DIR}/fa_out
parse_arg $@

cd ${PRJ_ROOT_DIR}
mkdir -p ${FA_OUT}

pt_info "preparing sys_config.fex"
cp -rvf ${SYS_CONFIG_DIR}/board/sys_config_${BOARD_NAME}.fex ${SYS_CONFIG_DIR}/sys_config.fex
touch ./linux-3.10/.scmversion

prepare_toolchain
if [ "x${PLATFORM}" = "xlinux" ]; then
    if [ "x${TARGET}" = "xpack" ]; then
        pack_lichee_4_linux
        pt_info "Pack lichee for ${LINUX_PLAT_MSG} success"   
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
        pt_info "Pack lichee for ${ANDROID_PLAT_MSG} success" 
    elif [ "x${TARGET}" = "xu-boot" ]; then
        build_uboot_4_android
    else
        build_lichee_4_android
    fi
fi