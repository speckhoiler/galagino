/*
 * M6809 CPU Emulator for Galagino
 * Lightweight implementation targeting Gyruss sub-CPU emulation.
 */

#include "m6809.h"

/* ============================================================
 * Helper macros
 * ============================================================ */

#define REG_D(s)  ((uint16_t)((s)->A << 8) | (s)->B)
#define SET_D(s,v) do { (s)->A = (uint8_t)((v) >> 8); (s)->B = (uint8_t)(v); } while(0)

#define FETCH8(s)   m6809_read(s, (s)->PC++)
#define FETCH_OP(s) m6809_read_opcode(s, (s)->PC++)

static inline uint16_t FETCH16(m6809_state *s) {
    uint8_t hi = m6809_read(s, s->PC++);
    uint8_t lo = m6809_read(s, s->PC++);
    return ((uint16_t)hi << 8) | lo;
}

#define READ8(s,a)    m6809_read(s, a)

static inline uint16_t READ16(m6809_state *s, uint16_t a) {
    uint8_t hi = m6809_read(s, a);
    uint8_t lo = m6809_read(s, (uint16_t)(a + 1));
    return ((uint16_t)hi << 8) | lo;
}

#define WRITE8(s,a,v) m6809_write(s, a, v)
#define WRITE16(s,a,v) do { m6809_write(s,a,(uint8_t)((v)>>8)); m6809_write(s,(uint16_t)((a)+1),(uint8_t)(v)); } while(0)

#define PUSH8(s,v)  do { (s)->S--; WRITE8(s, (s)->S, v); } while(0)
#define PUSH16(s,v) do { PUSH8(s, (uint8_t)(v)); PUSH8(s, (uint8_t)((v)>>8)); } while(0)
#define PULL8(s)    m6809_read(s, (s)->S++)

static inline uint16_t PULL16(m6809_state *s) {
    uint8_t hi = m6809_read(s, s->S++);
    uint8_t lo = m6809_read(s, s->S++);
    return ((uint16_t)hi << 8) | lo;
}

#define PUSHU8(s,v)  do { (s)->U--; WRITE8(s, (s)->U, v); } while(0)
#define PUSHU16(s,v) do { PUSHU8(s, (uint8_t)(v)); PUSHU8(s, (uint8_t)((v)>>8)); } while(0)
#define PULLU8(s)    m6809_read(s, (s)->U++)

static inline uint16_t PULLU16(m6809_state *s) {
    uint8_t hi = m6809_read(s, s->U++);
    uint8_t lo = m6809_read(s, s->U++);
    return ((uint16_t)hi << 8) | lo;
}

/* CC flag helpers */
#define SET_Z8(s,v)   do { if(!(uint8_t)(v)) (s)->CC |= M6809_CC_Z; else (s)->CC &= ~M6809_CC_Z; } while(0)
#define SET_Z16(s,v)  do { if(!(uint16_t)(v)) (s)->CC |= M6809_CC_Z; else (s)->CC &= ~M6809_CC_Z; } while(0)
#define SET_N8(s,v)   do { if((v)&0x80) (s)->CC |= M6809_CC_N; else (s)->CC &= ~M6809_CC_N; } while(0)
#define SET_N16(s,v)  do { if((v)&0x8000) (s)->CC |= M6809_CC_N; else (s)->CC &= ~M6809_CC_N; } while(0)

static void set_nz8(m6809_state *s, uint8_t v) {
    s->CC &= ~(M6809_CC_N | M6809_CC_Z);
    if (!v) s->CC |= M6809_CC_Z;
    if (v & 0x80) s->CC |= M6809_CC_N;
}

static void set_nz16(m6809_state *s, uint16_t v) {
    s->CC &= ~(M6809_CC_N | M6809_CC_Z);
    if (!v) s->CC |= M6809_CC_Z;
    if (v & 0x8000) s->CC |= M6809_CC_N;
}

/* ============================================================
 * Indexed addressing mode
 * ============================================================ */

static uint16_t *get_index_reg(m6809_state *s, uint8_t postbyte) {
    switch ((postbyte >> 5) & 3) {
        case 0: return &s->X;
        case 1: return &s->Y;
        case 2: return &s->U;
        case 3: return &s->S;
    }
    return &s->X;
}

static uint16_t indexed_addr(m6809_state *s) {
    uint8_t postbyte = FETCH8(s);
    uint16_t *reg = get_index_reg(s, postbyte);
    uint16_t ea;
    int16_t offset;

    if (!(postbyte & 0x80)) {
        /* 5-bit signed offset */
        offset = (int16_t)(int8_t)((postbyte & 0x1F) | ((postbyte & 0x10) ? 0xE0 : 0));
        s->cycles += 1;
        return *reg + offset;
    }

    /* Bit 4 = indirect flag, bits 0-3 = addressing mode */
    uint8_t mode = postbyte & 0x0F;
    uint8_t indirect = postbyte & 0x10;

    /* Special case: mode 0x0F + indirect = [n16] extended indirect */
    if (mode == 0x0F && indirect) {
        ea = FETCH16(s); s->cycles += 5;
        ea = READ16(s, ea);
        return ea;
    }

    switch (mode) {
        case 0x00: /* ,R+ */
            ea = *reg; (*reg)++; s->cycles += 2; break;
        case 0x01: /* ,R++ */
            ea = *reg; *reg += 2; s->cycles += 3; break;
        case 0x02: /* ,-R */
            (*reg)--; ea = *reg; s->cycles += 2; break;
        case 0x03: /* ,--R */
            *reg -= 2; ea = *reg; s->cycles += 3; break;
        case 0x04: /* ,R (no offset) */
            ea = *reg; s->cycles += 0; break;
        case 0x05: /* B,R */
            ea = *reg + (int16_t)(int8_t)s->B; s->cycles += 1; break;
        case 0x06: /* A,R */
            ea = *reg + (int16_t)(int8_t)s->A; s->cycles += 1; break;
        case 0x08: /* n8,R */
            offset = (int16_t)(int8_t)FETCH8(s);
            ea = *reg + offset; s->cycles += 1; break;
        case 0x09: /* n16,R */
            offset = (int16_t)FETCH16(s);
            ea = *reg + offset; s->cycles += 4; break;
        case 0x0B: /* D,R */
            ea = *reg + (int16_t)REG_D(s); s->cycles += 4; break;
        case 0x0C: /* n8,PC */
            offset = (int16_t)(int8_t)FETCH8(s);
            ea = s->PC + offset; s->cycles += 1; break;
        case 0x0D: /* n16,PC */
            offset = (int16_t)FETCH16(s);
            ea = s->PC + offset; s->cycles += 5; break;
        default:
            ea = 0; break;
    }

    /* Indirect: dereference the computed effective address */
    if (indirect) {
        ea = READ16(s, ea);
        s->cycles += 3;
    }

    return ea;
}

/* ============================================================
 * Addressing mode helpers
 * ============================================================ */

static uint16_t direct_addr(m6809_state *s) {
    return ((uint16_t)s->DP << 8) | FETCH8(s);
}

static uint16_t extended_addr(m6809_state *s) {
    return FETCH16(s);
}

/* ============================================================
 * ALU operations - 8 bit
 * ============================================================ */

static uint8_t op_add8(m6809_state *s, uint8_t a, uint8_t b) {
    uint16_t r = a + b;
    s->CC &= ~(M6809_CC_H | M6809_CC_N | M6809_CC_Z | M6809_CC_V | M6809_CC_C);
    if (((a ^ b ^ r) & 0x10)) s->CC |= M6809_CC_H;
    if (r & 0x80) s->CC |= M6809_CC_N;
    if (!(r & 0xFF)) s->CC |= M6809_CC_Z;
    if (((a ^ b ^ 0xFF) & (a ^ r)) & 0x80) s->CC |= M6809_CC_V;
    if (r & 0x100) s->CC |= M6809_CC_C;
    return (uint8_t)r;
}

