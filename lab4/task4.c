#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int digit_cnt(char *str) {
    int counter = 0;
    int i = 0;
    char temp = str[i];
    while (temp != '\0') {
        if ('0' <= temp && temp <= '9') 
            counter++;
        i++;
        temp = str[i];
    }
    return counter;
}

int main(int argc, char **argv) {
    int to_print;
    if (argc <= 1) printf("no value to check\n");
    else {
        to_print = digit_cnt(argv[1]);
        printf("there are: %d numbers\n", to_print);
    }
    return 0;
}