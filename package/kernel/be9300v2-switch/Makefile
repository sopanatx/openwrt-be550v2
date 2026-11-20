# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=be9300v2-switch
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/be9300v2-switch
  SUBMENU:=Network Devices
  TITLE:=TP-Link Archer BE9300 v2 LAN switch bring-up
  DEPENDS:=@TARGET_qualcommbe +kmod-libphy
  FILES:=$(PKG_BUILD_DIR)/be9300v2-switch.ko
  AUTOLOAD:=$(call AutoLoad,41,be9300v2-switch,1)
endef

define KernelPackage/be9300v2-switch/description
  Sets up the Realtek RTL8367S-VB class switch (chip ID 0x6642) and the
  LAN1 RTL8221B PHY of the Archer BE9300 v2 the way the stock firmware
  does, and leaves the switch unmanaged. No DSA, no per-port control.
endef

define Build/Compile
	+$(KERNEL_MAKE) $(PKG_JOBS) \
		M="$(PKG_BUILD_DIR)" \
		modules
endef

$(eval $(call KernelPackage,be9300v2-switch))