static uint8_t op_adc8(m6809_state *s, uint8_t a, uint8_t b) {
    uint16_t r = a + b + (s->CC & M6809_CC_C ? 1 : 0);
    s->CC &= ~(M6809_CC_H | M6809_CC_N | M6809_CC_Z | M6809_CC_V | M6809_CC_C);
    if (((a ^ b ^ r) & 0x10)) s->CC |= M6809_CC_H;
    if (r & 0x80) s->CC |= M6809_CC_N;
    if (!(r & 0xFF)) s->CC |= M6809_CC_Z;
    if (((a ^ b ^ 0xFF) & (a ^ r)) & 0x80) s->CC |= M6809_CC_V;
    if (r & 0x100) s->CC |= M6809_CC_C;
    return (uint8_t)r;
}

static uint8_t op_sub8(m6809_state *s, uint8_t a, uint8_t b) {
    uint16_t r = a - b;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V | M6809_CC_C);
    if (r & 0x80) s->CC |= M6809_CC_N;
    if (!(r & 0xFF)) s->CC |= M6809_CC_Z;
    if (((a ^ b) & (a ^ r)) & 0x80) s->CC |= M6809_CC_V;
    if (r & 0x100) s->CC |= M6809_CC_C;
    return (uint8_t)r;
}

static uint8_t op_sbc8(m6809_state *s, uint8_t a, uint8_t b) {
    uint16_t r = a - b - (s->CC & M6809_CC_C ? 1 : 0);
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V | M6809_CC_C);
    if (r & 0x80) s->CC |= M6809_CC_N;
    if (!(r & 0xFF)) s->CC |= M6809_CC_Z;
    if (((a ^ b) & (a ^ r)) & 0x80) s->CC |= M6809_CC_V;
    if (r & 0x100) s->CC |= M6809_CC_C;
    return (uint8_t)r;
}

static void op_cmp8(m6809_state *s, uint8_t a, uint8_t b) {
    op_sub8(s, a, b); /* discard result */
}

static uint8_t op_and8(m6809_state *s, uint8_t a, uint8_t b) {
    uint8_t r = a & b;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V);
    set_nz8(s, r);
    return r;
}

static uint8_t op_or8(m6809_state *s, uint8_t a, uint8_t b) {
    uint8_t r = a | b;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V);
    set_nz8(s, r);
    return r;
}

static uint8_t op_eor8(m6809_state *s, uint8_t a, uint8_t b) {
    uint8_t r = a ^ b;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V);
    set_nz8(s, r);
    return r;
}

static void op_bit8(m6809_state *s, uint8_t a, uint8_t b) {
    op_and8(s, a, b); /* discard result */
}

static void op_tst8(m6809_state *s, uint8_t v) {
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V);
    set_nz8(s, v);
}

/* ALU operations - 16 bit */

static uint16_t op_add16(m6809_state *s, uint16_t a, uint16_t b) {
    uint32_t r = (uint32_t)a + b;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V | M6809_CC_C);
    if (r & 0x8000) s->CC |= M6809_CC_N;
    if (!(r & 0xFFFF)) s->CC |= M6809_CC_Z;
    if (((a ^ b ^ 0xFFFF) & (a ^ r)) & 0x8000) s->CC |= M6809_CC_V;
    if (r & 0x10000) s->CC |= M6809_CC_C;
    return (uint16_t)r;
}

static uint16_t op_sub16(m6809_state *s, uint16_t a, uint16_t b) {
    uint32_t r = (uint32_t)a - b;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V | M6809_CC_C);
    if (r & 0x8000) s->CC |= M6809_CC_N;
    if (!(r & 0xFFFF)) s->CC |= M6809_CC_Z;
    if (((a ^ b) & (a ^ r)) & 0x8000) s->CC |= M6809_CC_V;
    if (r & 0x10000) s->CC |= M6809_CC_C;
    return (uint16_t)r;
}

static void op_cmp16(m6809_state *s, uint16_t a, uint16_t b) {
    op_sub16(s, a, b);
}

/* Shift/Rotate operations */

static uint8_t op_asl8(m6809_state *s, uint8_t v) {
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V | M6809_CC_C);
    if (v & 0x80) s->CC |= M6809_CC_C;
    uint8_t r = v << 1;
    set_nz8(s, r);
    if (((v ^ r) & 0x80)) s->CC |= M6809_CC_V;
    return r;
}

static uint8_t op_asr8(m6809_state *s, uint8_t v) {
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_C);
    if (v & 0x01) s->CC |= M6809_CC_C;
    uint8_t r = (v >> 1) | (v & 0x80);
    set_nz8(s, r);
    return r;
}

static uint8_t op_lsr8(m6809_state *s, uint8_t v) {
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_C);
    if (v & 0x01) s->CC |= M6809_CC_C;
    uint8_t r = v >> 1;
    set_nz8(s, r);
    return r;
}

static uint8_t op_rol8(m6809_state *s, uint8_t v) {
    uint8_t c = s->CC & M6809_CC_C ? 1 : 0;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V | M6809_CC_C);
    if (v & 0x80) s->CC |= M6809_CC_C;
    uint8_t r = (v << 1) | c;
    set_nz8(s, r);
    if (((v ^ r) & 0x80)) s->CC |= M6809_CC_V;
    return r;
}

static uint8_t op_ror8(m6809_state *s, uint8_t v) {
    uint8_t c = s->CC & M6809_CC_C ? 0x80 : 0;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_C);
    if (v & 0x01) s->CC |= M6809_CC_C;
    uint8_t r = (v >> 1) | c;
    set_nz8(s, r);
    return r;
}

static uint8_t op_neg8(m6809_state *s, uint8_t v) {
    uint8_t r = (uint8_t)(0 - v);
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V | M6809_CC_C);
    set_nz8(s, r);
    if (v == 0x80) s->CC |= M6809_CC_V;
    if (r) s->CC |= M6809_CC_C;
    return r;
}

static uint8_t op_com8(m6809_state *s, uint8_t v) {
    uint8_t r = ~v;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V);
    s->CC |= M6809_CC_C;
    set_nz8(s, r);
    return r;
}

static uint8_t op_inc8(m6809_state *s, uint8_t v) {
    uint8_t r = v + 1;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V);
    set_nz8(s, r);
    if (v == 0x7F) s->CC |= M6809_CC_V;
    return r;
}

static uint8_t op_dec8(m6809_state *s, uint8_t v) {
    uint8_t r = v - 1;
    s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V);
    set_nz8(s, r);
    if (v == 0x80) s->CC |= M6809_CC_V;
    return r;
}

static uint8_t op_clr8(m6809_state *s) {
    s->CC &= ~(M6809_CC_N | M6809_CC_V | M6809_CC_C);
    s->CC |= M6809_CC_Z;
    return 0;
}

