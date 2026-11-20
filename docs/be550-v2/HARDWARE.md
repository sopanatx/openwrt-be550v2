# TP-Link Archer BE550 v2 — hardware map

Source: stock firmware 1.3.3 P1 (20260623), FIT config `config@BE550_2_0_0`,
stock device tree dumped to `stock/BE550_2_0_0.dts`. File paths below are inside the stock root filesystem.

## SoC / memory / flash
- Qualcomm IPQ5332 (4x Cortex-A53), board `AP-MI01.6`, stock compatible `qcom,ipq5332-ap-mi01.6`.
- RAM 512 MiB: stock DTS `memory` node is 0x20000000 and the CDT is `cdt-AP-MI01.6_512M16_DDR4.bin`. (The forum port request says 1 GB; the firmware says 512 MiB.)
- 128 MiB QSPI NAND on `nand@79b0000` (ECC 4 bit / 512 B, 8-bit bus).
- Stock DTS also has SPI NOR node `n25q128a11`; unused by partition table (boot is NAND).

## NAND partition table (`etc/partition_config/partition-table`)
| name | offset | size |
|---|---|---|
| SBL1 | 0x00000000 | 0x00180000 |
| MIBIB | 0x00180000 | 0x00100000 |
| BOOTCONFIG | 0x00280000 | 0x00080000 |
| BOOTCONFIG1 | 0x00300000 | 0x00080000 |
| QSEE | 0x00380000 | 0x00240000 |
| DEVCFG | 0x005C0000 | 0x00080000 |
| TME | 0x00640000 | 0x00080000 |
| CDT | 0x006C0000 | 0x00080000 |
| APPSBLENV | 0x00740000 | 0x00080000 |
| APPSBL (u-boot) | 0x007C0000 | 0x00140000 |
| ART (Wi-Fi cal) | 0x00900000 | 0x00200000 |
| secure-binary | 0x00B00000 | 0x00080000 |
| rootfs (slot 0, UBI) | 0x00B80000 | 0x034C0000 |
| rootfs_1 (slot 1, UBI) | 0x04040000 | 0x034C0000 |
| tp_data (UBI) | 0x07500000 | 0x00A00000 |
| data | 0x07F00000 | 0x00080000 |
| reserverd_data | 0x07F80000 | 0x00080000 |

Dual-slot A/B layout. `default-mac`, `product-info`, `support-list` etc. are logical
partitions with size 0 (stored inside `tp_data`/firmware header, not raw NAND).

## Ethernet
- MDIO `qcom,ipq40xx-mdio` at 0x90000, PHY reset GPIOs 30 and 39 (active high in stock).
- PPE port 1 (uniphy0, mac mode 0x0c = SGMII+ / 2500base-x): **Realtek RTL8221B** 2.5G PHY at MDIO addr 0x07, C45. Stock `dp2`, link-poll. Likely WAN.
- PPE port 2 (uniphy1, mac mode 0x0d, `xfi-mode`, forced 10G full): **Realtek RTL8372** (rtl8372vb) 4x 2.5G switch, MDIO addr 0x1d. Stock `dp1`. 4 LAN ports.
- Stock SSDK: `switch_lan_bmp = 0x06`, `switch_wan_bmp = 0x00`, `port_type = "TP_EXT_SWITCH_RTL8372"`.
- Stock `qca-ssdk.ko` `rtl8372n_init()` (disassembled): RX PN swap on SDS0 and SDS1 (page 0 reg 0 bit 9, page 6 reg 2 bit 13), **no TX swap**, `0xa90 <- 0xc` (MDI reverse), `0xa94 <- 0x596a` (PHY TX polarity swap). Then `rtk_sdsMode_set(0, SERDES_10GR)`, N-way off, CPU port 3.
- Front-panel LAN order: stock `default-config`, section `BE550_2_0_0`, switch `rtl8372vb`: `lan_phy_port_set 1:5 2:4 3:6 4:7` (LAN1 = switch port 5, LAN2 = 4, LAN3 = 6, LAN4 = 7), `switch_port cpu` = 3.
- **Archer BE9300 v2 differs**: section `BE9300_2_0_0` uses switch `rtl8367s`, `lan_phy_port_set 1:16 2:0 3:1 4:2`, CPU port 17, labels `lan2g5_1 lan2 lan3 lan4`. See "Archer BE9300 v2" below.
- PHY reset GPIOs: stock `rtl8221b_init` names GPIO 39 for the WAN RTL8221B and GPIO 43 for the LAN one (BE9300 v2 only). GPIO 30 is then the switch reset (by elimination, not stated in stock).
- U-Boot env (stock `etc/uci-defaults/30_uboot-envtools`, board `qcom,ipq5332-ap-mi01.6`): `0:APPSBLENV`, offset 0x0, env size 0x40000, sector 0x20000, 2 sectors.

