/*********************************************************************
 *                     compress40.c (Implementation)
 *
 *     Assignment: HW4: arith
 *     Authors:  Hanfeng Xu (hxu06), William Huang (whuang08)
 *     Date:     March 21, 2020
 *     Purpose: This is the implementation for ppm image compression
 *              & decompression.
 *              Specifically, it contains functions for RGB <-> CV and 
 *              CV <-> DCT transformations, as well as for reading in
 *              ppm files and compressed files.
 *********************************************************************/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "assert.h"
#include "compress40.h"
#include "pnm.h"
#include "a2methods.h"
#include "a2plain.h"
#include "uarray2.h"
#include "arith40.h"
#include <math.h>
#include "calculation.h"
#include "bitpack.h"

#define A2 A2Methods_UArray2

/* 
 * the component video struct contains 3 variables: y, pb, pr, calculated 
 * from RGB values and can transform into DCT
 */
struct cv {
    float y;
    float pb;
    float pr;
};

/* 
 * the DCT struct contains 6 variables: a, b ,c ,d, avepbQUANT, aveprQUANT,
 * calculated from cv struct 
 */
struct DCT {
    int a;
    int b;
    int c;
    int d;
    unsigned avepbQUANT;
    unsigned aveprQUANT;
};

/* the Info struct contains A2Methods and A2 for passing into *cl */
typedef struct Info {
    A2Methods_T methods;
    A2 array;
} Info;


void compress40 (FILE *input);
void decompress40(FILE *input);/* reads compressed image, writes PPM */


void trimDimension(Pnm_ppm origImage, A2Methods_T methods);
void movePixel(A2Methods_T methods, A2 origArray, A2 finalArray, 
                        int col, int row, int newCol, int newRow);

A2 RGB_toCV(Pnm_ppm origImage, A2Methods_T methods);
void storeCV(int col, int row, A2 origArray, void *elem, void *cl);

void CV_toRGB(Pnm_ppm finalImage, A2 CV_pixels, A2Methods_T methods);
void storeRGB(int col, int row, A2 CV_pixels, void *elem, void *cl);

A2 CVtoDCT(A2 arrayYPP, A2Methods_T methods);
void store_CVtoDCT(int col, int row, A2 arrayDCT, void *elem, void *cl);

void packDCT(A2 arrayDCT, A2Methods_T methods);
void printPackedDCT(int col, int row, A2 arrayDCT, void *elem, void *cl);

Pnm_ppm readHeader(FILE* input, A2Methods_T methods);
void unpackDCT(A2 arrayDCT, FILE *input, A2Methods_T methods);
void readPackedDCT(int col, int row, A2 arrayDCT, void *elem, void *cl);

void DCTtoCV(A2 arrayDCT, A2 arrayYPP_back, A2Methods_T methods);
void store_DCTtoCV(int col, int row, A2 arrayDCT, void *elem, void *cl);


/*  Name: compress40
 *  Purpose: This function read in a ppmfile from the input file and print to
 *           stdout its compressed version with a specified format. 
 *  Input:  a pointer to the input file
 *  Input expectation: the parameters should not be NULL. 
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: CRE if input is null or Pnm_ppm is invalid.
 */
void compress40 (FILE *input)
{
    /* default to UArray2 methods */
    A2Methods_T methods = uarray2_methods_plain; 

    assert (input != NULL && methods != NULL);
    Pnm_ppm origImage = Pnm_ppmread(input, methods);
    
    trimDimension(origImage, methods);
    /* RGB -> CV */
    A2 arrayYPP = RGB_toCV(origImage, methods);
    
    /* CV -> DCT (1/4 sized DCT array)*/
    A2 arrayDCT = CVtoDCT(arrayYPP, methods);
    
    /* packing DCT info into codewords using bitpack.c */
    packDCT(arrayDCT, methods);
    
    
    methods->free(&arrayDCT);
    methods->free(&arrayYPP);
    Pnm_ppmfree(&origImage);
}


