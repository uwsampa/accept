/********************************************************************
* This program calculates the standardized normal law probabilities *
* (mean=0, standard deviation=1) for -37.0 <= X <= 37.0             *
* ----------------------------------------------------------------- *
* SAMPLE RUN:                                                       *
*   X=5.         Number of standard deviations                      *
*   P1           Probability to the left of X                       *
*   P2           Probability to the right of X                      *
*   Q            Probability Density for X                          *
* (Use successively function alnorm and subroutines NORMP and       *
*  NPROB).                                                          *
*                                                                   *
*   P1= 9.99999713348428E-001                                       *
*   P2= 2.86651571867397E-007                                       *
*                                                                   *
*   P1= 9.99999713348428E-0001                                      *
*   P2= 2.86651571892333E-0007                                      *
*   Q=  1.48671951473430E-0006                                      *
*                                                                   *
*   P1= 9?99999713348428E-0001                                      *
*   P2= 2.86651571867397E-0007                                      *
*   Q=  1.48671951467306E-0006                                      *
*                                                                   *
* ----------------------------------------------------------------- *
* Ref.: Journal of Applied Statistics (1973) vol22 no.3.            *
*                                                                   *
*                                C++ Release By J-P Moreau, Paris.  *
*                                       (www.jpmoreau.fr)           *
********************************************************************/
#include <stdio.h>
#include <math.h>

#define  FALSE 0
#define  TRUE  1

double zero = 0.0, one = 1.0, half = 0.5;
double ltone = 7.0, utzero = 18.66, con = 1.28;

double P1, P2, Q, X;
bool   UP;


