define Build/fit-inline-rootfs
	rm -f $@.dtb $@.kernel
	cp $@ $@.kernel
	cp $(word 2,$(1)) $@.dtb
	cp $@.kernel $@
	$(call Build/fit-its,$(word 1,$(1)) $@.dtb with-rootfs)
	$(call Build/fit-image,$(word 1,$(1)) $@.dtb with-rootfs)
	rootfs_offset="$$(grep -oba hsqs $@ | head -n1 | cut -d: -f1)"; \
	[ -n "$$rootfs_offset" ] || { echo "Failed to locate SquashFS in $@"; exit 1; }; \
	pad="$$(( (4096 - ($$rootfs_offset % 4096)) % 4096 ))"; \
	cp $(word 2,$(1)) $@.dtb; \
	dd if=/dev/zero bs=1 count="$$pad" >> $@.dtb 2>/dev/null; \
	cp $@.kernel $@; \
	$(call Build/fit-its,$(word 1,$(1)) $@.dtb with-rootfs)
	$(call Build/fit-image,$(word 1,$(1)) $@.dtb with-rootfs)
	rm -f $@.dtb $@.kernel
endef

define Build/gl-be9300-factory
	$(TOPDIR)/scripts/mkits-qsdk-ipq-image.sh $@.its \
		$(TOPDIR)/target/linux/qualcommbe/image/gl-be9300-factory.bootscript \
		hlos $(IMAGE_KERNEL) rootfs $@
	PATH=$(LINUX_DIR)/scripts/dtc:$(PATH) mkimage -f $@.its $@.new
	mv $@.new $@
	rm -f $@.its
endef

define Device/ubnt_u7-pro-xgs
	DEVICE_VENDOR := Ubiquiti
	DEVICE_MODEL := UniFi 7
	DEVICE_VARIANT := Pro XGS
	# Stock U-Boot probes config-a6a4 on this board.
	DEVICE_DTS_CONFIG := config-a6a4
	SOC := ipq5332
	SUPPORTED_DEVICES += ubnt,u7-pro-xgs
	DEVICE_PACKAGES := e2fsprogs f2fsck fitblk mkf2fs \
		kmod-ath12k ath12k-firmware-qcn9274 \
		ipq-wifi-ubnt_u7-pro-xgs kmod-leds-pwm \
		kmod-phy-realtek rtl826x-firmware
	KERNEL := kernel-bin | lzma
	KERNEL_INITRAMFS := kernel-bin | lzma | \
		fit lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb with-initrd | pad-to 64k
	KERNEL_INITRAMFS_SUFFIX := .itb
	IMAGE_SIZE := 128m
	IMAGES := sysupgrade.itb
	IMAGE/sysupgrade.itb := append-kernel | \
		fit-inline-rootfs lzma $$(KDIR)/image-$$(firstword $$(DEVICE_DTS)).dtb | \
		check-size | append-metadata
endef
TARGET_DEVICES += ubnt_u7-pro-xgs

define Device/glinet_gl-be6500
	$(call Device/FitImage)
	$(call Device/UbiFit)
	DEVICE_VENDOR := GL.iNet
	DEVICE_MODEL := GL-BE6500
	DEVICE_DTS_CONFIG := config@mi01.2
	SOC := ipq5332
	SUPPORTED_DEVICES += gl.inet,gl-be6500
	BLOCKSIZE := 128k
	PAGESIZE := 2048
	KERNEL_INSTALL := 1
	KERNEL_SIZE := 6096k
	IMAGE_SIZE := 25344k
	IMAGES += factory.bin
	IMAGE/factory.bin := append-ubi | append-gl-metadata
	DEVICE_PACKAGES := kmod-ath12k ath12k-firmware-ipq5332 \
		ath12k-firmware-qcn9274 ipq-wifi-glinet_gl-be6500 \
		kmod-hwmon-pwmfan kmod-qrtr-smd kmod-rtl837x-dsa
endef
TARGET_DEVICES += glinet_gl-be6500

define Device/glinet_gl-be9300
	$(call Device/FitImage)
	$(call Device/EmmcImage)
	DEVICE_VENDOR := GL.iNet
	DEVICE_MODEL := GL-BE9300
	DEVICE_ALT0_VENDOR := GL.iNet
	DEVICE_ALT0_MODEL := Flint 3
	# Stock U-Boot has no AP-MI01.6 entry in its board->config table, so
	# bootipq falls back to asking for "config-1". Any other name - a
	# board-specific config@mi01.6, or OpenWrt's own default config@1 -
	# fails with "Config not available" and the unit will not boot from
	# eMMC. (BE6500 can use config@mi01.2 because that board IS in the
	# table.)
	DEVICE_DTS_CONFIG := config-1
	SOC := ipq5332
	SUPPORTED_DEVICES += gl.inet,gl-be9300
	# The kernel FIT is dd'd into 0:HLOS (mmcblk0p12, 7 MiB) by
	# glinet_emmc_do_fit_upgrade() with no size check; a truncated kernel
	# never boots and the unit comes up dark. A default build is ~5.75 MiB,
	# so refuse to build anything the partition cannot hold. The initramfs
	# image is never written to eMMC and is exempt.
	KERNEL_SIZE := 7168k
	KERNEL := kernel-bin | libdeflate-gzip | fit gzip $$(KDIR)/image-$$(DEVICE_DTS).dtb | check-size $$(KERNEL_SIZE)
	KERNEL_INITRAMFS := kernel-bin | libdeflate-gzip | fit gzip $$(KDIR)/image-$$(DEVICE_DTS).dtb
	IMAGE/factory.bin := append-rootfs | pad-rootfs | pad-to 64k | \
		gl-be9300-factory | append-gl-metadata
	DEVICE_PACKAGES := kmod-ath12k ath12k-firmware-ipq5332 \
		ath12k-firmware-qcn9274 ipq-wifi-glinet_gl-be9300 \
		kmod-hwmon-pwmfan kmod-qrtr-smd kmod-rtl837x-dsa \
		kmod-phy-realtek ethtool e2fsprogs f2fsck mkf2fs dumpimage \
		kmod-usb-core kmod-usb2 kmod-usb3 kmod-usb-dwc3 \
		kmod-usb-dwc3-qcom kmod-usb-xhci-hcd kmod-scsi-core \
		kmod-usb-storage kmod-usb-storage-uas kmod-fs-ext4 \
		block-mount blockd usbutils
