#include "m6502.h"

#define FC M6502_FC
#define FZ M6502_FZ
#define FI M6502_FI
#define FD M6502_FD
#define FB M6502_FB
#define FU M6502_FU
#define FV M6502_FV
#define FN M6502_FN

#define RD(a)    cpu->read (cpu, (uint16_t)(a))
#define WR(a, v) cpu->write(cpu, (uint16_t)(a), (uint8_t)(v))
#define PUSH(v)  do { WR(0x0100 | cpu->sp, (v)); cpu->sp--; } while(0)
#define POP()    (cpu->sp++, RD(0x0100 | cpu->sp))

#define SET_NZ(v) do { \
    uint8_t _t = (uint8_t)(v); \
    cpu->p = (cpu->p & ~(FN|FZ)) | (_t & FN) | (_t ? 0 : FZ); \
} while(0)

static uint16_t rd16(m6502_t *cpu, uint16_t addr) {
    return (uint16_t)RD(addr) | ((uint16_t)RD(addr + 1) << 8);
}

/* JMP ($xxFF) 6502 page-wrap bug */
static uint16_t rd16_bug(m6502_t *cpu, uint16_t addr) {
    uint16_t hi_addr = (addr & 0xFF00) | ((addr + 1) & 0x00FF);
    return (uint16_t)RD(addr) | ((uint16_t)RD(hi_addr) << 8);
}

static void do_adc(m6502_t *cpu, uint8_t v) {
    uint16_t tmp = (uint16_t)cpu->a + v + (cpu->p & FC);
    uint8_t  res = (uint8_t)tmp;
    cpu->p = (cpu->p & ~(FC|FV|FN|FZ))
           | (tmp > 0xFF ? FC : 0)
           | ((~(cpu->a ^ v) & (cpu->a ^ res) & 0x80) ? FV : 0);
    cpu->a = res;
    SET_NZ(cpu->a);
}

static void do_sbc(m6502_t *cpu, uint8_t v) { do_adc(cpu, v ^ 0xFF); }

static void do_cmp(m6502_t *cpu, uint8_t reg, uint8_t v) {
    uint8_t r = reg - v;
    cpu->p = (cpu->p & ~(FC|FN|FZ)) | (reg >= v ? FC : 0);
    SET_NZ(r);
}

static void asl_mem(m6502_t *cpu, uint16_t ea) {
    uint8_t v = RD(ea);
    cpu->p = (cpu->p & ~FC) | (v >> 7);
    v <<= 1; WR(ea, v); SET_NZ(v);
}
static void lsr_mem(m6502_t *cpu, uint16_t ea) {
    uint8_t v = RD(ea);
    cpu->p = (cpu->p & ~FC) | (v & 1);
    v >>= 1; WR(ea, v); SET_NZ(v);
}
static void rol_mem(m6502_t *cpu, uint16_t ea) {
    uint8_t v = RD(ea), r = (v << 1) | (cpu->p & FC);
    cpu->p = (cpu->p & ~FC) | (v >> 7);
    WR(ea, r); SET_NZ(r);
}
static void ror_mem(m6502_t *cpu, uint16_t ea) {
    uint8_t v = RD(ea), r = (v >> 1) | ((cpu->p & FC) << 7);
    cpu->p = (cpu->p & ~FC) | (v & 1);
    WR(ea, r); SET_NZ(r);
}
static void inc_mem(m6502_t *cpu, uint16_t ea) {
    uint8_t v = RD(ea) + 1; WR(ea, v); SET_NZ(v);
}
static void dec_mem(m6502_t *cpu, uint16_t ea) {
    uint8_t v = RD(ea) - 1; WR(ea, v); SET_NZ(v);
}

static int branch(m6502_t *cpu, int cond) {
    int8_t off = (int8_t)RD(cpu->pc++);
    if (!cond) return 2;
    uint16_t old = cpu->pc;
    cpu->pc = (uint16_t)(cpu->pc + off);
    return 3 + ((old & 0xFF00) != (cpu->pc & 0xFF00) ? 1 : 0);
}

