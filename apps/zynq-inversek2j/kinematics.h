/*
 * kinematics.h
 *
 *  Created on: Apr 26, 2012
 *      Author: Hadi Esmaeilzadeh <hadianeh@cs.washington.edu>
 */

#ifndef INVERSEK2J_H_
#define INVERSEK2J_H_

//#include "npu.h"

extern float l1;
extern float l2;

void forwardk2j(float theta1, float theta2, float* x, float* y);
void inversek2j(float x, float y, float* theta1, float* theta2);

#endif /* INVERSEK2J_H_ */
