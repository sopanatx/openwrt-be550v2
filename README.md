# OpenWrt for the TP-Link Archer BE550 v2

OpenWrt source tree with board support for the **TP-Link Archer BE550 v2**
(Wi-Fi 7, Qualcomm IPQ5332). The **Archer BE9300 v2** shares the board and
has its own profile in the same tree.

Target: `qualcommbe/ipq53xx`, kernel 6.18.

> [!CAUTION]
> **This port has never run on real hardware.** The images build and were
> checked offline against the stock firmware 1.3.3, nothing more. It may
> not boot, and a failed install may leave the router unusable.
> Do not install it on a router you need, and do not install it without a
> UART adapter. No pre-built images are published until a first boot is
> confirmed.

> [!NOTE]
> Most of this port was written with an AI assistant and checked only
> against the stock firmware files. See
> [PROVENANCE.md](docs/be550-v2/PROVENANCE.md). Problems with this tree
> go to the issues of this repository, not to the OpenWrt project.

> [!WARNING]
> Unofficial community port. Not affiliated with, endorsed by, or supported
> by TP-Link or the OpenWrt project. No warranty of any kind. Installing
> third-party firmware can void the hardware warranty.

This is for **hardware version 2** only. The Archer BE550 v1 is a
different board.

## Supported models

The two models use the same stock firmware file but different LAN
hardware. Check the label under the router. The LAN jacks also tell them
apart:

| Model | LAN ports | Profile |
|---|---|---|
| Archer BE550 v2 | 4x 2.5G | `tplink_archer-be550-v2` |
| Archer BE9300 v2 | 1x 2.5G + 3x 1G | `tplink_archer-be9300-v2` |

Each image lists only its own model, so the stock web UI rejects the
image of the other model.

## Hardware

| Block | Detail |
|---|---|
| SoC | Qualcomm IPQ5332, 4x Cortex-A53, reference design AP-MI01.6 |
| RAM | 512 MiB DDR4 |
| Flash | 128 MiB SPI NAND, two firmware slots (A/B) |
| Wi-Fi 2.4 GHz | IPQ5332 internal radio, ath12k (AHB) |
| Wi-Fi 5 + 6 GHz | one QCN9224 (QCN9274 family), dual-MAC, ath12k (PCIe) |
| WAN | 2.5G, Realtek RTL8221B |
| LAN, BE550 v2 | 4x 2.5G, Realtek RTL8372 switch (DSA, `kmod-rtl837x-dsa`) |
| LAN, BE9300 v2 | RTL8367D-family switch (ID 0x6642) + second RTL8221B |
| USB | 1x USB 3.0 |
| LED | RGB LED driven by a microcontroller on I2C (address 0x50) |
| Buttons | reset, WPS, Wi-Fi, LED |
| UART | 115200 8N1 |

Full map (partitions, GPIOs, switch setup, LED protocol, stock upgrade
format): [docs/be550-v2/HARDWARE.md](docs/be550-v2/HARDWARE.md).
Stock device trees: [docs/be550-v2/stock/](docs/be550-v2/stock/).

## Status

Nothing in this table is confirmed on a router. "Built" means the code is
in the image and was checked against stock data or stock code.

| Feature | State | Note |
|---|---|---|
| Image build (both models) | Built | |
| Stock web UI accepts the image | Built | header, MD5, support list and version checked against the stock upgrade tool |
| Boot | Unknown | the FIT config name TP-Link U-Boot asks for is not known; the image carries four candidates |
| WAN 2.5G | Built | |
| LAN switch, BE550 v2 | Built | register setup matches the stock driver |
| LAN port order, BE550 v2 | Built | from the stock configuration, not checked with a cable |
| LAN switch, BE9300 v2 | Built | unmanaged, see below |
| Wi-Fi 2.4 / 5 / 6 GHz | Unknown | firmware signing, see "Wi-Fi firmware" |
| Wi-Fi MAC addresses, calibration | Built | read from the stock partitions |
| LED | Built | frames equal the stock ones byte for byte, not tried on the microcontroller |
| Buttons, USB | Built | |
| sysupgrade (A/B slots) | Built | |
| Return to stock | Untested | see "Return to stock" |

