#include <enerc.h>

struct mystruct {
    APPROX int field;
};
int main() {
    APPROX int x = 2;
    struct mystruct s;
    s.field = 5;
    struct mystruct s2 = s;
    struct mystruct s3 = {5};
}
