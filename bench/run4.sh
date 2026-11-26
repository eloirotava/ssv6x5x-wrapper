#!/bin/sh
# Roda no computador de controle. Boot limpo da caixa 6256, troca de driver, iperf3 a partir
# do AP ($AP, com iperf3) e validacao pelos bytes do wlan0.
#   BSSID=<ap> run4.sh <rotulo> <driver-esperado> <arquivo.ko|-> [params]
set -u
LABEL=$1; shift
T=${T:-10}
D=$(dirname "$0")/results-2026-11-26/$LABEL; rm -rf "$D"; mkdir -p "$D"
P=${PASSFILE:?arquivo com a senha ssh}
box() { sshpass -f $P ssh -o ConnectTimeout=6 root@${BOX:?IP da caixa} "$@"; }
ap()  { sshpass -f $P ssh -o ConnectTimeout=10 root@${AP:?IP do AP} "$@"; }

box 'rm -f /etc/modprobe.d/zz-bench.conf; sync; systemctl reboot' </dev/null 2>/dev/null
sleep 35
for i in $(seq 40); do
	box 'lsmod | grep -q "^ssv6xxx " && iw dev wlan0 link | grep -q Connected' </dev/null 2>/dev/null && break
	sleep 5
done
box "SUBNET=$SUBNET BSSID=$BSSID /root/bench/prep4.sh $*" </dev/null > "$D/prep.txt" 2>&1
IP=$(sed -n 's/^IP=//p' "$D/prep.txt" | tail -1)
if [ -z "$IP" ]; then echo "$LABEL FALHOU: $(grep ERRO "$D/prep.txt")"; exit 1; fi
echo "$LABEL $(grep '^DRV=' "$D/prep.txt") $(grep -oE '(freq|signal): .*' "$D/prep.txt" | tr '\n' ' ')"
cnt() { box 'cat /sys/class/net/wlan0/statistics/tx_bytes /sys/class/net/wlan0/statistics/rx_bytes' </dev/null | tr '\n' ' '; }
for dir in up down; do
	[ $dir = up ] && opt=-R || opt=
	set -- $(cnt); tx0=$1; rx0=$2
	ap "iperf3 -c $IP -t $T $opt -J" > "$D/$dir-1.json" 2>&1
	set -- $(cnt); tx1=$1; rx1=$2
	echo "$dir 1 tx=$((tx1 - tx0)) rx=$((rx1 - rx0))" >> "$D/bytes.txt"
	sleep 2
done
box "iw dev wlan0 link; iw dev wlan0 station dump; basename \$(readlink \$(ls -d /sys/bus/sdio/devices/*:0001:1 | head -1)/driver)" </dev/null > "$D/box.txt" 2>&1
echo "$LABEL ok"
