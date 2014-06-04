#ifndef _PERSON_H_
#define _PERSON_H_
typedef struct Person{

  float height;
  float weight;
  float foot;

}Person;

Person *newPerson(float h, float w, float f);

typedef Person featurizableData;
void getSamples(Person *p);
featurizableData *newFeaturizableData();
#endif