### Wi-Fi firmware

Both Wi-Fi chips check the signature of their firmware, and the firmware
that OpenWrt ships may be refused:

| Chip | Stock firmware | OpenWrt firmware |
|---|---|---|
| IPQ5332 Q6 (2.4 GHz) | TP-Link certificate chain | Qualcomm |
| QCN9274 family (5 + 6 GHz) | Qualcomm production root | Qualcomm test root |

The Archer BE550 v1 refuses the OpenWrt 5/6 GHz firmware for this reason
(`mhi mhi0: Image transfer failed`, `ERROR_CODE value: 0xef0a0002`) and
runs with the stock one. The v2 probably does the same. The three radios
are linked for MLO, so if one firmware is refused, no band comes up.

The stock firmware cannot be redistributed. The image takes it from your
own router instead, without user action:

1. The first boot copies the stock Wi-Fi firmware from the stock slot to
   `/etc/be550v2` (about 15 MB). A sysupgrade keeps it.
2. `be550v2-wifi-check` runs at each boot. If no Wi-Fi device appears in
   180 seconds, it changes to the next firmware set and reboots:

   | Set | 2.4 GHz | 5 + 6 GHz |
   |---|---|---|
   | `upstream` | OpenWrt | OpenWrt |
   | `stock-qcn` | OpenWrt | stock |
   | `stock-all` | stock | stock |
   | `stock-q6` | stock | OpenWrt |

3. After the last set it restores the OpenWrt firmware and stops. The
   kernel log of each failed set is in `/etc/be550v2/wifi-fail.log`.

So the first boot can take up to three extra reboots, about 4 minutes
each. State: `cat /etc/be550v2/wifi-fw-state` (for example
`stock-qcn-ok`). Manual control:

```sh
be550v2-stock-wifi-fw status
be550v2-stock-wifi-fw install qcn      # all | q6 | qcn
be550v2-stock-wifi-fw remove
```

When the image runs from RAM (initramfs), the check does not reboot. Use
the commands above and reload the ath12k modules.

It is possible that no firmware set works with ath12k. Then Wi-Fi stays
down until someone finds the cause.

### Archer BE9300 v2

No Linux driver supports this board's LAN switch with two SerDes ports.
The image does this instead:

- `kmod-be9300v2-switch` checks the chip ID, then replays the 602 register
  writes of the stock firmware for the switch, and the stock setup of the
  LAN1 PHY. The list came from running the stock switch code in a harness
  ([contrib/ssdk-harness](contrib/ssdk-harness)).
- The switch is unmanaged: one `lan` interface, all four LAN jacks in one
  segment, no link state, VLAN or statistics per port.
- `be9300v2-lan-rescue`: 3 minutes after boot, if no frame arrived on
  `lan` and WAN has no IPv4 address, the WAN jack joins `br-lan` until the
  next reboot. A PC on the WAN jack then reaches 192.168.1.1 even if the
  switch setup failed.

This model carries more risk than the BE550 v2. Check after install:
`dmesg | grep -i be9300v2` and `logread | grep rescue`.

## Build

Build on Linux (Debian or Ubuntu), or in a container. Do not build as
root. On macOS use Docker, with the tree on a Linux volume: the build
needs a case-sensitive file system.

Packages on Debian 12:

```sh
sudo apt install build-essential clang flex bison g++ gawk gcc-multilib \
  g++-multilib gettext git libncurses-dev libssl-dev python3-setuptools \
  python3-dev rsync swig unzip zlib1g-dev file wget zstd
```

(`gcc-multilib` and `g++-multilib` do not exist on arm64 hosts; leave
them out there.)

```sh
git clone <this repository> openwrt-be550v2
cd openwrt-be550v2
./scripts/feeds update -a
./scripts/feeds install -a
cp configs/be550-v2.diffconfig .config      # or configs/be9300-v2.diffconfig
make defconfig
make -j"$(nproc)"
```

The two `diffconfig` files select the device and LuCI. Use
`make menuconfig` to add packages.

