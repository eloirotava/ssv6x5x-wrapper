#!/bin/sh
# SSV6256: reescrita (ssv6xxx) x wrapper, em ate duas redes.
# Defina antes de rodar:
#   BOX=<ip da caixa> AP=<ip de um host com iperf3> PASSFILE=<senha ssh>
#   SUBNET=<rede, ex. 192.168.1.0/24>
#   BSSID24=<bssid 2,4 GHz>  BSSID5=<bssid 5 GHz, opcional>
cd "$(dirname "$0")"
export T=${T:-10} BOX AP PASSFILE SUBNET
W=/root/bench/ssv6x5x_original.ko
CFG=tu_stacfgpath=/lib/firmware/ssv6x5x-original/ssv6x5x-wifi.cfg
BSSID=$BSSID24 ./run4.sh g24-reescrita ssv6xxx -
BSSID=$BSSID24 ./run4.sh g24-wrapper TU_SSV6XXX_SDIO $W $CFG
if [ -n "${BSSID5:-}" ]; then
	BSSID=$BSSID5 ./run4.sh g5-reescrita ssv6xxx -
	BSSID=$BSSID5 ./run4.sh g5-wrapper TU_SSV6XXX_SDIO $W $CFG
fi
# volta a reescrita, sem BSSID travado
sshpass -f "$PASSFILE" ssh root@"$BOX" 'rm -f /etc/modprobe.d/zz-bench.conf; sync; systemctl reboot' </dev/null 2>/dev/null
echo SUITE-FIM
