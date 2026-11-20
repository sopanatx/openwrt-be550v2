// SPDX-License-Identifier: GPL-2.0
/*
 * TP-Link Archer BE9300 v2: bring-up of the LAN switch, unmanaged.
 *
 * The board has a Realtek switch of the rtl8367d family (chip ID 0x6642)
 * at MDIO address 29. Both external ports are SerDes ports in HSGMII
 * (2.5 Gbit/s) mode: port 17 goes to the IPQ5332 (PPE port 2), port 16 to
 * an RTL8221B PHY (MDIO address 6) that is the 2.5G "LAN1" jack. LAN2-4
 * are the switch's own 1G PHYs.
 *
 * No Linux driver handles this chip family with two SerDes ports. This
 * module only replays the register writes of the stock firmware (see
 * be9300v2-switch-seq.h) and the stock RTL8221B setup, then leaves the
 * switch in its default state: all ports in one VLAN, no CPU tag. Linux
 * sees one "lan" interface behind it.
 *
 * Not tested on hardware.
 */

#include <linux/delay.h>
#include <linux/mdio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy.h>

#define OP_SET	0	/* reg <- a */
#define OP_BITS	1	/* reg <- (reg & ~a) | ((b << lsb(a)) & a) */

struct be9300v2_op {
	u8 op;
	u16 reg;
	u16 a;
	u16 b;
};

#include "be9300v2-switch-seq.h"

/* Realtek SMI over MDIO, as in the stock rtl8367s_mdio_read/write */
#define SMI_CTRL0_REG		31
#define SMI_CTRL0_ADDR_OP	0x000e
#define SMI_CTRL1_REG		21
#define SMI_CTRL1_READ		0x0001
#define SMI_CTRL1_WRITE		0x0003
#define SMI_ADDR_REG		23
#define SMI_WDATA_REG		24
#define SMI_RDATA_REG		25

#define REG_MAGIC		0x13c2
#define REG_MAGIC_UNLOCK	0x0249
#define REG_CHIP_NUMBER		0x1300
#define REG_CHIP_VER		0x1301
#define CHIP_NUMBER_8367D	0x6642

/* reads the stock code takes branches on; expected 0 */
#define REG_TOP_CON0		0x1d70
#define TOP_CON0_MAC4_SEL_EXT1	BIT(12)
#define REG_13EB		0x13eb
#define REG_13EB_MASK		0xc000

#define RTL8221B_ID1		0x001c

static int smi_read(struct mdio_device *m, u16 addr, u16 *val)
{
	struct mii_bus *bus = m->bus;
	int ret;

	mutex_lock(&bus->mdio_lock);
	ret = __mdiobus_write(bus, m->addr, SMI_CTRL0_REG, SMI_CTRL0_ADDR_OP);
	if (!ret)
		ret = __mdiobus_write(bus, m->addr, SMI_ADDR_REG, addr);
	if (!ret)
		ret = __mdiobus_write(bus, m->addr, SMI_CTRL1_REG, SMI_CTRL1_READ);
	if (!ret)
		ret = __mdiobus_read(bus, m->addr, SMI_RDATA_REG);
	mutex_unlock(&bus->mdio_lock);
	if (ret < 0)
		return ret;
	*val = ret;
	return 0;
}

static int smi_write(struct mdio_device *m, u16 addr, u16 val)
{
	struct mii_bus *bus = m->bus;
	int ret;

	mutex_lock(&bus->mdio_lock);
	ret = __mdiobus_write(bus, m->addr, SMI_CTRL0_REG, SMI_CTRL0_ADDR_OP);
	if (!ret)
		ret = __mdiobus_write(bus, m->addr, SMI_ADDR_REG, addr);
	if (!ret)
		ret = __mdiobus_write(bus, m->addr, SMI_WDATA_REG, val);
	if (!ret)
		ret = __mdiobus_write(bus, m->addr, SMI_CTRL1_REG, SMI_CTRL1_WRITE);
	mutex_unlock(&bus->mdio_lock);
	return ret;
}

