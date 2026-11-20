# Stock device trees

Device trees taken from the kernel FIT of the TP-Link stock firmware
1.3.3 Build 20260623 (one firmware file serves both models):

| file | FIT config | model |
|---|---|---|
| `BE550_2_0_0.dtb` / `.dts` | `config@BE550_2_0_0` | Archer BE550 v2 |
| `BE9300_2_0_0.dtb` / `.dts` | `config@BE9300_2_0_0` | Archer BE9300 v2 |

The `.dts` files are `dtc -I dtb -O dts` output. They are reference
material for the OpenWrt device trees in
`target/linux/qualcommbe/dts/ipq5332-archer-be*-v2.dts`; the build does
not use them.