/* This file includes the Applied Statistics algorithm AS 66 for calculating
   the tail area under the normal curve, and two alternative routines which
   give higher accuracy.   The latter have been contributed by Alan Miller of
   CSIRO Division of Mathematics & Statistics, Clayton, Victoria.   Notice
   that each function or routine has different call arguments.            */

    double alnorm(double x, bool upper) {

/*      Algorithm AS66 Applied Statistics (1973) vol22 no.3

        Evaluates the tail area of the standardised normal curve
        from x to infinity if upper is .true. or
        from minus infinity to x if upper is .false.             */

//  Labels: e10,e20,e30,e40
    
    double z,y,Temp;
    double p,q,r,a1,a2,a3,b1,b2,c1,c2,c3,c4,c5,c6;
    double d1,d2,d3,d4,d5;
    bool   up;

/*** machine dependent constants  */
      p=0.398942280444; q=0.39990348504; r=0.398942280385;   
      a1=5.75885480458; a2=2.62433121679; a3=5.92885724438;  
      b1=-29.8213557807; b2=48.6959930692;
      c1=-3.8052E-8; c2=3.98064794E-4; c3=-0.151679116635;
      c4=4.8385912808; c5=0.742380924027; c6=3.99019417011;  
      d1=1.00000615302; d2=1.98615381364; d3=5.29330324926;  
      d4=-15.1508972451; d5=30.789933034;

      up=upper;
      z=x;
      if (z >= zero) goto e10;
      up= !up;
      z=-z;
e10:  if ((z <= ltone || up==TRUE) && z <= utzero) goto e20;
      Temp=zero;
      goto e40;
e20:  y=half*z*z;
      if (z > con) goto e30;
      Temp=half-z*(p-q*y/(y+a1+b1/(y+a2+b2/(y+a3))));
      goto e40;
e30:  Temp=r*exp(-y)/(z+c1+d1/(z+c2+d2/(z+c3+d3/(z+c4+d4/(z+c5+d5/(z+c6))))));
e40:  if (up) return Temp;
      else return (one-Temp);
    }


  void NORMP(double Z, double *P, double *Q, double *PDF) {

/*	Normal distribution probabilities accurate to 1.e-15.
 	Z = no. of standard deviations from the mean.
	P, Q = probabilities to the left & right of Z.   P + Q = 1.
        PDF = the probability density.
        Based upon algorithm 5666 for the error function, from:
        Hart, J.F. et al, 'Computer Approximations', Wiley 1968

        Programmer: Alan Miller

        Latest revision - 30 March 1986       */

    double P0,P1,P2,P3,P4,P5,P6;
    double Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7;
    double CUTOFF,EXPNTL,ROOT2PI,ZABS;

	P0=220.2068679123761; P1=221.2135961699311; P2=112.0792914978709;
    P3= 33.91286607838300; P4= 6.373962203531650;
    P5=  0.7003830644436881; P6= 0.3526249659989109E-01;
    Q0=440.4137358247522; Q1=793.8265125199484; Q2=637.3336333788311;
   	Q3=296.5642487796737; Q4= 86.78073220294608; Q5=16.06417757920695;
    Q6=  1.755667163182642; Q7=0.8838834764831844E-01;
    CUTOFF= 7.071; ROOT2PI=2.506628274631001;

	ZABS = fabs(Z);

//	|Z| > 37.0

	if (ZABS > 37.0) {
	  *PDF = zero;
	  if (Z >  zero) {
	    *P = one;
	    *Q = zero;
      }
	  else {
	    *P = zero;
	    *Q = one;
	  }
	  return;
	}

//  |Z| <= 37.0

	EXPNTL = exp(-half*ZABS*ZABS);
	*PDF = EXPNTL/ROOT2PI;

//  |Z| < CUTOFF = 10/sqrt(2)

	if (ZABS < CUTOFF)
	  *P = EXPNTL*((((((P6*ZABS + P5)*ZABS + P4)*ZABS + P3)*ZABS +
     	       P2)*ZABS + P1)*ZABS + P0)/(((((((Q7*ZABS + Q6)*ZABS  +
     	       Q5)*ZABS + Q4)*ZABS + Q3)*ZABS + Q2)*ZABS + Q1)*ZABS + Q0);

//  |Z| >= CUTOFF

	else
	  *P = *PDF/(ZABS + one/(ZABS + 2.0/(ZABS + 3.0/(ZABS + 4.0/
     		(ZABS + 0.65)))));

	if (Z < zero)
	  *Q = one - *P;
	else {
	  *Q = *P;
	  *P = one - *Q;
	}
  }


  void NPROB(double Z, double *P, double *Q, double *PDF) {

/*      P, Q = PROBABILITIES TO THE LEFT AND RIGHT OF Z
        FOR THE STANDARD NORMAL DISTRIBUTION.
        PDF  = THE PROBABILITY DENSITY FUNCTION

        REFERENCE: ADAMS,A.G. AREAS UNDER THE NORMAL CURVE,
        ALGORITHM 39, COMPUTER J., VOL. 12, 197-8, 1969.

        LATEST REVISION - 23 JANUARY 1981                 */

//    Labels: e10,e20,e30

    double A0,A1,A2,A3,A4,A5,A6,A7;
    double B0,B1,B2,B3,B4,B5,B6,B7,B8,B9,B10,B11;
    double Y, ZABS;

    A0=half; A1=0.398942280444; A2=0.399903438504;
    A3=5.75885480458; A4=29.8213557808;
    A5=2.62433121679; A6=48.6959930692; A7=5.92885724438;

    B0=0.398942280385; B1=3.8052E-8; B2=1.00000615302;
    B3=3.98064794E-4; B4=1.98615381364; B5=0.151679116635;
    B6=5.29330324926; B7=4.8385912808; B8=15.1508972451;
    B9=0.742380924027; B10=30.789933034; B11=3.99019417011;

    ZABS = fabs(Z);
    if (ZABS > 12.7) goto e20;
    Y = A0*Z*Z;
    *PDF = exp(-Y)*B0;
    if (ZABS > 1.28) goto e10;

//  Z BETWEEN -1.28 AND +1.28 

    *Q = A0-ZABS*(A1-A2*Y/(Y+A3-A4/(Y+A5+A6/(Y+A7))));
    if (Z < zero) goto e30;
    *P = one - *Q;
    return;

//  ZABS BETWEEN 1.28 AND 12.7

e10:*Q = *PDF/(ZABS-B1+B2/(ZABS+B3+B4/(ZABS-B5+B6/(ZABS+B7-B8/
         (ZABS+B9+B10/(ZABS+B11))))));
    if (Z < zero) goto e30;
    *P = one - *Q;
    return;

//  Z FAR OUT IN TAIL

e20:*Q = zero;
    *PDF = zero;
    if (Z < zero) goto e30;
    *P = one;
    return;

//  NEGATIVE Z, INTERCHANGE P AND Q

e30:*P = *Q;
    *Q = one - *P;

  }


int main()  {

  X=5.0;

  UP=FALSE;
  P1= alnorm(X,UP);
  UP=TRUE;
  P2= alnorm(X,UP);

  printf("\n  P1=%22.14e\n", P1);
  printf("  P2=%22.14e\n", P2);

  NORMP(X, &P1, &P2, &Q);
  printf("\n  P1=%22.14e\n", P1);
  printf("  P2=%22.14e\n", P2);
  printf("  Q =%22.14e\n", Q);

  NPROB(X, &P1, &P2, &Q);
  printf("\n  P1=%22.14e\n", P1);
  printf("  P2=%22.14e\n", P2);
  printf("  Q =%22.14e\n\n", Q);

  return 0;
}

// end of file Tnormal.cpp
