REQUIRE_IMAGE_METADATA=1
RAMFS_COPY_BIN='dumpimage fitblk fit_check_sign fw_printenv fw_setenv grep be550v2-i2cw'
RAMFS_COPY_DATA='/etc/fw_env.config /var/lock/fw_printenv.lock'

gl_be6500_remove_oem_rootfs() {
	local mtdnum
	local ubidev
	local ubivol

	mtdnum=$(find_mtd_index "$CI_UBIPART")
	if [ -z "$mtdnum" ]; then
		echo "Unable to find UBI MTD partition $CI_UBIPART"
		return 1
	fi

	ubidev=$(nand_find_ubi "$CI_UBIPART")
	if [ -z "$ubidev" ]; then
		ubiattach --mtdn="$mtdnum" || return 1
		ubidev=$(nand_find_ubi "$CI_UBIPART")
	fi

	[ -n "$ubidev" ] || return 1

	ubivol=$(nand_find_volume "$ubidev" ubi_rootfs)
	[ -z "$ubivol" ] || {
		echo "Removing legacy ubi_rootfs volume"
		ubirmvol "/dev/$ubidev" --name=ubi_rootfs || return 1
	}
}

tplink_be550_v2_check_env() {
	# fw_setenv on a wrong env offset/size would write a fresh, default
	# environment. Only continue if the existing one reads back with a
	# valid CRC and holds TP-Link's slot selector.
	local out

	out=$(fw_printenv 2>&1) || {
		echo "Cannot read the U-Boot environment, not upgrading"
		return 1
	}
	case "$out" in
	*"Bad CRC"*)
		echo "U-Boot environment has a bad CRC (wrong fw_env.config?), not upgrading"
		return 1
		;;
	esac
	echo "$out" | grep -q '^bootcmd=' || {
		echo "U-Boot environment has no bootcmd, not upgrading"
		return 1
	}
}

# Stock LED event UPGRADE (166) on the LED controller MCU (I2C 0x50). Runs
# in the sysupgrade ramfs, so no helper script; failure is not an error.
tplink_be550_v2_led_upgrade() {
	local d

	for d in /sys/bus/i2c/devices/i2c-*; do
		[ "$(cat "$d/of_node/name" 2>/dev/null)" = "i2c" ] || continue
		be550v2-i2cw "${d##*-}" 0x50 166 2>/dev/null
		return 0
	done
}

tplink_be550_v2_do_upgrade() {
	# Dual boot, as in the Archer BE550 v1 port by Patrick Lawler
	# (https://github.com/openwrt/openwrt/pull/25470): install into the inactive
	# rootfs/rootfs_1 slot and point tp_boot_idx at it only once it is
	# fully written. TP-Link's U-Boot falls back to the other slot if
	# the selected one fails to load.
	local idx=1

	CI_UBIPART="rootfs_1"
	if grep -q 'ubi.mtd=rootfs_1' /proc/cmdline; then
		idx=0
		CI_UBIPART="rootfs"
	fi

	tplink_be550_v2_check_env || nand_do_upgrade_failed
	tplink_be550_v2_led_upgrade

	# a slot last written by the stock firmware holds its "ubi_rootfs"
	# volume, which leaves no room for ours
	gl_be6500_remove_oem_rootfs || nand_do_upgrade_failed
	sync

	nand_do_flash_file "$1" || nand_do_upgrade_failed
	fw_setenv tp_boot_idx $idx || {
		echo "failed to set tp_boot_idx $idx"
		nand_do_upgrade_failed
	}
	nand_do_upgrade_success
}

platform_do_upgrade() {
	case "$(board_name)" in
	gl.inet,gl-be6500)
		CI_UBIPART="rootfs"
		gl_be6500_remove_oem_rootfs || return 1
		nand_do_upgrade "$1"
		;;
	gl.inet,gl-be9300)
		CI_KERNPART="0:HLOS"
		CI_ROOTPART="rootfs"
		glinet_emmc_do_upgrade "$1" || exit 1
		;;
	ubnt,u7-pro-xgs)
		CI_KERNPART="kernel0"
		fit_do_upgrade "$1"
		;;
	tplink,archer-be550-v2|\
	tplink,archer-be9300-v2)
		tplink_be550_v2_do_upgrade "$1"
		;;
	*)
		echo "Sysupgrade is not supported on your board yet."
		return 1
		;;
	esac
}

platform_check_image() {
	[ "$#" -gt 1 ] && return 1

	case "$(board_name)" in
	gl.inet,gl-be6500|\
	tplink,archer-be550-v2|\
	tplink,archer-be9300-v2)
		return 0
		;;
	gl.inet,gl-be9300)
		glinet_emmc_check_image "$1"
		;;
	ubnt,u7-pro-xgs)
		fit_check_image "$1"
		;;
	*)
		echo "Sysupgrade is not supported on your board yet."
		return 1
		;;
	esac
}

platform_copy_config() {
	case "$(board_name)" in
	gl.inet,gl-be9300|\
	ubnt,u7-pro-xgs)
		emmc_copy_config
		;;
	esac
}
