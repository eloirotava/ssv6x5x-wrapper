#!/bin/sh
# Roda NA CAIXA, logo apos um boot limpo (reescrita ssv6xxx carregada).
#   BSSID=<ap> prep4.sh <driver-esperado> <arquivo.ko|-> [parametros...]
# Trava o BSSID, troca o driver, confere o dono do cartao e o BSSID, prende
# o trafego do IP do wlan0 no wlan0 e sobe o iperf3 nesse IP.
set -u
EXP=$1; MOD=$2; shift 2
CARD=$(ls -d /sys/bus/sdio/devices/*:0001:1 | head -1)
sysctl -q -w kernel.panic=10 kernel.panic_on_oops=1
pkill -x iperf3 2>/dev/null
sed -i '/^  bssid=/d' /run/netplan/wpa-wlan0.conf
sed -i "/scan_ssid=1/a\\  bssid=$BSSID" /run/netplan/wpa-wlan0.conf
if [ "$MOD" != "-" ]; then
	echo "install ssv6xxx /bin/false" > /etc/modprobe.d/zz-bench.conf; sync
	systemctl stop netplan-wpa-wlan0; sleep 1
	rmmod ssv6xxx || { echo "ERRO rmmod ssv6xxx"; echo IP=; exit 1; }
	sleep 2
	modprobe -a mac80211 cfg80211 libarc4
	insmod "$MOD" "$@" || { echo "ERRO carga $MOD"; echo IP=; exit 1; }
	sleep 3
fi
systemctl restart netplan-wpa-wlan0
ok=0
for i in $(seq 90); do
	if iw dev wlan0 link 2>/dev/null | grep -q "Connected to $BSSID" &&
	   ip -4 addr show wlan0 | grep -q inet; then ok=1; break; fi
	sleep 1
done
DRV=$(basename "$(readlink $CARD/driver)")
echo "DRV=$DRV assoc_s=$i ok=$ok"
iw dev wlan0 link | grep -E "Connected|freq|signal|bitrate"
[ "$DRV" = "$EXP" ] || { echo "ERRO dono do cartao: $DRV (esperado $EXP)"; echo IP=; exit 2; }
[ $ok = 1 ] || { echo "ERRO nao associou em $BSSID"; echo IP=; exit 3; }
IP=$(ip -4 -o addr show wlan0 | awk '{print $4}' | cut -d/ -f1)
sysctl -q -w net.ipv4.conf.all.arp_ignore=1 net.ipv4.conf.all.arp_announce=2
ip rule del pref 100 2>/dev/null
ip route flush table 168 2>/dev/null
ip route add "${SUBNET:?defina SUBNET, ex. 192.168.1.0/24}" dev wlan0 src "$IP" table 168
ip rule add from "$IP" lookup 168 pref 100
iperf3 -s -B "$IP" -D
sleep 1
echo "IP=$IP"
