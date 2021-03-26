/*********************************************************************
 *                     calculation.h (Interface)
 *
 *     Assignment: HW4: arith
 *     Authors:  Hanfeng Xu (hxu06), William Huang (whuang08)
 *     Date:     March 20, 2020
 *     Purpose: This is the interface for the mathematical calculation 
 *              functions needed for compression and decompression of
 *              a ppm file.
 *              Specifically, it contains calculation helper functions 
 *              in RGB <-> CV and CV <-> DCT transformations. Note that
 *              this interface does NOT include the overarching transformation
 *              functions.
 *********************************************************************/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "assert.h"
#include "compress40.h"
#include "pnm.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "uarray2.h"
#include "uarray2b.h"
#include "arith40.h"
#include <math.h>


typedef struct cv cv;
typedef struct DCT DCT;


/* Function: calculateRGB() 
 * Job: for each pixel of cv, calculate its associated RGB values relative to
 * the denominator and store them in a 2D array of Pnm_rgb.
 * Designed as a helper function for storeRGB()
 * Expected input: a struct of {y, pb, pr}, the corresponding Pnm_rgb struct 
 * within the array, denominator
 * Expected output: NONE
 */
extern void calculateRGB(cv ypp, Pnm_rgb dest, int denom);

/* Function: scaleRGB() 
 * Job: Given an unscaled RGB value and the denominator, reset the unscaled 
 * RGB value to 0-1 and return a scaled RGB value. 
 * Designed as a helper function for calculateRGB()
 * Expected input: 1 of the unscaled RGB values, denominator 
 * Expected output: a scaled RGB value
 */
extern unsigned scaleRGB(float value, int denom);

/* Function: calculateCV() 
 * Job: Given a set of 3 scaled RGB values, calculate and store y, pb, pr in  
 * a cv struct and return the struct.
 * Designed as a helper function for storeCV()
 * Expected input: a set of 3 RGB values, denominator
 * Expected output: NONE
 */
extern cv calculateCV(int red, int green, int blue, int denom);

/* Function: calculate_CVtoDCT() 
 * Job: Given 4 cv structs in a 2x2 block and 1 DCT struct, calculate DCT 
 * space {Y1, Y2, Y3, Y4, avepbQUANT, aveprQUANT} and store them in the 
 * given DCT struct
 * Designed as a helper function for store_CVtoDCT()
 * Expected input: 4 cv structs in a 2x2 block and 1 DCT struct
 * Expected output: NONE
 */
extern void calculate_CVtoDCT(cv *elem1, cv *elem2, cv *elem3, cv *elem4, 
                              DCT *dest);
                                                        
/* Function: scaleDCT() 
 * Job: Given 1 number from a,b,c,d and its type (DCT_A or DCT_BCD), return
 *      its scaled value.
 * Designed as a helper function for calculate_CVtoDCT()
 * Expected input: 1 number from a,b,c,d and its type (DCT_A or DCT_BCD)
 * Expected output: scaled value of the input number
 * Error : DCT_type is neither DCT_A or DCT_BCD
 * Handling : abort by assertion
 */
extern int scaleDCT(float num, int DCT_type);

 /* Function: calculate_DCTtoCV() 
  * Job: Given 4 cv structs in a 2x2 block and 1 DCT struct, calculate 4 sets
  * of cv values in the given block {y, pb, pr} and store them in the 
  * given cv structs
  * Designed as a helper function for store_DCTtoCV()
  * Expected input: 4 cv structs in a 2x2 block and 1 DCT struct
  * Expected output: NONE
  */
extern void calculate_DCTtoCV(DCT *dest, cv *elem1, cv *elem2, cv *elem3, 
                                                               cv *elem4);

/* Function: unscaleDCT() 
 * Job: Given 1 scaled number from a,b,c,d and its type (DCT_A or DCT_BCD), 
 *      return its unscaled value.
 * Designed as a helper function for calculate_DCTtoCV()
 * Expected input: 1 scaled number from a,b,c,d and its type (DCT_A or DCT_BCD)
 * Expected output: unscaled value of the input number
 * Error : DCT_type is neither DCT_A or DCT_BCD
 * Handling : abort by assertion
 */
extern float unscaleDCT(int num, int DCT_type);

/* Function: setCV() 
 * Job: Given a cv struct and y, pb, pr value, store the 3 values
 * inside the cv struct 
 * Designed as a helper function for calculate_DCTtoCV()
 * Expected input: a cv struct and y, pb, pr value
 * Expected output: NONE
 */
extern void setCV(cv *elem, float y, float pb, float pr);