/*  Name: decompress40
 *  Purpose: This function read in a compressed from the input to a ppm file
 *           write the result ppm in standard output. 
 *  Input: A pointer to the input compressed file
 *  Input expectation: the parameters should not be NULL. 
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: CRE if input is null, does not conform to the specified 
 *                  format, or when input is not long enough.
 */
void decompress40(FILE *input)
{   
    /* default to UArray2 methods */
    A2Methods_T methods = uarray2_methods_plain; 

    assert (input != NULL && methods != NULL);
    
    /* read in header info of the compressed file */
    Pnm_ppm d_image = readHeader(input, methods);

    /* prepping for decompression */
    int width = d_image -> width;
    int height = d_image -> height;
    A2 arrayDCT = methods -> new(width/2, height/2, sizeof(DCT));
    assert(arrayDCT);
    /* expand each DCT element to 2*2 CV */
    A2 arrayYPP_back = methods -> new(width, height, sizeof(cv));
    assert(arrayYPP_back);
    
    /* decompression steps */
    unpackDCT(arrayDCT, input, methods);
    DCTtoCV(arrayDCT, arrayYPP_back, methods);
    CV_toRGB(d_image, arrayYPP_back, methods);
    
    /* write output to stdout */
    Pnm_ppmwrite(stdout, d_image);
    methods->free(&arrayDCT);
    methods->free(&arrayYPP_back);
    Pnm_ppmfree(&d_image);
}

/*  Name: trimDimension
 *  Purpose: This function trims the dimension of the imput Pnm_ppm to have an
 *           even width and height.
 *  Input: An already initialized Pnm_ppm, a pointer to function struct of
 *         chosen method
 *  Input expectation: the parameters should not be NULL.
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: CRE if either the width or height is less than 2, and 
 *                   when passed argument is NULL.
 */
void trimDimension(Pnm_ppm origImage, A2Methods_T methods)
{
    assert(origImage != NULL && methods != NULL);
    int width = origImage -> width;
    int height = origImage -> height;
    assert(width > 1 && height > 1);
    if (width % 2 == 0 && height % 2 == 0) {
        return;
    }
    
    /* update dimension */
    if (width % 2 != 0) {
        origImage -> width = width - 1;
    }
    if (height % 2 != 0) {
        origImage -> height = height - 1;
    }
    A2 origArray = origImage -> pixels;
    A2 finalArray = methods -> new(origImage -> width, origImage -> height, 
                                                  methods -> size(origArray));
    assert(finalArray != NULL);
    
    /* copy pixels to the new trimmed array */
    for (unsigned row = 0; row < origImage -> height; row++) {
        for (unsigned col = 0; col < origImage -> width; col++) {
            movePixel(methods, origArray, finalArray, col, row, col, row);
        }
    }
    origImage -> pixels = finalArray;
    methods -> free(&origArray);
    
}

/*  Name: movePixel
 *  Purpose: This function moves the pixel at given index from the first array
 *           to pixel at given index of the second array. 
 *  Input: two integers for column and row of origArray, two integers for
 *         column and row of the finalArray. Two pointers to A2array that
 *         stores the pixels' data. A pointer to function struct of
 *         chosen method. 
 *  Input expectation: the parameters should not be NULL.
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: EXIT_FAILURE if passed in parameter is NULL. 
 */
void movePixel(A2Methods_T methods, A2 origArray, A2 finalArray, 
                        int col, int row, int newCol, int newRow)
{
    if (methods == NULL || origArray == NULL || finalArray == NULL) {
        exit(EXIT_FAILURE);
    }

    /* copy RGB values from one pixel to another pixel */
    Pnm_rgb origPixel = methods -> at(origArray, col, row);
    Pnm_rgb finalPixel = methods -> at(finalArray, newCol, newRow);
    finalPixel -> red   = origPixel -> red;
    finalPixel -> green = origPixel -> green;
    finalPixel -> blue  = origPixel -> blue;    
}

