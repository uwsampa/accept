// written entirely by myself, this file didn't originally exist

#ifndef _STATS_H_
#define _STATS_H_

void NormalStats(double *a, int n, char flag, double *avg,
	               double *std_dev);

void LinearReg(double *x, double *y, int n, char flag,
	             double *a, double *b, double *s_yx, double *r);

void NormalArray(double *a, int n);

#endif  // _STATS_H_