void m6502_reset(m6502_t *cpu) {
    cpu->pc = rd16(cpu, 0xFFFC);
    cpu->sp = 0xFD;
    cpu->p  = FI | FU;
    cpu->a = cpu->x = cpu->y = 0;
    cpu->irq = cpu->nmi = 0;
}

M6502_IRAM int m6502_step(m6502_t *cpu) {
    uint8_t  op, tmp8;
    uint16_t ea;
    int      cross;

    if (cpu->nmi) {
        cpu->nmi = 0;
        PUSH(cpu->pc >> 8); PUSH(cpu->pc & 0xFF);
        PUSH(cpu->p & ~FB);
        cpu->p = (cpu->p | FI) & ~FB;
        cpu->pc = rd16(cpu, 0xFFFA);
        return 7;
    }
    if (cpu->irq && !(cpu->p & FI)) {
        PUSH(cpu->pc >> 8); PUSH(cpu->pc & 0xFF);
        PUSH(cpu->p & ~FB);
        cpu->p |= FI;
        cpu->pc = rd16(cpu, 0xFFFE);
        return 7;
    }

    op    = RD(cpu->pc++);
    cross = 0;

    switch (op) {
    /* LDA */
    case 0xA9: cpu->a = RD(cpu->pc++); SET_NZ(cpu->a); return 2;
    case 0xA5: cpu->a = RD(RD(cpu->pc++)); SET_NZ(cpu->a); return 3;
    case 0xB5: cpu->a = RD((RD(cpu->pc++) + cpu->x) & 0xFF); SET_NZ(cpu->a); return 4;
    case 0xAD: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cpu->a=RD(ea); SET_NZ(cpu->a); return 4;
    case 0xBD: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->x))>>8)&1; cpu->a=RD(ea+cpu->x); SET_NZ(cpu->a); return 4+cross;
    case 0xB9: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->y))>>8)&1; cpu->a=RD(ea+cpu->y); SET_NZ(cpu->a); return 4+cross;
    case 0xA1: { uint8_t p=(RD(cpu->pc++)+cpu->x)&0xFF; ea=rd16(cpu,p); cpu->a=RD(ea); SET_NZ(cpu->a); return 6; }
    case 0xB1: { uint8_t p=RD(cpu->pc++); ea=rd16(cpu,p); cross=((ea^(ea+cpu->y))>>8)&1; cpu->a=RD(ea+cpu->y); SET_NZ(cpu->a); return 5+cross; }
    /* LDX */
    case 0xA2: cpu->x=RD(cpu->pc++); SET_NZ(cpu->x); return 2;
    case 0xA6: cpu->x=RD(RD(cpu->pc++)); SET_NZ(cpu->x); return 3;
    case 0xB6: cpu->x=RD((RD(cpu->pc++)+cpu->y)&0xFF); SET_NZ(cpu->x); return 4;
    case 0xAE: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cpu->x=RD(ea); SET_NZ(cpu->x); return 4;
    case 0xBE: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->y))>>8)&1; cpu->x=RD(ea+cpu->y); SET_NZ(cpu->x); return 4+cross;
    /* LDY */
    case 0xA0: cpu->y=RD(cpu->pc++); SET_NZ(cpu->y); return 2;
    case 0xA4: cpu->y=RD(RD(cpu->pc++)); SET_NZ(cpu->y); return 3;
    case 0xB4: cpu->y=RD((RD(cpu->pc++)+cpu->x)&0xFF); SET_NZ(cpu->y); return 4;
    case 0xAC: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cpu->y=RD(ea); SET_NZ(cpu->y); return 4;
    case 0xBC: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->x))>>8)&1; cpu->y=RD(ea+cpu->x); SET_NZ(cpu->y); return 4+cross;
    /* STA */
    case 0x85: WR(RD(cpu->pc++), cpu->a); return 3;
    case 0x95: WR((RD(cpu->pc++)+cpu->x)&0xFF, cpu->a); return 4;
    case 0x8D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; WR(ea, cpu->a); return 4;
    case 0x9D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; WR(ea+cpu->x, cpu->a); return 5;
    case 0x99: ea=rd16(cpu,cpu->pc); cpu->pc+=2; WR(ea+cpu->y, cpu->a); return 5;
    case 0x81: { uint8_t p=(RD(cpu->pc++)+cpu->x)&0xFF; ea=rd16(cpu,p); WR(ea, cpu->a); return 6; }
    case 0x91: { uint8_t p=RD(cpu->pc++); ea=rd16(cpu,p); WR(ea+cpu->y, cpu->a); return 6; }
    /* STX */
    case 0x86: WR(RD(cpu->pc++), cpu->x); return 3;
    case 0x96: WR((RD(cpu->pc++)+cpu->y)&0xFF, cpu->x); return 4;
    case 0x8E: ea=rd16(cpu,cpu->pc); cpu->pc+=2; WR(ea, cpu->x); return 4;
    /* STY */
    case 0x84: WR(RD(cpu->pc++), cpu->y); return 3;
    case 0x94: WR((RD(cpu->pc++)+cpu->x)&0xFF, cpu->y); return 4;
    case 0x8C: ea=rd16(cpu,cpu->pc); cpu->pc+=2; WR(ea, cpu->y); return 4;
    /* Transfers */
    case 0xAA: cpu->x=cpu->a; SET_NZ(cpu->x); return 2;
    case 0xA8: cpu->y=cpu->a; SET_NZ(cpu->y); return 2;
    case 0x8A: cpu->a=cpu->x; SET_NZ(cpu->a); return 2;
    case 0x98: cpu->a=cpu->y; SET_NZ(cpu->a); return 2;
    case 0xBA: cpu->x=cpu->sp; SET_NZ(cpu->x); return 2;
    case 0x9A: cpu->sp=cpu->x; return 2;
    /* Stack */
    case 0x48: PUSH(cpu->a); return 3;
    case 0x68: cpu->a=POP(); SET_NZ(cpu->a); return 4;
    case 0x08: PUSH(cpu->p | FB | FU); return 3;
    case 0x28: cpu->p=(POP() | FU) & ~FB; return 4;
    /* AND */
    case 0x29: cpu->a&=RD(cpu->pc++); SET_NZ(cpu->a); return 2;
    case 0x25: cpu->a&=RD(RD(cpu->pc++)); SET_NZ(cpu->a); return 3;
    case 0x35: cpu->a&=RD((RD(cpu->pc++)+cpu->x)&0xFF); SET_NZ(cpu->a); return 4;
    case 0x2D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cpu->a&=RD(ea); SET_NZ(cpu->a); return 4;
    case 0x3D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->x))>>8)&1; cpu->a&=RD(ea+cpu->x); SET_NZ(cpu->a); return 4+cross;
    case 0x39: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->y))>>8)&1; cpu->a&=RD(ea+cpu->y); SET_NZ(cpu->a); return 4+cross;
    case 0x21: { uint8_t p=(RD(cpu->pc++)+cpu->x)&0xFF; ea=rd16(cpu,p); cpu->a&=RD(ea); SET_NZ(cpu->a); return 6; }
    case 0x31: { uint8_t p=RD(cpu->pc++); ea=rd16(cpu,p); cross=((ea^(ea+cpu->y))>>8)&1; cpu->a&=RD(ea+cpu->y); SET_NZ(cpu->a); return 5+cross; }
    /* ORA */
    case 0x09: cpu->a|=RD(cpu->pc++); SET_NZ(cpu->a); return 2;
    case 0x05: cpu->a|=RD(RD(cpu->pc++)); SET_NZ(cpu->a); return 3;
    case 0x15: cpu->a|=RD((RD(cpu->pc++)+cpu->x)&0xFF); SET_NZ(cpu->a); return 4;
    case 0x0D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cpu->a|=RD(ea); SET_NZ(cpu->a); return 4;
    case 0x1D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->x))>>8)&1; cpu->a|=RD(ea+cpu->x); SET_NZ(cpu->a); return 4+cross;
    case 0x19: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->y))>>8)&1; cpu->a|=RD(ea+cpu->y); SET_NZ(cpu->a); return 4+cross;
    case 0x01: { uint8_t p=(RD(cpu->pc++)+cpu->x)&0xFF; ea=rd16(cpu,p); cpu->a|=RD(ea); SET_NZ(cpu->a); return 6; }
    case 0x11: { uint8_t p=RD(cpu->pc++); ea=rd16(cpu,p); cross=((ea^(ea+cpu->y))>>8)&1; cpu->a|=RD(ea+cpu->y); SET_NZ(cpu->a); return 5+cross; }
    /* EOR */
    case 0x49: cpu->a^=RD(cpu->pc++); SET_NZ(cpu->a); return 2;
    case 0x45: cpu->a^=RD(RD(cpu->pc++)); SET_NZ(cpu->a); return 3;
    case 0x55: cpu->a^=RD((RD(cpu->pc++)+cpu->x)&0xFF); SET_NZ(cpu->a); return 4;
    case 0x4D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cpu->a^=RD(ea); SET_NZ(cpu->a); return 4;
    case 0x5D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->x))>>8)&1; cpu->a^=RD(ea+cpu->x); SET_NZ(cpu->a); return 4+cross;
    case 0x59: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->y))>>8)&1; cpu->a^=RD(ea+cpu->y); SET_NZ(cpu->a); return 4+cross;
    case 0x41: { uint8_t p=(RD(cpu->pc++)+cpu->x)&0xFF; ea=rd16(cpu,p); cpu->a^=RD(ea); SET_NZ(cpu->a); return 6; }
    case 0x51: { uint8_t p=RD(cpu->pc++); ea=rd16(cpu,p); cross=((ea^(ea+cpu->y))>>8)&1; cpu->a^=RD(ea+cpu->y); SET_NZ(cpu->a); return 5+cross; }
    /* BIT */
    case 0x24: tmp8=RD(RD(cpu->pc++)); cpu->p=(cpu->p&~(FN|FV|FZ))|(tmp8&(FN|FV))|(cpu->a&tmp8?0:FZ); return 3;
    case 0x2C: ea=rd16(cpu,cpu->pc); cpu->pc+=2; tmp8=RD(ea); cpu->p=(cpu->p&~(FN|FV|FZ))|(tmp8&(FN|FV))|(cpu->a&tmp8?0:FZ); return 4;
    /* ADC */
    case 0x69: do_adc(cpu,RD(cpu->pc++)); return 2;
    case 0x65: do_adc(cpu,RD(RD(cpu->pc++))); return 3;
    case 0x75: do_adc(cpu,RD((RD(cpu->pc++)+cpu->x)&0xFF)); return 4;
    case 0x6D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; do_adc(cpu,RD(ea)); return 4;
    case 0x7D: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->x))>>8)&1; do_adc(cpu,RD(ea+cpu->x)); return 4+cross;
    case 0x79: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->y))>>8)&1; do_adc(cpu,RD(ea+cpu->y)); return 4+cross;
    case 0x61: { uint8_t p=(RD(cpu->pc++)+cpu->x)&0xFF; ea=rd16(cpu,p); do_adc(cpu,RD(ea)); return 6; }
    case 0x71: { uint8_t p=RD(cpu->pc++); ea=rd16(cpu,p); cross=((ea^(ea+cpu->y))>>8)&1; do_adc(cpu,RD(ea+cpu->y)); return 5+cross; }
    /* SBC */
    case 0xE9: do_sbc(cpu,RD(cpu->pc++)); return 2;
    case 0xE5: do_sbc(cpu,RD(RD(cpu->pc++))); return 3;
    case 0xF5: do_sbc(cpu,RD((RD(cpu->pc++)+cpu->x)&0xFF)); return 4;
    case 0xED: ea=rd16(cpu,cpu->pc); cpu->pc+=2; do_sbc(cpu,RD(ea)); return 4;
    case 0xFD: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->x))>>8)&1; do_sbc(cpu,RD(ea+cpu->x)); return 4+cross;
    case 0xF9: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->y))>>8)&1; do_sbc(cpu,RD(ea+cpu->y)); return 4+cross;
    case 0xE1: { uint8_t p=(RD(cpu->pc++)+cpu->x)&0xFF; ea=rd16(cpu,p); do_sbc(cpu,RD(ea)); return 6; }
    case 0xF1: { uint8_t p=RD(cpu->pc++); ea=rd16(cpu,p); cross=((ea^(ea+cpu->y))>>8)&1; do_sbc(cpu,RD(ea+cpu->y)); return 5+cross; }
    /* CMP */
    case 0xC9: do_cmp(cpu,cpu->a,RD(cpu->pc++)); return 2;
    case 0xC5: do_cmp(cpu,cpu->a,RD(RD(cpu->pc++))); return 3;
    case 0xD5: do_cmp(cpu,cpu->a,RD((RD(cpu->pc++)+cpu->x)&0xFF)); return 4;
    case 0xCD: ea=rd16(cpu,cpu->pc); cpu->pc+=2; do_cmp(cpu,cpu->a,RD(ea)); return 4;
    case 0xDD: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->x))>>8)&1; do_cmp(cpu,cpu->a,RD(ea+cpu->x)); return 4+cross;
    case 0xD9: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->y))>>8)&1; do_cmp(cpu,cpu->a,RD(ea+cpu->y)); return 4+cross;
    case 0xC1: { uint8_t p=(RD(cpu->pc++)+cpu->x)&0xFF; ea=rd16(cpu,p); do_cmp(cpu,cpu->a,RD(ea)); return 6; }
    case 0xD1: { uint8_t p=RD(cpu->pc++); ea=rd16(cpu,p); cross=((ea^(ea+cpu->y))>>8)&1; do_cmp(cpu,cpu->a,RD(ea+cpu->y)); return 5+cross; }
    /* CPX */
    case 0xE0: do_cmp(cpu,cpu->x,RD(cpu->pc++)); return 2;
    case 0xE4: do_cmp(cpu,cpu->x,RD(RD(cpu->pc++))); return 3;
    case 0xEC: ea=rd16(cpu,cpu->pc); cpu->pc+=2; do_cmp(cpu,cpu->x,RD(ea)); return 4;
    /* CPY */
    case 0xC0: do_cmp(cpu,cpu->y,RD(cpu->pc++)); return 2;
    case 0xC4: do_cmp(cpu,cpu->y,RD(RD(cpu->pc++))); return 3;
    case 0xCC: ea=rd16(cpu,cpu->pc); cpu->pc+=2; do_cmp(cpu,cpu->y,RD(ea)); return 4;
    /* INC */
    case 0xE6: inc_mem(cpu,RD(cpu->pc++)); return 5;
    case 0xF6: inc_mem(cpu,(RD(cpu->pc++)+cpu->x)&0xFF); return 6;
    case 0xEE: ea=rd16(cpu,cpu->pc); cpu->pc+=2; inc_mem(cpu,ea); return 6;
    case 0xFE: ea=rd16(cpu,cpu->pc); cpu->pc+=2; inc_mem(cpu,ea+cpu->x); return 7;
    /* DEC */
    case 0xC6: dec_mem(cpu,RD(cpu->pc++)); return 5;
    case 0xD6: dec_mem(cpu,(RD(cpu->pc++)+cpu->x)&0xFF); return 6;
    case 0xCE: ea=rd16(cpu,cpu->pc); cpu->pc+=2; dec_mem(cpu,ea); return 6;
    case 0xDE: ea=rd16(cpu,cpu->pc); cpu->pc+=2; dec_mem(cpu,ea+cpu->x); return 7;
    /* INX INY DEX DEY */
    case 0xE8: cpu->x++; SET_NZ(cpu->x); return 2;
    case 0xC8: cpu->y++; SET_NZ(cpu->y); return 2;
    case 0xCA: cpu->x--; SET_NZ(cpu->x); return 2;
    case 0x88: cpu->y--; SET_NZ(cpu->y); return 2;
    /* ASL */
    case 0x0A: cpu->p=(cpu->p&~FC)|(cpu->a>>7); cpu->a<<=1; SET_NZ(cpu->a); return 2;
    case 0x06: asl_mem(cpu,RD(cpu->pc++)); return 5;
    case 0x16: asl_mem(cpu,(RD(cpu->pc++)+cpu->x)&0xFF); return 6;
    case 0x0E: ea=rd16(cpu,cpu->pc); cpu->pc+=2; asl_mem(cpu,ea); return 6;
    case 0x1E: ea=rd16(cpu,cpu->pc); cpu->pc+=2; asl_mem(cpu,ea+cpu->x); return 7;
    /* LSR */
    case 0x4A: cpu->p=(cpu->p&~FC)|(cpu->a&1); cpu->a>>=1; SET_NZ(cpu->a); return 2;
    case 0x46: lsr_mem(cpu,RD(cpu->pc++)); return 5;
    case 0x56: lsr_mem(cpu,(RD(cpu->pc++)+cpu->x)&0xFF); return 6;
    case 0x4E: ea=rd16(cpu,cpu->pc); cpu->pc+=2; lsr_mem(cpu,ea); return 6;
    case 0x5E: ea=rd16(cpu,cpu->pc); cpu->pc+=2; lsr_mem(cpu,ea+cpu->x); return 7;
    /* ROL */
    case 0x2A: { uint8_t o=cpu->a; cpu->a=(o<<1)|(cpu->p&FC); cpu->p=(cpu->p&~FC)|(o>>7); SET_NZ(cpu->a); return 2; }
    case 0x26: rol_mem(cpu,RD(cpu->pc++)); return 5;
    case 0x36: rol_mem(cpu,(RD(cpu->pc++)+cpu->x)&0xFF); return 6;
    case 0x2E: ea=rd16(cpu,cpu->pc); cpu->pc+=2; rol_mem(cpu,ea); return 6;
    case 0x3E: ea=rd16(cpu,cpu->pc); cpu->pc+=2; rol_mem(cpu,ea+cpu->x); return 7;
    /* ROR */
    case 0x6A: { uint8_t o=cpu->a; cpu->a=(o>>1)|((cpu->p&FC)<<7); cpu->p=(cpu->p&~FC)|(o&1); SET_NZ(cpu->a); return 2; }
    case 0x66: ror_mem(cpu,RD(cpu->pc++)); return 5;
    case 0x76: ror_mem(cpu,(RD(cpu->pc++)+cpu->x)&0xFF); return 6;
    case 0x6E: ea=rd16(cpu,cpu->pc); cpu->pc+=2; ror_mem(cpu,ea); return 6;
    case 0x7E: ea=rd16(cpu,cpu->pc); cpu->pc+=2; ror_mem(cpu,ea+cpu->x); return 7;
    /* JMP JSR RTS RTI */
    case 0x4C: cpu->pc=rd16(cpu,cpu->pc); return 3;
    case 0x6C: ea=rd16(cpu,cpu->pc); cpu->pc=rd16_bug(cpu,ea); return 5;
    case 0x20: { uint16_t t=rd16(cpu,cpu->pc); cpu->pc+=2; PUSH((cpu->pc-1)>>8); PUSH((cpu->pc-1)&0xFF); cpu->pc=t; return 6; }
    case 0x60: { uint16_t lo=POP(); uint16_t hi=POP(); cpu->pc=(lo|(hi<<8))+1; return 6; }
    case 0x40: { cpu->p=(POP()|FU)&~FB; uint16_t lo=POP(); uint16_t hi=POP(); cpu->pc=lo|(hi<<8); return 6; }
    /* BRK */
    case 0x00: { cpu->pc++; PUSH(cpu->pc>>8); PUSH(cpu->pc&0xFF); PUSH(cpu->p|FB|FU); cpu->p=(cpu->p|FI)&~FD; cpu->pc=rd16(cpu,0xFFFE); return 7; }
    /* Branches */
    case 0x90: return branch(cpu, !(cpu->p & FC));
    case 0xB0: return branch(cpu,  (cpu->p & FC));
    case 0xF0: return branch(cpu,  (cpu->p & FZ));
    case 0xD0: return branch(cpu, !(cpu->p & FZ));
    case 0x30: return branch(cpu,  (cpu->p & FN));
    case 0x10: return branch(cpu, !(cpu->p & FN));
    case 0x50: return branch(cpu, !(cpu->p & FV));
    case 0x70: return branch(cpu,  (cpu->p & FV));
    /* Flags */
    case 0x18: cpu->p &= ~FC; return 2;
    case 0x38: cpu->p |=  FC; return 2;
    case 0x58: cpu->p &= ~FI; return 2;
    case 0x78: cpu->p |=  FI; return 2;
    case 0xB8: cpu->p &= ~FV; return 2;
    case 0xD8: cpu->p &= ~FD; return 2;
    case 0xF8: cpu->p |=  FD; return 2;
    /* NOP */
    case 0xEA: return 2;
    /* Unofficial single-byte NOPs */
    case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: return 2;
    /* Unofficial 2-byte NOPs */
    case 0x04: case 0x44: case 0x64:
    case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4:
    case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2:
        cpu->pc++; return 3;
    /* Unofficial 3-byte NOPs */
    case 0x0C:
    case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC:
        cpu->pc += 2; return 4;
    /* LAX (unofficial) */
    case 0xA7: cpu->a=cpu->x=RD(RD(cpu->pc++)); SET_NZ(cpu->a); return 3;
    case 0xB7: cpu->a=cpu->x=RD((RD(cpu->pc++)+cpu->y)&0xFF); SET_NZ(cpu->a); return 4;
    case 0xAF: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cpu->a=cpu->x=RD(ea); SET_NZ(cpu->a); return 4;
    case 0xBF: ea=rd16(cpu,cpu->pc); cpu->pc+=2; cross=((ea^(ea+cpu->y))>>8)&1; cpu->a=cpu->x=RD(ea+cpu->y); SET_NZ(cpu->a); return 4+cross;
    case 0xA3: { uint8_t p=(RD(cpu->pc++)+cpu->x)&0xFF; ea=rd16(cpu,p); cpu->a=cpu->x=RD(ea); SET_NZ(cpu->a); return 6; }
    case 0xB3: { uint8_t p=RD(cpu->pc++); ea=rd16(cpu,p); cross=((ea^(ea+cpu->y))>>8)&1; cpu->a=cpu->x=RD(ea+cpu->y); SET_NZ(cpu->a); return 5+cross; }
    /* SAX (unofficial) */
    case 0x87: WR(RD(cpu->pc++), cpu->a & cpu->x); return 3;
    case 0x97: WR((RD(cpu->pc++)+cpu->y)&0xFF, cpu->a & cpu->x); return 4;
    case 0x8F: ea=rd16(cpu,cpu->pc); cpu->pc+=2; WR(ea, cpu->a & cpu->x); return 4;
    /* All other opcodes treated as NOP */
    default: return 2;
    }
}

M6502_IRAM void m6502_exec(m6502_t *cpu, int target) {
    int done = 0;
    while (done < target)
        done += m6502_step(cpu);
}
