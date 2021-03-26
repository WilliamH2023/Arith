/*********************************************************************
 *                     bitpack.c (Implementation)
 *
 *     Assignment: HW4: arith
 *     Authors:  Hanfeng Xu (hxu06), William Huang (whuang08)
 *     Date:     March 21, 2020
 *     Purpose: This is the implementation for packing bits into 
 *              64-bits signed and unsigned int. 
 *              Specifically, it contains functions for shift operation,
 *              width test, field extraction, field update.
 *              Note that this implementation only support  
 *              uint64_t & int64_t type of 64-bits int.
 *********************************************************************/


#include "bitpack.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "assert.h"

Except_T Bitpack_Overflow = { "Overflow packing bits" };

/* a unsigned 64 bit representation of Zero for repetitive use */
const uint64_t ZERO = 0;


bool Bitpack_fitsu(uint64_t n, unsigned width);
bool Bitpack_fitss(int64_t n, unsigned width);
uint64_t Bitpack_getu(uint64_t word, unsigned width, unsigned lsb);
int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb);
uint64_t Bitpack_newu(uint64_t word, unsigned width, unsigned lsb,
                      uint64_t value);
uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb,
                      int64_t value);
                      
uint64_t left_shiftu(uint64_t num, unsigned shift_num);
int64_t left_shifts(int64_t num, unsigned shift_num);
uint64_t right_shiftu(uint64_t num, unsigned shift_num);
int64_t right_shifts(int64_t num, unsigned shift_num);

/*  Name: Bitpack_fitsu
 *  Purpose: This function tell whether the argument unsigned n can be 
 *            represented in width bits
 *  Input: An unsigned 64 bit int n, and unsigned 32-bit int for its width. 
 *  Output: boolean value stating whether n can be represented or not.
 *  Output expectation: N/A
 *  Error condition: CRE if width is smaller than zero and larger than 64.
 */
bool Bitpack_fitsu(uint64_t n, unsigned width)
{
    
    assert(width > 0 && width <= 64);
    /* dividing by 2 width times should return 0 if fits */
    if (right_shiftu(n, width) == ZERO) {
        return true;
    }
    else{
        return false;
    }
}

/*  Name: Bitpack_fitsu
 *  Purpose: This function tell whether the argument signed n can be 
 *            represented in width bits
 *  Input: A signed 64 bit int n, and unsigned 32-bit int for its width. 
 *  Output: boolean value stating whether n can be represented or not.
 *  Output expectation: N/A
 *  Error condition: CRE if width is smaller than zero and larger than 64.
 *  Note: When width is one, we return false since its impossible to represent
 *        a signed int with 1 bit. 
 */
bool Bitpack_fitss(int64_t n, unsigned width)
{
    assert(width > 0 && width <= 64);
    if (width == 1) {
        return false;
    }
    int64_t base = 1;
    /* width -1 because half of the range is given to negative */
    int64_t max = left_shifts(base, width - 1) - 1;
    int64_t n_one = -1;
    int64_t min = n_one * (max + 1);

    if (n <= max && n >= min){
        return true;
    } else {
        return false;
    }
}
 

/*  Name: Bitpack_getu
 *  Purpose: This function extracts a field from a word given the width of the 
 *          field and the location of the field’s least significant bit.
 *          Then the field is returned in the form of a 64 bit unsigned int. 
 *  Input: An unsigned 64 bit int n, and unsigned ints for its width and lsb.
 *  Output: 64 bit unsigned int
 *  Error condition: CRE if width is smaller than 1 and larger than 64.
 *                  Also when lsb + width is larger than 64. 
 */ 
uint64_t Bitpack_getu(uint64_t word, unsigned width, unsigned lsb)
{
    assert( (lsb+width) <= 64 );
    assert(width > 0 && width <= 64);
    /* create mask/flag to extract the desired field */
    uint64_t mask = ~0;
    mask = right_shiftu(mask, 64 - width);
    mask = left_shiftu(mask, lsb);
    
    uint64_t result = word & mask;
    result = right_shiftu(result, lsb);
    
    return result;
}

/*  Name: Bitpack_gets
 *  Purpose: This function extracts a field from a word given the width of the 
 *          field and the location of the field’s least significant bit.
 *          Then the field is returned in the form of a 64 bit signed int. 
 *  Input: An unsigned 64 bit int n, and unsigned ints for its width and lsb.
 *  Output: 64 bit signed int
 *  Error condition: CRE if width is smaller than 1 and larger than 64.
 *                  Also when lsb + width is larger than 64. 
 */ 
int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb)
{
    assert( (lsb+width) <= 64 );
    assert(width > 1 && width <= 64);
    
    /* only change how its interpreted, not the bits */
    int64_t result_signed = Bitpack_getu(word, width,lsb);
    /* make left digits all 1 if its negative */
    result_signed = left_shifts(result_signed, 64 - width);
    result_signed = right_shifts(result_signed, 64 - width);
    
    return result_signed;
}


/*  Name: Bitpack_newu
 *  Purpose: This function return a new word which is identical to the original
 *        word, except that the field of width width with least significant bit
 *        at lsb will have been replaced by a width-bit representation of value
 *  Input: An unsigned 64 bit int word, value;
 *         and unsigned ints for its width and lsb.
 *  Output: 64 bit unsigned int
 *  Error condition: CRE if width is smaller than 1 and larger than 64.
 *                  Also when lsb + width is larger than 64.
 *                  Hanson exception is raised when value doesn't fit in width.
 */ 
