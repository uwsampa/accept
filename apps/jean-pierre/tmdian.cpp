/************************************************************************
*    Calculate the median value of an array with the Heapsort method    *
* --------------------------------------------------------------------- *
* REFERENCE:                                                            *
*                                                                       *
*            "NUMERICAL RECIPES By W.H. Press, B.P. Flannery,           *
*             S.A. Teukolsky and W.T. Vetterling, Cambridge             *
*             University Press, 1986" [BIBLI 08].                       *
*                                                                       *
*                                           C++ version by J-P Moreau   *
*                                               (www.jpmoreau.fr)       *
* --------------------------------------------------------------------- *
* SAMPLE RUN:                                                           *
*                                                                       *
* Table to be sorted:                                                   *
*                                                                       *
*   1.25 563.59 193.30 808.74 585.01 479.87 350.29 895.96 822.84 746.60 *
* 174.11 858.94 710.50 513.53 303.99  14.98  91.40 364.45 147.31 165.90 *
* 988.53 445.69 119.08   4.67   8.91 377.88 531.66 571.18 601.76 607.17 *
* 166.23 663.05 450.79 352.12  57.04 607.68 783.32 802.61 519.88 301.95 *
* 875.97 726.68 955.90 925.72 539.35 142.34 462.08 235.33 862.24 209.60 *
* 779.66 843.65 996.80 999.69 611.50 392.44 266.21 297.28 840.14  23.74 *
* 375.87  92.62 677.21  56.22   8.79 918.79 275.89 272.90 587.91 691.18 *
* 837.61 726.49 484.94 205.36 743.74 468.46 457.96 949.16 744.44 108.28 *
*                                                                       *
* Median Value = 499.2371                                               *
*                                                                       *
* Sorted table (Heapsort method):                                       *
*                                                                       *
*   1.25   4.67   8.79   8.91  14.98  23.74  56.22  57.04  91.40  92.62 *
* 108.28 119.08 142.34 147.31 165.90 166.23 174.11 193.30 205.36 209.60 *
* 235.33 266.21 272.90 275.89 297.28 301.95 303.99 350.29 352.12 364.45 *
* 375.87 377.88 392.44 445.69 450.79 457.96 462.08 468.46 479.87 484.94 *
* 513.53 519.88 531.66 539.35 563.59 571.18 585.01 587.91 601.76 607.17 *
* 607.68 611.50 663.05 677.21 691.18 710.50 726.49 726.68 743.74 744.44 *
* 746.60 779.66 783.32 802.61 808.74 822.84 837.61 840.14 843.65 858.94 *
* 862.24 875.97 895.96 918.79 925.72 949.16 955.90 988.53 996.80 999.69 *
*                                                                       *
*                                                                       *
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define   SIZE   1024                  //maximum size of table


float   A[SIZE];                       //table to be sorted
float   AMED;                          //Median value
float   MAX_VALUE;                     //maximum value of table
float   temp;                          //auxiliary variable 
int     i,N;


/****************************************************
*  Sorts an array RA of length n in ascending order *
*                by the Heapsort method             *
* ------------------------------------------------- *
* INPUTS:                                           *
*	    n	  size of table RA                      *
*	    RA	  table to be sorted                    *
* OUTPUT:                                           *
*	    RA	  table sorted in ascending order       *
*                                                   *
* NOTE: The Heapsort method is a N Log2 N routine,  *
*       and can be used for very large arrays.      *
****************************************************/         
void HPSORT(int n, float *RA)  {
// Labels: e10, e20
  int i,ir,j,l;
  float rra;

  l=(n/2)+1;
  ir=n;
  //The index L will be decremented from its initial value during the
  //"hiring" (heap creation) phase. Once it reaches 1, the index IR 
  //will be decremented from its initial value down to 1 during the
  //"retirement-and-promotion" (heap selection) phase.
e10: 
  if (l > 1) {
    l--;
    rra=RA[l];
  }
  else {
    rra=RA[ir];
    RA[ir]=RA[1];
    ir--;
    if (ir==1) {
      RA[1]=rra;
      return;
    }
  }
  i=l;
  j=l+l;
e20: 
  if (j <= ir) {
    if (j < ir)  
      if (RA[j] < RA[j+1]) j++;
    if (rra < RA[j]) {
      RA[i]=RA[j];
      i=j; j=j+j;
    }
    else
      j=ir+1;
    goto e20;
  }
  RA[i]=rra;
  goto e10;

} //HPSORT()


//write table of size N to standard output
void TWRIT(int N, float *ARR)  {
  int i; float tmp;
  printf("\n");;
  for (i=1; i<N+1; i++) {
    tmp=ARR[i];
    printf("%7.2f",tmp);
    if ((i % 10)==0) printf("\n");
  }
}

/******************************************************
* Given an array X of N numbers, returns their median *
* value XMED. The array X is modified and returned    *
* sorted in ascending order.                          *
******************************************************/
void MDIAN(float *X, int N, float *XMED)  {
  int N2;
  HPSORT(N,X);
  N2=N / 2;
  if (2*N2 == N)
    *XMED = (float) (0.5*(X[N2]+X[N2+1]));
  else
    *XMED = X[N2+1];
}


int main()  {

  N=80;     //initialize size of table
  MAX_VALUE = 1000.0;

  //generate random table of numbers (between 0 and MAX_VALUE)
  for (i=1; i<N+1; i++)  {
    temp = (float) rand();
    A[i] = MAX_VALUE*(temp/RAND_MAX);
  }

  printf("\n Table to be sorted:\n");
  TWRIT(N,A);
 
  //call MDIAN subroutine
  MDIAN(A, N, &AMED);

  printf("\n Median Value = %8.4f\n", AMED);

  printf("\n Sorted table (Heapsort method):\n");
  TWRIT(N,A);

  printf("\n");

  return 0;
}

// end of file tmdian.cpp
