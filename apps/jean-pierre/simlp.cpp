/**********************************************************
* Simple Linear Regression Minimizing the Sum of Absolute *
* Deviation (SAD)                                         *
* ------------------------------------------------------- *
* SAMPLE RUN:                                             *
* (Find Line Alpha + Beta X, minimizing SAD for next set  *
*  of data):                                              *
*                                                         *
* N = 10                                                  *
* X(1)=1  Y(1)=1.15                                       *
* X(2)=2  Y(2)=2.22                                       *
* X(3)=3  Y(3)=2.94                                       *
* X(4)=4  Y(4)=3.85                                       *
* X(5)=5  Y(5)=5.10                                       *
* X(6)=6  Y(6)=5.99                                       *
* X(7)=7  Y(7)=7.25                                       *
* X(8)=8  Y(8)=8.04                                       *
* X(9)=9  Y(9)=9.003                                      *
* X(10)=10 Y(10)=9.999                                    *
*                                                         *
*  Alpha =  0.1667778                                     *
*  Beta  =  0.9832222                                     *
*                                                         *
*  SAD =  0.8270000                                       *
*  ITER = 3                                               *
*  Error code: 0                                          *
*                                                         *
* ------------------------------------------------------- *
* Ref.: Journal of Applied Statistics (1978) vol.27 no.3. *
*                                                         *
*                      C++ Release By J-P Moreau, Paris.  *
*                            (www.jpmoreau.fr)            *
**********************************************************/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>  // I added this, it's necessary for abs

#define NMAX 25

double ACU = 1E-6;
double BIG = 1E19;
double HALF = 0.5;
double ZERO = 0.0;
double ONE = 1.0;
double TWO = 2.0;

