# Stock switch code harness

Runs functions of the stock `qca-ssdk.ko` (aarch64, firmware 1.3.3) as a
normal Linux aarch64 program and logs the register accesses. Used to get
the Archer BE9300 v2 switch and LAN1 PHY setup without hardware.

How it works: the `.ko` is a relocatable object. `objcopy
--globalize-symbol` exposes the entry points, undefined kernel symbols get
weak stubs that return 0, and the low-level access functions are patched
at run time to call a register model. `mrs xN, sp_el0` (kernel stack
canary) is emulated in a SIGILL handler.

    nm -u ssdk.ko | awk '{print $2}' | sort -u > und.txt        # minus libc names
    # stubs.c: __attribute__((weak)) long NAME(){return 0;} per symbol
    objcopy --globalize-symbol=... ssdk.ko ssdk_g.o
    gcc -w -O0 -static -no-pie -o h3 h3.c stubs.o ssdk_g.o -Wl,--allow-multiple-definition

- `h3.c`: switch, logs `rtl8367d_setAsicReg/RegBit/RegBits` (source of
  `be9300v2-switch-seq.h`). Argument: value of registers never written.
- `h2.c`: MDIO level (`hsl_phy_mii/mmd_reg_read/write`): SMI protocol and
  the RTL8221B init (`./h2 phy 0 1 6`).

Findings: chip ID 0x6367 (classic RTL8367S) rejects HSGMII on external
port 17 (error 3); chip ID 0x6642 accepts it on ports 16 and 17.