static int be9300v2_run(struct mdio_device *m, const struct be9300v2_op *seq,
			unsigned int n)
{
	unsigned int i;
	u16 v;
	int ret;

	for (i = 0; i < n; i++) {
		if (seq[i].op == OP_SET) {
			ret = smi_write(m, seq[i].reg, seq[i].a);
		} else {
			ret = smi_read(m, seq[i].reg, &v);
			if (ret)
				return ret;
			v &= ~seq[i].a;
			v |= (seq[i].b << __ffs(seq[i].a)) & seq[i].a;
			ret = smi_write(m, seq[i].reg, v);
		}
		if (ret)
			return ret;
	}
	return 0;
}

static int be9300v2_chip_id(struct mdio_device *m, u16 *num, u16 *ver)
{
	int ret;

	ret = smi_write(m, REG_MAGIC, REG_MAGIC_UNLOCK);
	if (ret)
		return ret;
	ret = smi_read(m, REG_CHIP_NUMBER, num);
	if (!ret)
		ret = smi_read(m, REG_CHIP_VER, ver);
	smi_write(m, REG_MAGIC, 0);
	return ret;
}

/* LAN1 PHY: the writes of the stock rtl8221b_init_common() */
static int phy_mod(struct mii_bus *bus, int addr, int devad, u16 reg,
		   u16 clear, u16 set)
{
	int v = mdiobus_c45_read(bus, addr, devad, reg);

	if (v < 0)
		return v;
	return mdiobus_c45_write(bus, addr, devad, reg, (v & ~clear) | set);
}

static int be9300v2_lan1_phy(struct device *dev, struct mii_bus *bus, int addr)
{
	int i, v, ret = 0;

	/* the PHY may just have left reset */
	for (i = 0; i < 20; i++) {
		v = mdiobus_c45_read(bus, addr, MDIO_MMD_PMAPMD, MDIO_DEVID1);
		if (v == RTL8221B_ID1)
			break;
		msleep(50);
	}
	if (v != RTL8221B_ID1) {
		dev_warn(dev, "LAN1 PHY not found at address %d (id1 %04x)\n",
			 addr, v);
		return -ENODEV;
	}

	for (i = 0; i < 20; i++) {
		v = mdiobus_c45_read(bus, addr, MDIO_MMD_VEND2, 0xa420);
		if (v >= 0 && (v & 0x7) == 0x3)
			break;
		msleep(50);
	}

	/* SerDes auto-negotiation off */
	ret = ret ?: mdiobus_c45_write(bus, addr, MDIO_MMD_VEND1, 0x7588, 0x0002);
	ret = ret ?: mdiobus_c45_write(bus, addr, MDIO_MMD_VEND1, 0x7589, 0x71d0);
	ret = ret ?: mdiobus_c45_write(bus, addr, MDIO_MMD_VEND1, 0x7587, 0x0003);
	/* SerDes option 3: HiSGMII for every copper speed */
	ret = ret ?: phy_mod(bus, addr, MDIO_MMD_VEND1, 0x75f3, BIT(0), 0);
	ret = ret ?: phy_mod(bus, addr, MDIO_MMD_VEND1, 0x697a, 0x003f, 0x0003);
	ret = ret ?: mdiobus_c45_write(bus, addr, MDIO_MMD_VEND1, 0x6a04, 0x0503);
	ret = ret ?: mdiobus_c45_write(bus, addr, MDIO_MMD_VEND1, 0x6f10, 0xd433);
	ret = ret ?: mdiobus_c45_write(bus, addr, MDIO_MMD_VEND1, 0x6f11, 0x8020);
	if (ret)
		return ret;

	/* stock: toggle a400 bit 14, then PMA reset */
	ret = phy_mod(bus, addr, MDIO_MMD_VEND2, 0xa400, 0, BIT(14));
	msleep(20);
	ret = ret ?: phy_mod(bus, addr, MDIO_MMD_VEND2, 0xa400, BIT(14), 0);
	ret = ret ?: phy_mod(bus, addr, MDIO_MMD_PMAPMD, MDIO_CTRL1, 0,
			     MDIO_CTRL1_RESET);
	if (ret)
		return ret;
	for (i = 0; i < 20; i++) {
		msleep(50);
		v = mdiobus_c45_read(bus, addr, MDIO_MMD_PMAPMD, MDIO_CTRL1);
		if (v >= 0 && !(v & MDIO_CTRL1_RESET))
			break;
	}

	/* advertise 10/100/1000/2500, pause; no EEE */
	ret = ret ?: phy_mod(bus, addr, MDIO_MMD_AN, MDIO_AN_ADVERTISE, 0, 0x0de0);
	ret = ret ?: phy_mod(bus, addr, MDIO_MMD_VEND2, 0xa412, 0, BIT(9));
	ret = ret ?: phy_mod(bus, addr, MDIO_MMD_AN, MDIO_AN_10GBT_CTRL, 0,
			     MDIO_AN_10GBT_CTRL_ADV2_5G);
	ret = ret ?: mdiobus_c45_write(bus, addr, MDIO_MMD_AN, MDIO_AN_EEE_ADV, 0);
	ret = ret ?: mdiobus_c45_write(bus, addr, MDIO_MMD_AN, MDIO_AN_EEE_ADV2, 0);
	/* stock powers the PHY down here and up again later; keep it up */
	ret = ret ?: phy_mod(bus, addr, MDIO_MMD_PMAPMD, MDIO_CTRL1,
			     MDIO_CTRL1_LPOWER, 0);
	ret = ret ?: phy_mod(bus, addr, MDIO_MMD_AN, MDIO_CTRL1, 0,
			     MDIO_AN_CTRL1_ENABLE | MDIO_AN_CTRL1_RESTART);
	return ret;
}

