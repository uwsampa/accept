/************************************************************
*                STUDENT  T-PROBABILITY  LAW                *
* --------------------------------------------------------- *
* SAMPLE RUN:                                               *
* (Calculate Student T-probability (lower tail and upper    *
*  tail) for T=0.257).                                      *
*                                                           *
*  X= 0.2570000                                             *
*  PROB1= 0.6000293                                         *
*  ERROR=0                                                  *
*  X= 0.2570000                                             *
*  PROB2= 0.3999706                                         *
*  ERROR=0                                                  *
*  PROB1+PROB2= 1.0000000                                   *
*                                                           *
* --------------------------------------------------------- *
* Ref.:"JOURNAL OF APPLIED STATISTICS (1968) VOL.17, P.189, *
*       & VOL.19, NO.1".                                    *
*                                                           *
*                        C++ Release By J-P Moreau, Paris.  *
*                               (www.jpmoreau.fr)           *
************************************************************/
#include <stdio.h>
#include <math.h>

#define  ZERO  0.0
#define  HALF  0.5
#define  ONE   1.0
#define  TWO   2.0
#define  FOUR  4.0

float X,XNU,PROB1,PROB2;
int   NU, ERROR;


    float PROBST(float T, int IDF, int *IFAULT) {
/* --------------------------------------------------------------------
!        ALGORITHM AS 3  APPL. STATIST. (1968) VOL.17, P.189
!
!        STUDENT T PROBABILITY (LOWER TAIL)
! -------------------------------------------------------------------*/
      const float PI = (float) 3.1415926535;
	  const float G1 = (float) ONE/PI;
      float A, B, C, F, S, FK;
      int   IM2,IOE,K,KS;

      *IFAULT = 1;
      if (IDF < 1) return ZERO;
      *IFAULT = 0;
      F = (float) ONE*IDF;
      A = (float) (T / sqrt(F));
      B = F / (F + T * T);
      IM2 = IDF - 2;
      IOE = IDF % 2;
      S = ONE;
      C = ONE;
      F = ONE;
      KS = 2 + IOE;
      FK = (float) KS;
      if (IM2 < 2) goto e20;
      K=KS;
      while (K <= IM2) {
        C = (float) (C * B * (FK - ONE) / FK);
        S = S + C;
        if (S == F) goto e20;
        F = S;
        FK += TWO;
        K += 2;
      }
e20:  if (IOE == 1) goto e30;
      return (float) (HALF + HALF * A * sqrt(B) * S);
e30:  if (IDF == 1) S = ZERO;
      return (float) (HALF + (A * B * S + atan(A)) * G1);
    }


    float STUDNT (float T, float DOFF, int *IFAULT) {
/* ---------------------------------------------------------------
!     ALGORITHM AS 27  APPL. STATIST. VOL.19, NO.1
!
!     Calculate the upper tail area under Student's t-distribution
!
!     Translated from Algol by Alan Miller
! --------------------------------------------------------------*/
      float V, X, TT;
      double A1, A2, A3, A4, A5, B1, B2, C1, C2, C3, C4, C5, D1, D2;
      double E1, E2, E3, E4, E5, F1, F2, G1, G2, G3, G4, G5, H1, H2;
      double I1, I2, I3, I4, I5, J1, J2;
      bool POS;

      A1=0.09979441; A2=-0.581821; A3=1.390993; A4=-1.222452; A5=2.151185;
      B1=5.537409; B2=11.42343;
      C1=0.04431742; C2=-0.2206018; C3=-0.03317253; C4=5.679969; C5=-12.96519;
      D1=5.166733; D2=13.49862;
      E1=0.009694901; E2=-0.1408854; E3=1.88993; E4=-12.75532; E5=25.77532;
      F1=4.233736; F2=14.3963;
      G1=-9.187228E-5; G2=0.03789901; G3=-1.280346; G4=9.249528; G5=-19.08115;
      H1=2.777816; H2=16.46132;
      I1=5.79602E-4; I2=-0.02763334; I3=0.4517029; I4=-2.657697; I5=5.127212;
      J1=0.5657187; J2=21.83269;

//    Check that number of degrees of freedom > 4

      if (DOFF < TWO) {
	    *IFAULT = 1;
	    return (float) (-ONE);
      }

      if (DOFF <= FOUR)
	    *IFAULT = (int) floor(DOFF);
      else
	    *IFAULT = 0;

//    Evaluate series

      V = (float) ONE/DOFF;
      POS = (T >= ZERO);
      TT = (float) fabs(T);
      X = (float) (ONE + TT * (((A1 + V * (A2 + V * (A3 + V * (A4 + V * A5)))) / (ONE - V * (B1 - V * B2))) +
           TT * (((C1 + V * (C2 + V * (C3 + V * (C4 + V * C5)))) / (ONE - V * (D1 - V * D2))) +
           TT * (((E1 + V * (E2 + V * (E3 + V * (E4 + V * E5)))) / (ONE - V * (F1 - V * F2))) +
           TT * (((G1 + V * (G2 + V * (G3 + V * (G4 + V * G5)))) / (ONE - V * (H1 - V * H2))) +
           TT * ((I1 + V * (I2 + V * (I3 + V * (I4 + V * I5)))) /
           (ONE - V * (J1 - V * J2))))))));
      X=(float) (HALF*pow(X,-8));
      if (POS)
	    return X;
      else
	    return (float) (ONE - X);
	}


int main()  {

  X=(float) 0.257;
  NU=19;

  PROB1=PROBST(X,NU,&ERROR);
  printf("\n  X=%10.7f\n", X);
  printf("  PROB1=%10.7f\n", PROB1);
  printf("  ERROR=%d\n", ERROR);

  XNU=19.0;

  PROB2=STUDNT(X,XNU,&ERROR);
  printf("  X=%10.7f\n", X);
  printf("  PROB2=%10.7f\n", PROB2);
  printf("  ERROR=%d\n", ERROR);

  printf("  PROB1+PROB2=%10.7f\n\n", PROB1+PROB2);

  return 0;
}

// end of file Student.cpp
