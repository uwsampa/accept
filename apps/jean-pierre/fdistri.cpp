/********************************************************** 
* This program calculates the F Distribution Q(F|nu1,nu2) *
* for F, nu1, nu2 given.                                  *
* ------------------------------------------------------- *
* Ref.: "Numerical Recipes, By W.H. Press, B.P. Flannery, *
* S.A. Teukolsky and T. Vetterling, Cambridge University  *
* Press, 1986, page 166" [BIBLI 08].                      *
* ------------------------------------------------------- *
* SAMPLE RUN:                                             *
*                                                         *
* Calculate F Distribution Function Q(F|nu1,nu2) for      * 
* F, nu1 and nu2 given.                                   *
*                                                         *
* Ratio F of dispersion of 1st sample / dispersion of     *
* 2nd sample: 1.25                                        *
* Degree of freedom nu1 of first  sample: 25              *
* Degree of freedom nu2 of second sample: 15              *
*                                                         *
* Number of iterations: 6                                 *
*                                                         *
*                                                         *
* Q = 0.332373                                            *
*                                                         *
* ------------------------------------------------------- *
*                       C++ version by J-P Moreau, Paris. *
*                              (www.jpmoreau.fr)          *
**********************************************************/
#include <stdio.h>
#include <math.h>

#define MAXIT 100
#define EPS   3e-7
#define FPMIN 1e-30

double  f,nu1,nu2;


/*********************************************************************
  Returns the value ln(Gamma(xx)) for xx>0. Full accuracy is obtained
  for xx > 1. For 0<xx<1, the reflection formula can be used first:

     Gamma(1-z) = pi/Gamma(z)/sin(pi*z) = pi*z/Gamma(1+z)/sin(pi*z)
*********************************************************************/ 
double gammln(double xx) {
double cof[7],stp,half,one,fpf,x,tmp,ser; //internal arithmetic in double precision
int j;
  cof[1]=76.18009173; cof[2]=-86.50532033; cof[3]=24.01409822;
  cof[4]=-1.231739516; cof[5]=0.120858003e-2; cof[6]=-0.536382e-5;
  stp=2.50662827465;
  half=0.5; one=1.0; fpf=5.5;
  x=xx-one;
  tmp=x+fpf;
  tmp=(x+half)*log(tmp)-tmp;
  ser=one;
  for (j=1; j<=6; j++) {
    x+=one;
	ser+=cof[j]/x;
  }
  return (tmp+log(stp*ser));
}

double betacf(double,double,double);

/**********************************************
  Returns the incomplete beta function Ix(a,b)
**********************************************/
double betai(double a,double b,double x) {
  double bt;
  if(x < 0 || x > 1) {
	printf (" Bad argument x in function BetaI.\n");
	return 0;
  }
  if(x==0 || x==1)
    bt=0;
  else {
    bt=exp(gammln(a+b)-gammln(a)-gammln(b)+a*log(x)+b*log(1-x));
  }
  if (x < (a+1.)/(a+b+2.))   //use continued fraction directly
    return (bt*betacf(a,b,x)/a);
  else
    return (1-bt*betacf(b,a,1.-x)/b); //use continued fraction after
	 				                  //making the symmetry transformation
}

/*****************************************************************
  Continued fraction for incomplete beta function (used by BETAI)
*****************************************************************/
double betacf(double a,double b,double x) {
  int m,m2;
  double aa,c,d,del,h,qab,qam,qap;

  qab=a+b; qap=a+1.0; qam=a-1.0;
  c=1.0;
  d=1.0-qab*x/qap;
  if (fabs(d) < FPMIN) d=FPMIN;
  d=1.0/d; h=d;
  for (m=1;m<=MAXIT;m++) {
    m2=2*m;
	aa=m*(b-m)*x/((qam+m2)*(a+m2));
    d=1.0+aa*d;
    if (fabs(d) < FPMIN) d=FPMIN;
    c=1.0+aa/c;
	if (fabs(c) < FPMIN) c=FPMIN;
	d=1.0/d;
	h*=d*c;
    aa=-(a+m)*(qab+m)*x/((qap+m2)*(a+m2));
    d=1.0+aa*d;
    if (fabs(d) < FPMIN) d=FPMIN;
	c=1.0+aa/c;
	if (fabs(c) < FPMIN) c=FPMIN;
	d=1.0/d;
	del=d*c;
	h*=del;
    if (fabs(del-1.0) < EPS) break;
  } //m loop
  if (m>MAXIT) 
	printf(" a or b too big, or MAXIT too small.\n");
  else 
    printf("\n Number of iterations: %d\n",m); 
  return h;
}


int main() {
//we use the formula: Q(F|nu1,nu2) = BetaI(nu2/2,nu1/2,nu2/(nu2+nu1*F))
  double q,tmp;
  printf("\n Calculate F Distribution Function Q(F|nu1,nu2) for\n");
  printf(" F, nu1 and nu2 given.\n\n");
  printf(" Ratio F of dispersion of 1st sample / dispersion of 2nd sample: ");
  scanf("%lf",&f);
  printf(" Degree of freedom nu1 of first  sample: ");
  scanf("%lf",&nu1);
  printf(" Degree of freedom nu2 of second sample: ");
  scanf("%lf",&nu2);

  tmp=nu2/(nu2+nu1*f);

  q=betai(nu2/2,nu1/2,tmp);

  printf("\n\n Q = %f\n\n",q);

  return 0;
}

//end of file fdistri.cpp