endef
TARGET_DEVICES += glinet_gl-be9300

# TP-Link's U-Boot picks a FIT configuration by name. The stock FIT for the
# BE550 v2 carries "config@BE550_2_0_0", "config@mi01.6" and the default
# "config@1"; which one the loader asks for is not known, so offer all of
# them (plus mkits' "config-1") pointing at the same kernel and DTB.
define Build/fit-tplink
	$(call Build/fit-its,$(1))
	$(TOPDIR)/target/linux/qualcommbe/image/fit-add-configs.sh $@.its \
		$(DEVICE_DTS_CONFIG) $(TPLINK_FIT_CONFIGS)
	$(call Build/fit-image,$(1))
endef

DEVICE_VARS += TPLINK_FIT_CONFIGS TPLINK_SOFT_VERSION TPLINK_SUPPORT_STRING

define Device/tplink_archer-be550-v2
	$(call Device/UbiFit)
	DEVICE_VENDOR := TP-Link
	DEVICE_MODEL := Archer BE550
	DEVICE_VARIANT := v2
	DEVICE_ALT0_VENDOR := TP-Link
	DEVICE_ALT0_MODEL := Archer BE9300
	DEVICE_ALT0_VARIANT := v2
	SOC := ipq5332
	DEVICE_DTS_CONFIG := config@1
	TPLINK_FIT_CONFIGS := config@BE550_2_0_0 config@mi01.6 config-1
	KERNEL_SUFFIX := -uImage.itb
	KERNEL_NAME := Image
	KERNEL = kernel-bin | lzma | fit-tplink lzma $$(KDIR)/image-$$(DEVICE_DTS).dtb
	KERNEL_INITRAMFS = kernel-bin | lzma | \
		fit-tplink lzma $$(KDIR)/image-$$(DEVICE_DTS).dtb with-initrd | pad-to 64k
	KERNEL_INITRAMFS_SUFFIX := .itb
	BLOCKSIZE := 128k
	PAGESIZE := 2048
	# one rootfs slot: 0x034C0000 (stock partition table)
	IMAGE_SIZE := 54016k
	NAND_SIZE := 128m
	IMAGE/sysupgrade.bin/squashfs := sysupgrade-tar | append-metadata
	IMAGES += web-ui-factory.bin
	IMAGE/web-ui-factory.bin := append-ubi | check-size | tplink-image-2023
	# Stock nvrammanager rejects an image whose soft_ver major.minor
	# (major.minor.patch on SG units) is older than the installed one.
	# This is the stock 1.3.3 string; set it to the installed version if
	# the router already runs a newer stock release.
	TPLINK_SOFT_VERSION := soft_ver:1.3.3 Build 20260623 rel.27273\n
	TPLINK_SUPPORT_STRING := SupportList:\n\
		{product_name:Archer BE550,product_ver:2.0.0,special_id:55530000}\n\
		{product_name:Archer BE550,product_ver:2.6.0,special_id:55530000}\n\
		{product_name:Archer BE550,product_ver:2.26.0,special_id:55530000}\n\
		{product_name:Archer BE550,product_ver:2.0.0,special_id:41530000}\n\
		{product_name:Archer BE550,product_ver:2.0.0,special_id:41550000}\n\
		{product_name:Archer BE550,product_ver:2.0.0,special_id:43410000}\n\
		{product_name:Archer BE550,product_ver:2.0.0,special_id:45550000}\n\
		{product_name:Archer BE550,product_ver:2.0.0,special_id:4A500000}\n\
		{product_name:Archer BE550,product_ver:2.0.0,special_id:4B520000}\n\
		{product_name:Archer BE550,product_ver:2.0.0,special_id:53470000}\n\
		{product_name:Archer BE9300,product_ver:2.0.0,special_id:55530000}\n
	DEVICE_PACKAGES := kmod-ath12k ath12k-firmware-ipq5332 \
		ath12k-firmware-qcn9274 ipq-wifi-tplink_archer-be550-v2 \
		kmod-qrtr-smd kmod-rtl837x-dsa kmod-phy-realtek \
		uboot-envtools ubi-utils be550v2-i2cw \
		kmod-usb-core kmod-usb3 kmod-usb-dwc3 \
		kmod-usb-dwc3-qcom kmod-usb-xhci-hcd
endef
TARGET_DEVICES += tplink_archer-be550-v2
