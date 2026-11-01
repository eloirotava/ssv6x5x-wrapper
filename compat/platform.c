// SPDX-License-Identifier: GPL-2.0
/*
 * platform.c - substitui vendor/ssv6x5x-generic-wlan.c, que liga o
 * chip por funcoes do kernel Rockchip (rockchip_wifi_power,
 * rockchip_wifi_set_carddetect). No mainline quem liga o chip e o
 * device tree (mmc-pwrseq) e o cartao ja aparece no barramento.
 */
#include <linux/module.h>

int tu_ssvdevice_init(void);
void tu_ssvdevice_exit(void);

static int __init ssv_wrapper_init(void)
{
	return tu_ssvdevice_init();
}

static void __exit ssv_wrapper_exit(void)
{
	tu_ssvdevice_exit();
}

module_init(ssv_wrapper_init);
module_exit(ssv_wrapper_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("SSV6x5x (driver Icomm original) com camada de compatibilidade");
