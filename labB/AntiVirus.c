#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct virus {
    unsigned short SigSize;
    char virusName[16];
    unsigned char* sig;
} virus;

typedef struct link link;

struct link {
link *nextVirus;
virus *vir;
};

struct fun_desc {
    char *name;
    link* (*fun)(link*, char*);
}; 

/* prints length bytes from memory location buffer, in hexadecimal format. */
void PrintHex(unsigned char *buffer, int length) {
    int i;
    for (i = 0; i < length; i++) {
        printf("%02hhX ", buffer[i]);
    }
    printf("\n");
}

/* prints length bytes from memory location buffer, in hexadecimal format, to output. */
void PrintHexO(unsigned char *buffer, int length, FILE *output) {
    int i;
    for (i = 0; i < length; i++) {
        if (i == length - 1)
            fprintf(output, "%02hhX", buffer[i]);
        else fprintf(output, "%02hhX ", buffer[i]);
    }
    fprintf(output, "\n");
}

/* receives a virus and a pointer to an output file. The function prints the virus to the given output.
It prints the virus name (in ASCII), the virus signature length (in decimal), and the virus signature (in hexadecimal representation). */
void printVirus(virus* virus, FILE* output) {
    fprintf(output, "Virus name: %s\n", virus->virusName);
    fprintf(output, "Virus size: %d\n", virus->SigSize);
    fprintf(output, "signature:\n");
    PrintHexO(virus->sig, virus->SigSize, output);
    fprintf(output, "\n");
}

/* receives a file pointer and returns a virus* that represents the next virus in the file. */
virus* readVirus(FILE* file) {
    virus* vrs = (virus*)malloc((sizeof(virus)));
    if (fread(vrs, 1, 18, file) == 0) { // end of file
        free(vrs);
        return NULL;
    }
    vrs->sig = malloc(vrs->SigSize);
    fread(vrs->sig, 1, vrs->SigSize, file);
    return vrs;
}

/* Print the data of every link in list to the given stream. Each item followed by a newline character. */
void list_print(link *virus_list, FILE* stream) {
    while (virus_list != NULL) {
        printVirus(virus_list->vir, stream);
        virus_list = virus_list->nextVirus;
    }
}

/* Add a new link with the given data to the list (AT BEGINNING),
and return a pointer to the list (i.e., the first link in the list).
If the list is null - create a new entry and return a pointer to the entry. */
link* list_append(link* virus_list, virus* data) {
    link *new_list = (link*)malloc(sizeof(link));
    new_list->vir = data;
    new_list->nextVirus = virus_list;
    return new_list;
}

/* Free the memory allocated by the list. */
void list_free(link *virus_list) {
    if (virus_list != NULL) {
        list_free(virus_list->nextVirus);
        free(virus_list->vir->sig);
        free(virus_list->vir);
        free(virus_list);
    }
}

/* get signaatures file, read it and return linked-list of viruses. */
link *load_fileToList(FILE *fin) {
    virus* nextVirus;
    link* virusList = NULL;
    while ((nextVirus = readVirus(fin)) != NULL) {
        virusList = list_append(virusList ,nextVirus);
    }
    return virusList;
}

