/*****************************************************
*        Computing the statistical functions         *
*             for one or two variables               *
*                                                    *
* -------------------------------------------------- *
* REFERENCE:  "Mathematiques et statistiques by H.   *
*              Haut, PSI Editions, France, 1981"     *
*              [BIBLI 13].                           *
*                                                    *
*                 C++ version by J-P Moreau, Paris   *
*                        (www.jpmoreau.fr)           *
* -------------------------------------------------- *
* SAMPLE RUN:                                        *
*                                                    *
* TUTORIAL                                           *
*                                                    *
* 1. Define type of calculus:"                       *
*    1: Statistical functions for a set X(i)         *
*    2: Statistical functions for a set X(i), Y(i)   *
*                                                    *
* 2. Input number n of data                          *
*                                                    *
* 3. Input successively the n values X(i) [and Y(i)] *
*                                                    *
* Type of calculus (1 or 2): 2                       *
*                                                    *
* Number of data: 5                                  *
*                                                    *
*   1  1 12                                          *
*   2  2 9                                           *
*   3  3 7                                           *
*   4  4 15                                          *
*   5  5 6                                           *
*                                                    * 
*                                                    *
* Mean of X(i)....................: 3.00000000       *
*                                                    *
* (n-1) standard deviation of X(i): 1.58113883       *
*   (n) standard deviation of X(i): 1.41421356       *
*                                                    *
* (n-1) standard dev. of X mean...: 0.70710678       *
*   (n) standard dev. of X mean...: 0.63245553       *
*                                                    *
* Mean of Y(i)....................: 9.80000000       *
*                                                    *
* (n-1) standard deviation of Y(i): 3.70135111       *
*   (n) standard deviation of Y(i): 3.31058908       *
*                                                    *
* (n-1) standard dev. of Y mean...: 1.65529454       *
*   (n) standard dev. of Y mean...: 1.48054045       *
*                                                    *
*                                                    *
* (n-1) covariance of X,Y.........: -1.50000000      *
*   (n) covariance of X,Y.........: -1.20000000      *
*                                                    *
* Correlation coefficient.........: -0.25630730      *
*                                                    *
******************************************************
 Description: This program computes the usual statistical functions
              for a set of n variables X(i) or for a set of n pairs
              X(i), Y(i).   */
#include <stdio.h>
#include <math.h>

#define  SIZE  100

double  X[SIZE], Y[SIZE];
double  a3,a4,a5,a6,a7,tmp,tmp1;
double  v1,v2,v3,v4,v5,v6,v7,v8;
int     i, n, nt;


/****************************************************
*       Subroutine of statistical functions         *
*              for 1 or 2 variables                 *
* ------------------------------------------------- *
* INPUTS:                                           *
*         nt: type of calculus =1 for one variable  *
*                              =2 for two variables *
*          n: number of data X(i) [and Y(i) ]       *
* OUTPUTS:                                          *
*         v1: mean of X(i)                          *
*         v2: mean of Y(i)                          *
*         v3: (n-1) standard deviation of v1        *
*         v4: (n-1) standard deviation of v2        *
*         v5:   (n) standard deviation of v1        *
*         v6:   (n) standard deviation of v2        *
*         v7: (n-1) covariance of X,Y               *
*         v8: correlation coefficient               *
*         a3: (n-1) standard deviation of X         *
*         a4: (n-1) standard deviation of Y         *
*         a5:   (n) standard deviation of X         *
*         a6:   (n) standard deviation of Y         *
*         a7:   (n) covariance of X,Y               *
****************************************************/
void Stat_functions()  {
//Label: e100
  int i,n1;
  double xnr;
  v1=0.0; v2=0.0; v3=0.0; v4=0.0; v5=0.0;
  n1=n-1; xnr=sqrt(n);
  //choose type of calculus
  if (nt==2) goto e100;
  //case of one set X(i)
  for (i=1; i<n+1; i++) {
    v1=v1+X[i];
    v3=v3+X[i]*X[i];
  }
  v6=v3-v1*v1/n;
  v1=v1/n;
  a3=sqrt(v6/n1); a5=sqrt(v6/n);
  v5=a5/xnr; v3=a3/xnr;
  return;
e100: //case of a set X(i), Y(i)
  for (i=1; i<n+1; i++) {
    v1=v1+X[i];
    v2=v2+Y[i];
    v3=v3+X[i]*X[i];
    v4=v4+Y[i]*Y[i];
    v5=v5+X[i]*Y[i];
  }
  v6=v3-v1*v1/n;
  v7=v4-v2*v2/n;
  v8=v5-v1*v2/n;
  v1=v1/n; v2=v2/n;
  a3=sqrt(v6/n1); a4=sqrt(v7/n1);
  a5=sqrt(v6/n); a6=sqrt(v7/n);
  v7=v8/n1; a7=v8/n;
  v3=a3/xnr; v4=a4/xnr;
  v5=a5/xnr; v6=a6/xnr;

  if (a3==0 || a4==0) return;  //correlation coefficient not defined
  v8=v8/(n*a5*a6);
}


int main()  {

  printf("\n TUTORIAL\n\n");
  printf(" 1. Define type of calculus:\n");
  printf("    1: Statistical functions for a set X(i)\n");
  printf("    2: Statistical functions for a set X(i), Y(i)\n\n");
  printf(" 2. Input number n of data\n\n");
  printf(" 3. Input successively the n values X(i) [and Y(i)]\n\n");
  printf(" Type of calculus (1 or 2): "); scanf("%d",&nt);
  printf("\n Number of data: "); scanf("%d",&n);
  printf("\n");
  //read n data
  for (i=1; i<n+1; i++) {
    printf("  %d  ",i);
    if (nt==1) scanf("%lf",&tmp); X[i]=tmp;
    if (nt==2) scanf("%lf %lf",&tmp,&tmp1);
	X[i]=tmp, Y[i]=tmp1;
  }

  //call stat subroutine
  Stat_functions();

  //printf; results
  printf("\n Mean of X(i)....................: %12.8f\n\n", v1);
  printf(" (n-1) standard deviation of X(i): %12.8f\n", a3);
  printf("   (n) standard deviation of X(i): %12.8f\n\n", a5);
  printf(" (n-1) standard dev. of X mean...: %12.8f\n", v3);
  printf("   (n) standard dev. of X mean...: %12.8f\n\n", v5);
  if (nt==1) return 1;
  printf(" Mean of Y(i)....................: %12.8f\n\n", v2);
  printf(" (n-1) standard deviation of Y(i): %12.8f\n", a4);
  printf("   (n) standard deviation of Y(i): %12.8f\n\n", a6);
  printf(" (n-1) standard dev. of Y mean...: %12.8f\n", v4);
  printf("   (n) standard dev. of Y mean...: %12.8f\n\n\n", v6);
  printf(" (n-1) covariance of X,Y.........: %12.8f\n", v7);
  printf("   (n) covariance of X,Y.........: %12.8f\n\n", a7);
  printf(" Correlation coefficient.........: %12.8f\n\n", v8);

  return 0;
}

// end of file fstat.cpp
