/*
 * Integer number functions.
 *
 *  Copyright (C) 2001  Peter Johnson
 *
 *  This file is part of YASM.
 *
 *  YASM is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  YASM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "util.h"
/*@unused@*/ RCSID("$IdPath$");

#include <ctype.h>

#include "bitvect.h"
#include "file.h"

#include "errwarn.h"
#include "intnum.h"


#define BITVECT_ALLOC_SIZE	80

struct intnum {
    union val {
	unsigned long ul;	/* integer value (for integers <=32 bits) */
	intptr bv;		/* bit vector (for integers >32 bits) */
    } val;
    enum { INTNUM_UL, INTNUM_BV } type;
    unsigned char origsize;	/* original (parsed) size, in bits */
};

/* static bitvect used for conversions */
static /*@only@*/ /*@null@*/ wordptr conv_bv = NULL;

void
intnum_shutdown(void)
{
    if (conv_bv) {
	BitVector_Destroy(conv_bv);
	conv_bv = NULL;
    }
}

intnum *
intnum_new_dec(char *str)
{
    intnum *intn = xmalloc(sizeof(intnum));

    intn->origsize = 0;	    /* no reliable way to figure this out */

    if (!conv_bv)
	conv_bv = BitVector_Create(BITVECT_ALLOC_SIZE, FALSE);
    if (BitVector_from_Dec(conv_bv, (unsigned char *)str) == ErrCode_Ovfl)
	Warning(_("Numeric constant too large for internal format"));
    if (Set_Max(conv_bv) < 32) {
	intn->type = INTNUM_UL;
	intn->val.ul = BitVector_Chunk_Read(conv_bv, 32, 0);
    } else {
	intn->type = INTNUM_BV;
	intn->val.bv = BitVector_Clone(conv_bv);
    }

    return intn;
}

intnum *
intnum_new_bin(char *str)
{
    intnum *intn = xmalloc(sizeof(intnum));

    intn->origsize = (unsigned char)strlen(str);

    if(intn->origsize > BITVECT_ALLOC_SIZE)
	Warning(_("Numeric constant too large for internal format"));

    if (!conv_bv)
	conv_bv = BitVector_Create(BITVECT_ALLOC_SIZE, FALSE);
    BitVector_from_Bin(conv_bv, (unsigned char *)str);
    if (Set_Max(conv_bv) < 32) {
	intn->type = INTNUM_UL;
	intn->val.ul = BitVector_Chunk_Read(conv_bv, 32, 0);
    } else {
	intn->type = INTNUM_BV;
	intn->val.bv = BitVector_Clone(conv_bv);
    }

    return intn;
}

intnum *
intnum_new_oct(char *str)
{
    intnum *intn = xmalloc(sizeof(intnum));

    intn->origsize = strlen(str)*3;

    if(intn->origsize > BITVECT_ALLOC_SIZE)
	Warning(_("Numeric constant too large for internal format"));

    if (!conv_bv)
	conv_bv = BitVector_Create(BITVECT_ALLOC_SIZE, FALSE);
    BitVector_from_Oct(conv_bv, (unsigned char *)str);
    if (Set_Max(conv_bv) < 32) {
	intn->type = INTNUM_UL;
	intn->val.ul = BitVector_Chunk_Read(conv_bv, 32, 0);
    } else {
	intn->type = INTNUM_BV;
	intn->val.bv = BitVector_Clone(conv_bv);
    }

    return intn;
}

intnum *
intnum_new_hex(char *str)
{
    intnum *intn = xmalloc(sizeof(intnum));

    intn->origsize = strlen(str)*4;

    if(intn->origsize > BITVECT_ALLOC_SIZE)
	Warning(_("Numeric constant too large for internal format"));

    if (!conv_bv)
	conv_bv = BitVector_Create(BITVECT_ALLOC_SIZE, FALSE);
    BitVector_from_Hex(conv_bv, (unsigned char *)str);
    if (Set_Max(conv_bv) < 32) {
	intn->type = INTNUM_UL;
	intn->val.ul = BitVector_Chunk_Read(conv_bv, 32, 0);
    } else {
	intn->type = INTNUM_BV;
	intn->val.bv = BitVector_Clone(conv_bv);
    }

    return intn;
}

