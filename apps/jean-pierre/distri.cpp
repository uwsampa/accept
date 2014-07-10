/**************************************************
*          Statistical distributions              *
* ----------------------------------------------- *
*   This program allows computing several distri- *
*   butions:                                      *
*     1.  binomial distribution                   *
*     2.  Poisson distribution                    *
*     3.  normal distribution                     *
*     4.  normal distribution (2 variables)       *
*     5.  chi-square distribution                 *    
*     6.  Student T distribution                  *
* ----------------------------------------------- *
* REFERENCE: "Mathématiques et statistiques By H. *
*             Haut, PSI Editions, France, 1981"   *
*             [BIBLI 13].                         *
*                                                 *
*                    C++ version by J-P Moreau.   *
*                        (www.jpmoreau.fr)        *
* ----------------------------------------------- *
* SAMPLE RUN:                                     *
*                                                 *
*       Statistical distributions                 *
*                                                 *
* Tutorial                                        *
*                                                 *
* 1. Input distribution number:                   *
*                                                 *
*    1: binomial distribution                     *
*    2: Poisson distribution                      *
*    3: normal distribution                       *
*    4: normal distribution (2 variables)         *
*    5: chi-square distribution                   *
*    6: Student T distribution                    *
*                                                 *
* 2. Define the parameters of chosen distribution *
*                                                 *
* 3. Input value of random variable               *
*                                                 *
*                                                 *
* Input distribution number (1 to 6): 3           *
*                                                 *
*   Normal distribution                           *
*                                                 *
*   MU=mean                                       *
*   S =standard deviation                         *
*   X =value of random variable                   *
*                                                 *
*   MU = 2                                        *
*   S  = 3                                        *
*   X  = 5                                        *
*                                                 *
*   Probability of random variable  = X: .0806569 *
*   Probability of random variable <= X: .8413447 *
*   Probability of random variable >= X: .1586552 *
*                                                 *
**************************************************/
#include <stdio.h>
#include <string.h>
#include <math.h>

// Labels:  e100,e200,e300,e400,e500,e600,e700,e800,e1000

double  B[4];   // used by normal distribution
int     ch,n,nu;
char    as[45],bs[45],cs[45],ds[45];

double  bt,fx,fxy,p,px,qx,ro,s,sx,sy,x,xm,y,ym;


/***************************************************
*            BINOMIAL  DISTRIBUTION                *
* ------------------------------------------------ *
* INPUTS:                                          *
*          p:  probability of success              *
*          n:  number of trials                    *
*          x:  value of random variable            *
* OUTPUTS:                                         *
*          fx: probability of random variable  = X *
*          px: probability of random variable <= X *
*          qx: probability of random variable >= X *
***************************************************/
void Binomial(double p,int n,double x,double *fx,double *px,
			                                     double *qx) {
//Label: e100
  double q,t; int i,ix,n1;
  q=1.0-p; t=p/q;
  n1=n+1;
  *fx=pow(q,n);   //fx=prob(0)
  *px=*fx;
  if (x==0.0) goto e100;
  ix=(int)floor(x);
  for (i=1; i<ix+1; i++) {
    *fx = (n1-i)*t*(*fx)/i;
    *px = *px + (*fx);
  }
e100: *qx=1.0+(*fx)-(*px);
}


/***************************************************
*              POISSON  DISTRIBUTION               *
* ------------------------------------------------ *
* INPUTS:                                          *
*          xm:  mean                               *
*           x:  value of random variable           *
* OUTPUTS:                                         *
*          fx: probability of random variable  = X *
*          px: probability of random variable <= X *
*          qx: probability of random variable >= X *
***************************************************/
void Poisson(double xm, double x,double *fx,double *px, double *qx) {
// Label: e100
  int i,ix;
  *fx=exp(-xm);
  *px=*fx;
  if (x==0.0) goto e100;
  ix=(int) floor(x);
  for (i=1; i<ix+1; i++) {
    *fx=*fx*xm/i;
    *px=*px+*fx;
  }
e100: *qx=1.0+*fx-*px;
}

/***************************************************
* INPUTS:                                          *
*          xm: mean                                *
*           s: standard deviation                  *
*           x: value of random variable            *
* OUTPUTS:                                         *
*          fx: probability of random variable  = X *
*          px: probability of random variable <= X *
*          qx: probability of random variable >= X *
***************************************************/
void Normal(double xm,double s,double x,double *fx,double *px,
			                                       double *qx) {
  double po,pp,t,y,zy; int i;
  //define coefficients B[i)
  B[0] =  1.330274429;
  B[1] = -1.821255978;
  B[2] =  1.781477937;
  B[3] = -0.356563782;
  B[4] =  0.319381530;
  pp=0.2316419;
  y = (x-xm)/s;   //reduced variable
  zy=exp(-y*y/2.0)/2.506628275;
  *fx=zy/s;
  //calculate qx by Horner"s method
  t=1.0/(1.0+pp*y);
  po=0.0;
  for (i=0; i<5; i++) 
    po=po*t + B[i];
  po=po*t;
  *qx=zy*po;
  *px=1.0-*qx;
}

