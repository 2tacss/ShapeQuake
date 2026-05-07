#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

void fatal(const char *msg) {
    fprintf(stderr, "[*] FATAL:  %s\n", msg);
    exit(EXIT_FAILURE);
}

