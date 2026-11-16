#!/bin/bash
set -euo pipefail
: "${PS4SDK:?Set PS4SDK}"
: "${OO_PS4_TOOLCHAIN:?Set OO_PS4_TOOLCHAIN}"
cd "$(dirname "$0")"
cd plugins
bash int3.sh 4096 > common/cave.inc.c
for mod in plugin_bootloader plugin_loader plugin_mono plugin_server plugin_shellcore; do
  make -C "$mod" clean
  make -C "$mod" all
done
cd ../hen/kpayload
make clean
make payload
make payload-include
cd ..
for mod in plugin_bootloader plugin_loader plugin_mono plugin_server plugin_shellcore; do
  (cd ../plugins/bin/plugins/prx && xxd -i "$mod.prx") | sed 's/^unsigned /static const unsigned /' > "installer/source/$mod.prx.inc.c"
done
xxd -i hen.ini | sed 's/^unsigned /static const unsigned /' > installer/source/hen.ini.inc.c
make -C installer clean
make -C installer all
cp installer/installer.bin ../../site/balencihen.bin
