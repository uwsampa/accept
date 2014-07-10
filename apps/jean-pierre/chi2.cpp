/*************************************************************************
*                  CHI2 AND INVERSE CHI2 FUNCTIONS                       *
* ---------------------------------------------------------------------- *
*  Reference:                                                            *
*                                                                        *
*  http://www.fourmilab.ch/rpkp/experiments/analysis/chiCalc.html        *
*                                                                        *
* The following JavaScript functions for calculating chi-square probabi- *
* lities and critical values were adapted by John Walker from C imple-   *
* mentations written by Gary Perlman of Wang Institute, Tyngsboro.       *
* Both the original C code and the JavaScript edition are in the public  *
* domain.                                                                *
* ---------------------------------------------------------------------- *
* SAMPLE RUN:                                                            *
*                                                                        *
*    CHI2 Law:                                                           *
*    ========                                                            *
*  Value of variable: 100                                                *
*  Degree of freedom: 100                                                *
*                                                                        *
*  Probability = 0.481192                                                *
*                                                                        *
*  Verify Inverse Law:                                                   *
*    X = 100.000000                                                      *
*                                                                        *
*                                   C++ Version By J-P Moreau, Paris.    *
*                                           (www.jpmoreau.fr)            *
*************************************************************************/
#include <stdio.h>
#include <math.h>


double poz (double z) {
/*-------------------------------------------------------------------------
  POZ  --  probability of standard normal z value

  Adapted from a polynomial approximation in:
  Ibbetson D, Algorithm 209
  Collected Algorithms of the CACM 1963 p. 616

  Note:
  This routine has six digits accuracy, so it is only useful for absolute
  z values < 6.  For z values >=  6.0, poz() returns 0.0.                 
--------------------------------------------------------------------------*/
    double Y, X, w, zmax;
    zmax = 6.0;
    if (z == 0.0)
      X = 0.0;
    else {
      Y = 0.5 * fabs(z);
      if (Y >= zmax * 0.5)
        X = 1.0;
      else if (Y < 1.0) {
        w = Y * Y;
        X = ((((((((0.000124818987*w -0.001075204047)*w + 0.005198775019)*w -0.019198292004)*w + 0.059054035642)*w
             -0.151968751364)*w + 0.319152932694) * w - 0.5319230073)*w + 0.7978845605929999)*Y*2;
      }
      else {
        Y = Y - 2;
        X = (((((((((((((-0.000045255659*Y + 0.00015252929)*Y -0.000019538132)*Y - 6.769049860000001E-04)*Y
             +0.001390604284)*Y-0.00079462082)*Y -0.002034254874)*Y + 0.006549791214)*Y - 0.010557625006)*Y
             +0.011630447319)*Y-9.279453341000001E-03)*Y + 0.005353579108)*Y - 0.002141268741)*Y
             +0.000535310849)*Y + 0.999936657524;
      }
    }
    if (z > 0.0)
      return ((X + 1) * 0.5);
    else
      return ((1 - X) * 0.5);
    
}


double chi2 (double X, int df) {
/*-------------------------------------------------------- 
 Adapted From:
 Hill, I. D. and Pike, M. C. Algorithm 299
 Collected Algorithms for the CACM 1967 p. 243
 Updated for rounding errors based on remark in
 ACM TOMS June 1985, page 185   
---------------------------------------------------------*/
    bool even;  //parity of df
    double a,bigx,c,e,lnpi, ipi,s,Y,z,PI;
	PI = 4.0*atan(1.0);
    bigx = 200.0;
    lnpi = log(sqrt(PI));
    ipi = 1.0 / log(PI);
    if (X <= 0.0 || df < 1)  return 1.0;
    a = 0.5 * X;
    if (2*(df/2) == df)
      even = true;
    else
      even = false;
    if (df > 1)  Y = exp(-a);
    if (even)
      s = Y;
    else
      s = 2.0 * poz(-sqrt((X)));
    if (df > 2) {
        X = 0.5 * (df - 1);
        if (even)
          z = 1.0;
        else
          z = 0.5;
        if (a > bigx) {
            if (even)
              e = 0.0;
            else
              e = lnpi;
            c = log(a);
            while (z <= X) {
              e = log(z) + e;
              s = s + exp(c * z - a - e);
              z = z + 1.0;
            }
            return s;
        }
        else {
            if (even)
              e = 1.0;
            else
              e = ipi / sqrt(a);
            c = 0.0;
            while (z <= X) {
              e = e * (a / z);
              c = c + e;
              z = z + 1.0;
            }
            return (c * Y + s);
        }
    }
    else
      return s;

}

double invchi2 (double P, int idf) {
    double chimax,chisqval,epsilon,minchisq,maxchisq;
    epsilon = 1e-6;
    chimax  = 1e6;
    minchisq = 0.0;
    maxchisq = chimax;
    if (P <= 0.0)
      chisqval = maxchisq;
    else
      if (P >= 1.0) chisqval = 0.0;
    chisqval = (1.0 * idf) / sqrt(P);
    while (fabs(maxchisq - minchisq) > epsilon) {
      if (chi2(chisqval, idf) < P)
		maxchisq = chisqval;
      else
        minchisq = chisqval;
      chisqval = (maxchisq + minchisq) * 0.5;
    }
    return chisqval;
}


int main() {

  double P,X,Y;
  int idf;

  printf("\n  CHI2 Law:\n");
  printf("  ========\n");
  printf(" Degree of freedom: "); scanf("%d", &idf);
  printf(" Value of variable: "); scanf("%lf", &X);
  P = chi2(X, idf);
  printf("\n Probability = %f\n\n", P);
  printf(" Verify Inverse Law:\n");
  Y = invchi2(P, idf);
  printf("   X = %f\n\n", Y);

  return 0;
}

// end of file chi2.cpp
