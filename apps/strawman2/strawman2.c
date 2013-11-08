#include <enerc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

APPROX int removeme () {
    return 34 / 3;
}

int main (int argc, char** argv) {
    APPROX int i;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s outfile\n", argv[0]);
        return 1;
    }

    FILE *fn = fopen(argv[1], "w");
    fprintf(fn, "1\n");
    fclose(fn);

    accept_roi_begin();
    i = removeme();
    sleep(1);
    accept_roi_end();
    return 0;
}
