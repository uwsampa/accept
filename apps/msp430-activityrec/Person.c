#include <stdlib.h>
#include "Person.h"

Person *newPerson(float h, float w, float f){

  Person *p = (Person*)malloc(sizeof(Person));
  p->height = h;
  p->weight = w;
  p->foot = f; 
  return p;

}

void getSamples(Person *p){
  p->height = 7;
  p->weight = 140;
  p->foot = 9; 
}

featurizableData *newFeaturizableData(){
  return newPerson(6,130,8);
}