/* Branch condition check */
static int branch_cond(m6809_state *s, uint8_t cond) {
    switch (cond & 0x0F) {
        case 0x0: return 1;                                         /* BRA */
        case 0x1: return 0;                                         /* BRN */
        case 0x2: return !(s->CC & (M6809_CC_C | M6809_CC_Z));     /* BHI */
        case 0x3: return  (s->CC & (M6809_CC_C | M6809_CC_Z)) != 0;/* BLS */
        case 0x4: return !(s->CC & M6809_CC_C);                    /* BCC/BHS */
        case 0x5: return  (s->CC & M6809_CC_C) != 0;               /* BCS/BLO */
        case 0x6: return !(s->CC & M6809_CC_Z);                    /* BNE */
        case 0x7: return  (s->CC & M6809_CC_Z) != 0;               /* BEQ */
        case 0x8: return !(s->CC & M6809_CC_V);                    /* BVC */
        case 0x9: return  (s->CC & M6809_CC_V) != 0;               /* BVS */
        case 0xA: return !(s->CC & M6809_CC_N);                    /* BPL */
        case 0xB: return  (s->CC & M6809_CC_N) != 0;               /* BMI */
        case 0xC: return !((s->CC & M6809_CC_N) != 0) == !((s->CC & M6809_CC_V) != 0); /* BGE */
        case 0xD: return  ((s->CC & M6809_CC_N) != 0) != ((s->CC & M6809_CC_V) != 0);  /* BLT */
        case 0xE: return !(s->CC & M6809_CC_Z) &&                  /* BGT */
                         (!!(s->CC & M6809_CC_N) == !!(s->CC & M6809_CC_V));
        case 0xF: return  (s->CC & M6809_CC_Z) ||                  /* BLE */
                         (!!(s->CC & M6809_CC_N) != !!(s->CC & M6809_CC_V));
    }
    return 0;
}

/* Register access by post-byte encoding (TFR/EXG) */
static uint16_t get_reg(m6809_state *s, uint8_t code) {
    switch (code & 0x0F) {
        case 0x0: return REG_D(s);
        case 0x1: return s->X;
        case 0x2: return s->Y;
        case 0x3: return s->U;
        case 0x4: return s->S;
        case 0x5: return s->PC;
        case 0x8: return s->A;
        case 0x9: return s->B;
        case 0xA: return s->CC;
        case 0xB: return s->DP;
        default:  return 0;
    }
}

static void set_reg(m6809_state *s, uint8_t code, uint16_t val) {
    switch (code & 0x0F) {
        case 0x0: SET_D(s, val); break;
        case 0x1: s->X = val; break;
        case 0x2: s->Y = val; break;
        case 0x3: s->U = val; break;
        case 0x4: s->S = val; s->nmi_armed = 1; break;
        case 0x5: s->PC = val; break;
        case 0x8: s->A = (uint8_t)val; break;
        case 0x9: s->B = (uint8_t)val; break;
        case 0xA: s->CC = (uint8_t)val; break;
        case 0xB: s->DP = (uint8_t)val; break;
    }
}

/* ============================================================
 * Push/Pull entire state for interrupts
 * ============================================================ */

static void push_entire_state(m6809_state *s) {
    s->CC |= M6809_CC_E;
    PUSH16(s, s->PC);
    PUSH16(s, s->U);
    PUSH16(s, s->Y);
    PUSH16(s, s->X);
    PUSH8(s, s->DP);
    PUSH8(s, s->B);
    PUSH8(s, s->A);
    PUSH8(s, s->CC);
}

/* ============================================================
 * Interrupt handling
 * ============================================================ */

static void check_interrupts(m6809_state *s) {
    if (s->nmi_pending && s->nmi_armed) {
        s->nmi_pending = 0;
        /* CWAI (halted==2) already pushed state; skip re-push */
        if (s->halted != 2) push_entire_state(s);
        s->halted = 0;
        s->CC |= M6809_CC_I | M6809_CC_F;
        s->PC = READ16(s, 0xFFFC);
        s->cycles += 19;
        return;
    }

    if (s->firq_pending && !(s->CC & M6809_CC_F)) {
        s->firq_pending = 0;
        if (s->halted != 2) {
            /* Normal FIRQ: push CC and PC only */
            s->CC &= ~M6809_CC_E;
            PUSH16(s, s->PC);
            PUSH8(s, s->CC);
        }
        /* CWAI already pushed entire state (E=1), just vector */
        s->halted = 0;
        s->CC |= M6809_CC_I | M6809_CC_F;
        s->PC = READ16(s, 0xFFF6);
        s->cycles += 10;
        return;
    }

    if (s->irq_pending && !(s->CC & M6809_CC_I)) {
        s->irq_pending = 0;
        /* CWAI (halted==2) already pushed state; skip re-push */
        if (s->halted != 2) push_entire_state(s);
        s->halted = 0;
        s->CC |= M6809_CC_I;
        s->PC = READ16(s, 0xFFF8);
        s->cycles += 19;
        return;
    }

    /* SYNC (halted==1): if interrupt arrived but is masked, just resume */
    if (s->halted == 1 && (s->irq_pending || s->firq_pending || s->nmi_pending)) {
        s->halted = 0;
    }
}

/* ============================================================
 * Reset
 * ============================================================ */

void m6809_reset(m6809_state *s) {
    s->A = s->B = 0;
    s->X = s->Y = s->U = s->S = 0;
    s->DP = 0;
    s->CC = M6809_CC_I | M6809_CC_F;
    s->irq_pending = s->firq_pending = s->nmi_pending = 0;
    s->nmi_armed = 0;
    s->halted = 0;
    s->cycles = 0;
    s->total_cycles = 0;
    s->PC = READ16(s, 0xFFFE);
}

void m6809_irq(m6809_state *s) {
    s->irq_pending = 1;
    check_interrupts(s);

    if (s->halted) {
      s->cycles = 1;
      s->total_cycles += 1;
      return; 
    }
}

void m6809_firq(m6809_state *s) {
    s->firq_pending = 1;
}

void m6809_nmi(m6809_state *s) {
    s->nmi_pending = 1;
}

/* ============================================================
 * Main execute step
 * ============================================================ */

