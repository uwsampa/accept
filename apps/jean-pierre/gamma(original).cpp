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
#include <math.h>

#define  half 0.5
#define  one  1.0
#define  fpf  5.5
#define  zero 0.0 

double x, y;
int i;  


/******************************************
*           FUNCTION  GAMMA(X)            *
* --------------------------------------- *
* Returns the value of Gamma(x) in double *
* precision as EXP(LN(GAMMA(X))) for X>0. *
******************************************/
double Gamma(double xx)  {
  double cof[7],stp,x,tmp,ser;
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
  printf("\n      X        Gamma(X)   \n");
  printf(" -------------------------  \n");
  x=zero;
  for (i=1; i<11; i++) {
    x += half;
    y = Gamma(x);
    printf("   %f    %f\n", x, y);
  }
  printf("\n");

  return 0;
}

// end of file gamma.cpp
