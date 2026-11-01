# Wrapper fino: compila o driver original da Icomm (vendor/, intocado)
# contra kernels novos. Toda adaptacao fica em compat/ e patches/.
#
#   make KDIR=/caminho/do/kernel ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
#
KDIR ?= /lib/modules/$(shell uname -r)/build
BUILD := $(CURDIR)/build

all: modules

$(BUILD)/.stamp: $(wildcard vendor/*/* vendor/* compat/* compat/include/*/* patches/*) Kbuild
	rm -rf $(BUILD)
	cp -a vendor $(BUILD)
	cp -a compat $(BUILD)/compat
	cp Kbuild $(BUILD)/Kbuild
	rm -f $(BUILD)/Makefile
	set -e; for p in $$(ls patches/*.patch 2>/dev/null | sort); do \
		echo "patch $$p"; patch -s -p1 -d $(BUILD) < $$p; done
	touch $@

modules: $(BUILD)/.stamp
	$(MAKE) -C $(KDIR) M=$(BUILD) modules

clean:
	rm -rf $(BUILD)

.PHONY: all modules clean
