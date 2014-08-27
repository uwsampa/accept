/********************************************
* Program to demonstrate the Gamma Function *
*                                           *
*               C++ version by J-P Moreau   *
*                   (www.jpmoreau.fr)       *
* ----------------------------------------- *
* Reference:                                * 
* "Numerical Recipes, By W.H. Press, B.P.   *
*  Flannery, S.A. Teukolsky and T. Vetter-  *
*  ling, Cambridge University Press, 1986"  *
*  [BIBLI 08].                              *
* ----------------------------------------- *
* SAMPLE RUN:                               *
*                                           *
*       X        Gamma(X)                   *   
*  -------------------------                *
*    0.500000    1.772454                   *
*    1.000000    1.000000                   *
*    1.500000    0.886227                   *
*    2.000000    1.000000                   *
*    2.500000    1.329340                   *
*    3.000000    2.000000                   *
*    3.500000    3.323351                   *
*    4.000000    6.000000                   *
*    4.500000    11.631728                  *
*    5.000000    24.000000                  *
*                                           *
********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../../../include/enerc.h"

#define  half 0.5
#define  increment 0.0000005
#define  one  1.0
#define  fpf  5.5
#define  zero 0.0
#define  number 1000000

APPROX double x, y;
int i;  


/******************************************
*           FUNCTION  GAMMA(X)            *
* --------------------------------------- *
* Returns the value of Gamma(x) in double *
* precision as EXP(LN(GAMMA(X))) for X>0. *
******************************************/
APPROX double Gamma(APPROX double xx)  {
  APPROX double cof[7],stp,x,tmp,ser;
  int j;
  cof[1]=76.18009173;
  cof[2]=-86.50532033;
  cof[3]=24.01409822;
  cof[4]=-1.231739516;
  cof[5]=0.120858003e-2;
  cof[6]=-0.536382e-5;
  stp=2.50662827465;
  
  x=xx-one;
  tmp=x+fpf;
  tmp=(x+half)*log(tmp)-tmp;
  ser=one;
  for (j=1; j<7; j++) {
    x=x+one;
    ser=ser+cof[j]/x;
  }
  return (exp(tmp+log(stp*ser)));
}

int main()  {
  FILE *file;
  char *outputfile = "my_output.txt";
  int retval;
  APPROX double xval[number];
  APPROX double yval[number];

  accept_roi_begin();
  x = zero;
  for (i = 0; i < number; i++) {
    x += increment;
    y = Gamma(x);
    xval[i] = x;
    yval[i] = y;
  }
  accept_roi_end();

  file = fopen(outputfile, "w");
  if (file == NULL) {
    perror("fopen for write failed\n");
    return EXIT_FAILURE;
  }
  for (i = 0; i < number; i++) {
    retval = fprintf(file, "%.10f\n", ENDORSE(xval[i]));
    if (retval < 0) {
      perror("fprintf of x-value failed\n");
      return EXIT_FAILURE;
    }
    retval = fprintf(file, "%.10f\n", ENDORSE(yval[i]));
    if (retval < 0) {
      perror("fprintf of y-value failed\n");
      return EXIT_FAILURE;
    }
  }
  retval = fclose(file);
  if (retval != 0) {
    perror("fclose failed\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

// end of file gamma.cpp
