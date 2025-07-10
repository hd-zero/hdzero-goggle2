armv7-eabihf--musl--stable-2018.02-2


Toolchains are hosted here: http://toolchains.free-electrons.com/

All the licenses can be found here: http://toolchains.free-electrons.com/downloads/releases/licenses/
All the sources can be found here: http://toolchains.free-electrons.com/downloads/releases/sources/


PACKAGE      VERSION                     LICENSE
buildroot    2018.02-rc2-00006-g39101b7  GPL-2.0+
lzip         1.19                        GPL-2.0+
gcc-final    6.4.0                       unknown
binutils     2.29.1                      GPL-3.0+, libiberty LGPL-2.1+
gmp          6.1.2                       LGPL-3.0+ or GPL-2.0+
m4           1.4.18                      GPL-3.0+
mpc          1.0.3                       LGPL-3.0+
mpfr         3.1.6                       LGPL-3.0+
gcc-initial  6.4.0                       unknown
gdb          7.11.1                      GPL-2.0+, LGPL-2.0+, GPL-3.0+, LGPL-3.0+
expat        2.2.5                       MIT
pkgconf      0.9.12                      pkgconf license
ncurses      6.0                         MIT with advertising clause
patchelf     0.9                         GPL-3.0+
musl           1.1.18   MIT
linux-headers  4.1.49   GPL-2.0
dash           0.5.9.1  BSD-3-Clause, GPL-2.0+ (mksignames.c)
gdb            7.11.1   GPL-2.0+, LGPL-2.0+, GPL-3.0+, LGPL-3.0+

For those who would like to reproduce the toolchain, you can just follow these steps:

    git clone https://github.com/free-electrons/buildroot-toolchains.git buildroot
    cd buildroot
    git checkout 39101b773ea178b9190f295bb0aea669b76cd0af

    curl http://toolchains.free-electrons.com/downloads/releases/toolchains/armv7-eabihf/build_fragments/armv7-eabihf--musl--stable-2018.02-2.defconfig > .config
    make olddefconfig
    make

This toolchain has been built, and the test system built with it has
successfully booted.
This doesn't mean that this toolchain will work in every cases, but it is at
least capable of building a Linux kernel with a basic rootfs that boots.
FLAG: TEST-OK