/*  Name: RGB_toCV
 *  Purpose: This function creates and returns a new initialized A2 array that
 *           stores Component Video
 *           representation of the passed in Pnm_ppm pixels. 
 *  Input: An already initialized Pnm_ppm, a pointer to function struct of
 *         chosen method
 *  Input expectation: The parameters should not be NULL.
 *  Output: An A2 array of CV representation of RGB values in Pnm_ppm with 
 *         the exact same dimensions. 
 *  Output expectation: N/A
 *  Error condition: CRE if any of the parameter is NULL.
 */
A2 RGB_toCV(Pnm_ppm origImage, A2Methods_T methods)
{
    assert(origImage != NULL && methods != NULL );
    int width = origImage -> width;
    int height = origImage -> height;
    
    A2 arrayYPP = methods -> new(width, height, sizeof(cv));
    assert(arrayYPP != NULL);
    methods -> map_default(arrayYPP, storeCV, origImage);
        
    return arrayYPP;
}


/*  Name: storeCV
 *  Purpose: This function is the apply function for method's map function. 
 *           Specifically, it is applied in RGB_toCV.
 *           It simply read one element from the pixels in the passed in Pnm,
 *           and convert its RGB values to CV values and stores it in the
 *           corresponding index in the destination Array. 
 *  Input: two integers for column and row, a pointer to A2array that stores
 *         CV data, an void pointer that represent a CV struct in the 
 *         destination array, and the closure pointer to original image.
 *         Pnm_ppm struct.
 *  Input expectation: the parameters should not be NULL.
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: N/A
 */
void storeCV(int col, int row, A2 dest_Array, void *elem, void *cl)
{
    assert(dest_Array != NULL && cl != NULL);

    Pnm_ppm origImage = cl;
    assert(origImage -> methods != NULL);
    
    Pnm_rgb source_elem = origImage -> methods-> at(origImage->pixels, 
                                                    col, row);
    cv *result = origImage -> methods->at(dest_Array, col, row);
    cv component = calculateCV(source_elem -> red,
                               source_elem -> green,
                               source_elem -> blue,
                               origImage   -> denominator);

    *result = component;
    (void) elem;
}


/*  Name: RGB_toCV
 *  Purpose: This function creates and initialize an A2 array that
 *           stores RGB calculated from the corresponding index in the
 *           cv array. Then it is set as the pixels of the passed in Pnm_ppm.
 *  Input: An already initialized Pnm_ppm with uninitialized pixels, a pointer
 *         to function struct of chosen method, and an A2array with CV pixels. 
 *  Input expectation: The parameters should not be NULL.
 *  Output: N/A. The newly created A2array is attached to the Pnm_ppm.
 *  Output expectation: N/A
 *  Error condition: CRE if any of the parameter is NULL.
 */
void CV_toRGB(Pnm_ppm finalImage, A2 CV_pixels, A2Methods_T methods)
{
        
    assert(finalImage != NULL && CV_pixels != NULL && methods != NULL);
    
    int width = finalImage -> width;
    int height = finalImage -> height;
    
    A2 arrayRGB = methods -> new(width, height, sizeof(struct Pnm_rgb));
    assert(arrayRGB != NULL);
    finalImage -> pixels = arrayRGB;
    
    methods -> map_default(CV_pixels, storeRGB, finalImage);
}

/*  Name: storeRGB
 *  Purpose: This function is the apply function for method's map function. 
 *           Specifically, it is applied in CV_toRGB.
 *           It simply read one element from the pixels in the passed in Pnm,
 *           and convert its CV values to RGB values and stores it in the
 *           corresponding index in the pixels of Pnm_ppm. 
 *  Input: two integers for column and row, a pointer to A2array that stores
 *         CV data, an void pointer that represent a CV struct in the 
 *         destination array, and the closure pointer to final image.
 *         Pnm_ppm struct.
 *  Input expectation: the parameters should not be NULL.
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: N/A
 */