## Archer BE9300 v2 (Ethernet only; the rest is the same board)
- Stock DTB `BE9300_2_0_0` vs `BE550_2_0_0`: `phy-reset-gpio` adds GPIO 43, `switch_lan_bmp 0x0e`, `switch_mac_mode1 0x0c` (SGMII+), port 2 `forced-speed 2500`, no `xfi-mode`, `port_type "TP_EXT_SWITCH_RTL8367S"`, dp1 `phy-mode "sgmii"`.
- Switch at MDIO 29, Realtek SMI over MDIO: reg 31 <- 0x000e, reg 23 <- address, reg 24 <- data, reg 21 <- 3 (write) / 1 (read), data from reg 25.
- Chip ID: reg 0x13c2 <- 0x0249, read 0x1300 / 0x1301. Stock maps 0x0276, 0x0597, 0x6367, 0x0652, 0x6368, 0x0801, 0x6511 to the rtl8367c back-end and **0x6642** to rtl8367d.
- Stock sets external ports 17 and 16 both to HSGMII (mode 10, forced 2.5G full, link up, pause), SGMII N-way off. Running the stock code with chip ID 0x6367 fails on port 17 (one SerDes only), so the board has the 0x6642 chip.
- LAN1: RTL8221B at MDIO 6. Stock `rtl8221b_init_common`: SerDes N-way off (30.7588 <- 2, 30.7589 <- 0x71d0, 30.7587 <- 3), 30.75f3 bit 0 clear, SerDes option 30.697a <- 3 (HiSGMII), 30.6a04 <- 0x0503, 30.6f10 <- 0xd433, 30.6f11 <- 0x8020, reset, advertise 10/100/1000/2500 + pause, EEE off, then power down (powered up later).
- OpenWrt: `kmod-be9300v2-switch` replays this; switch unmanaged, SoC port 2 is `2500base-x` fixed link.

## Wi-Fi
- 2.4 GHz: IPQ5332 internal radio (AHB, `qcom,cnss-qca5332`), BDF `lib/firmware/IPQ5332/WIFI_FW/bdwlan.b12`, board_id 0x16.
- 5 + 6 GHz: **one** QCN9224 on `pcie@18000000` (only PCIe enabled), **dual-MAC** firmware (`qcn9224/amss_dualmac.bin`, `Data_dualmac.msc`), board_id 0x1019, BDF `qcn9224/bdwlan.b1019`.
- Calibration in ART partition (0x00900000).
- Stock driver is proprietary `qca_ol`/`umac`/`wifi_3_0`; OpenWrt uses ath12k.
- Stock IPQ5332 Q6 firmware (`q6_fw0.mdt`, WLAN.WBE.1.4-01734) carries a **TP-Link certificate chain** (`www.tp-link.com.cn` CA). The SoC may only accept TP-Link-signed Q6 images. The QCN9224 firmware is Qualcomm-signed.
- Reserved memory is identical to the GL-BE9300 (same AP-MI01.6 design).

## PCIe PERST GPIOs
- pcie@10000000 (disabled): GPIO 44
- pcie@18000000 (QCN9224): GPIO 47
- pcie@20000000 (disabled): GPIO 38

## Buttons (gpio-keys-polled, active low)
| label | GPIO | code |
|---|---|---|
| reset | 29 | KEY_RESTART (0x198) |
| wps | 35 | KEY_WPS_BUTTON (0x211) |
| wifi | 36 | KEY_RFKILL (0xee) |
| ledswitch | 24 | KEY_LIGHTS_TOGGLE (0xf4) |

