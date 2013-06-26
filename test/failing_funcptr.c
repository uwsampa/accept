#include <enerc.h>
void (*g)(int *x);
void f() {
    APPROX int *y;
    g(y); // crash
}
