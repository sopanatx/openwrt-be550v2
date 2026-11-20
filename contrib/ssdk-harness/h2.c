// SPDX-License-Identifier: GPL-2.0-only
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <unistd.h>

extern int rtk_switch_init(void);
extern int rtk_port_macForceLinkExt_set(unsigned port, unsigned mode, void *ab);
extern int rtk_port_sgmiiNway_set(unsigned port, unsigned en);
extern long hsl_phy_mii_reg_read(), hsl_phy_mii_reg_write(), hsl_phy_mmd_reg_read(), hsl_phy_mmd_reg_write();
extern long __hsl_phy_mii_reg_read(), __hsl_phy_mii_reg_write(), __hsl_phy_mmd_reg_read(), __hsl_phy_mmd_reg_write();
extern int rtl8221b_lan_init(unsigned, unsigned, unsigned);

static uint8_t faketask[0x4000];
static unsigned fill;			/* value of registers never written */
static uint16_t mii[32][32]; static uint8_t mii_set[32][32];
static uint16_t mmd[32][32][0x10000 / 1]; /* phy, devad -> too big; use hash below */
static uint16_t sw[0x10000]; static uint8_t sw_set[0x10000];
static unsigned sw_addr, chip_id = 0x6642, chip_ver = 0x0010;
static int lvl;	/* 0: log mdio, 1: log decoded switch regs */

