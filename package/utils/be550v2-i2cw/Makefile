# SPDX-License-Identifier: GPL-2.0-or-later

include $(TOPDIR)/rules.mk

PKG_NAME:=be550v2-i2cw
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-or-later

include $(INCLUDE_DIR)/package.mk

define Package/be550v2-i2cw
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=I2C write helper for the Archer BE550 v2 LED controller
  DEPENDS:=@TARGET_qualcommbe +kmod-i2c-core
endef

define Package/be550v2-i2cw/description
  Sends one I2C write message (I2C_RDWR) to a device. Used by
  be550v2-led to talk to the LED controller MCU.
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -Wall -Werror \
		-o $(PKG_BUILD_DIR)/be550v2-i2cw $(PKG_BUILD_DIR)/be550v2-i2cw.c
endef

define Package/be550v2-i2cw/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/be550v2-i2cw $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,be550v2-i2cw))
