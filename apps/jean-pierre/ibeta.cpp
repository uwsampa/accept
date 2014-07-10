/*******************************************************
*      Calculate Incomplete Beta Function Ix(a,b)      *
* ---------------------------------------------------- *
* SAMPLE RUN:                                          *
*                                                      *
*  Incomplete Beta Function Ix(a,b)                    *
*                                                      *
*  A = 0.500000                                        *
*  B = 5.000000                                        *
*  x = 0.200000                                        *
*                                                      *
*  Y = 0.855072                                        *
*                                                      *
* ---------------------------------------------------- *
* Reference:                                           * 
* "Numerical Recipes, By W.H. Press, B.P. Flannery,    *
*  S.A. Teukolsky and T. Vetterling, Cambridge         *
*  University Press, 1986" [BIBLI 08].                 *
*                                                      *
*                    C++ Release By J-P Moreau, Paris. *
*                           (www.jpmoreau.fr)          *
*******************************************************/
#include <stdio.h>
#include <math.h>

  double a,b,x,y;

  double GAMMLN(double XX);
  double BETACF(double A,double B,double X);


double BETAI(double A,double B,double X) {
// returns the incomplete Beta function Ix(a,b)
  double BT;

  if (X < 0.0 || X > 1.0) {
    printf(" BETAI: Bad argument X (must be 0<=X<=1).\n");
    return 0.0;
  }

  if (X == 0.0 || X == 1.0)
    BT=0.0;
  else
    BT=exp(GAMMLN(A+B)-GAMMLN(A)-GAMMLN(B)+A*log(X)+B*log(1.0-X));

  if (X < (A+1.0)/(A+B+2.0))
	return (BT*BETACF(A,B,X)/A);
  else
    return (1.0-BT*BETACF(B,A,1.0-X)/B);
}


double BETACF(double A,double B,double X) {
// continued fraction for incomplete Beta function, used by BETAI
const  int ITMAX = 100;
const  double EPS = 3e-7;
double AM,BM,AZ,QAB,QAP,QAM,BZ,EM,TEM,D,AP,BP,APP,BPP,AOLD;
int    M;

  AM=1.0;
  BM=1.0;
  AZ=1.0;
  QAB=A+B;
  QAP=A+1.0;
  QAM=A-1.0;
  BZ=1.0-QAB*X/QAP;
  for (M=1; M<=ITMAX; M++) {
    EM=M;
    TEM=EM+EM;
    D=EM*(B-M)*X/((QAM+TEM)*(A+TEM));
    AP=AZ+D*AM;
    BP=BZ+D*BM;
    D=-(A+EM)*(QAB+EM)*X/((A+TEM)*(QAP+TEM));
    APP=AP+D*AZ;
    BPP=BP+D*BZ;
    AOLD=AZ;
    AM=AP/BPP;
    BM=BP/BPP;
    AZ=APP/BPP;
    BZ=1.0;
    if (fabs(AZ-AOLD) < EPS*fabs(AZ)) goto e1;
  }
  printf(" BETACF: A or B too big, or ITMAX too small.\n");
  return 0.0;
e1:return AZ;
}
  
double GAMMLN(double XX) {
// returns the value Ln(Gamma(XX) for XX>0
double STP,HALF,ONE,FPF,X,TMP,SER;
double COF[7];
int    J;
  COF[1]= 76.18009173; COF[2]=-86.50532033; COF[3]=24.01409822;
  COF[4]=-1.231739516; COF[5]=0.120858003E-2; COF[6]=-0.536382E-5;
  STP=2.50662827465;
  HALF=0.5; ONE=1.0; FPF=5.5;
  X=XX-ONE;
  TMP=X+FPF;
  TMP=(X+HALF)*log(TMP)-TMP;
  SER=ONE;
  for (J=1; J<=6; J++) {
    X=X+ONE;
    SER += COF[J]/X;
  }
  return (TMP+log(STP*SER));
}


int main()  {

  a=0.50; b=5.0; x=2e-01;

  y = BETAI(a,b,x);

  printf("\n Incomplete Beta Function Ix(A,B)\n\n");
  printf(" A = %f\n", a);
  printf(" B = %f\n", b);
  printf(" x = %f\n\n", x);
  printf(" Y = %f\n\n", y);

  return 0;
}

// end of file ibeta.cpp
