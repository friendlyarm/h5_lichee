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

pt_info "This script is only for Linux."
cd ..
ROOT=`pwd`
execute_cmd "cd ./brandy && ./build.sh -p sun50iw2p1 && cd -"
execute_cmd "touch ./linux-3.10/.scmversion"
execute_cmd "echo -e \"1\n2\n1\n\" | ./build.sh config && ./build.sh pack"
cd ${ROOT}/fa_tools
pt_info "build lichee for Linux success"