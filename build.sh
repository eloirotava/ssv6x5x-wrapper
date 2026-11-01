#!/bin/sh
# compila com o kernel de teste local (ARM, 7.2.15), preso nos nucleos lentos
exec taskset -c 2,3 nice -n 19 make -k -j2 KDIR=/root/Projetos/ssv-wrapper/kernel/linux-7.2.15 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- KBUILD_MODPOST_WARN=1 "$@"
