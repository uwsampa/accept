#include <enerc.h>

struct mystruct {
    APPROX int field;
};
class myclass {
public:
  int meth(APPROX int *x);
};

int main() {
    APPROX int x = 2;
    struct mystruct s;
    s.field = 5;
    struct mystruct s2 = s;
    struct mystruct s3 = {5};

    myclass obj;
    obj.meth(&x);
}
