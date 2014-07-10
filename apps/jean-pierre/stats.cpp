/*****************************************
* Module for basic statistical Functions *
*****************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stats.h"

double Dot_Product(int n, double *x, double *y) {
  int i; double temp;
  temp=0.0;
  for (i=0; i<n; i++)  temp += x[i]*y[i];
  return temp;
}

double Sum(int n, double *x) {
  int i; double temp;
  temp=0.0;
  for (i=0; i<n; i++)  temp += x[i];
  return temp;
}

double Sqr(double x) {
  return x*x;
}

/*--------------------------------------------------------
! Basic description statistics for normally distributed
! data. Calculates either sample or population statistics.
! Sets std_dev to -1 if error condition is detected.
!-------------------------------------------------------*/
  void NormalStats(double *a, int n, char flag, double *avg,
	               double *std_dev) {
    double sum_, sum_sq, variance;

    sum_ = Sum(n,a);
    sum_sq = Dot_Product(n,a,a);

    if (flag=='p' || flag=='P')
      variance = (sum_sq-Sqr(sum_)/(1.0*n))/(1.0*n);
    else if (flag=='s' || flag=='S')
      variance = (sum_sq-Sqr(sum_)/(1.0*(n-1)))/(1.0*(n-1));
    else {
      printf(" From NormalStats: Flag Error, <P> assumed.\n");
      variance = (sum_sq-Sqr(sum_)/(1.0*n))/(1.0*n);
    }
	
    if (variance < 0.0) {  // an error exists
      printf(" From NormalStats: negative variance %f\n", variance);
      *std_dev = -1.0;
    }
    else
      *std_dev = sqrt(variance);

    *avg = sum_/n;

  } //NormalStats
	  	 
/*---------------------------------------------------------
! For data to be represented by y=ax+b, calculates linear 
! regression coefficients, sample standard error of y on x,
! and sample correlation coefficients. Sets r=0 if an error
! exists. If the intercept coefficient a is set to 0 on
! input, the regression is forced through (0,0).
!--------------------------------------------------------*/
  void LinearReg(double *x, double *y, int n, char flag,
	             double *a, double *b, double *s_yx, double *r) {
    double avg, std_dev;
    double sum_x,sum_y,sum_xy,sum_xx,sum_yy,temp;

    sum_x = Sum(n,x);
    sum_y = Sum(n,y);
    sum_xy = Dot_Product(n,x,y);
    sum_xx = Dot_Product(n,x,x);
    sum_yy = Dot_Product(n,y,y);

    if (*a != 0.0) {  // calculate full expression
      temp = n*sum_xx - Sqr(sum_x);
      *a = (sum_y*sum_xx - sum_x*sum_xy)/temp;
      *b = (n*sum_xy - sum_x*sum_y)/temp;
      *s_yx = sqrt((sum_yy - (*a)*sum_y - (*b)*sum_xy)/n);
    }
    else {           //just calculate slope
      *b = sum_y/sum_x;
      *s_yx = sqrt((sum_yy - 2.0*(*b)*sum_xy + (*b)*(*b)*sum_xx)/n);
    }

    if (flag=='s' || flag=='S')
      *s_yx = *s_yx * sqrt((1.0*n)/(1.0*(n-2)));
	
// Use NormalStats to get standard deviation of y
    NormalStats(y,n,flag,&avg,&std_dev);
	
    if (std_dev > 0.0) {
      temp = 1.0 - Sqr(*s_yx/std_dev);
      if (temp > 0.0)
        *r = sqrt(temp);
      else {  // an error exists
        *r = 0.0;
        printf(" From LinearReg: error in temp %f\n", temp);
      }
    }
    else   // an error exists
      *r = 0.0;
	
  } // LinearReg 	        		 

/*----------------------------------------------------------
! Generates an array of normal random numbers from pairs of
! uniform random numbers in range [0,1].
!---------------------------------------------------------*/
  void NormalArray(double *a, int n) {
    int i; double pi, temp, u1,u2;

	double MAX_VALUE = 1.0;

    pi = 3.1415926535;

// fills array with uniform random
    for (i=0; i<n; i++) {
      temp = (double) rand();
      a[i] = MAX_VALUE*(temp/RAND_MAX);      
    }

    i=0;
    while (i < n) {
      u1 = a[i];
      u2 = a[i+1];
      if (u1==0.0)  u1 = 1e-15;  // u must not be zero
      if (u2==0.0)  u2 = 1e-15;
      a[i] = sqrt(-2.0*log(u1))*cos(2.0*pi*u2);
      a[i+1] = sqrt(-2.0*log(u2))*sin(2.0*pi*u2);
      i += 2;
    }
	
    if ((n % 2) != 0) {  // there is one extra element
      if (a[n] == 0.0) a[n] = 1e-15;  	   
      a[n] = sqrt(-2.0*log(a[n]))*sin(2.0*pi*a[n]);
    }

  } // NormalArray


// end of file stats.cpp