/***************************************************
*        Normal distribution (2 variables)         *
* ------------------------------------------------ *
* INPUTS:                                          *
*          xm: mean of x                           *
*          ym: mean of y                           *
*          sx: standard deviation of x             *
*          sy: standard deviation of y             *
*          ro: correlation coefficient             *
*           x: value of 1st random variable        *
*           y: value of 2nd random variable        *
* OUTPUTS:                                         *
*         fxy: probability of random variables     *
*              = (X,Y)                             *
***************************************************/
void Normal2(double xm,double ym,double sx,double sy,double ro,
			 double x,double y,double *fxy)  {
  double r,s,t,vt,pi;
  pi = 3.1415926535;
  s=(x-xm)/sx;  //1st reduced variable
  t=(y-ym)/sy;  //2nd reduced variable
  r=2.0*(1.0-ro*ro);
  vt=s*s+t*t-2.0*ro*s*t;
  vt=exp(-vt/r);
  s=sx*sy*sqrt(2.0*r)*pi;
  *fxy=vt/s;
}

/***************************************************
*             Chi-square distribution              *
* ------------------------------------------------ *
* INPUTS:                                          *
*          nu: number of degrees of freedom        *
*           x: value of random variable            *
* OUTPUTS:                                         *
*          fx: probability of random variable = X  *
*          px: probability of random variable <= X *
*          qx: probability of random variable >= X *
***************************************************/
void Chisqr(int nu, double x, double *fx, double *px, double *qx) {
//Labels: e100,e200,e300,e400
  double aa,bb,fa,fm,gn,xn2; int i,n;
  // Calculate gn=Gamma(nu/2)
  xn2=(double) nu/2;
  gn=1.0;
  n=nu / 2;
  if (nu==2*n) goto e200;
  // Calculate gn for nu odd
  if (nu==1) goto e100;
  i=1;
  while (i<=2*n-1) {
    gn = gn * i / 2.0;
    i = i+2;
  }
e100: gn = gn * 1.7724528509;
  goto e300;
e200: // Calculate gn for nu even
  if (nu==2) goto e300;
  for (i=1; i<n; i++) gn *= i;
e300: // Calculate fx
  *fx = pow(x,xn2-1);
  aa = exp(-x/2.0);
  bb = gn*pow(2.0,xn2);
  *fx = *fx * aa / bb;
  // Calculate px
  fa = pow((x/2.0),xn2);
  fa = fa * aa / (xn2*gn);;
  fm = 1.0; *px = 1.0; i = 2;
e400:  fm = (double) (fm * x / (nu+i));
  *px = *px + fm;
  if (fm*fa > 1e-8) {
    i=i+2;
    goto e400;
  }
  // Accuracy is obtained
  *px = *px * fa;
  *qx = 1.0 - *px;
}

/***************************************************
*            Student's T distribution              *
* ------------------------------------------------ *
* INPUTS:                                          *
*          nu: number of degrees of freedom        *
*           x: value of random variable            *
* OUTPUTS:                                         *
*          bt: probability of random variable      *
*              between -X and +X                   *
*          px: probability of random variable <= X *
*          qx: probability of random variable >= X *
***************************************************/
void Student(int nu,double x,double *bt,double *px,double *qx) {
//Labels e100,e200,e300,e500
  double c,t,tt,vt;
  int i,nt;
  c=0.6366197724;
  x=x/sqrt(nu);
  t=atan(x);    //t=theta
  if (nu==1) {
    *bt=t*c;
    goto e500;
  }
  nt=2*(nu/2);
  if (nt==nu) goto e200;
  //calculate A(X/NU) for nu odd
  *bt=cos(t);
  tt=*bt*(*bt);
  vt=*bt;
  if (nu==3) goto e100;
  i=2;
  while (i<=nu-3)  {
    vt=vt*i*tt/(i+1);
    *bt=*bt+vt;
    i=i+2;
  }
e100: *bt=*bt*sin(t);
  *bt=(*bt+t)*c;
  goto e500;
  //calculate A(X/NU) for nu even
e200: tt=pow(cos(t),2);
  *bt=1.0; vt=1.0;
  if (nu==2) goto e300;
  i=1;
  while (i<=nu-3)  {
    vt=vt*i*tt/(i+1);
    *bt=*bt+vt;
    i=i+2;
  }
e300: *bt=*bt*sin(t);
e500: *px = (1.0+*bt)/2.0;
  *qx=1.0-(*px);
}