int m6809_step(m6809_state *s, char count) {
  uint8_t op, val8, postbyte;
  uint16_t ea, val16; 
  int16_t offset;
  s->cycles = 0;

  //check_interrupts(s);
  //if (s->halted) {
  //    s->cycles = 1;
  //    s->total_cycles += 1;
  //    return 1; 
  //}

  for (int i=0; i < count; i++) {
    op = FETCH_OP(s);

    switch (op) {
        /* ======== Page 0 inherent ======== */
        case 0x12: /* NOP */
            s->cycles = 2; break;

        case 0x13: /* SYNC - halt until any interrupt (halted=1) */
            s->halted = 1;
            s->cycles = 4; break;

        case 0x19: /* DAA */
        {
            uint16_t t = s->A;
            uint8_t cf = 0;
            uint8_t msn = s->A & 0xF0;
            uint8_t lsn = s->A & 0x0F;
            if (lsn > 9 || (s->CC & M6809_CC_H)) { cf |= 0x06; }
            if (msn > 0x80 && lsn > 9) { cf |= 0x60; }
            if (msn > 0x90 || (s->CC & M6809_CC_C)) { cf |= 0x60; }
            t += cf;
            s->CC &= ~(M6809_CC_N | M6809_CC_Z | M6809_CC_V);
            if (t & 0x100) s->CC |= M6809_CC_C;
            s->A = (uint8_t)t;
            set_nz8(s, s->A);
            s->cycles = 2;
            break;
        }

        case 0x1A: /* ORCC #imm */
            s->CC |= FETCH8(s);
            s->cycles = 3; break;

        case 0x1C: /* ANDCC #imm */
            s->CC &= FETCH8(s);
            s->cycles = 3; break;

        case 0x1D: /* SEX */
            s->A = (s->B & 0x80) ? 0xFF : 0x00;
            s->CC &= ~(M6809_CC_N | M6809_CC_Z);
            set_nz16(s, REG_D(s));
            s->cycles = 2; break;

        case 0x1E: /* EXG r,r */
            postbyte = FETCH8(s);
            val16 = get_reg(s, postbyte >> 4);
            ea = get_reg(s, postbyte & 0x0F);
            set_reg(s, postbyte >> 4, ea);
            set_reg(s, postbyte & 0x0F, val16);
            s->cycles = 8; break;

        case 0x1F: /* TFR r,r */
            postbyte = FETCH8(s);
            val16 = get_reg(s, postbyte >> 4);
            set_reg(s, postbyte & 0x0F, val16);
            s->cycles = 6; break;

        /* ======== Branches (short) ======== */
        case 0x20: case 0x21: case 0x22: case 0x23:
        case 0x24: case 0x25: case 0x26: case 0x27:
        case 0x28: case 0x29: case 0x2A: case 0x2B:
        case 0x2C: case 0x2D: case 0x2E: case 0x2F:
            offset = (int16_t)(int8_t)FETCH8(s);
            if (branch_cond(s, op))
                s->PC += offset;
            s->cycles = 3;
            break;

        /* ======== LEA ======== */
        case 0x30: /* LEAX */
            s->X = indexed_addr(s);
            s->CC &= ~M6809_CC_Z;
            if (!s->X) s->CC |= M6809_CC_Z;
            s->cycles += 4; break;

        case 0x31: /* LEAY */
            s->Y = indexed_addr(s);
            s->CC &= ~M6809_CC_Z;
            if (!s->Y) s->CC |= M6809_CC_Z;
            s->cycles += 4; break;

        case 0x32: /* LEAS */
            s->S = indexed_addr(s);
            s->nmi_armed = 1;
            s->cycles += 4; break;

        case 0x33: /* LEAU */
            s->U = indexed_addr(s);
            s->cycles += 4; break;

        /* ======== PSHS/PULS/PSHU/PULU ======== */
        case 0x34: /* PSHS */
            postbyte = FETCH8(s);
            s->cycles = 5;
            if (postbyte & 0x80) { PUSH16(s, s->PC); s->cycles += 2; }
            if (postbyte & 0x40) { PUSH16(s, s->U); s->cycles += 2; }
            if (postbyte & 0x20) { PUSH16(s, s->Y); s->cycles += 2; }
            if (postbyte & 0x10) { PUSH16(s, s->X); s->cycles += 2; }
            if (postbyte & 0x08) { PUSH8(s, s->DP); s->cycles += 1; }
            if (postbyte & 0x04) { PUSH8(s, s->B); s->cycles += 1; }
            if (postbyte & 0x02) { PUSH8(s, s->A); s->cycles += 1; }
            if (postbyte & 0x01) { PUSH8(s, s->CC); s->cycles += 1; }
            break;

        case 0x35: /* PULS */
            postbyte = FETCH8(s);
            s->cycles = 5;
            if (postbyte & 0x01) { s->CC = PULL8(s); s->cycles += 1; }
            if (postbyte & 0x02) { s->A = PULL8(s); s->cycles += 1; }
            if (postbyte & 0x04) { s->B = PULL8(s); s->cycles += 1; }
            if (postbyte & 0x08) { s->DP = PULL8(s); s->cycles += 1; }
            if (postbyte & 0x10) { s->X = PULL16(s); s->cycles += 2; }
            if (postbyte & 0x20) { s->Y = PULL16(s); s->cycles += 2; }
            if (postbyte & 0x40) { s->U = PULL16(s); s->cycles += 2; }
            if (postbyte & 0x80) { s->PC = PULL16(s); s->cycles += 2; }
            break;

        case 0x36: /* PSHU */
            postbyte = FETCH8(s);
            s->cycles = 5;
            if (postbyte & 0x80) { PUSHU16(s, s->PC); s->cycles += 2; }
            if (postbyte & 0x40) { PUSHU16(s, s->S); s->cycles += 2; }
            if (postbyte & 0x20) { PUSHU16(s, s->Y); s->cycles += 2; }
            if (postbyte & 0x10) { PUSHU16(s, s->X); s->cycles += 2; }
            if (postbyte & 0x08) { PUSHU8(s, s->DP); s->cycles += 1; }
            if (postbyte & 0x04) { PUSHU8(s, s->B); s->cycles += 1; }
            if (postbyte & 0x02) { PUSHU8(s, s->A); s->cycles += 1; }
            if (postbyte & 0x01) { PUSHU8(s, s->CC); s->cycles += 1; }
            break;

        case 0x37: /* PULU */
            postbyte = FETCH8(s);
            s->cycles = 5;
            if (postbyte & 0x01) { s->CC = PULLU8(s); s->cycles += 1; }
            if (postbyte & 0x02) { s->A = PULLU8(s); s->cycles += 1; }
            if (postbyte & 0x04) { s->B = PULLU8(s); s->cycles += 1; }
            if (postbyte & 0x08) { s->DP = PULLU8(s); s->cycles += 1; }
            if (postbyte & 0x10) { s->X = PULLU16(s); s->cycles += 2; }
            if (postbyte & 0x20) { s->Y = PULLU16(s); s->cycles += 2; }
            if (postbyte & 0x40) { s->S = PULLU16(s); s->cycles += 2; s->nmi_armed = 1; }
            if (postbyte & 0x80) { s->PC = PULLU16(s); s->cycles += 2; }
            break;

        case 0x39: /* RTS */
            s->PC = PULL16(s);
            s->cycles = 5; break;

        case 0x3A: /* ABX */
            s->X += s->B;
            s->cycles = 3; break;

        case 0x3B: /* RTI */
            s->CC = PULL8(s);
            s->cycles = 6;
            if (s->CC & M6809_CC_E) {
                s->A = PULL8(s);
                s->B = PULL8(s);
                s->DP = PULL8(s);
                s->X = PULL16(s);
                s->Y = PULL16(s);
                s->U = PULL16(s);
                s->cycles += 9;
            }
            s->PC = PULL16(s);
            break;

        case 0x3C: /* CWAI - pre-push state, then halt (halted=2) */
            s->CC &= FETCH8(s);
            push_entire_state(s);
            s->halted = 2;
            s->cycles = 20; break;

        case 0x3D: /* MUL */
            val16 = (uint16_t)s->A * (uint16_t)s->B;
            SET_D(s, val16);
            s->CC &= ~(M6809_CC_Z | M6809_CC_C);
            if (!val16) s->CC |= M6809_CC_Z;
            if (s->B & 0x80) s->CC |= M6809_CC_C;
            s->cycles = 11; break;

        case 0x3F: /* SWI */
            push_entire_state(s);
            s->CC |= M6809_CC_I | M6809_CC_F;
            s->PC = READ16(s, 0xFFFA);
            s->cycles = 19; break;

        /* ======== Direct page 8-bit operations ======== */
        case 0x00: ea = direct_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_neg8(s, val8)); s->cycles = 6; break;
        case 0x03: ea = direct_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_com8(s, val8)); s->cycles = 6; break;
        case 0x04: ea = direct_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_lsr8(s, val8)); s->cycles = 6; break;
        case 0x06: ea = direct_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_ror8(s, val8)); s->cycles = 6; break;
        case 0x07: ea = direct_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_asr8(s, val8)); s->cycles = 6; break;
        case 0x08: ea = direct_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_asl8(s, val8)); s->cycles = 6; break;
        case 0x09: ea = direct_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_rol8(s, val8)); s->cycles = 6; break;
        case 0x0A: ea = direct_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_dec8(s, val8)); s->cycles = 6; break;
        case 0x0C: ea = direct_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_inc8(s, val8)); s->cycles = 6; break;
        case 0x0D: ea = direct_addr(s); val8 = READ8(s, ea); op_tst8(s, val8); s->cycles = 6; break;
        case 0x0E: ea = direct_addr(s); s->PC = ea; s->cycles = 3; break; /* JMP direct */
        case 0x0F: ea = direct_addr(s); WRITE8(s, ea, op_clr8(s)); s->cycles = 6; break;

        /* ======== Inherent register operations ======== */
        case 0x40: s->A = op_neg8(s, s->A); s->cycles = 2; break;
        case 0x43: s->A = op_com8(s, s->A); s->cycles = 2; break;
        case 0x44: s->A = op_lsr8(s, s->A); s->cycles = 2; break;
        case 0x46: s->A = op_ror8(s, s->A); s->cycles = 2; break;
        case 0x47: s->A = op_asr8(s, s->A); s->cycles = 2; break;
        case 0x48: s->A = op_asl8(s, s->A); s->cycles = 2; break;
        case 0x49: s->A = op_rol8(s, s->A); s->cycles = 2; break;
        case 0x4A: s->A = op_dec8(s, s->A); s->cycles = 2; break;
        case 0x4C: s->A = op_inc8(s, s->A); s->cycles = 2; break;
        case 0x4D: op_tst8(s, s->A); s->cycles = 2; break;
        case 0x4F: s->A = op_clr8(s); s->cycles = 2; break;

        case 0x50: s->B = op_neg8(s, s->B); s->cycles = 2; break;
        case 0x53: s->B = op_com8(s, s->B); s->cycles = 2; break;
        case 0x54: s->B = op_lsr8(s, s->B); s->cycles = 2; break;
        case 0x56: s->B = op_ror8(s, s->B); s->cycles = 2; break;
        case 0x57: s->B = op_asr8(s, s->B); s->cycles = 2; break;
        case 0x58: s->B = op_asl8(s, s->B); s->cycles = 2; break;
        case 0x59: s->B = op_rol8(s, s->B); s->cycles = 2; break;
        case 0x5A: s->B = op_dec8(s, s->B); s->cycles = 2; break;
        case 0x5C: s->B = op_inc8(s, s->B); s->cycles = 2; break;
        case 0x5D: op_tst8(s, s->B); s->cycles = 2; break;
        case 0x5F: s->B = op_clr8(s); s->cycles = 2; break;

        /* ======== Indexed operations ======== */
        case 0x60: ea = indexed_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_neg8(s, val8)); s->cycles += 6; break;
        case 0x63: ea = indexed_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_com8(s, val8)); s->cycles += 6; break;
        case 0x64: ea = indexed_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_lsr8(s, val8)); s->cycles += 6; break;
        case 0x66: ea = indexed_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_ror8(s, val8)); s->cycles += 6; break;
        case 0x67: ea = indexed_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_asr8(s, val8)); s->cycles += 6; break;
        case 0x68: ea = indexed_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_asl8(s, val8)); s->cycles += 6; break;
        case 0x69: ea = indexed_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_rol8(s, val8)); s->cycles += 6; break;
        case 0x6A: ea = indexed_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_dec8(s, val8)); s->cycles += 6; break;
        case 0x6C: ea = indexed_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_inc8(s, val8)); s->cycles += 6; break;
        case 0x6D: ea = indexed_addr(s); val8 = READ8(s, ea); op_tst8(s, val8); s->cycles += 6; break;
        case 0x6E: ea = indexed_addr(s); s->PC = ea; s->cycles += 3; break; /* JMP indexed */
        case 0x6F: ea = indexed_addr(s); WRITE8(s, ea, op_clr8(s)); s->cycles += 6; break;

        /* ======== Extended operations ======== */
        case 0x70: ea = extended_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_neg8(s, val8)); s->cycles = 7; break;
        case 0x73: ea = extended_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_com8(s, val8)); s->cycles = 7; break;
        case 0x74: ea = extended_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_lsr8(s, val8)); s->cycles = 7; break;
        case 0x76: ea = extended_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_ror8(s, val8)); s->cycles = 7; break;
        case 0x77: ea = extended_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_asr8(s, val8)); s->cycles = 7; break;
        case 0x78: ea = extended_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_asl8(s, val8)); s->cycles = 7; break;
        case 0x79: ea = extended_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_rol8(s, val8)); s->cycles = 7; break;
        case 0x7A: ea = extended_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_dec8(s, val8)); s->cycles = 7; break;
        case 0x7C: ea = extended_addr(s); val8 = READ8(s, ea); WRITE8(s, ea, op_inc8(s, val8)); s->cycles = 7; break;
        case 0x7D: ea = extended_addr(s); val8 = READ8(s, ea); op_tst8(s, val8); s->cycles = 7; break;
        case 0x7E: ea = extended_addr(s); s->PC = ea; s->cycles = 4; break; /* JMP extended */
        case 0x7F: ea = extended_addr(s); WRITE8(s, ea, op_clr8(s)); s->cycles = 7; break;

        /* ======== A register ALU - immediate ======== */
        case 0x80: s->A = op_sub8(s, s->A, FETCH8(s)); s->cycles = 2; break;
        case 0x81: op_cmp8(s, s->A, FETCH8(s)); s->cycles = 2; break;
        case 0x82: s->A = op_sbc8(s, s->A, FETCH8(s)); s->cycles = 2; break;
        case 0x83: val16 = FETCH16(s); { uint16_t d = REG_D(s); SET_D(s, op_sub16(s, d, val16)); } s->cycles = 4; break; /* SUBD imm */
        case 0x84: s->A = op_and8(s, s->A, FETCH8(s)); s->cycles = 2; break;
        case 0x85: op_bit8(s, s->A, FETCH8(s)); s->cycles = 2; break;
        case 0x86: s->A = FETCH8(s); set_nz8(s, s->A); s->CC &= ~M6809_CC_V; s->cycles = 2; break; /* LDA */
        case 0x88: s->A = op_eor8(s, s->A, FETCH8(s)); s->cycles = 2; break;
        case 0x89: s->A = op_adc8(s, s->A, FETCH8(s)); s->cycles = 2; break;
        case 0x8A: s->A = op_or8(s, s->A, FETCH8(s)); s->cycles = 2; break;
        case 0x8B: s->A = op_add8(s, s->A, FETCH8(s)); s->cycles = 2; break;
        case 0x8C: val16 = FETCH16(s); op_cmp16(s, s->X, val16); s->cycles = 4; break; /* CMPX imm */
        case 0x8D: /* BSR */
            offset = (int16_t)(int8_t)FETCH8(s);
            PUSH16(s, s->PC);
            s->PC += offset;
            s->cycles = 7; break;
        case 0x8E: s->X = FETCH16(s); set_nz16(s, s->X); s->CC &= ~M6809_CC_V; s->cycles = 3; break; /* LDX imm */

        /* ======== A register ALU - direct ======== */
        case 0x90: ea = direct_addr(s); s->A = op_sub8(s, s->A, READ8(s, ea)); s->cycles = 4; break;
        case 0x91: ea = direct_addr(s); op_cmp8(s, s->A, READ8(s, ea)); s->cycles = 4; break;
        case 0x92: ea = direct_addr(s); s->A = op_sbc8(s, s->A, READ8(s, ea)); s->cycles = 4; break;
        case 0x93: ea = direct_addr(s); { uint16_t d = REG_D(s); uint16_t m = READ16(s,ea); SET_D(s, op_sub16(s,d,m)); } s->cycles = 6; break;
        case 0x94: ea = direct_addr(s); s->A = op_and8(s, s->A, READ8(s, ea)); s->cycles = 4; break;
        case 0x95: ea = direct_addr(s); op_bit8(s, s->A, READ8(s, ea)); s->cycles = 4; break;
        case 0x96: ea = direct_addr(s); s->A = READ8(s, ea); set_nz8(s, s->A); s->CC &= ~M6809_CC_V; s->cycles = 4; break;
        case 0x97: ea = direct_addr(s); WRITE8(s, ea, s->A); set_nz8(s, s->A); s->CC &= ~M6809_CC_V; s->cycles = 4; break; /* STA */
        case 0x98: ea = direct_addr(s); s->A = op_eor8(s, s->A, READ8(s, ea)); s->cycles = 4; break;
        case 0x99: ea = direct_addr(s); s->A = op_adc8(s, s->A, READ8(s, ea)); s->cycles = 4; break;
        case 0x9A: ea = direct_addr(s); s->A = op_or8(s, s->A, READ8(s, ea)); s->cycles = 4; break;
        case 0x9B: ea = direct_addr(s); s->A = op_add8(s, s->A, READ8(s, ea)); s->cycles = 4; break;
        case 0x9C: ea = direct_addr(s); op_cmp16(s, s->X, READ16(s, ea)); s->cycles = 6; break;
        case 0x9D: ea = direct_addr(s); PUSH16(s, s->PC); s->PC = ea; s->cycles = 7; break; /* JSR direct */
        case 0x9E: ea = direct_addr(s); s->X = READ16(s, ea); set_nz16(s, s->X); s->CC &= ~M6809_CC_V; s->cycles = 5; break;
        case 0x9F: ea = direct_addr(s); WRITE16(s, ea, s->X); set_nz16(s, s->X); s->CC &= ~M6809_CC_V; s->cycles = 5; break; /* STX */

        /* ======== A register ALU - indexed ======== */
        case 0xA0: ea = indexed_addr(s); s->A = op_sub8(s, s->A, READ8(s, ea)); s->cycles += 4; break;
        case 0xA1: ea = indexed_addr(s); op_cmp8(s, s->A, READ8(s, ea)); s->cycles += 4; break;
        case 0xA2: ea = indexed_addr(s); s->A = op_sbc8(s, s->A, READ8(s, ea)); s->cycles += 4; break;
        case 0xA3: ea = indexed_addr(s); { uint16_t d = REG_D(s); uint16_t m = READ16(s,ea); SET_D(s, op_sub16(s,d,m)); } s->cycles += 6; break;
        case 0xA4: ea = indexed_addr(s); s->A = op_and8(s, s->A, READ8(s, ea)); s->cycles += 4; break;
        case 0xA5: ea = indexed_addr(s); op_bit8(s, s->A, READ8(s, ea)); s->cycles += 4; break;
        case 0xA6: ea = indexed_addr(s); s->A = READ8(s, ea); set_nz8(s, s->A); s->CC &= ~M6809_CC_V; s->cycles += 4; break;
        case 0xA7: ea = indexed_addr(s); WRITE8(s, ea, s->A); set_nz8(s, s->A); s->CC &= ~M6809_CC_V; s->cycles += 4; break;
        case 0xA8: ea = indexed_addr(s); s->A = op_eor8(s, s->A, READ8(s, ea)); s->cycles += 4; break;
        case 0xA9: ea = indexed_addr(s); s->A = op_adc8(s, s->A, READ8(s, ea)); s->cycles += 4; break;
        case 0xAA: ea = indexed_addr(s); s->A = op_or8(s, s->A, READ8(s, ea)); s->cycles += 4; break;
        case 0xAB: ea = indexed_addr(s); s->A = op_add8(s, s->A, READ8(s, ea)); s->cycles += 4; break;
        case 0xAC: ea = indexed_addr(s); op_cmp16(s, s->X, READ16(s, ea)); s->cycles += 6; break;
        case 0xAD: ea = indexed_addr(s); PUSH16(s, s->PC); s->PC = ea; s->cycles += 7; break; /* JSR indexed */
        case 0xAE: ea = indexed_addr(s); s->X = READ16(s, ea); set_nz16(s, s->X); s->CC &= ~M6809_CC_V; s->cycles += 5; break;
        case 0xAF: ea = indexed_addr(s); WRITE16(s, ea, s->X); set_nz16(s, s->X); s->CC &= ~M6809_CC_V; s->cycles += 5; break;

        /* ======== A register ALU - extended ======== */
        case 0xB0: ea = extended_addr(s); s->A = op_sub8(s, s->A, READ8(s, ea)); s->cycles = 5; break;
        case 0xB1: ea = extended_addr(s); op_cmp8(s, s->A, READ8(s, ea)); s->cycles = 5; break;
        case 0xB2: ea = extended_addr(s); s->A = op_sbc8(s, s->A, READ8(s, ea)); s->cycles = 5; break;
        case 0xB3: ea = extended_addr(s); { uint16_t d = REG_D(s); uint16_t m = READ16(s,ea); SET_D(s, op_sub16(s,d,m)); } s->cycles = 7; break;
        case 0xB4: ea = extended_addr(s); s->A = op_and8(s, s->A, READ8(s, ea)); s->cycles = 5; break;
        case 0xB5: ea = extended_addr(s); op_bit8(s, s->A, READ8(s, ea)); s->cycles = 5; break;
        case 0xB6: ea = extended_addr(s); s->A = READ8(s, ea); set_nz8(s, s->A); s->CC &= ~M6809_CC_V; s->cycles = 5; break;
        case 0xB7: ea = extended_addr(s); WRITE8(s, ea, s->A); set_nz8(s, s->A); s->CC &= ~M6809_CC_V; s->cycles = 5; break;
        case 0xB8: ea = extended_addr(s); s->A = op_eor8(s, s->A, READ8(s, ea)); s->cycles = 5; break;
        case 0xB9: ea = extended_addr(s); s->A = op_adc8(s, s->A, READ8(s, ea)); s->cycles = 5; break;
        case 0xBA: ea = extended_addr(s); s->A = op_or8(s, s->A, READ8(s, ea)); s->cycles = 5; break;
        case 0xBB: ea = extended_addr(s); s->A = op_add8(s, s->A, READ8(s, ea)); s->cycles = 5; break;
        case 0xBC: ea = extended_addr(s); op_cmp16(s, s->X, READ16(s, ea)); s->cycles = 7; break;
        case 0xBD: ea = extended_addr(s); PUSH16(s, s->PC); s->PC = ea; s->cycles = 8; break; /* JSR extended */
        case 0xBE: ea = extended_addr(s); s->X = READ16(s, ea); set_nz16(s, s->X); s->CC &= ~M6809_CC_V; s->cycles = 6; break;
        case 0xBF: ea = extended_addr(s); WRITE16(s, ea, s->X); set_nz16(s, s->X); s->CC &= ~M6809_CC_V; s->cycles = 6; break;

        /* ======== B register ALU - immediate ======== */
        case 0xC0: s->B = op_sub8(s, s->B, FETCH8(s)); s->cycles = 2; break;
        case 0xC1: op_cmp8(s, s->B, FETCH8(s)); s->cycles = 2; break;
        case 0xC2: s->B = op_sbc8(s, s->B, FETCH8(s)); s->cycles = 2; break;
        case 0xC3: { uint16_t d = REG_D(s); uint16_t m = FETCH16(s); SET_D(s, op_add16(s,d,m)); } s->cycles = 4; break; /* ADDD imm */
        case 0xC4: s->B = op_and8(s, s->B, FETCH8(s)); s->cycles = 2; break;
        case 0xC5: op_bit8(s, s->B, FETCH8(s)); s->cycles = 2; break;
        case 0xC6: s->B = FETCH8(s); set_nz8(s, s->B); s->CC &= ~M6809_CC_V; s->cycles = 2; break;
        case 0xC8: s->B = op_eor8(s, s->B, FETCH8(s)); s->cycles = 2; break;
        case 0xC9: s->B = op_adc8(s, s->B, FETCH8(s)); s->cycles = 2; break;
        case 0xCA: s->B = op_or8(s, s->B, FETCH8(s)); s->cycles = 2; break;
        case 0xCB: s->B = op_add8(s, s->B, FETCH8(s)); s->cycles = 2; break;
        case 0xCC: { uint16_t v = FETCH16(s); SET_D(s, v); set_nz16(s, v); s->CC &= ~M6809_CC_V; } s->cycles = 3; break; /* LDD imm */
        case 0xCE: s->U = FETCH16(s); set_nz16(s, s->U); s->CC &= ~M6809_CC_V; s->cycles = 3; break; /* LDU imm */

        /* ======== B register ALU - direct ======== */
        case 0xD0: ea = direct_addr(s); s->B = op_sub8(s, s->B, READ8(s, ea)); s->cycles = 4; break;
        case 0xD1: ea = direct_addr(s); op_cmp8(s, s->B, READ8(s, ea)); s->cycles = 4; break;
        case 0xD2: ea = direct_addr(s); s->B = op_sbc8(s, s->B, READ8(s, ea)); s->cycles = 4; break;
        case 0xD3: ea = direct_addr(s); { uint16_t d = REG_D(s); uint16_t m = READ16(s,ea); SET_D(s, op_add16(s,d,m)); } s->cycles = 6; break;
        case 0xD4: ea = direct_addr(s); s->B = op_and8(s, s->B, READ8(s, ea)); s->cycles = 4; break;
        case 0xD5: ea = direct_addr(s); op_bit8(s, s->B, READ8(s, ea)); s->cycles = 4; break;
        case 0xD6: ea = direct_addr(s); s->B = READ8(s, ea); set_nz8(s, s->B); s->CC &= ~M6809_CC_V; s->cycles = 4; break;
        case 0xD7: ea = direct_addr(s); WRITE8(s, ea, s->B); set_nz8(s, s->B); s->CC &= ~M6809_CC_V; s->cycles = 4; break;
        case 0xD8: ea = direct_addr(s); s->B = op_eor8(s, s->B, READ8(s, ea)); s->cycles = 4; break;
        case 0xD9: ea = direct_addr(s); s->B = op_adc8(s, s->B, READ8(s, ea)); s->cycles = 4; break;
        case 0xDA: ea = direct_addr(s); s->B = op_or8(s, s->B, READ8(s, ea)); s->cycles = 4; break;
        case 0xDB: ea = direct_addr(s); s->B = op_add8(s, s->B, READ8(s, ea)); s->cycles = 4; break;
        case 0xDC: ea = direct_addr(s); { uint16_t v = READ16(s,ea); SET_D(s,v); set_nz16(s,v); s->CC &= ~M6809_CC_V; } s->cycles = 5; break;
        case 0xDD: ea = direct_addr(s); { uint16_t v = REG_D(s); WRITE16(s,ea,v); set_nz16(s,v); s->CC &= ~M6809_CC_V; } s->cycles = 5; break;
        case 0xDE: ea = direct_addr(s); s->U = READ16(s, ea); set_nz16(s, s->U); s->CC &= ~M6809_CC_V; s->cycles = 5; break;
        case 0xDF: ea = direct_addr(s); WRITE16(s, ea, s->U); set_nz16(s, s->U); s->CC &= ~M6809_CC_V; s->cycles = 5; break;

        /* ======== B register ALU - indexed ======== */
        case 0xE0: ea = indexed_addr(s); s->B = op_sub8(s, s->B, READ8(s, ea)); s->cycles += 4; break;
        case 0xE1: ea = indexed_addr(s); op_cmp8(s, s->B, READ8(s, ea)); s->cycles += 4; break;
        case 0xE2: ea = indexed_addr(s); s->B = op_sbc8(s, s->B, READ8(s, ea)); s->cycles += 4; break;
        case 0xE3: ea = indexed_addr(s); { uint16_t d = REG_D(s); uint16_t m = READ16(s,ea); SET_D(s, op_add16(s,d,m)); } s->cycles += 6; break;
        case 0xE4: ea = indexed_addr(s); s->B = op_and8(s, s->B, READ8(s, ea)); s->cycles += 4; break;
        case 0xE5: ea = indexed_addr(s); op_bit8(s, s->B, READ8(s, ea)); s->cycles += 4; break;
        case 0xE6: ea = indexed_addr(s); s->B = READ8(s, ea); set_nz8(s, s->B); s->CC &= ~M6809_CC_V; s->cycles += 4; break;
        case 0xE7: ea = indexed_addr(s); WRITE8(s, ea, s->B); set_nz8(s, s->B); s->CC &= ~M6809_CC_V; s->cycles += 4; break;
        case 0xE8: ea = indexed_addr(s); s->B = op_eor8(s, s->B, READ8(s, ea)); s->cycles += 4; break;
        case 0xE9: ea = indexed_addr(s); s->B = op_adc8(s, s->B, READ8(s, ea)); s->cycles += 4; break;
        case 0xEA: ea = indexed_addr(s); s->B = op_or8(s, s->B, READ8(s, ea)); s->cycles += 4; break;
        case 0xEB: ea = indexed_addr(s); s->B = op_add8(s, s->B, READ8(s, ea)); s->cycles += 4; break;
        case 0xEC: ea = indexed_addr(s); { uint16_t v = READ16(s,ea); SET_D(s,v); set_nz16(s,v); s->CC &= ~M6809_CC_V; } s->cycles += 5; break;
        case 0xED: ea = indexed_addr(s); { uint16_t v = REG_D(s); WRITE16(s,ea,v); set_nz16(s,v); s->CC &= ~M6809_CC_V; } s->cycles += 5; break;
        case 0xEE: ea = indexed_addr(s); s->U = READ16(s, ea); set_nz16(s, s->U); s->CC &= ~M6809_CC_V; s->cycles += 5; break;
        case 0xEF: ea = indexed_addr(s); WRITE16(s, ea, s->U); set_nz16(s, s->U); s->CC &= ~M6809_CC_V; s->cycles += 5; break;

        /* ======== B register ALU - extended ======== */
        case 0xF0: ea = extended_addr(s); s->B = op_sub8(s, s->B, READ8(s, ea)); s->cycles = 5; break;
        case 0xF1: ea = extended_addr(s); op_cmp8(s, s->B, READ8(s, ea)); s->cycles = 5; break;
        case 0xF2: ea = extended_addr(s); s->B = op_sbc8(s, s->B, READ8(s, ea)); s->cycles = 5; break;
        case 0xF3: ea = extended_addr(s); { uint16_t d = REG_D(s); uint16_t m = READ16(s,ea); SET_D(s, op_add16(s,d,m)); } s->cycles = 7; break;
        case 0xF4: ea = extended_addr(s); s->B = op_and8(s, s->B, READ8(s, ea)); s->cycles = 5; break;
        case 0xF5: ea = extended_addr(s); op_bit8(s, s->B, READ8(s, ea)); s->cycles = 5; break;
        case 0xF6: ea = extended_addr(s); s->B = READ8(s, ea); set_nz8(s, s->B); s->CC &= ~M6809_CC_V; s->cycles = 5; break;
        case 0xF7: ea = extended_addr(s); WRITE8(s, ea, s->B); set_nz8(s, s->B); s->CC &= ~M6809_CC_V; s->cycles = 5; break;
        case 0xF8: ea = extended_addr(s); s->B = op_eor8(s, s->B, READ8(s, ea)); s->cycles = 5; break;
        case 0xF9: ea = extended_addr(s); s->B = op_adc8(s, s->B, READ8(s, ea)); s->cycles = 5; break;
        case 0xFA: ea = extended_addr(s); s->B = op_or8(s, s->B, READ8(s, ea)); s->cycles = 5; break;
        case 0xFB: ea = extended_addr(s); s->B = op_add8(s, s->B, READ8(s, ea)); s->cycles = 5; break;
        case 0xFC: ea = extended_addr(s); { uint16_t v = READ16(s,ea); SET_D(s,v); set_nz16(s,v); s->CC &= ~M6809_CC_V; } s->cycles = 6; break;
        case 0xFD: ea = extended_addr(s); { uint16_t v = REG_D(s); WRITE16(s,ea,v); set_nz16(s,v); s->CC &= ~M6809_CC_V; } s->cycles = 6; break;
        case 0xFE: ea = extended_addr(s); s->U = READ16(s, ea); set_nz16(s, s->U); s->CC &= ~M6809_CC_V; s->cycles = 6; break;
        case 0xFF: ea = extended_addr(s); WRITE16(s, ea, s->U); set_nz16(s, s->U); s->CC &= ~M6809_CC_V; s->cycles = 6; break;

        /* ======== Page 2 prefix (0x10) ======== */
        case 0x10:
            op = FETCH_OP(s);
            switch (op) {
                /* Long branches */
                case 0x20: case 0x21: case 0x22: case 0x23:
                case 0x24: case 0x25: case 0x26: case 0x27:
                case 0x28: case 0x29: case 0x2A: case 0x2B:
                case 0x2C: case 0x2D: case 0x2E: case 0x2F:
                    offset = (int16_t)FETCH16(s);
                    if (branch_cond(s, op))
                        s->PC += offset;
                    s->cycles = 5;
                    break;

                case 0x3F: /* SWI2 */
                    push_entire_state(s);
                    s->PC = READ16(s, 0xFFF4);
                    s->cycles = 20; break;

                /* CMPD */
                case 0x83: val16 = FETCH16(s); op_cmp16(s, REG_D(s), val16); s->cycles = 5; break;
                case 0x93: ea = direct_addr(s); op_cmp16(s, REG_D(s), READ16(s,ea)); s->cycles = 7; break;
                case 0xA3: ea = indexed_addr(s); op_cmp16(s, REG_D(s), READ16(s,ea)); s->cycles += 7; break;
                case 0xB3: ea = extended_addr(s); op_cmp16(s, REG_D(s), READ16(s,ea)); s->cycles = 8; break;

                /* LDY */
                case 0x8E: s->Y = FETCH16(s); set_nz16(s, s->Y); s->CC &= ~M6809_CC_V; s->cycles = 4; break;
                case 0x9E: ea = direct_addr(s); s->Y = READ16(s,ea); set_nz16(s, s->Y); s->CC &= ~M6809_CC_V; s->cycles = 6; break;
                case 0xAE: ea = indexed_addr(s); s->Y = READ16(s,ea); set_nz16(s, s->Y); s->CC &= ~M6809_CC_V; s->cycles += 6; break;
                case 0xBE: ea = extended_addr(s); s->Y = READ16(s,ea); set_nz16(s, s->Y); s->CC &= ~M6809_CC_V; s->cycles = 7; break;

                /* STY */
                case 0x9F: ea = direct_addr(s); WRITE16(s,ea,s->Y); set_nz16(s, s->Y); s->CC &= ~M6809_CC_V; s->cycles = 6; break;
                case 0xAF: ea = indexed_addr(s); WRITE16(s,ea,s->Y); set_nz16(s, s->Y); s->CC &= ~M6809_CC_V; s->cycles += 6; break;
                case 0xBF: ea = extended_addr(s); WRITE16(s,ea,s->Y); set_nz16(s, s->Y); s->CC &= ~M6809_CC_V; s->cycles = 7; break;

                /* CMPY */
                case 0x8C: val16 = FETCH16(s); op_cmp16(s, s->Y, val16); s->cycles = 5; break;
                case 0x9C: ea = direct_addr(s); op_cmp16(s, s->Y, READ16(s,ea)); s->cycles = 7; break;
                case 0xAC: ea = indexed_addr(s); op_cmp16(s, s->Y, READ16(s,ea)); s->cycles += 7; break;
                case 0xBC: ea = extended_addr(s); op_cmp16(s, s->Y, READ16(s,ea)); s->cycles = 8; break;

                /* LDS */
                case 0xCE: s->S = FETCH16(s); set_nz16(s, s->S); s->CC &= ~M6809_CC_V; s->nmi_armed = 1; s->cycles = 4; break;
                case 0xDE: ea = direct_addr(s); s->S = READ16(s,ea); set_nz16(s, s->S); s->CC &= ~M6809_CC_V; s->nmi_armed = 1; s->cycles = 6; break;
                case 0xEE: ea = indexed_addr(s); s->S = READ16(s,ea); set_nz16(s, s->S); s->CC &= ~M6809_CC_V; s->nmi_armed = 1; s->cycles += 6; break;
                case 0xFE: ea = extended_addr(s); s->S = READ16(s,ea); set_nz16(s, s->S); s->CC &= ~M6809_CC_V; s->nmi_armed = 1; s->cycles = 7; break;

                /* STS */
                case 0xDF: ea = direct_addr(s); WRITE16(s,ea,s->S); set_nz16(s, s->S); s->CC &= ~M6809_CC_V; s->cycles = 6; break;
                case 0xEF: ea = indexed_addr(s); WRITE16(s,ea,s->S); set_nz16(s, s->S); s->CC &= ~M6809_CC_V; s->cycles += 6; break;
                case 0xFF: ea = extended_addr(s); WRITE16(s,ea,s->S); set_nz16(s, s->S); s->CC &= ~M6809_CC_V; s->cycles = 7; break;

                default:
                    s->cycles = 2; break; /* Unknown page 2 opcode */
            }
            break;

        /* ======== Page 3 prefix (0x11) ======== */
        case 0x11:
            op = FETCH_OP(s);
            switch (op) {
                case 0x3F: /* SWI3 */
                    push_entire_state(s);
                    s->PC = READ16(s, 0xFFF2);
                    s->cycles = 20; break;

                /* CMPU */
                case 0x83: val16 = FETCH16(s); op_cmp16(s, s->U, val16); s->cycles = 5; break;
                case 0x93: ea = direct_addr(s); op_cmp16(s, s->U, READ16(s,ea)); s->cycles = 7; break;
                case 0xA3: ea = indexed_addr(s); op_cmp16(s, s->U, READ16(s,ea)); s->cycles += 7; break;
                case 0xB3: ea = extended_addr(s); op_cmp16(s, s->U, READ16(s,ea)); s->cycles = 8; break;

                /* CMPS */
                case 0x8C: val16 = FETCH16(s); op_cmp16(s, s->S, val16); s->cycles = 5; break;
                case 0x9C: ea = direct_addr(s); op_cmp16(s, s->S, READ16(s,ea)); s->cycles = 7; break;
                case 0xAC: ea = indexed_addr(s); op_cmp16(s, s->S, READ16(s,ea)); s->cycles += 7; break;
                case 0xBC: ea = extended_addr(s); op_cmp16(s, s->S, READ16(s,ea)); s->cycles = 8; break;

                default:
                    s->cycles = 2; break;
            }
            break;

        default:
            /* Unknown opcode - treat as NOP */
            s->cycles = 2;
            break;
    }

    s->total_cycles += s->cycles;
  }
  return s->cycles;
}
