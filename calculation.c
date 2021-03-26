/*********************************************************************
 *                     calculation.c (Implementation)
 *
 *     Assignment: HW4: arith
 *     Authors:  Hanfeng Xu (hxu06), William Huang (whuang08)
 *     Date:     March 20, 2020
 *     Purpose: This is the implementation for the mathematical calculation 
 *              functions needed for compression and decompression of
 *              a ppm file.
 *              Specifically, it contains calculation helper functions 
 *              in RGB <-> CV and CV <-> DCT transformations. Note that
 *              this implementation does NOT include the overarching 
 *              transformation functions for compression/decompression.
 *********************************************************************/


#include "calculation.h"

/* 
 * the component video struct contains 3 variables: y, pb, pr, calculated 
 * from RGB values and can transform into DCT
 */
struct cv {
    float y;
    float pb;
    float pr;
};

/* the DCT struct contains 6 variables: y, pb, pr, calculated from cv struct */
struct DCT {
    int a;
    int b;
    int c;
    int d;
    unsigned avepbQUANT;
    unsigned aveprQUANT;
};

const int DCT_A = 1;
const int DCT_BCD = 2;

/* Function: calculateRGB() 
 * Job: for each pixel of cv, calculate its associated RGB values relative to
 * the denominator and store them in a 2D array of Pnm_rgb.
 * Designed as a helper function for storeRGB()
 * Expected input: a struct of {y, pb, pr}, the corresponding Pnm_rgb struct 
 * within the array, denominator
 * Expected output: NONE
 */
void calculateRGB(cv ypp, Pnm_rgb dest, int denom)
{
    float y = ypp.y;
    float pb = ypp.pb;
    float pr = ypp.pr;
    float red = 1.0 * y + 0.0 *pb + 1.402 * pr;
    float green = 1.0 * y - 0.344136 * pb - 0.714136 * pr;
    float blue = 1.0 * y + 1.772 * pb + 0.0 * pr;

    
    dest->red = scaleRGB(red, denom);
    dest->green = scaleRGB(green, denom);
    dest->blue = scaleRGB(blue, denom);
}

/* Function: scaleRGB() 
 * Job: Given an unscaled RGB value and the denominator, reset the unscaled 
 * RGB value to 0-1 and return a scaled RGB value. 
 * Designed as a helper function for calculateRGB()
 * Expected input: 1 of the unscaled RGB values, denominator 
 * Expected output: a scaled RGB value
 */
unsigned scaleRGB(float value, int denom)
{
    if (value < 0) {
        value = 0;
    }
    if (value > 1) {
        value = 1;
    }
    return round(value * denom);
}

/* Function: calculateCV() 
 * Job: Given a set of 3 scaled RGB values, calculate and store y, pb, pr in  
 * a cv struct and return the struct.
 * Designed as a helper function for storeCV()
 * Expected input: a set of 3 RGB values, denominator
 * Expected output: NONE
 */
cv calculateCV(int red, int green, int blue, int denom)
{
    float r = (float)red/(float)denom;
    float g = (float)green/(float)denom;
    float b = (float)blue/(float)denom;
    
    cv component;
    component.y = 0.299 * r + 0.587 * g + 0.114 * b;
    component.pb = -0.168736 * r - 0.331264 * g + 0.5 * b;
    component.pr = 0.5 * r - 0.418688 * g - 0.081312 * b;
    return component;
}

/* Function: calculate_CVtoDCT() 
 * Job: Given 4 cv structs in a 2x2 block and 1 DCT struct, calculate DCT 
 * space {Y1, Y2, Y3, Y4, avepbQUANT, aveprQUANT} and store them in the 
 * given DCT struct
 * Designed as a helper function for store_CVtoDCT()
 * Expected input: 4 cv structs in a 2x2 block and 1 DCT struct
 * Expected output: NONE
 */
void calculate_CVtoDCT(cv *elem1, cv *elem2, cv *elem3, cv *elem4, DCT *dest){
    float y1 = elem1->y;
    float y2 = elem2->y;
    float y3 = elem3->y;
    float y4 = elem4->y;

    float a = (y4+y3+y2+y1)/4.0;
    float b = (y4+y3-y2-y1)/4.0;
    float c = (y4-y3+y2-y1)/4.0;
    float d = (y4-y3-y2+y1)/4.0;
    
    dest->a = scaleDCT(a, DCT_A);
    dest->b = scaleDCT(b, DCT_BCD);
    dest->c = scaleDCT(c, DCT_BCD);
    dest->d = scaleDCT(d, DCT_BCD);
    
    float avepb = (elem1 -> pb + elem2 -> pb + elem3 -> pb + elem4 -> pb)/4.0;
    float avepr = (elem1 -> pr + elem2 -> pr + elem3 -> pr + elem4 -> pr)/4.0;
    
    dest->avepbQUANT = Arith40_index_of_chroma(avepb);
    dest->aveprQUANT = Arith40_index_of_chroma(avepr);
}

