/*******************************************************************
* This Program tests Module STATS for basic statistical Functions  *
* ---------------------------------------------------------------- *
* SAMPLE RUN:                                                      *
*                                                                  *
*  Population mean and standard deviation:   0.156989    0.997428  *
*                                                                  *
*  Sample mean and standard deviation:   0.156989    1.002327      *
*                                                                  *
*  For full regression:                                            *
*               regression coefficients:  -1.351709    2.056521    *
*  standard error of estimate of y on x:   9.913653                *
*               correlation coefficient:   0.986076                *
*                                                                  *
*  For regression forced through (0,0):                            *
*               regression coefficients:   0.000000    2.029214    *
*  standard error of estimate of y on x:   9.945579                *
*               correlation coefficient:   0.985986                *
*                                                                  *
* ---------------------------------------------------------------- *
* Ref.: "Problem Solving with Fortran 90 By David R.Brooks,        *
*        Springer-Verlag New York, 1997".                          *
*                                                                  *
*                              C++ Release By J-P Moreau, Paris.   *
*                                      (www.jpmoreau.fr)           *
*******************************************************************/
// To link with stats.cpp

#include <stdio.h>
#include <math.h>
#include "stats.h"  // added for functions

  //see file stats.f90
  // void NormalStats(double *, int, char, double *, double *);  I already declared it in stats.h
  // void LinearReg(double *, double *, int, char, double *, 
  //	             double *, double *, double *);  I already declared it in stats.h
  // void NormalArray(double *, int);  I already declared it in stats.h


int main() {

  double x[100], y[100];
  double avg, std_dev,
  a, b,          //for linear regression y=ax+b
  s_yx,          //standard error of estimate of y on x
  corr;          //correlation coefficient
  int n,         //# of points in array
  i;

  FILE *fp;

//read array x from input text file
  fp = fopen("tstats.dat","r");
  fscanf(fp,"%d", &n);
  for (i=0; i<100; i++) fscanf(fp,"%lf", &x[i]); 
  fclose(fp);

// Test basic statistics
  NormalStats(x,n,'p',&avg,&std_dev);
  
  printf("\n Population mean and standard deviation: %10.6f  %10.6f\n", avg, std_dev);
  
  NormalStats(x,n,'s',&avg,&std_dev);       

  printf("\n Sample mean and standard deviation: %10.6f  %10.6f\n\n", avg, std_dev);

// Test linear regression
  NormalArray(y,n);

// Create a linear relationship with 'noise'
  for (i=0; i<n; i++) {
    x[i] = 1.0*i;
    y[i] = 2.0*i + 10.0*y[i];
  }
  
// Set a <> zero for full regression analysis
  a=1.0;
  LinearReg(x,y,n,'s',&a,&b,&s_yx,&corr);
  
  printf(" For full regression:\n");
  printf("              regression coefficients: %10.6f  %10.6f\n", a, b);
  printf(" standard error of estimate of y on x: %10.6f\n", s_yx);
  printf("              correlation coefficient: %10.6f\n\n", corr);  	    

// Set a = zero for full regression forced through (0,0)
  a=0.0;
  LinearReg(x,y,n,'s',&a,&b,&s_yx,&corr);
  
  printf(" For regression forced through (0,0):\n");
  printf("              regression coefficients: %10.6f  %10.6f\n", a, b);
  printf(" standard error of estimate of y on x: %10.6f\n", s_yx);
  printf("              correlation coefficient: %10.6f\n\n", corr);  

  return 0;
}

// end of file tsats.cpp
