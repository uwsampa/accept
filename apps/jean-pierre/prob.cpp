/************************************************************
* Evaluate probabilities by parsing a string expression     *
*        and performing the implied calculations            *
* --------------------------------------------------------- *
* SAMPLE RUN:                                               *
*                                                           *
* Type expression to be evaluated (no syntax checking).     *
* Example: '1-c(10,0,0.2)-c(10,1,0.2)'                      *
*                                                           *
* 'c(10,0,.1)+c(10,1,0.1)+c(10,2,0.1)'                      *
*                                                           *
* C(10,0) = 1                                               *
* C(10,1) = 10                                              *
* C(10,2) = 45                                              *
* probability =    0.9298                                   *
*                                                           *
* --------------------------------------------------------- *
* Ref.: "Problem Solving with Fortran 90 By David R.Brooks, *
*        Springer-Verlag New York, 1997".                   *
*                                                           *
*                        C++ Release By J-P Moreau, Paris.  *
*                               (www.jpmoreau.fr)           *
*************************************************************
!Explanations:
!------------
!   A manufacturer's experience has shown that 10% of all integrated
! circuits (ICs) will be defective. Because of this high failure rate,
! a quality control engineer monitors the manufacturing process by
! testing random samples every day. What is the probability that:
!   (a) exactly two ICs in a sample of 10 will be defective?
!   (b) at least two will be defective?
!   (c) no more than two will be defective?
! Answers:
! (a) the probability that a particular sample of 10 will contain
!                                     2     8
! exactly two defective ICs is:  (0.1) (0.9) = 0.004305. However,
! there are C(10,2)=45 possible combinations of two defective and eight
! good ICs. From probability theory, the number of combinations of n
! things taken k at a time is:  C(n,k) = n!/[k!(n-k)!]  where
! ! indicates the factorial function (n! = 1 x 2 x3 x ....n). Therefore,
! the probability that a sample of 10 will contain exactly two defects
! is:                          2     8
!          P(=2) = C(10,2)(0.1) (0.9)  = 45 x 0.004305 = 0.1937 
!
! (b) the probability of finding at least two defective ICs is equal to
!  1 minus the probability of 0 defective IC minus the probability of
!  1 defective IC:              0     10              1     9
!      P(>=2) = 1 - C(10,0)(0.1) (0.9)  - C(10,0)(0.1) (0.9) = 0.2639
!
! (Reemeber that 0! = 1 by definition.)
!
! (c) the probability of finding no more than two defective ICs is:
!                           0     10              1     9
!      P(<=2) = C(10,0)(0.1) (0.9)  + C(10,1)(0.1) (0.9)
!                             2     8
!               + C(10,2)(0.1) (0.9) = 0.9298   
!
! For example, for part (b) of the problem, the user will type the
! string:  '1-c(10,0,.1)-c(10,1,.1)'.
!---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


    char a[80];        // string to be calculated
    char s[20], s1[20];
    int i, j, jj, k, len, n, sign, left;
    double probability, prob_a;


double Fact(double x) {
// Calculate x!
  double prod;
  int i,ix;
  prod = 1.0;
  ix=(int) floor(x);
  for (i=2; i<=ix; i++)  prod *= i;
  return (prod);
}

int C(int n, int k) {
// Calculate combinations of n things taken k at a time
  double temp, denom; int ic;
  denom = Fact(1.0*k)*Fact(1.0*(n-k));
  temp = Fact(n) / denom;
  ic=(int) floor(temp);
  return (ic);
}

int main() {

  printf("\n Type expression to be evaluated (no syntax checking).\n");
  printf(" Example: 1-c(10,0,0.2)-c(10,1,0.2)\n\n");
  printf(" "); scanf("%s", a); 

  printf("\n");

  len = strlen(a);
  probability = 0.0;
  if (a[0]=='1')  probability = 1.0;
  sign=1;    // a leading + sign is optional
  for (i=0; i<len; i++) {
    if (a[i]=='+')  sign=1;
    if (a[i]=='-')  sign=-1;
    if (a[i]=='(')  left=i;
    if (a[i]==')') {
//    Read(a(left+1:i-1),*) n, k, prob_a
      strcpy(s,"");
      for (j=left+1; j<=i; j++) s[j-left-1]=a[j];
      strcpy(s1,""); j=0;
      while (s[j]!=',' && s[j]!=')') {
		s1[j]=s[j]; s1[j+1]='\0'; j++; }
      n = atoi(s1);
      strcpy(s1,""); j++; jj=0;
      while (s[j]!=',' && s[j]!=')') {
	    s1[jj]=s[j]; s1[jj+1]='\0'; j++; jj++; }
      k = atoi(s1);
      strcpy(s1,""); j++; jj=0;
      while (s[j]!=',' && s[j]!=')') {
	    s1[jj]=s[j]; s1[jj+1]='\0'; j++; jj++; }
      prob_a = atof(s1);
      probability += sign*C(n,k)*pow(prob_a,k)*pow(1.0-prob_a,n-k);
      printf(" C(%d,%d) = %d\n", n, k, C(n,k)); 
    }
  }
  printf(" probability = %10.4f\n\n", probability);	  

  return 0;
}

// end of file prob.cpp
