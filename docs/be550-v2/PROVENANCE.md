# Where things come from

This file says who wrote what, which files come from the TP-Link stock
firmware, and how the hardware facts were found. If something here is
wrong or a credit is missing, open an issue; it will be corrected.

## Base tree

[perceival/openwrt-flint3](https://github.com/perceival/openwrt-flint3),
branch `flint3-be9300`, commit `f351b9ebcb`: OpenWrt plus the IPQ5332
target, the RTL8372 DSA driver and the ath12k work for the GL.iNet
Flint 3. Everything below that commit is the work of OpenWrt and of that
project. The `.github/FUNDING.yml` sponsor link is theirs and stays.

The CI workflows of the base tree were removed here. They belong to the
OpenWrt infrastructure and publish images; this tree publishes none.

## Code from other people

| What | Author | Source | How it is in this tree |
|---|---|---|---|
| `scripts/tplink-mkimage-2023.py`, `Build/tplink-image-2023` | Sidhant Goel | Archer BE800 v1 port, [openwrt/openwrt#20373](https://github.com/openwrt/openwrt/pull/20373) | original commit, cherry-picked |
| typo fixes in that script | Patrick Lawler | [openwrt/openwrt#25470](https://github.com/openwrt/openwrt/pull/25470) | original commit, cherry-picked |
| `lib/preinit/09_mount_tp_data` | Patrick Lawler | Archer BE550 v1 port, #25470 | copied, board names changed |
| dual-slot sysupgrade (`tp_boot_idx`), web UI install method | Patrick Lawler, Alexandru Gagniuc | Archer BE550 v1 port, #25470 and [#20778](https://github.com/openwrt/openwrt/pull/20778) | same method, code rewritten for this board |
| stock 5/6 GHz firmware in `firmware-2.bin` (features 0x03, dual-MAC image in both elements) | user `ernie96` | test report in #25470 | method only |

Neither of those pull requests is merged in OpenWrt at the time of
writing. Their authors did not review this port and are not responsible
for it.

## Files from the TP-Link stock firmware

From firmware 1.3.3 Build 20260623 for the Archer BE550 v2 / BE9300 v2:

| File in this tree | What it is |
|---|---|
| `package/firmware/ipq-wifi/src/board-tplink_archer-be550-v2.ipq5332` | Wi-Fi board data for the 2.4 GHz radio, repacked from the stock `bdwlan` file |
| `package/firmware/ipq-wifi/src/board-tplink_archer-be550-v2.qcn9274` | Wi-Fi board data for the 5/6 GHz chip, same |
| `docs/be550-v2/stock/*.dtb`, `*.dts` | device trees from the stock kernel image (the kernel is GPL-2.0) |

Board data files have no licence statement from TP-Link. OpenWrt carries
such files for many routers in the same way
([firmware_qca-wireless](https://github.com/openwrt/firmware_qca-wireless));
they are needed for the radios to work within their limits. If TP-Link
objects, they will be removed.

Not in this tree, and not to be added: the stock firmware, the Wi-Fi
firmware of the chips, calibration data. Where the port needs the stock
Wi-Fi firmware, it copies it at run time from the router's own flash.

## Facts found by examination of the stock firmware

No vendor source code was available or used. These facts were read from
the stock firmware image, for interoperability:

| Fact | Found in |
|---|---|
| partitions, GPIOs, PHY and switch wiring, memory map | stock device tree |
| RTL8372 setup values (polarity, SerDes mode, CPU port) | disassembly of the stock `qca-ssdk.ko` kernel module |
| BE9300 v2 switch register sequence (`be9300v2-switch-seq.h`) | log of the register writes of the stock `qca-ssdk.ko`, run in a harness ([contrib/ssdk-harness](../../contrib/ssdk-harness)) |
| LED controller protocol | disassembly of the stock `led_combination` program |
| checks of the stock firmware updater | disassembly of the stock `nvrammanager` program |
| LAN port order, LED event bytes | the stock default configuration |

The stock default configuration is encrypted with a key that is stored
in the firmware itself. The key is not published in this repository.

## AI assistance

Most of the code and documents of this port were written with an AI
assistant (Claude, by Anthropic), directed and reviewed by the
maintainer. The commits say so with an `Assisted-by:` trailer.

What this means for you:

- The analysis was done on the stock firmware files, without a router on
  the bench. Each claim in the README status table is "built", not
  "works", until a hardware report confirms it.
- An AI assistant can be wrong with confidence. Hardware reports and
  reviews are the way to find the errors. Please be direct about them.

## Trademarks

TP-Link and Archer are trademarks of TP-Link. GL.iNet, Qualcomm, Realtek
and OpenWrt are trademarks of their owners. This project has no
connection with any of them.
