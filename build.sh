#!/bin/sh
# Compilacao cruzada para ARM. Aponte KDIR para a arvore do kernel alvo:
#   KDIR=/caminho/linux-7.2.15 ./build.sh
: "${KDIR:?defina KDIR com o caminho da arvore do kernel}"
exec make -k -j2 KDIR="$KDIR" ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- KBUILD_MODPOST_WARN=1 "$@"
