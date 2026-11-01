# Kbuild do wrapper. Usa as flags originais do fabricante
# (vendor/platform-config.mak -> config_common.mak) e troca so o que e
# de plataforma (vendor/ssv6x5x.cfg era o do Android/Rockchip).
KMODULE_NAME := ssv6x5x_original
KBUILD_TOP := $(src)

include $(src)/platform-config.mak
ccflags-y := $(filter-out -Werror -g -D__CHECK_ENDIAN__,$(ccflags-y))

# Plataforma: Linux generico. Do ssv6x5x.cfg original fica tudo, menos
# CONFIG_SSV_SUPPORT_ANDROID; entra BUILD_AS_ONE_KO (um .ko so).
ccflags-y += -DCONFIG_SSV_BUILD_AS_ONE_KO
ccflags-y += -DCONFIG_FW_ALIGNMENT_CHECK
ccflags-y += -DCONFIG_PLATFORM_SDIO_OUTPUT_TIMING=3
ccflags-y += -DCONFIG_PLATFORM_SDIO_BLOCK_SIZE=128
ccflags-y += -DMULTI_THREAD_ENCRYPT
ccflags-y += -DKTHREAD_BIND
ccflags-y += -DCONFIG_SSV_RSSI
ccflags-y += -DCONFIG_SSV_VENDOR_EXT_SUPPORT

ccflags-y += -I$(src) -I$(src)/include -I$(src)/compat/include
ccflags-y += -include $(src)/compat/compat.h
CFLAGS_compat/compat.o := -DSSV_COMPAT_IMPL
ccflags-y += -Wno-error

$(shell env ccflags="$(ccflags-y)" bash $(src)/parser-conf.sh $(src)/include/ssv_conf_parser.h)

# lista do vendor/Makefile para esta configuracao
# (HAL + SSV6006, sem 6051, sem 6006AB, crypto local, sem smartlink)
KERN_SRCS := ssvdevice/ssvdevice.c ssvdevice/ssv_cmd.c hci/ssv_hci.c
KERN_SRCS += smac/init.c smac/ssv_skb.c smac/dev.c
KERN_SRCS += smac/ssv_rc_minstrel.c smac/ssv_rc_minstrel_ht.c
KERN_SRCS += smac/ap.c smac/ampdu.c smac/efuse.c smac/ssv_pm.c
KERN_SRCS += smac/sec_ccmp.c smac/sec_tkip.c smac/sec_wep.c
KERN_SRCS += smac/wapi_sms4.c smac/sec_wpi.c
KERN_SRCS += smac/hal/hal.c
KERN_SRCS += hwif/usb/usb.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006_common.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006C_mac.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006_phy.c
KERN_SRCS += smac/hal/ssv6006c/ssv6006_turismoC.c
KERN_SRCS += hwif/sdio/sdio.c
KERN_SRCS += compat/platform.c compat/compat.c

$(KMODULE_NAME)-y := $(KERN_SRCS:.c=.o)
obj-m := $(KMODULE_NAME).o
