#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct fun_desc {
    char *name;
    char (*fun)(char);
}; 

char* map(char *array, int array_length, char (*f) (char)){
  char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
  /* TODO: Complete during task 2.a */
  int i;
  for (i = 0; i < array_length; i++) {
      mapped_array[i] = f(array[i]);
  }
  return mapped_array;
}

/* Ignores c, reads and returns a character from stdin using fgetc. */
char my_get(char c) {
    return fgetc(stdin);
}
 
/* If c is a number between 0x20 and 0x7E, cprt prints the character of ASCII value c 
followed by a new line. Otherwise, cprt prints the dot ('.') character. 
After printing, cprt returns the value of c unchanged. */
char cprt(char c) {
    if (0x20 <= c && c <= 0x7E) printf("%c\n", c);
    else printf(".\n");
    return c;
}

/* Gets a char c and returns its encrypted form by adding 1 to its value.
If c is not between 0x20 and 0x7E it is returned unchanged */
char encrypt(char c) {
    if (0x20 <= c && c <= 0x7E) return (c + 1);
    return c;
}

/* Gets a char c and returns its decrypted form by reducing 1 from its value. 
If c is not between 0x20 and 0x7E it is returned unchanged */
char decrypt(char c) {
    if (0x20 <= c && c <= 0x7E) return c-1;
    return c;
}

/* xprt prints the value of c in a hexadecimal representation followed by a new line, 
and returns c unchanged. */ 
char xprt(char c) {
    if (0x20 <= c && c <= 0x7E) printf("%x\n", c);
    else printf(".\n");
    return c;
}

int main(int argc, char **argv) {
    char *carray = (char*)(malloc(5*sizeof(char)));
    struct fun_desc menu[] = { { "Get String", my_get },{ "Print String", cprt },{ "Print Hex", xprt }, 
        { "Encrypt", encrypt },{ "Decrypt", decrypt },{ NULL, NULL } };
    char *tmp;
    char str[10];
    int i = -1, bound = 0, num;
    carray[0] = '\0';
    while (menu[bound].name != NULL) bound++; // length of array menu
    do {
        if (i != -1) { // not the first time of the loop
            num = atoi(str);
            if (num == 0 && strcmp(str, "0\n") != 0) { // str is not a number (invalid input)
                printf("Not within bounds\n");
                free(carray);
                return 0;
            }
            else if (0 <= num && num < bound) { // valid input
                printf("Within bounds\n");
                tmp = map(carray, 5, menu[num].fun);
                free(carray);
                carray = tmp;
                printf("DONE.\n\n");
            }
            else {  // not in bound (invalid input)
                printf("Not within bounds\n");
                free(carray);
                return 0;
            }
        }
        printf("Select operation from the following menu:\n");
        i = 0;
        while (menu[i].name != NULL) { // print options
            printf("%d. %s\n", i, menu[i].name);
            i++;
        }
        printf("Select number: (or ctrl+D for exit) ");
    }
    while (fgets(str, 10, stdin) != NULL);
    printf("\n");
    free(carray);
    return 0;
}