void storeRGB(int col, int row, A2 CV_pixels, void *elem, void *cl)
{
    assert(cl != NULL && elem != NULL);
    Pnm_ppm finalImage = cl;
    cv *cv_elem = elem;
    assert(finalImage -> methods != NULL);
    
    Pnm_rgb final_pixel = finalImage -> methods -> at(finalImage -> pixels, 
                                                      col, row);
    /* convert CV to RGB and save into the final_pixel */
    calculateRGB(*cv_elem, final_pixel, finalImage -> denominator);
    (void) CV_pixels;
}


/*  Name: CVtoDCT
 *  Purpose: This function creates and initialize an A2 array that
 *           stores DCT calculated from the corresponding indexes in the
 *           cv array. This A2 array is then returned.
 *  Input: An already initialized arrayYPP with cv values; a pointer to
 *         function struct of chosen method. 
 *  Input expectation: The parameters should not be NULL.
 *  Output: A A2array of DCT values with 1/4 the size of original arrayYPP.
 *  Output expectation: N/A
 *  Error condition: CRE if any of the parameter is NULL.
 */
A2 CVtoDCT(A2 arrayYPP, A2Methods_T methods)
{
    assert(arrayYPP != NULL && methods != NULL);
    /* each 2*2 block is converted to 1 element */
    A2 arrayDCT = methods -> new(methods -> width(arrayYPP)/2, 
                                 methods -> height(arrayYPP)/2, sizeof(DCT));
    assert(arrayDCT);
    
    Info arr_method = {methods, arrayYPP};
    methods -> map_default(arrayDCT, store_CVtoDCT, &arr_method);
    return arrayDCT;
}


/*  Name: store_CVtoDCT
 *  Purpose: This function is the apply function for method's map function. 
 *           Specifically, it is applied in CVtoDCT.
 *           It reads four element from the A2array of CV, then calculate the
 *           corresponding DCT values and stores it in the array of DCT. 
 *           Each 2*2 block is converted to one element of DCT.
 *  Input: two integers for column and row, a pointer to A2array that stores
 *         DCT data, an void pointer that represent an element in arrayDCT.
 *         Closure pointer include struct of Info, which contains the CV array
 *         and the method suite.
 *  Input expectation: the parameters should not be NULL.
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: N/A
 */
void store_CVtoDCT(int col, int row, A2 arrayDCT, void *elem, void *cl)
{
    assert(cl != NULL);
    Info *arr_method = cl;
    A2 arrayYPP = arr_method -> array;
    assert(arrayYPP && arr_method -> methods);
    
    cv *elem1 = arr_method -> methods -> at(arrayYPP, col*2,   row*2   );
    cv *elem2 = arr_method -> methods -> at(arrayYPP, col*2+1, row*2   );
    cv *elem3 = arr_method -> methods -> at(arrayYPP, col*2,   row*2+1 );
    cv *elem4 = arr_method -> methods -> at(arrayYPP, col*2+1, row*2+1 );
    
    DCT *result = elem;
    calculate_CVtoDCT(elem1, elem2, elem3, elem4, result);
    (void) arrayDCT;
    (void) elem;    
}


/*  Name: readHeader
 *  Purpose: This function reads and checks the given header of the Compressed
 *           image and initialize the Pnm_ppm values according to the given 
 *           dimensions.
 *  Input: A file pointer to read the inputs from, and the pointer to the 
 *         method suite.
 *  Input expectation: the parameters should not be NULL.
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: N/A
 */
Pnm_ppm readHeader(FILE* input, A2Methods_T methods)
{
    assert(input != NULL && methods != NULL);
    /* read in header info of the compressed file */
    unsigned height, width;
    int read = fscanf(input, "COMP40 Compressed image format 2\n%u %u", 
                      &width, &height);
    assert(read == 2);
    int c = getc(input);
    assert(c == '\n');

    /* store header info into our ppm output file */
    Pnm_ppm d_image = malloc(sizeof(struct Pnm_ppm));
    assert(d_image != NULL);
    d_image -> denominator = 255;
    d_image -> width = width;
    d_image -> height = height;
    d_image -> methods = methods;
    return d_image;
}