A tree holds one device at a time. To change the model, delete `.config`,
copy the other `diffconfig` and run `make defconfig` again. (If you edit
`.config` by hand, remove the `CONFIG_PACKAGE_kmod-rtl837x-dsa` and
`CONFIG_PACKAGE_kmod-be9300v2-switch` lines, or the switch driver of the
other model stays selected.)

Images are in `bin/targets/qualcommbe/ipq53xx/`:

| File | Use |
|---|---|
| `*-initramfs.itb` | boots from RAM over TFTP, writes nothing to flash (needs UART) |
| `*-squashfs-web-ui-factory.bin` | first install from the stock web UI |
| `*-squashfs-sysupgrade.bin` | later upgrades from OpenWrt |
| `*-squashfs-factory.ubi` | raw UBI image of one firmware slot, for U-Boot |

## Install

### Before you start

- Get a USB-UART adapter. Measure the voltage on the router's TX pin
  first (3.3 V or 1.8 V) and set the adapter to match. Connect GND, RX and
  TX only, never VCC. Settings: 115200 8N1.
- Without UART there is no known way to recover a router that does not
  boot. On the Archer BE550 v1, holding reset at power-on opens a U-Boot
  web page that accepts only signed TP-Link images, and there is no TFTP
  recovery. Expect the same on the v2.
- On the v1 the UART header is 4 pins (TX, RX, GND, VCC) at 3.3 V. The
  v2 header is not documented yet: measure before you connect.

### A. Test from RAM (recommended first step)

Boot the `initramfs.itb` image from U-Boot over TFTP. Flash is not
written; a power cycle returns to the stock firmware.

The exact U-Boot commands are not written here yet: the load address and
the FIT config name must come from a real unit. If you have UART access,
please send the stock boot log and the output of `printenv` (see
"Reports wanted").

### B. Install from the stock web UI

At your own risk. The stock updater writes only the inactive slot, and
the stock firmware stays in the other slot. It is not confirmed that
U-Boot falls back to that slot when the new one fails.

1. Stock web UI: Advanced > System > Firmware Update. Upload
   `*-squashfs-web-ui-factory.bin` for your model.
2. OpenWrt answers at `192.168.1.1` on LAN. SSH user `root`, no password.
   Set a password at once.
3. Back up the calibration and factory data before anything else, and
   copy the files off the router:

   ```sh
   cat /proc/mtd                    # find ART and tp_data
   cat /dev/mtdX > /tmp/art.bin
   ```

   The ART partition holds the radio calibration of your unit. It cannot
   be replaced.
4. Wait for Wi-Fi on the first boot. The router can reboot up to three
   times by itself to find a Wi-Fi firmware that the chips accept (about
   4 minutes each).

### Upgrade

From OpenWrt: `sysupgrade <file>-squashfs-sysupgrade.bin`.

### Return to stock

Untested. The stock firmware stays in the slot that OpenWrt did not
overwrite, until the first sysupgrade writes to it. The U-Boot variable
`tp_boot_idx` selects the slot (`fw_printenv tp_boot_idx`). Do not rely
on this before someone confirms it with UART attached.

## LED

```sh
be550v2-led status internet-up      # stock status events
be550v2-led on ff8800               # static colour, RRGGBB
be550v2-led mode breathing 0000ff
be550v2-led off
```

Boot, WAN up/down, reboot and sysupgrade send the stock status events.

## Reports wanted

The most useful things, in this order:

1. Stock boot log over UART, and `printenv` from U-Boot.
2. Boot log of the `initramfs.itb` image.
3. After a boot, the output of:

   ```sh
   dmesg | grep -iE 'ath12k|rtl837|be9300v2|ppe|i2c|ubi'
   cat /etc/be550v2/wifi-fw-state
   iw dev
   ip -br link
   ```

4. Which LAN jack is which interface (`lan1`..`lan4`), and whether the
   LED commands above work.

Use the "Hardware test report" issue form. Remove MAC addresses, serial
numbers and Wi-Fi keys from logs first.

## Related work

