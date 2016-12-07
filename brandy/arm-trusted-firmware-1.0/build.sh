# build.sh
#
# (c) Copyright 2013
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# Martin.Zheng <zhengjiewen@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.


BUILD_CHIP="Sun8iW1"
BUILD_CONFIG_FILE=".config"
set -e

build_select_chip()
{

	local count=0
	printf "All valid Sunxi chip:\n"
	for chip in $( find plat -mindepth 1 -maxdepth 1 -type d -name "sun[0-9]*" |sort); do
		chips[$count]=`basename $chip`
		printf "$count. ${chips[$count]}\n"
		let count=$count+1
	done

	while true; do
		read -p "Please select a chip:"
		RES=`expr match $REPLY "[0-9][0-9]*$"`
		if [ "$RES" -le 0 ]; then
			echo "please use index number"
			continue
		fi
		if [ "$REPLY" -ge $count ]; then
			echo "too big"
			continue
		fi
		if [ "$REPLY" -lt "0" ]; then
			echo "too small"
			continue
		fi
		break
	done
    BUILD_CHIP=${chips[$REPLY]}
}


build_select_build_spd()
{
	local count=0
	local length=0

	build_spd=(none tspd)
	printf "All valid SPD type:\n"

	length=`expr ${#build_spd[@]} - 1`
	for count in `seq 0 $length`; do
		printf "$count. ${build_spd[$count]}\n"
	done

	let count=$count+1

	while true; do
		read -p "Please select a build type:"
		RES=`expr match $REPLY "[0-9][0-9]*$"`
		if [ "$RES" -le 0 ]; then
			echo "please use index number"
			continue
		fi
		if [ "$REPLY" -ge $count ]; then
			echo "too big"
			continue
		fi
		if [ "$REPLY" -lt "0" ]; then
			echo "too small"
			continue
		fi
		break
	done

	BUILD_SPD=${build_spd[$REPLY]}

}

build_get_config_form_user()
{
	build_select_chip
	build_select_build_spd
}

build_write_config_to_file()
{
	rm -rf $BUILD_CONFIG_FILE
	echo "BUILD_CHIP       :${BUILD_CHIP}" >> $BUILD_CONFIG_FILE
	echo "BUILD_SPD        :$BUILD_SPD" >> $BUILD_CONFIG_FILE
}

build_get_config_from_file()
{
	BUILD_CHIP=`cat $BUILD_CONFIG_FILE | awk -F"[:|=]" '(NF&&$1~/^[[:space:]]*BUILD_CHIP/) {printf "%s",$2}'`
	BUILD_SPD=`cat $BUILD_CONFIG_FILE | awk -F"[:|=]" '(NF&&$1~/^[[:space:]]*BUILD_SPD/) {printf "%s",$2}'`
}

build_show_config()
{
	printf "\nconfig information is:\n"
	echo -e '\033[0;31;36m'
	printf "BUILD_CHIP       : ${BUILD_CHIP%%Pkg*}\n"
	printf "BUILD_SPD        : $BUILD_SPD\n"
	echo -e '\033[0m'
}

build_show_help()
{
printf "
 (c) Copyright 2013
 Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 wangwei <wangwei@allwinnertech.com>

NAME
	build - The top level build script to build Sunxi platform ATF

SYNOPSIS
	build [-h] | [clean] [distclean] | [showconfig]

OPTIONS
	-h             Display help message
	clean          clean the compile file in the output dir
	config         config the platform which we want to build
	distclean      clean all the compile file
	showconfig     show the current compile config

"

}


if [ "$1" == "-h" ]; then
	build_show_help
	exit
fi

if [[ "$1" == config ]]; then
	build_get_config_form_user
	build_write_config_to_file
	exit
fi

if [ -f $BUILD_CONFIG_FILE ]; then
	build_get_config_from_file
else
	echo -e '\033[0;31;36m'
	echo "you should run ./build.sh config at first"
	echo -e '\033[0m'
	exit
fi

echo "BUILD_CHIP = $BUILD_CHIP SPD = $BUILD_SPD "
#
# Build the edk2 SunxiPlatform code
#

for arg in "$@"
do
	if [[ $arg == clean ]]; then
		echo "clean the build..."
		make PLAT=$BUILD_CHIP SPD=$BUILD_SPD clean
		exit
	elif [[ $arg == distclean ]]; then
		echo "distclean the build..."
		make PLAT=$BUILD_CHIP SPD=$BUILD_SPD distclean
		rm -rf $BUILD_CONFIG_FILE
		exit
	elif [[ $arg == showconfig ]]; then
		build_show_config
		exit
	fi
done

build_show_config
make PLAT=$BUILD_CHIP SPD=$BUILD_SPD