static int be9300v2_switch_probe(struct mdio_device *m)
{
	struct device *dev = &m->dev;
	u32 phy_addr = 6;
	u16 num = 0, ver = 0, v;
	int ret;

	ret = be9300v2_chip_id(m, &num, &ver);
	if (ret)
		return dev_err_probe(dev, ret, "cannot read the chip ID\n");
	if (num != CHIP_NUMBER_8367D) {
		dev_err(dev, "chip ID %04x (version %04x) is not 0x6642, switch left alone\n",
			num, ver);
		return -ENODEV;
	}
	dev_info(dev, "Realtek switch, chip ID %04x version %04x\n", num, ver);

	if (!smi_read(m, REG_TOP_CON0, &v) && (v & TOP_CON0_MAC4_SEL_EXT1))
		dev_warn(dev, "reg 1d70 = %04x: stock takes another path here\n", v);
	if (!smi_read(m, REG_13EB, &v) && (v & REG_13EB_MASK))
		dev_warn(dev, "reg 13eb = %04x: stock takes another path here\n", v);

	ret = be9300v2_run(m, be9300v2_seq_cpu, ARRAY_SIZE(be9300v2_seq_cpu));
	ret = ret ?: be9300v2_run(m, be9300v2_seq_cpu_nway,
				  ARRAY_SIZE(be9300v2_seq_cpu_nway));
	if (ret)
		return dev_err_probe(dev, ret, "CPU port (17) setup failed\n");

	ret = be9300v2_run(m, be9300v2_seq_lan1, ARRAY_SIZE(be9300v2_seq_lan1));
	ret = ret ?: be9300v2_run(m, be9300v2_seq_lan1_nway,
				  ARRAY_SIZE(be9300v2_seq_lan1_nway));
	if (ret)
		dev_warn(dev, "LAN1 port (16) setup failed: %d\n", ret);

	of_property_read_u32(dev->of_node, "tplink,lan1-phy-addr", &phy_addr);
	ret = be9300v2_lan1_phy(dev, m->bus, phy_addr);
	if (ret)
		dev_warn(dev, "LAN1 PHY setup failed: %d\n", ret);

	dev_info(dev, "switch set up, unmanaged\n");
	return 0;
}

static const struct of_device_id be9300v2_switch_of_match[] = {
	{ .compatible = "tplink,archer-be9300-v2-switch" },
	{ }
};
MODULE_DEVICE_TABLE(of, be9300v2_switch_of_match);

static struct mdio_driver be9300v2_switch_driver = {
	.probe = be9300v2_switch_probe,
	.mdiodrv.driver = {
		.name = "be9300v2-switch",
		.of_match_table = be9300v2_switch_of_match,
	},
};
mdio_module_driver(be9300v2_switch_driver);

MODULE_DESCRIPTION("TP-Link Archer BE9300 v2 LAN switch bring-up");
MODULE_LICENSE("GPL");
