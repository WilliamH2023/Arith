#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "pnm.h"
#include "a2methods.h"
#include "a2plain.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

FILE** checkArgs(int argc, char *argv[]);
void checkDimension(Pnm_ppm* image);
double Calc_difference(A2Methods_T methods, Pnm_ppm* image);

int main(int argc, char *argv[]) {
    FILE** files = checkArgs(argc, argv);
    assert(files[0] != NULL && files[1] != NULL);

    /* default to UArray2 methods */
    A2Methods_T methods = uarray2_methods_plain; 
    assert(methods);
    (void) methods;
    /* default to best map */
    A2Methods_mapfun *map = methods->map_default; 
    assert(map);
    (void) map;
    Pnm_ppm image[] = { Pnm_ppmread(files[0], methods), 
                        Pnm_ppmread(files[1], methods) };
    checkDimension(image);
    double diff = Calc_difference(methods, image);
    
    
    
    Pnm_ppmfree(&(image[0]));
    Pnm_ppmfree(&(image[1]));
    
    fclose(files[0]);
    fclose(files[1]);
    free(files);
    
    if (diff > 1){
        fprintf(stderr, "1.0\n");
        exit(EXIT_FAILURE);
    }
    
    printf("%.4f\n", diff);
    return 0;
}

FILE** checkArgs(int argc, char *argv[])
{
    FILE **inputfp = malloc(2*sizeof(FILE *));
    if (argc != 3) {
        fprintf(stderr, "ERROR: Need 2 Arguments!\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[1], "-") == 0) {
        inputfp[0] = stdin;
        inputfp[1] = fopen(argv[2], "r");
    }
    else if (strcmp(argv[2], "-") == 0) {
        inputfp[0] = fopen(argv[1], "r");
        inputfp[1] = stdin;
    }
    else{
        inputfp[0] = fopen(argv[1], "r");
        inputfp[1] = fopen(argv[2], "r");
    }
    for (int i = 0; i < 2; i++) {
        if (inputfp[i] == NULL || ferror(inputfp[i])) {
            fprintf(stderr, "ERROR: Failure to open file NO.%d!\n", i);
            exit(EXIT_FAILURE);
        }    
    }
    return inputfp;
}

void checkDimension(Pnm_ppm* image){
    int width[] = { image[0] -> width, image[1] -> width }; 
    int height[] = { image[0] -> height, image[1] -> height };
    if ( abs(width[0] - width[1]) > 1 ) {
        fprintf(stderr, "ERROR: Difference in width is more than 1!\n");
        exit(EXIT_FAILURE);
    }
    if ( abs(height[0] - height[1]) > 1 ) {
        fprintf(stderr, "ERROR: Difference in height is more than 1!\n");
        exit(EXIT_FAILURE);
    }
}

double Calc_difference(A2Methods_T methods, Pnm_ppm* image){
    double sum = 0;
    int width = MIN(image[0] -> width, image[1] -> width); 
    int height = MIN(image[0] -> height, image[1] -> height);
    int deno = MIN(image[0] -> denominator, image[1] -> denominator);
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            Pnm_rgb pixel0 = methods -> at(image[0]->pixels, j ,i);
            Pnm_rgb pixel1 = methods -> at(image[1]->pixels, j ,i);
            
            
            int red_diff = (int)pixel0 -> red - (int)pixel1->red;
            int green_diff = (int)pixel0 -> green - (int)pixel1 -> green;
            int blue_diff = (int)pixel0 -> blue - (int)pixel1 -> blue;
            double red_diff_value = (double)red_diff/(double)deno;
            double green_diff_value = (double)green_diff/(double)deno;
            double blue_diff_value = (double)blue_diff/(double)deno;
            
            double squareOfdiff = red_diff_value*red_diff_value + green_diff_value*green_diff_value + blue_diff_value*blue_diff_value;

            sum += squareOfdiff;

    
        }
    }

    int dividor = 3 * width * height;

    double afterdivision = (double)sum / (double)dividor;

    double diff = sqrt(afterdivision);
    return diff;
}

