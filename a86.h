#ifndef A86_H			/* Decode 8086 asm bin file */
#define A86_H

#include <stdio.h>
#include <string.h>

enum a86_err {			/* Error codes */
	A86_ERR_OK = 0,		/* Success, no errors */
	A86_ERR_WIP,		/* Parsing in progress */
	A86_ERR_TODO,		/* Instruction not yet implemented */
	A86_ERR_WHAT,		/* Unknown instruction */
	A86_ERR_MISS,		/* Missing byte */
	A86_ERR__SIZ		/* For a86__err array size */
};
enum a86_opc {			/* Instruction operation code */
	A86_OPC_NUL = 0,	/* Unknown */
	A86_OPC_MOV,		/* Move */
	A86_OPC_PUSH,		/* Push */
	A86_OPC_POP,		/* Pop */
	A86_OPC__SIZ		/* For a86__opc_str array size */
};
enum a86_f {			/* Instruction flags */
	A86_F_NUL  = 0,		/* No flags */
	/* Instruction parts */
	A86_F_D    = 1 << 0,	/* Get D */
	A86_F_W0   = 1 << 1,	/* Get W */
	A86_F_W3   = 1 << 2,	/* Get W (in the middle) */
	A86_F_MOD  = 1 << 3,	/* Get and interprate MOD */
	A86_F_REG0 = 1 << 4,	/* Get REG */
	A86_F_REG3 = 1 << 5,	/* Get REG (at the beggining) */
	A86_F_DATA = 1 << 6,	/* Get Data (W=0 byte, W=1 word) */
	A86_F_ADDR = 1 << 7,	/* Get address (accumulator 16b) */
	/* Values */
	A86_F_FLIP = 1 << 8,	/* Set when W is not 0 */
	A86_F_WORD = 1 << 9,	/* Set for word, unset for byte */
	A86_F_EFF  = 1 << 10,	/* Use effective address calculation */
	A86_F_DA   = 1 << 11	/* Use direct address */
};
enum a86_reg {			/* Registers */
	A86_REG_NUL = 0,
	A86_REG_AL,    A86_REG_BL,    A86_REG_CL,    A86_REG_DL,
	A86_REG_AH,    A86_REG_BH,    A86_REG_CH,    A86_REG_DH,
	A86_REG_AX,    A86_REG_BX,    A86_REG_CX,    A86_REG_DX,
	A86_REG_SP,    A86_REG_BP,    A86_REG_SI,    A86_REG_DI,
	A86_REG_BX_SI, A86_REG_BX_DI, A86_REG_BP_SI, A86_REG_BP_DI,
	A86_REG__SIZ		/* For a86__reg_str array size */
};
enum a86_sr {			/* Segment registers */
	A86_SR_NUL = 0,
	A86_SR_ES, A86_SR_CS, A86_SR_SS, A86_SR_DS,
	A86_SR__SIZ		/* For a86__sr_str array size */
};
struct a86_ins {		/* Instruction */
	int f;			/* Flags, enum a86_f */
	enum a86_opc opc;	/* Operation code */
	enum a86_reg dst, src;	/* Destination and source registers */
	int disp, addr, data;	/* Displacement, address, data */
};

/* Registers map. */
const enum a86_reg a86__reg[3][8] = {
	/* Default */
	{A86_REG_AL,    A86_REG_CL,    A86_REG_DL,    A86_REG_BL,
	 A86_REG_AH,    A86_REG_CH,    A86_REG_DH,    A86_REG_BH},
	/* A86_F_WORD */
	{A86_REG_AX,    A86_REG_CX,    A86_REG_DX,    A86_REG_BX,
	 A86_REG_SP,    A86_REG_BP,    A86_REG_SI,    A86_REG_DI},
	/* A86_F_EFF */
	{A86_REG_BX_SI, A86_REG_BX_DI, A86_REG_BP_SI, A86_REG_BP_DI,
	 A86_REG_SI,    A86_REG_DI,    A86_REG_BP,    A86_REG_BX}};

/* Segment register code map. */
const enum a86_sr a86__sr[4] = {A86_SR_ES, A86_SR_CS, A86_SR_SS, A86_SR_DS};

/* Error messages map. */
const char *a86__err[A86_ERR__SIZ] = {
	"INFO: Success, no errors\n",
	"WARN: Parsing in progress\n",
	"ERROR: Unknown instruction\n",
	"ERROR: Not implemented\n",
	"ERROR: Missing byte\n"
};

/* Map instruction operation code to string. */
const char *a86__opc_str[A86_OPC__SIZ] = {"", "mov", "push", "pop"};

