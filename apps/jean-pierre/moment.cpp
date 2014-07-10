/*****************************************************
*         Computing the means and moments            *
*           of a statistical variable                *
*                                                    *
* -------------------------------------------------- *
* REFERENCE:  "Mathematiques et statistiques By H.   *
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
* Means and moments of a statistical variable X(i)   *
* with frequency F(i)                                *
*                                                    *
* 1. Input number n of data                          *
*                                                    *
* 2. Input successively the n values X(i) and F(i)   *
*                                                    *
* Number of data: 3                                  *
*                                                    *
*   1  1 6                                           *
*   2  3 4                                           *
*   3  5 8                                           *
*                                                    *
* Calculate generalized mean for t= 2                *
*                                                    *
*                                                    *
* Arithmetic mean:  3.22222222                       *
* Geometric mean :  2.61023904                       *
* Harmonic mean  :  2.01492537                       *
*                                                    *
* Moments:                                           *
*                                                    *
*   M1=    3.22222222                                *
*   M2=    3.06172840                                *
*   M3=   -1.16323733                                *
*   M4=   12.56881570                                *
*                                                    *
* Flatness coefficient....:   1.34079084             *
* Coefficient of asymmetry:  -0.21712925             *
*                                                    *
* Gen. mean M( 2): 3.66666667                        *
*                                                    *
*****************************************************/
#include <stdio.h>
#include <math.h>

#define  SIZE  101

int    i,ite,n;
double F[SIZE],X[SIZE];
double a,ap,asy,g,h,t,xm1,xm2,xm3,xm4,xmt;
double tmpx,tmpf;


/****************************************************
*  Function to calculate the means and moments of a *
*            statistical variable X[i]              *
* ------------------------------------------------- *
* INPUTS:                                           *
*          n: number of data X[i] and F[i]          *
*          X: n values X[i]                         *
*          F: n values F[i], frequency of X[i]      *
*          t: coefficient of generalized mean M(t)  *
*             to calculate                          *
* OUTPUTS:                                          *
*          a: arithmetic mean of X[i]               *
*          g: geometric mean of X[i]                *
*          h: harmonic mean of X[i]                 *
*        xm1: moment of first order                 *
*        xm2: moment of second order                *
*        xm3: moment of third order                 *
*        xm4: moment of fourth order                *
*         ap: flatness coefficient                  *
*        asy: coefficient of assymetry              *
*        xmt: generalized mean M(t)                 *
*        ite: test flag (if te=1, harmonic mean h   *
*             is not defined                        *
****************************************************/
void Calculate() {
//Label: e100
   double v1,v2,v3,v4,v5,v6,v7,vt,xm;
   int i;

  ite=0;
  v1=0.0; v2=0.0; v3=0.0; v4=0.0;
  v5=1.0; v6=0.0; v7=0.0; xm=0.0;
  // Calculate necessary sums
  for (i=1; i<n+1; i++) {
    vt=F[i]*X[i];
    v1=v1+vt;
    v2=v2+vt*X[i];
    v3=v3+vt*X[i]*X[i];
    v4=v4+vt*X[i]*X[i]*X[i];
    xm=xm+F[i];
    v7=v7+F[i]*pow(X[i],t);
    // test for a X[i]=0
    if (X[i]==0.0) {
      ite=1;
      goto e100;
    }
    // if one X[i]=0, the harmonic mean is undefined
    // and the geometric mean= 0
    if (ite==1) goto e100;
    v5=v5*pow(X[i],F[i]);
    v6=v6+F[i]/X[i];
e100: ; }
  // prepare outputs
  a=v1/xm;
  xmt=pow((v7/xm),(1.0/t));
  xm1=a;
  xm2=(v2-2.0*a*v1+a*a*xm)/xm;
  xm3=(v3-3.0*a*v2+3.0*a*a*v1-xm*pow(a,3))/xm;
  xm4=(v4-4.0*a*v3+6.0*a*a*v2-4.0*v1*pow(a,3)+xm*pow(a,4))/xm;
  ap=xm4/xm2/xm2;
  asy=xm3/pow(xm2,1.5);
  if (ite==1)  { // one X[i]=0
    g=0.0;
    return;
  }
  g=pow(v5,(1.0/xm));
  h=xm/v6;
}


int main()  {

  printf("\n TUTORIAL\n\n");
  printf(" Means and moments of a statistical variable X(i)\n");
  printf(" with frequency F(i) \n\n");
  printf(" 1. Input number n of data\n\n");
  printf(" 2. Input sucessively the n values X(i) and F(i)\n\n");
  printf(" Number of data: "); scanf("%d",&n);
  printf("\n");

  for (i=1; i<n+1; i++) {
    printf("  %d  ", i); scanf("%lf %lf",&tmpx,&tmpf);
	X[i] = tmpx; F[i] = tmpf;
  }
  printf("\n");
  printf(" Calculate generalized mean for t= "); scanf("%lf",&t);
  printf("\n");

  Calculate();

  // print results
  printf("\n Arithmetic mean: %13.8f\n\n", a);
  printf(" Geometric mean : %13.8f\n\n", g);
  if (ite==1) 
    printf(" Harmonic mean undefined.\n");
  else
    printf(" Harmonic mean  : %13.8f\n\n", h);
  printf(" Moments:\n\n");
  printf("   M1= %13.8f\n", xm1);
  printf("   M2= %13.8f\n", xm2);
  printf("   M3= %13.8f\n", xm3);
  printf("   M4= %13.8f\n\n", xm4);
  printf(" Flatness coefficient....: %13.8f\n", ap);
  printf(" Coefficient of asymmetry: %13.8f\n\n", asy);
  printf(" Gen. mean M(%2.0f): %13.8f\n\n", t, xmt);

  return 0;
}

// End of file moment.cpp