/*@-usedef -compdef -uniondef@*/
intnum *
intnum_new_charconst_nasm(const char *str)
{
    intnum *intn = xmalloc(sizeof(intnum));
    size_t len = strlen(str);

    if (len > 4)
	Warning(_("character constant too large, ignoring trailing characters"));

    intn->val.ul = 0;
    intn->type = INTNUM_UL;
    intn->origsize = len*8;

    switch (len) {
	case 4:
	    intn->val.ul |= (unsigned long)str[3];
	    intn->val.ul <<= 8;
	    /*@fallthrough@*/
	case 3:
	    intn->val.ul |= (unsigned long)str[2];
	    intn->val.ul <<= 8;
	    /*@fallthrough@*/
	case 2:
	    intn->val.ul |= (unsigned long)str[1];
	    intn->val.ul <<= 8;
	    /*@fallthrough@*/
	case 1:
	    intn->val.ul |= (unsigned long)str[0];
    }

    return intn;
}
/*@=usedef =compdef =uniondef@*/

intnum *
intnum_new_int(unsigned long i)
{
    intnum *intn = xmalloc(sizeof(intnum));

    intn->val.ul = i;
    intn->type = INTNUM_UL;
    intn->origsize = 0;

    return intn;
}

intnum *
intnum_copy(const intnum *intn)
{
    intnum *n = xmalloc(sizeof(intnum));

    switch (intn->type) {
	case INTNUM_UL:
	    n->val.ul = intn->val.ul;
	    break;
	case INTNUM_BV:
	    n->val.bv = BitVector_Clone(intn->val.bv);
	    break;
    }
    n->type = intn->type;
    n->origsize = intn->origsize;

    return n;
}

void
intnum_delete(intnum *intn)
{
    if (intn->type == INTNUM_BV)
	BitVector_Destroy(intn->val.bv);
    xfree(intn);
}