/*  Name: DCTtoCV
 *  Purpose: This function stores CV values in an initialize A2 array that was
 *           calculated from the corresponding indexes in the DCT array.
 *  Input: An already initialized array with values; a pointer to function
 *         struct of chosen method. An arrayYPP_back with empty values.
 *  Input expectation: The parameters should not be NULL.
 *  Output: N/A. Stores value into A2array of YPP values with 4 times the size 
 *          of original arrayYPP.
 *  Output expectation: N/A
 *  Error condition: CRE if any of the parameter is NULL.
 */
void DCTtoCV(A2 arrayDCT, A2 arrayYPP_back, A2Methods_T methods)
{
    assert(arrayDCT != NULL && arrayYPP_back != NULL && methods != NULL);
                            
    Info arr_method = {methods, arrayYPP_back};
    methods -> map_default(arrayDCT, store_DCTtoCV, &arr_method);
}

/*  Name: store_DCTtoCV
 *  Purpose: This function is the apply function for method's map function. 
 *           Specifically, it is applied in DCTtoCV.
 *           It reads a element from the A2array of DCT, then calculate the
 *           corresponding CV values and stores it in the array of CV. 
 *           Each one element of block is converted to 2*2 blocks of CV.
 *  Input: two integers for column and row, a pointer to A2array that stores
 *         DCT data, an void pointer that represent an element in arrayDCT.
 *         Closure pointer include struct of Info, which contains the CV array
 *         to be populated and the method suite.
 *  Input expectation: the parameters should not be NULL.
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: N/A
 */
void store_DCTtoCV(int col, int row, A2 arrayDCT, void *elem, void *cl)
{
    assert(elem != NULL && cl != NULL);
    Info *arr_method = cl;
    A2 arrayYPP_back = arr_method -> array;
    assert(arrayYPP_back);
    DCT *origin_elem = elem;
    /* get the elements from the corresponding blocks */
    cv *elem1 = arr_method -> methods -> at(arrayYPP_back, col*2,   row*2   );
    cv *elem2 = arr_method -> methods -> at(arrayYPP_back, col*2+1, row*2   );
    cv *elem3 = arr_method -> methods -> at(arrayYPP_back, col*2,   row*2+1 );
    cv *elem4 = arr_method -> methods -> at(arrayYPP_back, col*2+1, row*2+1 );
    assert(elem1 && elem2 && elem3 && elem4);
    
    calculate_DCTtoCV(origin_elem, elem1, elem2, elem3, elem4);
    (void)arrayDCT;
}

/*  Name: packDCT
 *  Purpose: This function print out to stdout the bitpacked version of the
 *           input A2Array DCT. 
 *  Input: An already initialized array with DCT values; a pointer to function
 *         struct of chosen method.
 *  Input expectation: The parameters should not be NULL.
 *  Output: N/A. Print out the packed DCT data in form of chars to stdout. 
 *  Output expectation: N/A
 *  Error condition: CRE if any of the parameter is NULL.
 */
void packDCT(A2 arrayDCT, A2Methods_T methods)
{
    assert(arrayDCT != NULL && methods != NULL);
    fprintf(stdout,"COMP40 Compressed image format 2\n%u %u",
            methods -> width(arrayDCT) * 2, methods -> height(arrayDCT) * 2);
    fprintf(stdout,"\n");
    methods -> map_default(arrayDCT, printPackedDCT, NULL);
}

