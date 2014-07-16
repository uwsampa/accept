/*
 * rgb_image.c
 *
 *  Created on: May 1, 2012
 *      Author: Hadi Esmaeilzadeh <hadianeh@cs.washington.edu>
 */

#include "rgb_image.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SRC_IMAGE_ADDR 0x08000000

void initRgbImage(APPROX RgbImage* image) {
    image->w = 0;
    image->h = 0;
    image->pixels = NULL;
    image->meta = NULL;
}

int readCell(char **fp, char* w) {
    char c;
    int i;

    w[0] = 0;
    // HACK: '%' character stands for EOF
    for (c = **fp, (*fp)++, i = 0; c != '%'; c = **fp, (*fp)++) {
        if (c == ' ' || c == '\t') {
            if (w[0] != '\"')
                continue;
        }

        if (c == ',' || c == '\n') {
            if (w[0] != '\"')
                break;
            else if (c == '\"') {
                w[i] = c;
                i++;
                break;
            }
        }

        w[i] = c;
        i++;
    }
    w[i] = 0;

    return c;
}

int loadRgbImage(const char* fileName, RgbImage* image) {
    int c;
    int i;
    int j;
    char w[256];
    APPROX RgbPixel** pixels;
    char *fp;

#if DEBUG_MODE==1
    printf("Loading %s ...\n", fileName);
#endif //DEBUG_MODE

    // the rgb file has to be loaded in this specific address in memory
    fp = (char*) SRC_IMAGE_ADDR;

    c = readCell(&fp, w);
    image->w = atoi(w);
    c = readCell(&fp, w);
    image->h = atoi(w);

#if DEBUG_MODE==1
    printf("%d x %d\n", image->w, image->h);
#endif //DEBUG_MODE

    pixels = (RgbPixel**)malloc(image->h * sizeof(RgbPixel*));

    if (pixels == NULL) {
        printf("Warning: Oops! Cannot allocate memory for the pixels!\n");
        
        return 0;
    }

    c = 0;
    for(i = 0; i < image->h; i++) {
        pixels[i] = (RgbPixel*)malloc(image->w * sizeof(RgbPixel));
        if (pixels[i] == NULL) {
            c = 1;
            break;
        }
    }

    if (c == 1) {
        printf("Warning: Oops! Cannot allocate memory for the pixels!\n");
        
        for (i--; i >= 0; i--)
            free(pixels[i]);
        free(pixels);


        return 0;
    }

    for(i = 0; i < image->h; i++) {
        for(j = 0; j < image->w; j++) {
            c = readCell(&fp, w);
            pixels[i][j].r = atoi(w);

            c = readCell(&fp, w);
            pixels[i][j].g = atoi(w);

            c = readCell(&fp, w);
            pixels[i][j].b = atoi(w);
        }
    }
    image->pixels = pixels;

    c = readCell(&fp, w);
    image->meta = (char*)malloc(strlen(w) * sizeof(char));
    if(image->meta == NULL) {
        printf("Warning: Oops! Cannot allocate memory for the pixels!\n");

        for (i = 0; i < image->h; i++)
            free(pixels[i]);
        free(pixels);

        return 0;

    }
    strcpy(image->meta, w);

#if DEBUG_MODE==1
    printf("%s\n", image->meta);
#endif //DEBUG_MODE

    return 1;
}

/*int saveRgbImage(RgbImage* image, const char* fileName, float scale) {
    int i;
    int j;
    FILE *fp;

    printf("Saving %s ...\n", fileName);

    fp = fopen(fileName, "w");
    if (!fp) {
        printf("Warning: Oops! Cannot open %s!\n", fileName);
        return 0;
    }

    fprintf(fp, "%d,%d\n", image->w, image->h);
    printf("%d,%d\n", image->w, image->h);

    for(i = 0; i < image->h; i++) {
        for(j = 0; j < (image->w - 1); j++) {
            fprintf(fp, "%d,%d,%d,", int(image->pixels[i][j].r * scale), int(image->pixels[i][j].g * scale), int(image->pixels[i][j].b * scale));
        }
        fprintf(fp, "%d,%d,%d\n", int(image->pixels[i][j].r * scale), int(image->pixels[i][j].g * scale), int(image->pixels[i][j].b * scale));
    }

    fprintf(fp, "%s", image->meta);
    printf("%s\n", image->meta);

    fclose(fp);

    return 1;
}*/

void freeRgbImage(RgbImage* image) {
    int i;

    if (image->meta != NULL)
        free(image->meta);

    if (image->pixels == NULL)
        return;

    for (i = 0; i < image->h; i++)
        if (image->pixels != NULL)
            free(image->pixels[i]);

    free(image->pixels);
}

void makeGrayscale(RgbImage* image) {
    int i;
    int j;
    float luminance;

    float rC = 0.30 / 256;
    float gC = 0.59 / 256;
    float bC = 0.11 / 256;

    for(i = 0; i < image->h; i++) {
        for(j = 0; j < image->w; j++) {
            luminance =
                rC * ENDORSE(image->pixels[i][j].r) +
                gC * ENDORSE(image->pixels[i][j].g) +
                bC * ENDORSE(image->pixels[i][j].b);

            image->pixels[i][j].r = luminance;
            image->pixels[i][j].g = luminance;
            image->pixels[i][j].b = luminance;
        }
    }
}
