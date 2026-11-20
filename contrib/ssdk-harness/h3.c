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
extern int rtl8367d_setAsicReg(), rtl8367d_getAsicReg(), rtl8367d_setAsicRegBit(), rtl8367d_getAsicRegBit(), rtl8367d_setAsicRegBits(), rtl8367d_getAsicRegBits();
extern int rtl8367c_smi_read(), rtl8367c_smi_write();
static int c_rd(unsigned a, unsigned *v){*v=0;return 0;}
static int c_wr(unsigned a, unsigned v){return 0;}
static uint8_t faketask[0x4000];
static unsigned fill; static uint16_t sw[0x10000]; static uint8_t sw_set[0x10000];
static unsigned rd(unsigned a){ a&=0xffff; if(a==0x1300) return 0x6642; if(a==0x1301) return 0x0010; return sw_set[a]?sw[a]:fill; }
static void wr(unsigned a, unsigned v){ a&=0xffff; sw[a]=v; sw_set[a]=1; }
static int lsb(unsigned m){ int i=0; while(!(m&1)){m>>=1;i++;} return i; }
static int s_reg(unsigned a, unsigned v){ printf("SET  %04x %04x\n",a,v&0xffff); wr(a,v); return 0; }
static int g_reg(unsigned a, unsigned *v){ *v=rd(a); printf("GET  %04x -> %04x\n",a,*v); return 0; }
static int s_bit(unsigned a, unsigned b, unsigned v){ printf("BIT  %04x %u %u\n",a,b,v); wr(a,(rd(a)&~(1u<<b))|((v&1)<<b)); return 0; }
static int g_bit(unsigned a, unsigned b, unsigned *v){ *v=(rd(a)>>b)&1; printf("GBIT %04x %u -> %u\n",a,b,*v); return 0; }
static int s_bits(unsigned a, unsigned m, unsigned v){ printf("BITS %04x %04x %x\n",a,m,v); wr(a,(rd(a)&~m)|((v<<lsb(m))&m)); return 0; }
static int g_bits(unsigned a, unsigned m, unsigned *v){ *v=(rd(a)&m)>>lsb(m); printf("GBTS %04x %04x -> %x\n",a,m,*v); return 0; }
int printk(const char *fmt, ...){ va_list ap; va_start(ap, fmt); if (fmt[0]==1) fmt+=2; printf("# "); vprintf(fmt, ap); va_end(ap); return 0; }
static void patch(void *fn, void *to){ uint32_t *p=fn; uintptr_t pg=(uintptr_t)fn&~0xfffUL; mprotect((void*)pg,0x2000,PROT_READ|PROT_WRITE|PROT_EXEC); p[0]=0x58000050; p[1]=0xd61f0200; memcpy(p+2,&to,8); __builtin___clear_cache((char*)p,(char*)(p+4)); }
static void ill(int s, siginfo_t *si, void *uc_){ ucontext_t *uc=uc_; uint32_t insn=0; if(s==SIGILL) insn=*(uint32_t*)uc->uc_mcontext.pc;
	if((insn&0xffffffe0)==0xd5384100){ uc->uc_mcontext.regs[insn&31]=(uintptr_t)faketask; uc->uc_mcontext.pc+=4; return; }
	printf("== SIG %d pc=%llx\n",s,(unsigned long long)uc->uc_mcontext.pc); _exit(3); }
int main(int argc, char **argv){
	struct sigaction sa={.sa_sigaction=ill,.sa_flags=SA_SIGINFO}; unsigned ab7[7]={1,4,1,1,0,1,1}; int r;
	setvbuf(stdout,NULL,_IONBF,0); sigaction(SIGILL,&sa,NULL); sigaction(SIGSEGV,&sa,NULL); sigaction(SIGBUS,&sa,NULL);
	fill=strtoul(argv[1],NULL,16);
	patch(rtl8367c_smi_read,c_rd); patch(rtl8367c_smi_write,c_wr); patch(rtl8367d_setAsicReg,s_reg); patch(rtl8367d_getAsicReg,g_reg); patch(rtl8367d_setAsicRegBit,s_bit); patch(rtl8367d_getAsicRegBit,g_bit); patch(rtl8367d_setAsicRegBits,s_bits); patch(rtl8367d_getAsicRegBits,g_bits);
	printf("== init\n"); r=rtk_switch_init(); printf("== ret %d\n",r);
	printf("== ext17\n"); r=rtk_port_macForceLinkExt_set(17,10,ab7); printf("== ret %d\n",r);
	printf("== nway17\n"); r=rtk_port_sgmiiNway_set(17,0); printf("== ret %d\n",r);
	printf("== ext16\n"); r=rtk_port_macForceLinkExt_set(16,10,ab7); printf("== ret %d\n",r);
	printf("== nway16\n"); r=rtk_port_sgmiiNway_set(16,0); printf("== ret %d\n",r);
	return 0; }