struct kv { uint32_t k; uint16_t v; int used; };
static struct kv tab[1 << 16];
static uint16_t *mmd_slot(unsigned phy, unsigned dev, unsigned r, int create)
{
	uint32_t k = phy << 24 | dev << 16 | r, h = (k * 2654435761u) >> 16;
	for (;; h = (h + 1) & 0xffff) {
		if (tab[h].used && tab[h].k == k) return &tab[h].v;
		if (!tab[h].used) { if (!create) return NULL; tab[h].used = 1; tab[h].k = k; return &tab[h].v; }
	}
}
static long mii_rd(unsigned dev, unsigned phy, unsigned r)
{
	unsigned v = mii_set[phy][r] ? mii[phy][r] : fill;
	if (phy == 0 && r == 25) {
		v = sw_set[sw_addr] ? sw[sw_addr] : fill;
		if (sw_addr == 0x1300) v = chip_id;
		if (sw_addr == 0x1301) v = chip_ver;
		if (lvl) { printf("R %04x -> %04x\n", sw_addr, v); return v; }
	}
	if (!lvl || phy != 0) printf("MR %u.%u -> %04x\n", phy, r, v);
	return v;
}
static long mii_wr(unsigned dev, unsigned phy, unsigned r, unsigned v)
{
	v &= 0xffff;
	mii[phy][r] = v; mii_set[phy][r] = 1;
	if (phy == 0) {
		if (r == 23) sw_addr = v;
		if (r == 21 && v == 3) {
			sw[sw_addr] = mii[0][24]; sw_set[sw_addr] = 1;
			if (lvl) printf("W %04x <- %04x\n", sw_addr, mii[0][24]);
		}
	}
	if (!lvl || phy != 0) printf("MW %u.%u <- %04x\n", phy, r, v);
	return 0;
}
static long mmd_rd(unsigned dev, unsigned phy, unsigned c45, unsigned devad, unsigned r)
{
	uint16_t *s = mmd_slot(phy, devad, r, 0);
	unsigned v = s ? *s : fill;
	if (devad == 31 && r == 0xa420) v = (v & ~7u) | 3;
	if (devad == 1 && r == 2) v = 0x001c;
	if (devad == 1 && r == 3) v = 0xc849;
	if (devad == 1 && r == 0) v &= ~0x8000u;
	if (devad == 31 && r == 0xa400) v &= ~0x8000u;
	printf("CR %u %u.%04x c45=%u -> %04x\n", phy, devad, r, c45, v);
	return v;
}
static long mmd_wr(unsigned dev, unsigned phy, unsigned c45, unsigned devad, unsigned r, unsigned v)
{
	*mmd_slot(phy, devad, r, 1) = v;
	printf("CW %u %u.%04x c45=%u <- %04x\n", phy, devad, r, c45, v & 0xffff);
	return 0;
}
int printk(const char *fmt, ...)
{
	va_list ap; va_start(ap, fmt);
	if (fmt[0] == 1) fmt += 2;
	printf("# "); vprintf(fmt, ap); va_end(ap);
	return 0;
}
static void patch(void *fn, void *to)
{
	uint32_t *p = fn;
	uintptr_t pg = (uintptr_t)fn & ~0xfffUL;
	mprotect((void *)pg, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);
	p[0] = 0x58000050; p[1] = 0xd61f0200; memcpy(p + 2, &to, 8);
	__builtin___clear_cache((char *)p, (char *)(p + 4));
}
static void ill(int s, siginfo_t *si, void *uc_)
{
	ucontext_t *uc = uc_;
	uint32_t insn = 0;
	if (s == SIGALRM) { printf("== TIMEOUT pc=%llx\n", (unsigned long long)uc->uc_mcontext.pc); _exit(4); }
	if (s == SIGILL) insn = *(uint32_t *)uc->uc_mcontext.pc;
	if ((insn & 0xffffffe0) == 0xd5384100) {
		uc->uc_mcontext.regs[insn & 31] = (uintptr_t)faketask;
		uc->uc_mcontext.pc += 4;
		return;
	}
	printf("== SIG %d pc=%llx lr=%llx fault=%p\n", s, (unsigned long long)uc->uc_mcontext.pc,
		(unsigned long long)uc->uc_mcontext.regs[30], si->si_addr);
	_exit(3);
}
int main(int argc, char **argv)
{
	struct sigaction sa = { .sa_sigaction = ill, .sa_flags = SA_SIGINFO };
	unsigned ab7[7] = {1, 4, 1, 1, 0, 1, 1};
	int r;

	setvbuf(stdout, NULL, _IONBF, 0);
	sigaction(SIGILL, &sa, NULL); sigaction(SIGSEGV, &sa, NULL); sigaction(SIGBUS, &sa, NULL); sigaction(SIGALRM, &sa, NULL);
	alarm(20);
	fill = strtoul(argv[2], NULL, 16);
	patch(hsl_phy_mii_reg_read, mii_rd); patch(hsl_phy_mii_reg_write, mii_wr);
	patch(__hsl_phy_mii_reg_read, mii_rd); patch(__hsl_phy_mii_reg_write, mii_wr);
	patch(hsl_phy_mmd_reg_read, mmd_rd); patch(hsl_phy_mmd_reg_write, mmd_wr);
	patch(__hsl_phy_mmd_reg_read, mmd_rd); patch(__hsl_phy_mmd_reg_write, mmd_wr);
	if (!strcmp(argv[1], "sw") || !strcmp(argv[1], "swmdio")) {
		lvl = !strcmp(argv[1], "sw");
		printf("== init\n"); r = rtk_switch_init(); printf("== ret %d\n", r);
		printf("== ext17\n"); r = rtk_port_macForceLinkExt_set(17, 10, ab7); printf("== ret %d\n", r);
		printf("== nway17\n"); r = rtk_port_sgmiiNway_set(17, 0); printf("== ret %d\n", r);
		printf("== ext16\n"); r = rtk_port_macForceLinkExt_set(16, 10, ab7); printf("== ret %d\n", r);
		printf("== nway16\n"); r = rtk_port_sgmiiNway_set(16, 0); printf("== ret %d\n", r);
	} else {
		unsigned a1 = strtoul(argv[3], NULL, 0), addr = strtoul(argv[4], NULL, 0);
		printf("== phy init a1=%u addr=%u\n", a1, addr);
		r = rtl8221b_lan_init(0, a1, addr); printf("== ret %d\n", r);
	}
	return 0;
}