/* Map register to string. */
const char *a86__reg_str[A86_REG__SIZ] = {
	"",
	"al",    "bl",    "cl",    "dl",
	"ah",    "bh",    "ch",    "dh",
	"ax",    "bx",    "cx",    "dx",
	"sp",    "bp",    "si",    "di",
	"bx+si", "bx+di", "bp+si", "bp+di"};

/* Map segment register to string. */
const char *a86__sr_str[A86_SR__SIZ] = {"", "es", "cs", "ss", "sd"};

/* Print error message for ERR code in FP file. */
void a86_fperror(FILE *fp, enum a86_err err);

/* Set OPC instruction operation code and F flags describing parts of
 * OPC based on given B byte.  Return non 0 value when failed to set
 * flags.  It's possible to not set the OPC in case of some
 * instruction but flags should always be defined. */
int a86_opc_f(enum a86_opc *opc, int *f, int b);

/* Parse INS instruction with bytes from IN file. */
enum a86_err a86_ins_parse(struct a86_ins *ins, FILE *in);

/* Print INS to OUT file. */
enum a86_err a86_ins_print(struct a86_ins *ins, FILE *out);

/* Translate all instructions in IN input file and print asembler
 * string representation in OUT output file. */
enum a86_err a86_trans(FILE *in, FILE *out);

#endif /* A86_H */
#ifdef A86_IMPLEMENTATION /* ====================================== */

void
a86_fperror(FILE *fp, enum a86_err err)
{
	fputs(a86__err[err], fp);
}

int
a86_opc_f(enum a86_opc *opc, int *f, int b)
{
	*opc = A86_OPC_MOV;
	if ((b>>2 & 0x3f) == 0x22) { *f = A86_F_D | A86_F_W0 | A86_F_MOD; return 0; }
	if ((b>>1 & 0x7f) == 0x63) { *f = A86_F_D | A86_F_W0 | A86_F_MOD | A86_F_DATA; return 0; }
	if ((b>>4 & 0x0f) == 0x0b) { *f = A86_F_W3 | A86_F_REG3 | A86_F_DATA; return 0; }
	if ((b>>1 & 0x7f) == 0x50) { *f = A86_F_D | A86_F_W0 | A86_F_ADDR; return 0; }
	if ((b>>1 & 0x7f) == 0x51) { *f = A86_F_D | A86_F_W0 | A86_F_ADDR; return 0; }
	if ((b    & 0xff) == 0x8e) { return 1; } /* TODO */
	if ((b    & 0xff) == 0x8c) { return 1; } /* TODO */
	*opc = A86_OPC_PUSH;
	if ((b    & 0xff) == 0xff) { *f = A86_F_MOD; return 0; }
	if ((b>>3 & 0x1f) == 0x0a) { *f = A86_F_REG3; return 0; }
	if ((b    & 0x07) == 0x06 &&
	    (b>>6 & 0x07) == 0x00) { *f = A86_F_REG0; return 0; }
	*opc = A86_OPC_POP;
	if ((b    &0xff ) == 0x8f) { *f = A86_F_MOD; return 0; }
	if ((b>>3 & 0x1f) == 0x0b) { *f = A86_F_REG3; return 0; }
	if ((b    & 0x07) == 0x07 &&
	    (b>>6 & 0x07) == 0x00) { *f = A86_F_REG0; return 0; }
	*opc = 0;
	return 1;
}