/* get fileName from stdin, open and read the file to linked-list of viruses and return it. */
link *load_sig(link *virusList, char* f) {
    char filename[50];
    char buf[4];
    FILE *fin;

    printf("Enter signature file name: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strlen(filename) - 1] = '\0';
    // open file
    fin = fopen(filename, "rb");
    if (fin == NULL) {
        fprintf(stderr, "Error opening file.\n");
        list_free(virusList);
        return NULL;
    }
    // read magic number
    fread(buf, 1, 4, fin);
    if (memcmp(buf, "VISL", 4) != 0) { // if not little ending
        fprintf(stderr, "Incorrect magic number\n");
        fclose(fin);
        list_free(virusList);
        return NULL;
    }

    list_free(virusList);
    virusList = load_fileToList(fin);
    printf("DONE.\n");

    fclose(fin);
    return virusList;
}

/* print virusList to stdout and return it. */
link *print_sig(link* virusList, char* f) {
    list_print(virusList, stdout);
    printf("DONE.\n");
    return virusList;
}

/* get text and virus and return the starting byte location in the text of the virus, or -1 if its not exist. */
int detectSingelVirus(char* text, size_t textLen, virus* virusToCheck){
    int i;
    for (i = 0; i <= textLen - virusToCheck->SigSize; i++) {
        if (memcmp(virusToCheck->sig, &(text[i]), virusToCheck->SigSize) == 0) {
            return i;
        }
    }
    return -1;
} 

/* get text and list of viruses and print summery about all the viruses in the text. */
void detect_virus(char* text, size_t textLen, link *virusList) {
    int ptr;
    while (virusList != NULL) {
        ptr = detectSingelVirus(text, textLen, virusList->vir);
        if (ptr != -1) {
            printf("Virus Detected!\n");
            printf("Virus name: %s\n", virusList->vir->virusName);
            printf("Virus start at: %d (0x%X)\n", ptr, ptr);
            printf("Virus size: %d\n", virusList->vir->SigSize);
        }
        virusList = virusList->nextVirus;
    }
}

/* get file name, open it to "rb" mode and read it to text. return length of read text. */
size_t read_file(char* fileName, char *text, size_t max_size) {
    size_t readLen;
    FILE *fin = fopen(fileName, "rb");
    if (fin == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return -1;
    }
    readLen = fread(text, 1, max_size, fin);
    fclose(fin);
    return readLen;
}

/* detect virus from virusList in fileName and print them. */
link *detect_vir(link *virusList, char *fileName) {
    size_t size = 10000;
    size_t readLen;
    char *text = (char*)malloc(size);

    readLen = read_file(fileName, text, size);
    if (readLen == -1) { // error with file
        free(text);
        return virusList;
    }
    // detect virus
    detect_virus(text, readLen, virusList);
    printf("DONE.\n");

    free(text);
    return virusList;
}

/* neuteralize virus in signatureOffset from fileName. */
void neutralize_virus(char *fileName, int signatureOffset) {
    FILE *file = fopen(fileName, "rb+");
    fseek(file, signatureOffset, SEEK_SET);
    unsigned char byte = 0xC3; // RET
    fwrite(&byte, sizeof(byte), 1, file);
    fclose(file);
}

/* neutralize all viruses from file fileName. */
link *fix_file(link *virusList, char *fileName) {
    size_t size = 10000;
    int readLen, ptr;
    link *curVir = virusList;
    char *text = (char*)malloc(size);
    // read file
    readLen = read_file(fileName, text, size);
    if (readLen == -1) { // error with file
        free(text);
        return virusList;
    }
    // detect and nuetralize
    while (curVir != NULL) {
        ptr = detectSingelVirus(text, readLen, curVir->vir);
        if (ptr != -1) // found virus
            neutralize_virus(fileName, ptr);
        curVir = curVir->nextVirus;
    }
    printf("DONE.\n");

    free(text);
    return virusList;
}

/* release memory and quit */
link *quit(link *virusList, char *fileName) {
    list_free(virusList);
    exit(0);
}

int main(int argc, char **argv) {
    int i = -1, num = -1, bound, sscan;
    char input[10];
    link *virusList = NULL;
    struct fun_desc menu[] = { { "Load signatures", load_sig },
        { "Print signatures", print_sig },
        { "Detect viruses", detect_vir }, 
        { "Fix file", fix_file },
        { "Quit", quit },
        { NULL, NULL } };
    bound = (sizeof(menu) - sizeof(menu[0])) / sizeof(menu[0]);
    if (argc < 2) {
        fprintf(stderr, "Should be filename parameter in command line.\n");
        exit(1);
    }

    do {
        if (i != -1) { // not the first time of the loop
            sscan = sscanf(input, "%d\n", &num);
            if (sscan == EOF || sscan == -1 || sscan == 0 || num <= 0 || num > bound) { // invalid input
                fprintf(stderr, "Not within bounds\n");
                list_free(virusList);
                exit(1);
            }
            else { // valid input
                printf("Within bounds\n\n");
                virusList = menu[num - 1].fun(virusList, argv[1]);
                printf("\n");
            }
        }
        printf("Select operation from the following menu:\n");
        i = 0;
        while (menu[i].name != NULL) { // print options
            printf("%d. %s\n", (i + 1), menu[i].name);
            i++;
        }
        printf("Select number: ");
    }
    while (fgets(input, 10, stdin) != NULL);
    
    list_free(virusList);
    return 0;
}