/*********************************************************
* Calculate the mean and first third moments of a set of *
* data Y(i)                                              *
* ------------------------------------------------------ *
* Description:                                           *
*                                                        *
* Input data:                                            *
*		ndata:	Integer size of data set         *
*		Y:	Real Vector of ndata ordinates   *
* Output data:                                           *
*		S1:	ndata-Mean of ndata values       *
*       	S2:	Sum (Y(i)-S1)^2 for i=1..ndata   *
*		S3:	Sum (Y(i)-S1)^3 for i=1..ndata   *
*		S4:	Sum (Y(i)-S1)^4 for i=1..ndata   *
* ------------------------------------------------------ *
* SAMPLE RUN:                                            *
*  Number of data: 5                                     *
*  1: 12                                                 *
*  2: 9                                                  *
*  3: 7                                                  *
*  4: 15                                                 *
*  5: 6                                                  *
*                                                        *
*  S1=  9.800000                                         *
*  S2=  54.800003                                        *
*  S3=  73.920021                                        *
*  S4=  1024.976196                                      *
*                                                        *
*  Error code: 0                                         *
*                                                        *
* ------------------------------------------------------ *
* Ref.: "Journal of Applied Statistics (1972) vol.21,    *
*        page 226".                                      *
*                                                        *
*                      C++ Release By J-P Moreau, Paris. *
*                             (www.jpmoreau.fr)          *
*********************************************************/
#include <stdio.h>
#include <math.h>

#define  NMAX  25

int   error,i,ndata;
float Y[NMAX];
float S1,S2,S3,S4;



    void MOMNTS(float X, int K, int N, float *S1, float *S2, float *S3, 
		        float *S4, int *IFAULT)  {
/* ------------------------------------------------------------------
         ALGORITHM AS 52  APPL. STATIST. (1972) VOL.21, P.226

         ADDS A NEW VALUE, X, WHEN CALCULATING A MEAN, AND SUMS
         OF POWERS OF DEVIATIONS. N IS THE CURRENT NUMBER OF
         OBSERVATIONS, AND MUST BE SET TO ZERO BEFORE FIRST ENTRY
  ----------------------------------------------------------------- */
      float AN, AN1, DX, DX2, ZERO, ONE, TWO, THREE, FOUR, SIX;

      ZERO=0.0; ONE=1.0; TWO=2.0; THREE=3.0; FOUR=4.0; SIX=6.0;

      if (K > 0 && K < 5 && N >= 0) goto e10;
      *IFAULT = 1;
      return;
e10:  *IFAULT = 0;
      N++;
      if (N > 1) goto e20;

//    FIRST ENTRY, SO INITIALISE

      *S1 = X;
      *S2 = ZERO;
      *S3 = ZERO;
      *S4 = ZERO;
      return;

//    SUBSEQUENT ENTRY, SO UPDATE

e20:  AN = (float) N;
      AN1 = AN - ONE;
      DX = (X - *S1) / AN;
      DX2 = DX * DX;
      switch(K) {
        case 1:goto e60; break;  //calculate only mean S1
        case 2:goto e50; break;  //calculate S1 and S2
        case 3:goto e40; break;  //calculate S1, S2 and S3
        case 4:goto e30;         //calculate S1..S4
      }
e30:  *S4 = *S4 - DX * (FOUR * (*S3) - DX * (SIX * (*S2) + AN1 *
           (ONE + AN1*AN1*AN1) * DX2));
e40:  *S3 = *S3 - DX * (THREE * (*S2) - AN * AN1 * (AN - TWO) * DX2);
e50:  *S2 = *S2 + AN * AN1 * DX2;
e60:  *S1 = *S1 + DX;

   } 


int main()  {

  printf("\n  Number of data: "); scanf("%d", &ndata);
  for (i=0; i<ndata; i++) {
    printf("  %d: ",i+1); scanf("%f", &Y[i]);
  }

  for (i=0; i<ndata; i++)
    MOMNTS(Y[i],4,i,&S1,&S2,&S3,&S4,&error);

  printf("\n");
  printf("  S1= %f\n", S1);
  printf("  S2= %f\n", S2);
  printf("  S3= %f\n", S3);
  printf("  S4= %f\n", S4);
  printf("\n  Error code: %d\n\n", error);

  return 0;
}

// end of file momnts.cpp