/*@-nullderef -nullpass -branchstate@*/
void
intnum_calc(intnum *acc, ExprOp op, intnum *operand)
{
    wordptr result = (wordptr)NULL, op1 = (wordptr)NULL, op2 = (wordptr)NULL;
    wordptr spare = (wordptr)NULL;
    boolean carry;

    /* upsize to bitvector op if one of two parameters is bitvector already.
     * BitVector results must be calculated through intermediate storage.
     */
    if (acc->type == INTNUM_BV || (operand && operand->type == INTNUM_BV)) {
	result = BitVector_Create(BITVECT_ALLOC_SIZE, FALSE);
	spare = BitVector_Create(BITVECT_ALLOC_SIZE, FALSE);

	if (acc->type == INTNUM_BV)
	    op1 = acc->val.bv;
	else {
	    op1 = BitVector_Create(BITVECT_ALLOC_SIZE, TRUE);
	    BitVector_Chunk_Store(op1, 32, 0, acc->val.ul);
	}

	if (operand) {
	    if (operand->type == INTNUM_BV)
		op2 = acc->val.bv;
	    else {
		op2 = BitVector_Create(BITVECT_ALLOC_SIZE, TRUE);
		BitVector_Chunk_Store(op2, 32, 0, operand->val.ul);
	    }
	}
    }

    if (!operand && op != EXPR_NEG && op != EXPR_NOT && op != EXPR_LNOT)
	InternalError(_("Operation needs an operand"));

    /* A operation does a bitvector computation if result is allocated. */
    switch (op) {
	case EXPR_ADD:
	    if (result)
		BitVector_add(result, op1, op2, &carry);
	    else
		acc->val.ul = acc->val.ul + operand->val.ul;
	    break;
	case EXPR_SUB:
	    if (result)
		BitVector_sub(result, op1, op2, &carry);
	    else
		acc->val.ul = acc->val.ul - operand->val.ul;
	    break;
	case EXPR_MUL:
	    if (result)
		/* TODO: Make sure result size = op1+op2 */
		BitVector_Multiply(result, op1, op2);
	    else
		acc->val.ul = acc->val.ul * operand->val.ul;
	    break;
	case EXPR_DIV:
	    if (result) {
		/* TODO: make sure op1 and op2 are unsigned */
		BitVector_Divide(result, op1, op2, spare);
	    } else
		acc->val.ul = acc->val.ul / operand->val.ul;
	    break;
	case EXPR_SIGNDIV:
	    if (result)
		BitVector_Divide(result, op1, op2, spare);
	    else
		acc->val.ul = (unsigned long)((signed long)acc->val.ul /
					      (signed long)operand->val.ul);
	    break;
	case EXPR_MOD:
	    if (result) {
		/* TODO: make sure op1 and op2 are unsigned */
		BitVector_Divide(spare, op1, op2, result);
	    } else
		acc->val.ul = acc->val.ul % operand->val.ul;
	    break;
	case EXPR_SIGNMOD:
	    if (result)
		BitVector_Divide(spare, op1, op2, result);
	    else
		acc->val.ul = (unsigned long)((signed long)acc->val.ul %
					      (signed long)operand->val.ul);
	    break;
	case EXPR_NEG:
	    if (result)
		BitVector_Negate(result, op1);
	    else
		acc->val.ul = -(acc->val.ul);
	    break;
	case EXPR_NOT:
	    if (result)
		Set_Complement(result, op1);
	    else
		acc->val.ul = ~(acc->val.ul);
	    break;
	case EXPR_OR:
	    if (result)
		Set_Union(result, op1, op2);
	    else
		acc->val.ul = acc->val.ul | operand->val.ul;
	    break;
	case EXPR_AND:
	    if (result)
		Set_Intersection(result, op1, op2);
	    else
		acc->val.ul = acc->val.ul & operand->val.ul;
	    break;
	case EXPR_XOR:
	    if (result)
		Set_ExclusiveOr(result, op1, op2);
	    else
		acc->val.ul = acc->val.ul ^ operand->val.ul;
	    break;
	case EXPR_SHL:
	    if (result) {
		if (operand->type == INTNUM_UL) {
		    BitVector_Copy(result, op1);
		    BitVector_Move_Left(result, (N_int)operand->val.ul);
		} else	/* don't even bother, just zero result */
		    BitVector_Empty(result);
	    } else
		acc->val.ul = acc->val.ul << operand->val.ul;
	    break;
	case EXPR_SHR:
	    if (result) {
		if (operand->type == INTNUM_UL) {
		    BitVector_Copy(result, op1);
		    BitVector_Move_Right(result, (N_int)operand->val.ul);
		} else	/* don't even bother, just zero result */
		    BitVector_Empty(result);
	    } else
		acc->val.ul = acc->val.ul >> operand->val.ul;
	    break;
	case EXPR_LOR:
	    if (result) {
		BitVector_Empty(result);
		BitVector_LSB(result, !BitVector_is_empty(op1) ||
			      !BitVector_is_empty(op2));
	    } else
		acc->val.ul = acc->val.ul || operand->val.ul;
	    break;
	case EXPR_LAND:
	    if (result) {
		BitVector_Empty(result);
		BitVector_LSB(result, !BitVector_is_empty(op1) &&
			      !BitVector_is_empty(op2));
	    } else
		acc->val.ul = acc->val.ul && operand->val.ul;
	    break;
	case EXPR_LNOT:
	    if (result) {
		BitVector_Empty(result);
		BitVector_LSB(result, BitVector_is_empty(op1));
	    } else
		acc->val.ul = !acc->val.ul;
	    break;
	case EXPR_EQ:
	    if (result) {
		BitVector_Empty(result);
		BitVector_LSB(result, BitVector_equal(op1, op2));
	    } else
		acc->val.ul = acc->val.ul == operand->val.ul;
	    break;
	case EXPR_LT:
	    if (result) {
		BitVector_Empty(result);
		BitVector_LSB(result, BitVector_Lexicompare(op1, op2) < 0);
	    } else
		acc->val.ul = acc->val.ul < operand->val.ul;
	    break;
	case EXPR_GT:
	    if (result) {
		BitVector_Empty(result);
		BitVector_LSB(result, BitVector_Lexicompare(op1, op2) > 0);
	    } else
		acc->val.ul = acc->val.ul > operand->val.ul;
	    break;
	case EXPR_LE:
	    if (result) {
		BitVector_Empty(result);
		BitVector_LSB(result, BitVector_Lexicompare(op1, op2) <= 0);
	    } else
		acc->val.ul = acc->val.ul <= operand->val.ul;
	    break;
	case EXPR_GE:
	    if (result) {
		BitVector_Empty(result);
		BitVector_LSB(result, BitVector_Lexicompare(op1, op2) >= 0);
	    } else
		acc->val.ul = acc->val.ul >= operand->val.ul;
	    break;
	case EXPR_NE:
	    if (result) {
		BitVector_Empty(result);
		BitVector_LSB(result, !BitVector_equal(op1, op2));
	    } else
		acc->val.ul = acc->val.ul != operand->val.ul;
	    break;
	case EXPR_IDENT:
	    if (result)
		BitVector_Copy(result, op1);
	    break;
    }

    /* If we were doing a bitvector computation... */
    if (result) {
	BitVector_Destroy(spare);

	if (op1 && acc->type != INTNUM_BV)
	    BitVector_Destroy(op1);
	if (op2 && operand && operand->type != INTNUM_BV)
	    BitVector_Destroy(op2);

	/* Try to fit the result into 32 bits if possible */
	if (Set_Max(result) < 32) {
	    if (acc->type == INTNUM_BV) {
		BitVector_Destroy(acc->val.bv);
		acc->type = INTNUM_UL;
	    }
	    acc->val.ul = BitVector_Chunk_Read(result, 32, 0);
	    BitVector_Destroy(result);
	} else {
	    if (acc->type == INTNUM_BV) {
		BitVector_Copy(acc->val.bv, result);
		BitVector_Destroy(result);
	    } else {
		acc->type = INTNUM_BV;
		acc->val.bv = result;
	    }
	}
    }
}
/*@=nullderef =nullpass =branchstate@*/