int main()  {
  printf("\n       Statistical distributions\n\n");
  printf(" Tutorial\n\n");
  printf(" 1. Input distribution number:\n\n");
  printf("    1: binomial distribution\n");
  printf("    2: Poisson distribution\n");
  printf("    3: normal distribution\n");
  printf("    4: normal distribution (2 variables)\n");
  printf("    5: chi-square distribution\n");
  printf("    6: Student T distribution\n\n");
  printf(" 2. Define the parameters of chosen distribution\n\n");
  printf(" 3. Input value of random variable\n\n\n");

  printf(" Input distribution number (1 to 6): "); scanf("%d",&ch);

  if (ch<1 || ch>6) {
    printf("\n Error: Invalid choice!\n");
    return 1;
  }

  //call appropriate input section
  switch(ch) {
    case 1: goto e100; break;
    case 2: goto e200; break;
    case 3: goto e300; break;
    case 4: goto e400; break;
    case 5: goto e500; break;
    case 6: goto e600;
  }

e100:
  printf("\n   Binomial distribution:\n\n");
  printf("   P=probability of success\n");
  printf("   N=number of trials\n");
  printf("   X=value of random variable\n\n");
  printf("   P = "); scanf("%lf",&p);
  printf("   N = "); scanf("%d", &n);
  printf("   X = "); scanf("%lf",&x);
  goto e700;

e200:
  printf("\n   Poisson distribution\n\n");
  printf("   MU=mean\n");
  printf("   X =value of random variable\n\n");
  printf("   MU = "); scanf("%lf",&xm);
  printf("   X  = "); scanf("%lf",&x);
  goto e700;

e300:
  printf("\n   Normal distribution\n\n");
  printf("   MU=mean\n");
  printf("   S =standard deviation\n");
  printf("   X =value of random variable\n\n");
  printf("   MU = "); scanf("%lf",&xm);
  printf("   S  = "); scanf("%lf",&s);
  printf("   X  = "); scanf("%lf",&x);
  goto e700;

e400:
  printf("\n   Normal distribution (2 variables)\n\n");
  printf("   MX=mean of X\n");
  printf("   MY=mean of Y\n");
  printf("   SX=standard deviation of X\n");
  printf("   SY=standard deviation of Y\n");
  printf("   RO=correlation coefficient\n");
  printf("   X =value of 1st random variable\n");
  printf("   Y =value of 2nd random variable\n\n");
  printf("   MX = "); scanf("%lf",&xm);
  printf("   SX = "); scanf("%lf",&sx);
  printf("   MY = "); scanf("%lf",&ym);
  printf("   SY = "); scanf("%lf",&sy);
  printf("   RO = "); scanf("%lf",&ro);
  printf("   X  = "); scanf("%lf",&x);
  printf("   Y  = "); scanf("%lf",&y);
  goto e700;

e500:
  printf("\n   chi-square distribution\n\n");
  printf("   NU=number of degrees of freedom\n");
  printf("   X =value of random variable\n\n");
  printf("   NU = "); scanf("%d",&nu);
  printf("   X  = "); scanf("%lf",&x);
  goto e700;

e600:
  printf("\n   Student's T distribution\n\n");
  printf("   NU=number of degrees of freedom\n");
  printf("   X =value of random variable\n\n");
  printf("   NU = "); scanf("%d",&nu);
  printf("   X  = "); scanf("%lf",&x);

e700: //call appropriate subroutine
  switch(ch) {
    case 1: Binomial(p,n,x,&fx,&px,&qx); break;
    case 2: Poisson(xm,x,&fx,&px,&qx);  break;
    case 3: Normal(xm,s,x,&fx,&px,&qx); break;
    case 4: Normal2(xm,ym,sx,sy,ro,x,y,&fxy); break;
    case 5: Chisqr(nu,x,&fx,&px,&qx); break;
    case 6: Student(nu,x,&bt,&px,&qx);
  }

  // print results
  printf("\n");
  printf("\n");
  strcpy(as,"   Probability of random variable  = X: ");
  strcpy(bs,"   Probability of random variable <= X: ");
  strcpy(cs,"   Probability of random variable >= X: ");
  strcpy(ds,"   Probability of random variables = X,Y: ");
  if (ch==4)  {
    printf("%s %f\n\n",ds,fxy);
    return 0;
  }
  if (ch!=6) goto e800;
  printf("   Prob(-X<=random variable<=X) = %f\n",bt);
  printf("%s %f\n",bs, px);
  printf("%s %f\n\n",cs, qx);
  return 0;
e800: printf("%s %f\n",as, fx);
  printf("%s %f\n",bs, px);
  printf("%s %f\n\n",cs, qx);
  
}
    
// End of file distri.cpp
