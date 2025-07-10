#!/bin/bash
set -e

if [ ! -d toolchain ]; then
	echo "Extracting toolchain..."
	mkdir toolchain && cd toolchain && tar xjf "../armv7-eabihf--musl--stable-2018.02-2.tar.bz2" --strip-components=1 && cd ..
	cd lib/softwinner/lib && tar xzvf softwinner.lib.tar.gz  && cd ../../../
	cd lib/ffmpeg/lib && tar xzvf ffmpeg.lib.tar.gz && cd ../../../
fi

rm -rf build && mkdir build
cmake . -DCMAKE_TOOLCHAIN_FILE=toolchain/share/buildroot/toolchainfile.cmake -Bbuild