/*  Name: printPackedDCT
 *  Purpose: This function is the apply function for method's map function. 
 *           Specifically, it is applied in packDCT.
 *           It reads a element from the A2array of DCT, then pack the elements
 *           and print the packed Char/byte out to stdout.  
 *  Input: two integers for column and row, a pointer to A2array that stores
 *         DCT data, an void pointer that represent an element in arrayDCT.
 *         Closure pointer that is Null. 
 *  Input expectation: the parameters should not be NULL except cl.
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: N/A
 */
void printPackedDCT(int col, int row, A2 arrayDCT, void *elem, void *cl)
{
    assert(elem != NULL);
    DCT *element = elem;

    uint64_t packed = 0;
    /* pack the elements of DCT into the specified index*/
    packed = Bitpack_newu(packed, 4, 0,  element -> aveprQUANT);
    packed = Bitpack_newu(packed, 4, 4,  element -> avepbQUANT);
    packed = Bitpack_news(packed, 6, 8,  element -> d);
    packed = Bitpack_news(packed, 6, 14, element -> c);
    packed = Bitpack_news(packed, 6, 20, element -> b);
    packed = Bitpack_newu(packed, 6, 26, element -> a);
    
    /* print out the occupied bits byte by byte in forms of chars */
    for (int lsb = 24; lsb >= 0; lsb -= 8) {
        putchar(Bitpack_getu(packed, 8, lsb));
    }
    (void) col;
    (void) row;
    (void) arrayDCT;
    (void) cl;
    
}

/*  Name: unpackDCT
 *  Purpose: This function read from the file pointer's input and stores the
 *           decompressed DCT into the A2 arrayDCT.
 *  Input: An A2 DCT with empty values; a pointer to function
 *         struct of chosen method. And a pointer to the file of input.
 *  Input expectation: The parameters should not be NULL.
 *  Output: N/A.
 *  Output expectation: N/A
 *  Error condition: CRE if any of the parameter is NULL.
 */
void unpackDCT(A2 arrayDCT, FILE *input, A2Methods_T methods)
{

    assert(arrayDCT != NULL && input != NULL && methods != NULL);
    methods -> map_default(arrayDCT, readPackedDCT, input);
}


/*  Name: readPackedDCT
 *  Purpose: This function is the apply function for method's map function. 
 *           Specifically, it is applied in unpackDCT.
 *           It reads a element from the input, and then get out the elements
 *           and stores the values insiade the A2 arrayDCT.
 *  Input: two integers for column and row, a pointer to A2array that stores
 *         DCT data, an void pointer that represent an element in arrayDCT.
 *         Closure pointer that represent the input file pointer. 
 *  Input expectation: the parameters should not be NULL.
 *  Output: N/A
 *  Output expectation: N/A
 *  Error condition: CRE when the file is too short or incomplete, or argument
 *                   is NULL. 
 */
void readPackedDCT(int col, int row, A2 arrayDCT, void *elem, void *cl)
{
    assert(arrayDCT != NULL && elem != NULL && cl != NULL);
    FILE *input = cl;
    DCT *dest_elem = elem;

    /* convert codeword back to the original 64bit int */
    uint64_t unpacked_int = 0;
    for (int i = 24; i >= 0; i-= 8) {
        uint64_t character = getc(input);
        /* assume we are only using ascii from 0-256. character will be 
         * a very large unsigned int when getc returns -1/eof */
        assert(character < 256);
        unpacked_int = Bitpack_newu(unpacked_int, 8, i, character);
    }

    /* store DCT info into the DCT struct */
    dest_elem->aveprQUANT = Bitpack_getu(unpacked_int, 4, 0);
    dest_elem->avepbQUANT = Bitpack_getu(unpacked_int, 4, 4);
    dest_elem->d = Bitpack_gets(unpacked_int, 6, 8);
    dest_elem->c = Bitpack_gets(unpacked_int, 6, 14);
    dest_elem->b = Bitpack_gets(unpacked_int, 6, 20);
    dest_elem->a = Bitpack_getu(unpacked_int, 6, 26);
    
    (void) col;
    (void) row;
    (void) arrayDCT;
}