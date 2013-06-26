#include <enerc.h>
struct s {
    APPROX int *a[10];
};
void f(struct s *v) {
    // This emits a "discards qualifiers" warning:
    APPROX int *b = v->a[2];

    // Whereas this does not:
    APPROX int *a[10];
    APPROX int *c = a[2];

    // I don't yet know why.
}
