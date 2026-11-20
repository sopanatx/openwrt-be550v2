# Contributing

This is an unofficial OpenWrt tree for the TP-Link Archer BE550 v2 and
Archer BE9300 v2. Help is welcome, most of all tests on real hardware.

## Where to report what

| Problem | Where |
|---|---|
| This port (BE550 v2 / BE9300 v2 device support) | issues of this repository |
| IPQ5332 target, ath12k, RTL8372 driver, also seen on the GL.iNet Flint 3 | [perceival/openwrt-flint3](https://github.com/perceival/openwrt-flint3) |
| OpenWrt itself, seen on an official OpenWrt build | [openwrt/openwrt](https://github.com/openwrt/openwrt) |

Do not report problems of this port to the OpenWrt project, the OpenWrt
forum or the Flint 3 project as their bug. Their developers do not
support this tree. In the OpenWrt forum, say that you run an unofficial
build.

## Test reports

Use the "Hardware test report" issue form. A report with "it does not
boot" and a UART log is as useful as a success.

Before you post a log, remove MAC addresses, serial numbers, Wi-Fi keys
and public IP addresses.

## What must not be posted here

- TP-Link stock firmware files, or files taken out of them, other than
  what the tree already carries (see
  [docs/be550-v2/PROVENANCE.md](docs/be550-v2/PROVENANCE.md)).
- Dumps of the `ART` or `tp_data` partitions. They hold the calibration,
  MAC addresses and serial data of one unit.
- Vendor source code that has no GPL-compatible licence.
- Pre-built images, until a first boot on hardware is confirmed and the
  README says so.

## Patches

The rules are those of OpenWrt:
<https://openwrt.org/submitting-patches>. In short:

- One logical change per commit. Subject `area: what changes`, 60
  characters or less. The body says why, and what you tested.
- `Signed-off-by: Your Real Name <address>` on each commit
  (`git commit -s`). With it you certify the
  [Developer Certificate of Origin](https://developercertificate.org/).
  No anonymous or `noreply` addresses.
- State the test status honestly. "Not tested on hardware" is accepted;
  a claim that something works without a test is not.
- Code from another project: keep the author of the original commit
  (`git cherry-pick -x`) when you can. When you adapt it, name the
  author and the source in the commit message and in the file.
- New files carry an SPDX licence line. The tree is GPL-2.0.

### AI-assisted changes

Large parts of this port were written with an AI assistant; see
PROVENANCE.md. The same rule applies to everyone:

- Mark the commit with `Assisted-by: <tool>:<model>`, as OpenWrt
  contributors do.
- You are the author and you sign off. Read and understand the change,
  and check each fact against the hardware, the stock firmware or the
  source. Do not submit text that you cannot explain.

## Going upstream

The goal is support in OpenWrt itself. That needs the IPQ5332 subtarget
upstream first, and a port that is confirmed on hardware. Until then
this tree stays a fork of the Flint 3 tree. Nobody should send these
commits to OpenWrt as tested work before that.