enum a86_err
a86_ins_parse(struct a86_ins *ins, FILE *in)
{
	int mod=0, reg=0, rm=0; /* Values not stored in INS */
	char byte;		/* Current byte */
	if (fread(&byte, 1, 1, in) == 0) {
		return A86_ERR_OK; /* End parsing here */
	}
	memset(ins, 0, sizeof(struct a86_ins));
	/* Get instruction OPC and set flags. */
	if (a86_opc_f(&ins->opc, &ins->f, byte)) {
		return A86_ERR_WHAT;
	}
	/* Get value of D. */
	if (ins->f & A86_F_D && (byte >> 1 & 0x01)) {
		ins->f |= A86_F_FLIP;
	}
	/* Get value of W. */
	if ((ins->f & A86_F_W0) && (byte >> 0 & 0x01)) {
		ins->f |= A86_F_WORD;
	}
	if ((ins->f & A86_F_W3) && (byte >> 3 & 0x01)) {
		ins->f |= A86_F_WORD;
	}
	/* Get value of REG. */
	reg = (ins->f & A86_F_REG0) ? (byte >> 3 & 0x07) :
	      (ins->f & A86_F_REG3) ? (byte >> 0 & 0x07) : 0;
	/* Handle MOD. */
	if (ins->f & A86_F_MOD) {
		if (!fread(&byte, 1, 1, in)) return A86_ERR_MISS;
		mod = byte >> 6 & 0x03;
		reg = byte >> 3 & 0x07;
		rm  = byte >> 0 & 0x07;
		/* Use effective address calculation */
		if (mod < 3) {
			ins->f |= A86_F_EFF;
		}
		/* Use direct address. */
		if (mod == 0 && rm == 0x06) {
			ins->f |= A86_F_DA;
		}
		/* Get displacement low byte. */
		if (mod == 1 || mod == 2 || (ins->f & A86_F_DA)) {
			if (!fread(&byte, 1, 1, in)) return A86_ERR_MISS;
			ins->disp = byte;
		}
		/* Get displacement high byte. */
		if (mod == 2 || (ins->f & A86_F_DA)) {
			if (!fread(&byte, 1, 1, in)) return A86_ERR_MISS;
			ins->disp = byte<<8 | (ins->disp & 0xff);
		}
	}
	/* TODO(irek): ADDR and DATA have the same logic.  This could
	 * be a function and then maybe it can be also used on DISP. */
	/* Get address/accumulator value. */
	if (ins->f & A86_F_ADDR) { /* Address, accumulator */
		if (!fread(&byte, 1, 1, in)) return A86_ERR_MISS;
		ins->addr = byte;
		if (ins->f & A86_F_WORD) {
			if (!fread(&byte, 1, 1, in)) return A86_ERR_MISS;
			ins->addr = byte<<8 | (ins->addr & 0xff);
		}
	}
	/* Get data value. */
	if (ins->f & A86_F_DATA) { /* Data */
		if (!fread(&byte, 1, 1, in)) return A86_ERR_MISS;
		ins->data = byte;
		if (ins->f & A86_F_WORD) {
			if (!fread(&byte, 1, 1, in)) return A86_ERR_MISS;
			ins->data = byte<<8 | (ins->data & 0xff);
		}
	}
	/* Set destination registers. */
	ins->dst = a86__reg[(ins->f & A86_F_EFF) ? 2 : !!(ins->f & A86_F_WORD)][rm];
	/* Set source registers. */
	/* TODO(irek): if (ins->f & A86_F_MOD) { */
	ins->src = a86__reg[(ins->f & A86_F_DA)  ? 1 : !!(ins->f & A86_F_WORD)][reg];
	/* There might be another instruction in IN file to parse. */
	return A86_ERR_WIP;
}

enum a86_err
a86_ins_print(struct a86_ins *ins, FILE *out)
{
	char buf1[32], buf2[32]; /* Buffers for printing */
	char *dstp, *srcp;	 /* Pointers to BUF1 & BUF2 */
	int comma = 0;		 /* If true, print comma */
	/* Print: INS[ dst][, src[ (+|-)disp]][, (word|byte) [-]data] */
	fputs(a86__opc_str[ins->opc], out);
	/* Set BUF1, usually a destination register. */
	if ((ins->f & A86_F_DA)) {
		sprintf(buf1, "[%d]", ins->disp);
	} else if ((ins->f & A86_F_EFF) && ins->disp) {
		sprintf(buf1, "[%s %+d]", a86__reg_str[ins->dst], ins->disp);
	} else if ((ins->f & A86_F_EFF)) {
		sprintf(buf1, "[%s]", a86__reg_str[ins->dst]);
	} else {
		sprintf(buf1, "%s", a86__reg_str[ins->dst]);
	}
	/* Set BUF2, usually a source register. */
	if (ins->f & A86_F_ADDR) {
		sprintf(buf2, "[%d]", ins->addr);
	} else {
		sprintf(buf2, "%s", a86__reg_str[ins->src]);
	}
	/* Point to proper destination and source registers taking
	 * FLIP (value of D) flag into consideration. */
	dstp = (ins->f & A86_F_FLIP) ? buf2 : buf1;
	srcp = (ins->f & A86_F_FLIP) ? buf1 : buf2;
	/* Print destination register if DATA not used. */
	if (!(ins->f & A86_F_DATA) && dstp[0]) {
		fprintf(out, " %s", dstp);
		comma=1;
	}
	/* Print source register. */
	if (srcp[0]) {
		if (comma) fputs(",", out);
		fprintf(out, " %s", srcp);
		comma=1;
	}
	/* Append data if defined. */
	if (ins->f & A86_F_DATA) {
		if (comma) fputs(",", out);
		fprintf(out, " %s", (ins->f & A86_F_WORD) ? "word" : "byte");
		if (ins->data) fprintf(out, " %d", ins->data);
	}
	fputc('\n', out);
	return 0;
}

enum a86_err
a86_trans(FILE *in, FILE *out)
{
	enum a86_err err = 0;
	struct a86_ins ins;
	fputs("bits 16\n\n", out);
	while ((err = a86_ins_parse(&ins, in)) == A86_ERR_WIP) {
		if ((err = a86_ins_print(&ins, out))) {
			break;
		}
	}
	return err;
}
#endif	/* A86_IMPLEMENTATION */