## LEDs
- gpio-leds: `usb_enabled` GPIO 50 (USB power enable, not a visible LED).
- Visible LEDs: LED controller MCU on I2C. Stock DT enables `i2c@78b7000` (blsp2_i2c1, GPIO 33/34, 400 kHz) = `/dev/i2c-0`; the other two I2C controllers are disabled. `iplatform.config`: `LED_TYPE="ledcontroller"`.
- Stock `usr/bin/led_combination` (disassembled):
  - MCU slave address **0x50**, plain `I2C_RDWR` writes, 3 tries.
  - RGB effect = one 12-byte write: `[0]` mode, `[1..3]` colour 1 RGB, `[4..6]` colour 2 RGB, `[7..9]` 0, `[10..11]` CRC16-XMODEM (poly 0x1021, init 0) over bytes 0..9, big endian.
  - Modes: none 0xC0, breathing 0xC1, flash 0xC2, rainbow 0xC3, spectrum 0xC4, comet 0xC5, wave 0xC6, fire 0xC7, ripple 0xC8, mountain 0xC9, night 0xCA, loop 0xCB, static 0xCC.
  - Status events = one 1-byte write. Bytes from `default-config`, section `led_combination` (`cmd`): GENERAL_OFF 160, REBOOT 161, INTERNET_UP 162, INTERNET_DOWN and WAN_DOWN 163, INTERNET_WIFI_DOWN 164, WIFI_DOWN 165 (also FACTORY_TEST_BEGIN), UPGRADE 166, RESET 167, WPS_INPROGRESS 168. WIFI_UP, INIT_END, GENERAL_ON have no byte. No event has `rgb_cmd`.
  - MCU firmware update path, never send: 0x20 (CHECK_VERSION, then reads 3 bytes), 0x30 / 0x31 / 0x32 (Ymodem upgrade), 0x18.
- `etc/led_controller/firmware` only has MCU images for other models (BE550 **v1**, GE800, GE650, GXE75).
- OpenWrt: I2C node added in the DTS (mainline has this QUP only as SPI); `be550v2-i2cw` sends the write. `be550v2-led status <name> | on [rrggbb] | off | mode <name> [c1 [c2]] | event <byte>`. Boot, WAN hotplug, reboot and sysupgrade send status events. Whether the v2 MCU honours the RGB frame (no stock event has `rgb_cmd`) is untested.

## Stock configuration data (`etc/partition_config/`)
- `profile`, `default-config`, `ap-def-config`, `re-def-config` are AES-256-CBC + zlib. Key and IV are plain text in `usr/lib/lua/luci/model/crypto.lua`. Decrypt: `openssl aes-256-cbc -d -K <key> -iv <iv> -in profile | openssl zlib -d` (or Python `zlib.decompress`). Result is XML.
- `default-config` holds common settings, then per-country and per-model sections (`BE550_2_0_0`, `BE9300_2_0_0`).

## Stock upgrade (`usr/bin/nvrammanager`, disassembled)
- Container: "tplink2023" — u32 size, 16-byte MD5, 0x1000 header area, section table at 0x1014 (`support-list`, `soft-version`), then UBI.
- `fw-type:` in the header is looked up in a one-entry table (`Cloud` → RSA-2048 PSS + AES). **Any other or missing fw-type falls back to MD5-only** (salt `7a2b15ed9b98596de504ab44ac2a9f4e`).
- Then `support-list` must match the unit, and `nm_checkSoftVer` rejects an image whose `soft_ver:` major.minor (major.minor.patch on SG units) is older than the installed one.
- The `soft_ver` check runs only when an internal mode flag is 2; it rejects when installed > image. The image version is only compared. Logical partitions are read from `/tp_data/<name>`, else `/etc/partition_config/<name>` of the running rootfs; only `default-mac`, `pin`, `device-id`, `product-info`, `radio_bk`, `radio_sec_bk` are ever written to `/tp_data`. So a high `soft_ver` in an image leaves nothing behind.
- Writes the inactive slot (`rootfs`/`rootfs_1`) and sets `tp_boot_idx` with `fw_setenv`.
- Stock kernel FIT has only crc32/sha1 hashes, no signature nodes: U-Boot does not authenticate the kernel.