int
intnum_is_zero(intnum *intn)
{
    return ((intn->type == INTNUM_UL && intn->val.ul == 0) ||
	    (intn->type == INTNUM_BV && BitVector_is_empty(intn->val.bv)));
}

int
intnum_is_pos1(intnum *intn)
{
    return ((intn->type == INTNUM_UL && intn->val.ul == 1) ||
	    (intn->type == INTNUM_BV && Set_Max(intn->val.bv) == 0));
}

int
intnum_is_neg1(intnum *intn)
{
    return ((intn->type == INTNUM_UL && (long)intn->val.ul == -1) ||
	    (intn->type == INTNUM_BV && BitVector_is_full(intn->val.bv)));
}

unsigned long
intnum_get_uint(const intnum *intn)
{
    switch (intn->type) {
	case INTNUM_UL:
	    return intn->val.ul;
	case INTNUM_BV:
	    return BitVector_Chunk_Read(intn->val.bv, 32, 0);
	default:
	    InternalError(_("unknown intnum type"));
	    /*@notreached@*/
	    return 0;
    }
}

long
intnum_get_int(const intnum *intn)
{
    switch (intn->type) {
	case INTNUM_UL:
	    return (long)intn->val.ul;
	case INTNUM_BV:
	    if (BitVector_msb(intn->val.bv)) {
		/* it's negative: negate the bitvector to get a positive
		 * number, then negate the positive number.
		 */
		intptr abs_bv = BitVector_Create(BITVECT_ALLOC_SIZE, FALSE);
		long retval;

		BitVector_Negate(abs_bv, intn->val.bv);
		retval = -((long)BitVector_Chunk_Read(abs_bv, 32, 0));

		BitVector_Destroy(abs_bv);
		return retval;
	    } else
		return (long)BitVector_Chunk_Read(intn->val.bv, 32, 0);
	default:
	    InternalError(_("unknown intnum type"));
	    /*@notreached@*/
	    return 0;
    }
}

