/************************************************************************
*        Calculate the median value of an array without sorting         *
* --------------------------------------------------------------------- *
* REFERENCE:                                                            *
*                                                                       *
*            "NUMERICAL RECIPES By W.H. Press, B.P. Flannery,           *
*             S.A. Teukolsky and W.T. Vetterling, Cambridge             *
*             University Press, 1986" [BIBLI 08].                       *
*                                                                       *
*                                           C++ Version by J-P Moreau   *
*                                               (www.jpmoreau.fr)       *
* --------------------------------------------------------------------- *
* SAMPLE RUN:                                                           *
*                                                                       *
* N=80                                                                  *
*                                                                       *
* Input Table:                                                          *
*                                                                       *
*  407.8  192.8  851.1  604.4  932.3  799.4  914.5  965.8  453.7  295.1 *
*  154.5  977.4  410.2  916.2  934.7  504.8  823.2  225.2  456.6   49.0 *
*  933.5  663.0  335.3  346.6  568.7  956.1  654.7  300.7  379.6  591.9 *
*  992.9  689.6  644.7  305.4  148.2  257.2  664.6  612.1  713.0   99.7 *
*   46.5  167.6  984.6  847.2   55.4   82.7  999.0   10.7  877.7  929.4 *
*  398.1  972.8  874.1  755.1  472.1  122.8  671.4   35.5  128.8   76.8 *
*  454.2  959.2  510.1  791.3  122.8  176.6  237.9  995.8  548.3  309.8 *
*  162.6  996.5  750.0  250.6  577.7  761.1  101.9  797.1  539.0  723.5 *
*                                                                       *
* Median Value = 558.5000                                               *
*                                                                       *
*                                                                       *
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define   SIZE   1024                  //maximum size of table


float   A[SIZE];                       //table to be sorted
float   AMED;                          //Median value
int     i, N;

FILE    *fp;                           //pointer to input text file


//write table of size N to standard output
void TWRIT(int N, float *ARR)  {
  int i; float tmp;
  printf("\n");;
  for (i=1; i<N+1; i++) {
    tmp=ARR[i];
    printf("%7.1f",tmp);
    if ((i % 10)==0) printf("\n");
  }
}

float MAX(float a, float b) {
  if (a>=b) return a;
  else return b;
}

float MIN(float a, float b) {
  if (a<=b) return a;
  else return b;
}

/******************************************************
* Given an array X of N numbers, returns their median *
* value XMED. The array X is not modified, and is     *
* accessed sequentially in each consecutive pass.     *
******************************************************/
void MDIAN1(float *X, int N, float *XMED)  {

const float BIG = (float) 1.2e16;
const float AFAC = (float) 1.5; 
const float AMP = (float)  1.5;
/*Here, AMP is an overconvergence factor: on each iteration,
  we move the guess by this factor. AFAC is a factor used to
  optimize the size of the "smoothing constant" EPS at each
  iteration. */

float A1,AA,AM,AP,DUM,EPS,SUM,SUMX,XM,XP,XX;
int   J,NM,NP;

  A1=(float) (0.5*(X[1]+X[N]));
  EPS=(float) fabs(X[N]-X[1]);
  AP=BIG;
  AM=-BIG;
e1:SUM=0.0;
  SUMX=0.0;
  NP=0;
  NM=0;
  XP=BIG;
  XM=-BIG;
  for (J=1; J<=N; J++) {
    XX=X[J];
    if (XX != A1) {
	  if (XX > A1) {
        NP++;
	    if (XX < XP)  XP=XX;
      }
	  else if (XX < A1) {
        NM++;
	    if (XX > XM)  XM=XX;
      }
      DUM=(float) (1.0/(EPS+fabs(XX-A1)));
      SUM += DUM;
      SUMX += XX*DUM;
    }
  }
  if (NP-NM >= 2) {        //guess is too low, make another pass
    AM=A1;
    AA=XP+MAX(0.0,SUMX/SUM-A1)*AMP;
    if (AA > AP)  AA=(float) (0.5*(A1+AP));
    EPS=(float) (AFAC*fabs(AA-A1));
    A1=AA;
    goto e1;
  }
  else if (NM-NP >= 2) {   //guess is too high
    AM=A1;
    AA=XM+MIN(0.0,SUMX/SUM-A1)*AMP;
    if (AA < AM)  AA=(float) (0.5*(A1+AM));
    EPS=(float) (AFAC*fabs(AA-A1));
    A1=AA;
    goto e1;
  }
  else {                    //guess is ok
	if ((N % 2) == 0)       //for even N median is an average
      if (NP == NM)
	    *XMED=(float) (0.5*(XP+XM));
      else if (NP > NM)
	    *XMED=(float) (0.5*(A1+XP));
      else
	    *XMED=(float) (0.5*(XM+A1));
    else
      if (NP == NM)
	    *XMED=A1;
      else if (NP > NM)
	    *XMED=XP;
      else
	    *XMED=XM;
  }
}


int main()  {

  fp = fopen("tmdian1.dat","r");

  //read size of table
  fscanf(fp,"%d", &N);

  //read input table A[i]
  for (i=1; i<N+1; i++) fscanf(fp,"%f", &A[i]);

  fclose(fp);

  printf("\n N=%d\n", N);
  printf("\n Input Table:\n");
  TWRIT(N,A);
 
  //call MDIAN subroutine
  MDIAN1(A, N, &AMED);

  printf("\n Median Value = %8.4f\n\n", AMED);

  return 0;
}

// end of file tmdian1.cpp
