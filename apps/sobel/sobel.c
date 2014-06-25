/* sobel.c */
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "mypgm.h"
#include <enerc.h>

void sobel_filtering( )
     /* Spatial filtering of image data */
     /* Sobel filter (horizontal differentiation */
     /* Input: image1[y][x] ---- Outout: image2[y][x] */
{
  /* Definition of Sobel filter in horizontal direction */
  int weight[3][3] = {{ -1,  0,  1 },
		      { -2,  0,  2 },
		      { -1,  0,  1 }};
  double pixel_value;
  double min, max;
  int x, y, i, j;  /* Loop variable */
  
  /* Maximum values calculation after filtering*/
  printf("Now, filtering of input image is performed\n\n");
  min = DBL_MAX;
  max = -DBL_MAX;
  for (y = 1; y < y_size1 - 1; y++) {
    for (x = 1; x < x_size1 - 1; x++) {
      pixel_value = 0.0;
      for (j = -1; j <= 1; j++) {
	    for (i = -1; i <= 1; i++) {
	      pixel_value += weight[j + 1][i + 1] * image1[y + j][x + i];
	    }
      }
      if (pixel_value < min) min = pixel_value;
      if (pixel_value > max) max = pixel_value;
    }
  }
  if ((int)(max - min) == 0) {
    printf("Nothing exists!!!\n\n");
    exit(1);
  }

  /* New loop variables APPROX */
  APPROX int xa;
  APPROX int ya;
  /* New pixel_value APPROX */
  APPROX double pixel_value_app;
  /* Initialization of image2[y][x] */
  x_size2 = x_size1;
  y_size2 = y_size1;
  for (ya = 0; ENDORSE(ya < y_size2); ya++) {
    for (xa = 0; ENDORSE(xa < x_size2); xa++) {
      image2[ya][xa] = 0;
    }
  }
  /* Generation of image2 after linear transformtion */
  for (ya = 1; ENDORSE(ya < y_size1 - 1); ya++) {
    for (xa = 1; ENDORSE(xa < x_size1 - 1); xa++) {
      pixel_value_app = 0.0;
      for (j = -1; j <= 1; j++) {
	    for (i = -1; i <= 1; i++) {
	      pixel_value_app += weight[j + 1][i + 1] * image1[ya + j][xa + i];
	    }
      }
      pixel_value_app = MAX_BRIGHTNESS * (pixel_value_app - min) / (max - min);
      image2[ya][xa] = (unsigned char)pixel_value_app;
    }
  }
}

int main(int argc, const char** argv)
{
  load_image_data(argv[1]);   /* Input of image1 */ 
  accept_roi_begin();
  sobel_filtering( );   /* Sobel filter is applied to image1 */
  accept_roi_end();
  save_image_data( );   /* Output of image2 */
  return 0;
}