void
intnum_get_sized(const intnum *intn, unsigned char *ptr, size_t size)
{
    unsigned long ul;
    unsigned char *buf;
    unsigned int len;

    switch (intn->type) {
	case INTNUM_UL:
	    ul = intn->val.ul;
	    while (size-- > 0) {
		WRITE_BYTE(ptr, ul);
		if (ul != 0)
		    ul >>= 8;
	    }
	    break;
	case INTNUM_BV:
	    buf = BitVector_Block_Read(intn->val.bv, &len);
	    if (len < (unsigned int)size)
		InternalError(_("Invalid size specified (too large)"));
	    memcpy(ptr, buf, size);
	    xfree(buf);
	    break;
    }
}

/* Return 1 if okay size, 0 if not */
int
intnum_check_size(const intnum *intn, size_t size, int is_signed)
{
    if (is_signed) {
	long absl;

	switch (intn->type) {
	    case INTNUM_UL:
		if (size >= 4)
		    return 1;
		/* absl = absolute value of (long)intn->val.ul */
		absl = (long)intn->val.ul;
		if (absl < 0)
		    absl = -absl;

		switch (size) {
		    case 3:
			return ((absl & 0x00FFFFFF) == absl);
		    case 2:
			return ((absl & 0x0000FFFF) == absl);
		    case 1:
			return ((absl & 0x000000FF) == absl);
		}
		break;
	    case INTNUM_BV:
		if (size >= 10)
		    return 1;
		if (BitVector_msb(intn->val.bv)) {
		    /* it's negative */
		    intptr abs_bv = BitVector_Create(BITVECT_ALLOC_SIZE,
						     FALSE);
		    int retval;

		    BitVector_Negate(abs_bv, intn->val.bv);
		    retval = Set_Max(abs_bv) < size*8;

		    BitVector_Destroy(abs_bv);
		    return retval;
		} else
		    return (Set_Max(intn->val.bv) < size*8);
	}
    } else {
	switch (intn->type) {
	    case INTNUM_UL:
		if (size >= 4)
		    return 1;
		switch (size) {
		    case 3:
			return ((intn->val.ul & 0x00FFFFFF) == intn->val.ul);
		    case 2:
			return ((intn->val.ul & 0x0000FFFF) == intn->val.ul);
		    case 1:
			return ((intn->val.ul & 0x000000FF) == intn->val.ul);
		}
		break;
	    case INTNUM_BV:
		if (size >= 10)
		    return 1;
		else
		    return (Set_Max(intn->val.bv) < size*8);
	}
    }
    return 0;
}

void
intnum_print(FILE *f, const intnum *intn)
{
    unsigned char *s;

    switch (intn->type) {
	case INTNUM_UL:
	    fprintf(f, "0x%lx/%u", intn->val.ul, (unsigned int)intn->origsize);
	    break;
	case INTNUM_BV:
	    s = BitVector_to_Hex(intn->val.bv);
	    fprintf(f, "0x%s/%u", (char *)s, (unsigned int)intn->origsize);
	    xfree(s);
	    break;
    }
}
