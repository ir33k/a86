#define _GNU_SOURCE		/* Needed for fmemopen */
#include <stdio.h>
#include <string.h>
#include "walter.h"
#define A86_IMPLEMENTATION
#include "a86.h"

const char *out_head = "bits 16\n\n";	/* Top of the decoded file */

/* Convert STR string being bits in string format to real bytes
 * puting them in BUF buffer of max SIZ size.  Return number of
 * written bytes. */
size_t str_bytes(char *str, char *buf, size_t siz);

/* Test decode function with STR input against EXP expected result.
 * Return non zero value if test passed. */
int test_trans(char *str, char *exp);

size_t
str_bytes(char *str, char *buf, size_t siz)
{
	size_t i, b = 0;	/* Index for STR and BUF */
	memset(buf, 0, siz);
	for (i = 0; str[i]; i++) {
		if (str[i] <= ' ') { /* Skip whitespaces */
			continue;
		}
		/* Less significant bit at the end. */
		buf[b/8] |= (str[i] == '1') << (7 - b%8);
		b++;
	}
	return b/8;
}

int
test_trans(char *str, char *exp)
{
	enum a86_err err;
	char in[BUFSIZ], out[BUFSIZ], *outp;
	FILE *fp_in, *fp_out;
	size_t siz = str_bytes(str, in, BUFSIZ);
	if ((fp_in = fmemopen(in, siz, "rb")) == 0) {
		return 0;
	}
	if ((fp_out = fmemopen(out, BUFSIZ, "wb")) == 0) {
		return 0;
	}
	err = a86_trans(fp_in, fp_out);
	fclose(fp_in);
	fclose(fp_out);	/* OUT is filled with FP content on fclose */
	if (err) {
		a86_fperror(stderr, err);
		return 0;
	}
	outp = out;
	if (strlen(out) < strlen(out_head)) {
		return 1;
	}
	outp += strlen(out_head);
	/* Use internal Walter macro that return bool. */
	if (WH__STR_EQ(outp, exp)) {
		return 1;
	} else {
		fprintf(stderr, "%s\n", outp);
		return 0;
	}
}

/* Custom Walter test macro, wrapper for test_decode function. */
#define TEST_TRANS(in, exp) \
	ASSERT(test_trans(in, exp), "TEST_TRANS("#in", "#exp")")

TEST("MOV")
{
	/*                  d w mod reg rm */
	TEST_TRANS("100010 0 0 11  000 110", "mov dh, al\n");
	TEST_TRANS("100010 0 1 11  011 001", "mov cx, bx\n");
	TEST_TRANS("100010 0 1 11  011 110", "mov si, bx\n");
        TEST_TRANS("100010 0 0 11  000 110", "mov dh, al\n");
        TEST_TRANS("100010 0 0 11  001 000", "mov al, cl\n");
        TEST_TRANS("100010 0 0 11  100 101", "mov ch, ah\n");
        TEST_TRANS("100010 0 0 11  101 101", "mov ch, ch\n");
        TEST_TRANS("100010 0 1 11  000 011", "mov bx, ax\n");
        TEST_TRANS("100010 0 1 11  000 101", "mov bp, ax\n");
        TEST_TRANS("100010 0 1 11  011 001", "mov cx, bx\n");
        TEST_TRANS("100010 0 1 11  011 010", "mov dx, bx\n");
        TEST_TRANS("100010 0 1 11  011 110", "mov si, bx\n");
        TEST_TRANS("100010 0 1 11  110 011", "mov bx, si\n");
        TEST_TRANS("100010 0 1 11  111 011", "mov bx, di\n");
        TEST_TRANS("100010 0 1 11  111 100", "mov sp, di\n");
	TEST_TRANS("100010 1 0 00  000 000", "mov al, [bx+si]\n");
	TEST_TRANS("100010 1 1 00  011 011", "mov bx, [bp+di]\n");
	/*                  d w mod reg rm  disp-log */
	TEST_TRANS("100010 0 0 01  101 110 00000000", "mov [bp], ch\n");
	TEST_TRANS("100010 1 0 01  100 000 00000100", "mov ah, [bx+si +4]\n");
	TEST_TRANS("100010 1 1 01  000 001 11011011", "mov ax, [bx+di -37]\n");
	TEST_TRANS("100010 1 1 01  010 110 00000000", "mov dx, [bp]\n");
        TEST_TRANS("100010 1 0 01  000 000 00001100", "mov al, [bx+si +12]\n");
        TEST_TRANS("100010 1 0 01  000 000 11110100", "mov al, [bx+si -12]\n");
	TEST_TRANS("100010 1 1 01  010 111 11100000", "mov dx, [bx -32]\n");
	/*                  d w mod reg rm  disp-low disp-hig */
	TEST_TRANS("100010 0 1 10  001 100 11010100 11111110", "mov [si -300], cx\n");
	TEST_TRANS("100010 1 0 00  011 110 10000010 00001101", "mov bx, [3458]\n");
	TEST_TRANS("100010 1 0 10  000 000 10000111 00010011", "mov al, [bx+si +4999]\n");
        TEST_TRANS("100010 1 0 00  101 110 00000101 00000000", "mov bp, [5]\n");
        TEST_TRANS("100010 1 1 10  000 000 01101100 00001111", "mov ax, [bx+si +3948]\n");
        TEST_TRANS("100010 1 1 10  000 000 10010100 11110000", "mov ax, [bx+si -3948]\n");
	/*                   w mod 000 rm  data byte */
	TEST_TRANS("1100011 0 00  000 011 00000111", "mov [bp+di], byte 7\n");
	/*                   w mod 000 rm  disp-low disp-hi   data-low data-hi */
	TEST_TRANS("1100011 1 10  000 101 10000101 00000011  01011011 00000001", "mov [di +901], word 347\n");
	/*                w reg data-low */
	TEST_TRANS("1011 0 001 00001100", "mov cl, byte 12\n");
	TEST_TRANS("1011 0 101 11110100", "mov ch, byte -12\n");
	/*                w reg data-low data-hig */
        TEST_TRANS("1011 1 001 00001100 00000000", "mov cx, word 12\n");
        TEST_TRANS("1011 1 001 11110100 11111111", "mov cx, word -12\n");
        TEST_TRANS("1011 1 010 01101100 00001111", "mov dx, word 3948\n");
        TEST_TRANS("1011 1 010 10010100 11110000", "mov dx, word -3948\n");
	/*                   w addr-low addr-hig */
	TEST_TRANS("1010000 1 11111011 00001001", "mov ax, [2555]\n");
	TEST_TRANS("1010000 1 00010000 00000000", "mov ax, [16]\n");
	TEST_TRANS("1010000 0 00000101", "mov al, [5]\n"); /* Not sure about "al" */
	/*                   w addr-low addr-hig */
	TEST_TRANS("1010001 1 11111010 00001001", "mov [2554], ax\n");
	TEST_TRANS("1010001 1 00001111 00000000", "mov [15], ax\n");
}

SKIP("PUSH") {
	/*                   mod 110 rm */
	TEST_TRANS("11111111 11  110 001", "push cl, dh\n");
	/*                   mod 110 rm  disp-low */
	TEST_TRANS("11111111 01  110 010 00000001", "push [bp+si +1], dh\n");
	TEST_TRANS("11111111 01  110 010 00000001", "push [bp+si +1], dh\n");
}

SKIP("POP") {
	/* TODO */
}
