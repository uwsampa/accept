/***********************************************************
*  This program calculates the statistical moments of a    *
*  distribution: Mean, Variance, Skewness, etc.            *
* -------------------------------------------------------- *
* Ref.: "Numerical Recipes, by W.H. Press, B.P. Flannery,  *
*        S.A. Teukolsky and T. Vetterling, Cambridge       *
*        University Press, 1986".                          *
*                                                          *
*                       C++ Release By J-P Moreau, Paris.  *
* -------------------------------------------------------- *
* SAMPLE RUN:                                              *
*                                                          *
* Number of data: 5                                        *
*   1: 12                                                  *
*   2: 9                                                   *
*   3: 7                                                   *
*   4: 15                                                  *
*   5: 6                                                   *
*                                                          *
* Average ..........:  9.800000                            *
* Average  Deviation:  2.960000                            *
* Standard Deviation:  3.701351                            *
* Variance .........:  13.700000                           *
* Skewness .........:  0.291549                            *
* Kurtosis .........: -1.907799                            *
*                                                          *  
***********************************************************/
#include <stdio.h>
#include <math.h>

#define NMAX  1024


int i,ndata;
double Y[NMAX];
double average,avdev,stdev,variance,skewness,kurtosis;


/***********************************************************
* Given an array of Data of length n, this routine returns * 
* its mean ave, average deviation adev, standard deviation *
* sdev, variance var, skewness skew, and kurtosis curt.    *
***********************************************************/
void Moment(double *data, int n, double *ave, double *adev, double *sdev,
			double *var, double *skew, double *curt) {

  int j; double p,s;

  if (n <= 1) {
    printf(" N must be at least 2!\n");
    return;
  }
  s=0.0;
  for (j=1; j<=n; j++)   s += data[j];
// calculate mean
  *ave=s/n;
  *adev=0.0;
  *var=0.0;
  *skew=0.0;
  *curt=0.0;
  for (j=1; j<=n; j++) {
    s=data[j]-(*ave);
    *adev=*adev+fabs(s);
    p=s*s;
    *var=*var+p;
    p=p*s;
    *skew=*skew+p;
    p=p*s;
    *curt=*curt+p;
  }
  *adev=*adev/n;
  *var=*var/(n-1);
  *sdev=sqrt(*var);
  if (*var != 0.0) {
    *skew=*skew/(n*(*sdev)*(*sdev)*(*sdev));
    *curt=*curt/(n*(*var)*(*var))-3.0;
  }
  else
    printf(" No skew or kurtosis when zero variance.\n");
}


int main()  {

  printf("\n Number of data: "); scanf("%d", &ndata);

  for (i=1; i<=ndata; i++) {
    printf("  %d: ", i); scanf("%lf",&Y[i]);
  }

  Moment(Y,ndata,&average,&avdev,&stdev,&variance,&skewness,&kurtosis);

// print results
  printf("\n");
  printf(" Average ..........: %f\n", average);
  printf(" Average  Deviation: %f\n", avdev);
  printf(" Standard Deviation: %f\n", stdev);
  printf(" Variance .........: %f\n", variance);
  printf(" Skewness .........: %f\n", skewness);
  printf(" Kurtosis .........: %f\n", kurtosis);
  printf("\n");

  return 0;
}

// end of file tmoment.cpp
