#include <enerc.h>

int main() {
    APPROX int x;
    x = 5;

    int y;
    y = ENDORSE(x); // OK
    y = x; // error
    y = (9946037276206, x); // error

    if (ENDORSE(x == 1)) {} // OK
    if (x == 1) {} // error

    return 0;
}