int    N;                //Number of points
double X[NMAX], Y[NMAX]; //Input Data Set
double D[NMAX];          //Utility vector
int    INEXT[NMAX];      //    idem
double SAD;              //Sum of Absolute Deviation
double Alpha,Beta;       //Coefficients of linear regression
int    Iter;             //Number of iterations
int    Ifault;           //Error code (must be zero)
int    I;                //loop index

    int ISign(int a, int b) {
      if (b <0) return (-abs(a));
      else return abs(a);
    }

    double Sign(double a, double b) {
      if (b < ZERO) return (-fabs(a));
      else return  fabs(a);
    }


    void SIMLP(int N, double *X, double *Y, double *SAD, double *ALPHA, double *BETA,
		       double *D, int *ITER, int *INEXT, int *IFAULT)  {

/*  ALGORITHM AS 132  APPL. STATIST. (1978) VOL.27, NO.3

      SIMPL:   Fit  Y = ALPHA + BETA.X + error             */

    double A1,A2,AAAA,BBBB,AHALF,AONE,DDD,DET,TOT1,TOT2,Y1,Y2;
    double AAA,BBB,RATIO,RHO,SUBT,SUM,T,TEST,ZZZ;
    int    I,IBAS1,IBAS2,IFLAG,IIN,IOUT,ISAVE,J;

//    Initial settings

      *IFAULT = 0;
      *ITER = 0;
      AHALF = HALF + ACU;
      AONE = AHALF + AHALF;

//    Determine initial basis

      D[1] = ZERO;
      Y1 = Y[1];
      IBAS1 = 1;
      A1 = X[1];
      for (I = 2; I<=N; I++) {
	    if (fabs(A1 - X[I]) < ACU)  goto e10;
	    A2 = X[I];
	    IBAS2 = I;
	    Y2 = Y[I];
	    goto e20;
e10: ;}
      *IFAULT = 1;
      return;

//    Calculate initial beta value

e20:  DET = ONE / (A2 - A1);
      AAAA = (A2 * Y1 - A1 * Y2) * DET;
      BBBB = (Y2 - Y1) * DET;

//    Calculate initial D-vector

      for (I = 2; I<=N; I++) { 
	    DDD = Y[I] - (AAAA + BBBB * X[I]);
		D[I] = Sign(ONE, DDD);
      }
      TOT1 = ONE;
      TOT2 = X[IBAS2];
      D[IBAS2] = - ONE;
      for (I = 2; I<=N; I++) { 
	    TOT1 = TOT1 + D[I];
	    TOT2 = TOT2 + D[I] * X[I];
      }
      T = (A2 * TOT1 - TOT2) * DET;
      if (fabs(T) < AONE) goto e50;
      DET = - DET;
      goto e70;

//    Main iterative loop begins

e50:  T = (TOT2 - A1 * TOT1) * DET;
      if (fabs(T) < AONE) goto e160;
      IFLAG = 2;
      IOUT = IBAS2;
      X[IOUT] = A1;
      AAA = A1;
      BBB = A2;
      goto e80;
e60:  T = (TOT2 - A2 * TOT1) * DET;
      if (fabs(T) < AONE) goto e160;
e70:  IFLAG = 1;
      BBB = A1;
      AAA = A2;
      IOUT = IBAS1;
e80:  RHO = Sign(ONE, T);
      T = HALF * fabs(T);
      DET = DET * RHO;

//    Perform partial sort of ratios

      INEXT[IBAS1] = IBAS2;
      RATIO = BIG;
      SUM = AHALF;
      for (I = 1; I<=N; I++) {
	    DDD = (X[I] - AAA) * DET;
	    if (DDD * D[I] <= ACU) goto e120;
	    TEST = (Y[I] - AAAA - BBBB * X[I]) / DDD;
	    if (TEST >= RATIO) goto e120;
	    J = IBAS1;
	    SUM += fabs(DDD);
e90:    ISAVE = abs(INEXT[J]);
	    if (TEST >= D[ISAVE]) goto e110;
	    if (SUM < T) goto e100;
	    SUBT = fabs((X[ISAVE] - AAA) * DET);
	    if (SUM - SUBT <  T) goto e100;
	    SUM -= SUBT;
	    D[ISAVE] = Sign(1, INEXT[J]);
	    INEXT[J] = INEXT[ISAVE];
	    goto e90;
e100:   J = ISAVE;
	    ISAVE = abs(INEXT[J]);
	    if (TEST < D[ISAVE]) goto e100;
e110:   INEXT[I] = INEXT[J];
	    INEXT[J] = ISign(I, (int) floor(D[I]));
	    D[I] = TEST;
	    if (SUM <  T) goto e120;
	    IIN = abs(INEXT[IBAS1]);
	    RATIO = D[IIN];
e120:;}

//    Update basic indicators

      IIN = abs(INEXT[IBAS1]);
      J = IIN;
e130: ISAVE = abs(INEXT[J]);
      if (ISAVE == IBAS2) goto e140;
      ZZZ = ISign(1, INEXT[J]);
      TOT1 = TOT1 - ZZZ - ZZZ;
      TOT2 = TOT2 - TWO * ZZZ * X[ISAVE];
      D[ISAVE] = - ZZZ;
      J = ISAVE;
      goto e130;
e140: ZZZ = ISign(1, INEXT[IBAS1]);
      TOT1 = TOT1 - RHO - ZZZ;
      TOT2 = TOT2 - RHO * BBB - ZZZ * X[IIN];
      D[IOUT] = - RHO;
      *ITER=*ITER+1;
      if (IFLAG == 1) goto e150;
      X[IBAS2] = A2;
      IBAS2 = IIN;
      D[IBAS2] = - ONE;
      A2 = X[IIN];
      Y2 = Y[IIN];
      DET = ONE / (A1 - A2);
      AAAA = (A1 * Y2 - A2 * Y1) * DET;
      BBBB = (Y1 - Y2) * DET;
      goto e60;
e150: IBAS1 = IIN;
      A1 = X[IIN];
      D[IBAS1] = ZERO;
      Y1 = Y[IIN];
      DET = ONE / (A2 - A1);
      AAAA = (A2 * Y1 - A1 * Y2) * DET;
      BBBB = (Y2 - Y1) * DET;
      goto e50;

//    Calculate optimal sum of absolute deviations

e160: *SAD = ZERO;
      for (I = 1; I<=N; I++) {
	    D[I] = Y[I] - (AAAA + BBBB * X[I]);
	    *SAD = *SAD + fabs(D[I]);
      }
      *ALPHA = AAAA;
      *BETA = BBBB;

}


int main()  {

  N=10;

  for (I=1; I<=N; I++)  X[I]=1.0*I;
  Y[1]=1.15; Y[2]=2.22; Y[3]=2.94; Y[4]=3.85; Y[5]=5.10;
  Y[6]=5.99; Y[7]=7.25; Y[8]=8.04; Y[9]=9.003; Y[10]=9.999;

  SIMLP(N,X,Y,&SAD,&Alpha,&Beta,D,&Iter,INEXT,&Ifault);

  printf("\n  Alpha = %10.7f\n", Alpha);
  printf("  Beta  = %10.7f\n", Beta);
  printf("\n  SAD = %10.7f\n", SAD);
  printf("  ITER = %d\n", Iter);
  printf("  Error code: %d\n\n", Ifault);

  return 0;
}

// end of file simlp.cpp
