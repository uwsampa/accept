#include <enerc.h>
#include <stdio.h>
#include <stdlib.h>

#define INPUT_FILE_ADDR 0x08000000

int main() {
    int x = 5;
    APPROX int y = 2;

    // The data file, loaded into memory.
    char *datastr = (char *)INPUT_FILE_ADDR;

    // Truncate the string at the first newline.
    char *pos;
    for (pos = datastr; *pos != '\n'; ++pos) {}
    *pos = '\0';

    // Parse as an integer!
    int datanum = atoi(datastr);

    accept_roi_begin();
    y = y + 1;
    if (x) {
        y = 3;
    }
    accept_roi_end();

    printf("number: %i\n", y);
    printf("data: %i\n", datanum);
    return x;
}