/* Function: scaleDCT() 
 * Job: Given 1 number from a,b,c,d and its type (DCT_A or DCT_BCD), return
 *      its scaled value.
 * Designed as a helper function for calculate_CVtoDCT()
 * Expected input: 1 number from a,b,c,d and its type (DCT_A or DCT_BCD)
 * Expected output: scaled value of the input number
 * Error : DCT_type is neither DCT_A or DCT_BCD
 * Handling : abort by assertion
 */
int scaleDCT(float num, int DCT_type)
{
    int scale = 0;
    assert(DCT_type == DCT_A || DCT_type == DCT_BCD);
    
    /* for value a(range: 0 ~ 1), scale it to 9 bits by *511 and round() */
    if (DCT_type == DCT_A) {
        scale = round(num * 63.0);
        
        if (scale > 63) {
            scale = 63;
        }
        else if (scale < 0){
            scale = 0;
        }
    }
    /* 
     * for value b/c/d(range: -0.3 ~ 0.3), scale it to 5 bits signed by *50 
     * and round(). Note that the scaled range is -15 ~ 15. 
     */
    if (DCT_type == DCT_BCD) {
        scale = round(num * 103.0);
        if (scale > 31) {
            scale = 31;
        }
        else if (scale < -31){
            scale = -31;
        }
    }
    return scale;
}

/* Function: unscaleDCT() 
 * Job: Given 1 scaled number from a,b,c,d and its type (DCT_A or DCT_BCD), 
 *      return its unscaled value.
 * Designed as a helper function for calculate_DCTtoCV()
 * Expected input: 1 scaled number from a,b,c,d and its type (DCT_A or DCT_BCD)
 * Expected output: unscaled value of the input number
 * Error : DCT_type is neither DCT_A or DCT_BCD
 * Handling : abort by assertion
 */
float unscaleDCT(int num, int DCT_type)
{
    float unscale = 0;
    assert(DCT_type == DCT_A || DCT_type == DCT_BCD);
    
    /* for value a(9 bits unsigned), unscale it to 0~1 by dividing 511 */
    if (DCT_type == DCT_A) {
        unscale = (float)num / 63.0;
        if (unscale > 1.0) {
            unscale = 1.0;
        }
        else if (unscale < 0.0){
            unscale = 0.0;
        }
    }
    /* 
     * for value b/c/d(5 bits signed, -15~15), unscale it to -0.3~0.3 by  
     * dividing 50.
     */
    if (DCT_type == DCT_BCD) {
        unscale = (float)num / 103.0;
        if (unscale > 0.3) {
            unscale = 0.3;
        }
        else if (unscale < -0.3){
            unscale = -0.3;
        }
    }
    return unscale;
    
}

/* Function: calculate_DCTtoCV() 
 * Job: Given 4 cv structs in a 2x2 block and 1 DCT struct, calculate 4 sets
 * of cv values in the given block {y, pb, pr} and store them in the 
 * given cv structs
 * Designed as a helper function for store_DCTtoCV()
 * Expected input: 4 cv structs in a 2x2 block and 1 DCT struct
 * Expected output: NONE
 */
void calculate_DCTtoCV(DCT *origin, cv *elem1, cv *elem2, cv *elem3, cv *elem4)
{
    
    float a  = unscaleDCT(origin -> a, DCT_A);
    float b  = unscaleDCT(origin -> b, DCT_BCD);
    float c  = unscaleDCT(origin -> c, DCT_BCD);
    float d  = unscaleDCT(origin -> d, DCT_BCD);
    float y1 = a - b - c + d;
    float y2 = a - b + c - d;
    float y3 = a + b - c - d;
    float y4 = a + b + c + d;
    float pb = Arith40_chroma_of_index(origin->avepbQUANT);
    float pr = Arith40_chroma_of_index(origin->aveprQUANT);

    setCV(elem1, y1, pb, pr);
    setCV(elem2, y2, pb, pr);
    setCV(elem3, y3, pb, pr);
    setCV(elem4, y4, pb, pr);
}

/* Function: setCV() 
 * Job: Given a cv struct and y, pb, pr value, store the 3 values
 * inside the cv struct 
 * Designed as a helper function for calculate_DCTtoCV()
 * Expected input: a cv struct and y, pb, pr value
 * Expected output: NONE
 */
void setCV(cv *elem, float y, float pb, float pr)
{
    elem -> y = y;
    elem -> pb = pb;
    elem -> pr = pr;
}