No other OpenWrt port for the hardware version 2 was found (November
2026). The v2 was requested in the
[OpenWrt forum](https://forum.openwrt.org/t/port-request-for-be550/251308)
and in the v1 pull request.

- **Archer BE550 v1** is a different platform (IPQ9554, QCA8084 switch,
  QCA8081 WAN PHY). Port: [openwrt/openwrt#25470](https://github.com/openwrt/openwrt/pull/25470),
  based on [#20778](https://github.com/openwrt/openwrt/pull/20778). It
  runs on hardware. Its device tree does not apply to the v2, but the
  two models share TP-Link's firmware layout. This port takes from it the
  web UI image tool (by Sidhant Goel, from the Archer BE800 v1 port
  [#20373](https://github.com/openwrt/openwrt/pull/20373)), the `tp_data`
  mount and the method for the two firmware slots, and from its test
  reports the fact that the 5/6 GHz chip needs the stock firmware.
- Reports from v1 users that probably apply to the v2 too, none checked
  here: 5 GHz works at 80 MHz but not 160 MHz on channel 36; 6 GHz works
  at 160 MHz but not 320 MHz; `wifi reload` on an MLO interface can leave
  it down (`wifi down; wifi up` recovers); a flood of ath12k messages on
  the serial console can stall Wi-Fi (lower `conloglevel`).
- The FCC photos of the v2 (FCC ID 2BH7FBE550V2) show a 1 GB DDR4 chip.
  The stock device tree and the stock memory configuration say 512 MiB.
  `free -m` on a running unit will settle it.

## Where the port is

| Path | Content |
|---|---|
| `target/linux/qualcommbe/dts/ipq5332-archer-be550-v2.dts` | device tree, BE550 v2 |
| `target/linux/qualcommbe/dts/ipq5332-archer-be9300-v2.dts` | device tree, BE9300 v2 |
| `target/linux/qualcommbe/image/ipq53xx.mk` | image recipes |
| `target/linux/qualcommbe/ipq53xx/base-files/` | network setup, MAC addresses, Wi-Fi firmware fallback, LED, sysupgrade |
| `scripts/tplink-mkimage-2023.py` | TP-Link web UI image container |
| `package/firmware/ipq-wifi/` | Wi-Fi board files |
| `package/utils/be550v2-i2cw/` | I2C writer for the LED microcontroller |
| `package/kernel/be9300v2-switch/` | BE9300 v2 switch setup |
| `configs/be550-v2.diffconfig`, `configs/be9300-v2.diffconfig` | build configuration |
| `docs/be550-v2/` | hardware notes, stock device trees, provenance |
| `contrib/ssdk-harness/` | harness used to get the BE9300 v2 switch setup |

The port is the commits on top of `f351b9ebcb`:
`git log --oneline f351b9ebcb..`.

## Credits

- Base tree: [perceival/openwrt-flint3](https://github.com/perceival/openwrt-flint3),
  the OpenWrt port for the GL.iNet Flint 3 (GL-BE9300). It uses the same
  Qualcomm reference design, and provides the IPQ5332 target, the RTL8372
  switch driver and the ath12k work this port depends on. The other
  documents in `docs/` and `2.4GHZ-Q6-PAS-FINDINGS.md` come from that
  project and describe the Flint 3.
- Sidhant Goel: `tplink-mkimage-2023.py` (Archer BE800 v1 port).
- Patrick Lawler and Alexandru Gagniuc: Archer BE550 v1 port, the source
  of the `tp_data` mount and of the dual-slot upgrade method.
- The testers in [openwrt/openwrt#25470](https://github.com/openwrt/openwrt/pull/25470)
  and [#20778](https://github.com/openwrt/openwrt/pull/20778) for the
  UART, recovery and Wi-Fi firmware findings on the v1.
- [OpenWrt](https://openwrt.org/).

None of them reviewed or endorses this port. Details:
[docs/be550-v2/PROVENANCE.md](docs/be550-v2/PROVENANCE.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md): where to report what, what must
not be posted, and the patch rules (OpenWrt's, with `Signed-off-by`).

## License

GPL-2.0, as OpenWrt. See [COPYING](COPYING) and [LICENSES](LICENSES).
The Wi-Fi board files in `package/firmware/ipq-wifi/` and the device trees
in `docs/be550-v2/stock/` come from the TP-Link stock firmware.