uint64_t Bitpack_newu(uint64_t word, unsigned width, unsigned lsb,
                      uint64_t value)
{
    /* check if the value can fit in width */
    if (!Bitpack_fitsu(value, width)){
        RAISE(Bitpack_Overflow);
    }
    assert(width >= ZERO && width <= 64);
    assert((lsb+width) <= 64);
    
    /* shift the value to the desired position */
    value = left_shiftu(value,lsb);
    
    /* create mask to clear the original value */
    uint64_t mask_front = ~0;
    mask_front = left_shiftu(mask_front,(lsb + width));
    
    uint64_t mask_back = ~0;
    mask_back = right_shiftu(mask_back,(64-lsb));
    uint64_t mask = mask_front | mask_back;
    
    /* this step clears the given field */
    uint64_t result = word & mask;
    /* this step put the value to the cleared field */
    result = result | value;
    
    return result;
}

/*  Name: Bitpack_newu
 *  Purpose: This function return a new word which is identical to the original
 *        word, except that the field of width width with least significant bit
 *        at lsb will have been replaced by a width-bit representation of value
 *  Input: An unsigned 64 bit int word, a signed 64 bit int for the value;
 *         unsigned ints for its width and lsb.
 *  Output: 64 bit unsigned int
 *  Error condition: CRE if width is smaller than 0 and larger than 64.
 *                  Also when lsb + width is larger than 64.
 *                  Hanson exception is raised when value doesn't fit in width.
 */ 
uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb,
                      int64_t value)
{
    if (!Bitpack_fitss(value, width)){
        RAISE(Bitpack_Overflow);
    }
    
    assert(width >= ZERO && width <= 64);
    assert((lsb+width) <= 64);
    
    uint64_t value_unsigned = value;
    /* only get the lower end portion that still represent the value */ 
    value_unsigned = Bitpack_getu(value_unsigned, width, 0);
    
    /* shift the value to the desired position */
    value_unsigned = left_shifts(value_unsigned,lsb);

    /* create mask to clear the original value */
    uint64_t mask_front = ~0;
    mask_front = left_shiftu(mask_front,(lsb + width));
    
    uint64_t mask_back = ~0;
    mask_back = right_shiftu(mask_back,(64-lsb));
    uint64_t mask = mask_front | mask_back;
    
    /* this step clears the given field */
    uint64_t result = word & mask;
    /* this step put the value to the cleared field*/
    result = result | value_unsigned;
    
    return result;
}

/*  Name: left_shiftu
 *  Purpose: This function provides a more sensible version of left shifting 
 *           operations that checks error shift number and handles shift by 64. 
 *           Designed as helper functions for left shift unsigned int.
 *  Input: An unsigned 64 bit int num,
 *         unsigned ints shift_num
 *  Output: 64 bit unsigned int
 *  Error condition: CRE if width is larger than 64.
 *  Note: a shift of 64 will set the value to zero.
 */ 
uint64_t left_shiftu(uint64_t num, unsigned shift_num)
{
    assert(shift_num <= 64); 
    if (shift_num == 64) {
        num = 0;
    }
    else{
        num = num << shift_num;
    }
    return num;
}

/*  Name: left_shifts
 *  Purpose: This function provides a more sensible version of left shifting 
 *           operations that checks error shift number and handles shift by 64. 
 *           Designed as helper functions for left shift signed int.
 *  Input: An signed 64 bit int num,
 *         unsigned ints shift_num
 *  Output: 64 bit signed int
 *  Error condition: CRE if width is larger than 64.
 *  Note: a shift of 64 will set the value to zero.
 */ 
int64_t left_shifts(int64_t num, unsigned shift_num)
{
    assert(shift_num <= 64); 
    if (shift_num == 64) {
        num = 0;
    }
    else{
        num = num << shift_num;
    }
    return num;
}

/*  Name: right_shiftu
 *  Purpose: This function provides a more sensible version of right shifting 
 *           operations that checks error shift number and handles shift by 64. 
 *           Designed as helper functions for right shift unsigned int.
 *  Input: An unsigned 64 bit int num,
 *         unsigned ints shift_num
 *  Output: 64 bit unsigned int
 *  Error condition: CRE if width is larger than 64.
 *  Note: a shift of 64 will set the value to zero.
 *        Since for unsigned int, the logical shift fills the left end with 0s. 
 */ 
uint64_t right_shiftu(uint64_t num, unsigned shift_num)
{
    assert(shift_num <= 64); 
    if (shift_num == 64) {
        num = 0;
    }
    else{
        num = num >> shift_num;
    }
    return num;
}

/*  Name: right_shifts
 *  Purpose: This function provides a more sensible version of right shifting 
 *           operations that checks error shift number and handles shift by 64. 
 *           Designed as helper functions for right shift signed int.
 *  Input: An signed 64 bit int num,
 *         unsigned ints shift_num
 *  Output: 64 bit signed int
 *  Error condition: CRE if width is larger than 64.
 *  Note: a shift of 64 will set the value to 0 when input num > 0; the value 
 *        is ~0 when num < 0.
 *        Since for signed int, the arith shift fills the left end with k 
 *        repetitions of the most significant bit
 */ 
int64_t right_shifts(int64_t num, unsigned shift_num)
{
    assert(shift_num <= 64);
    if (shift_num == 64) {
        if (num < 0) {
            num = ~0;
        }
        else{
            num = 0;
        }
    }
    else{
        num = num >> shift_num;
    }
    return num;
}