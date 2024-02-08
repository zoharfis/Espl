#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct state {
    char debug_mode;
    char file_name[100];
    int unit_size;
    unsigned char mem_buf[10000];
    size_t mem_count;
    char display_mode;
} state;

struct fun_desc {
    char *name;
    void (*fun_ptr)(state*);
}; 

static char* hex_formats[] = {"%#hhx\n", "%#hx\n", "No such unit", "%#x\n"};
static char* dec_formats[] = {"%#hhd\n", "%#hd\n", "No such unit", "%#d\n"};

FILE* open_file(state* s, char* mode) {
    FILE* file;
    if(strcmp(s->file_name, "") == 0) {
        fprintf(stdout, "Error: file name is empty\n");
        return NULL;
    }
    file = fopen(s->file_name, mode);
    if (file == NULL) {
        fprintf(stdout, "Error: failed to open the file\n");
        return NULL;
    }
    return file;
}

void toggle_debug_mode(state* s) {  
    if (s->debug_mode == '1') {
        s->debug_mode = '0';
        fprintf(stdout, "Debug flag now off\n");
    }
    else {
        s->debug_mode = '1';
        fprintf(stdout, "Debug flag now on\n");
    }
}

void set_file_name(state* s) {
    fprintf(stdout, "Enter file name\n");
    fgets(s->file_name, 100, stdin);
    s->file_name[strlen(s->file_name)-1] = '\0';
    if (s->debug_mode == '1') {
        fprintf(stderr, "Debug: file name set to: %s\n" , s->file_name);
    }
}

void set_unit_size(state* s) {
    int unit_size;
    char input[10];
    fprintf(stdout, "Enter unit size (1|2|4)\n");
    fgets(input, 10 , stdin);
    unit_size = atoi(input);
    if (unit_size == 1 || unit_size == 2  || unit_size == 4) {
        s->unit_size = unit_size;
        if (s->debug_mode == '1') {
            fprintf(stderr,"Debug: set size to: %d\n" , s->unit_size);
        }
    }
    else 
        fprintf(stdout, "Error: invalid number for unit-size\n");
}

void load_into_memory(state* s){
    FILE* file;
    char input[50];
    int length, location;
    file = open_file(s, "rb");
    if (file == NULL) 
        return;
    printf("Please enter <location> <length>\n");
    printf("(Note that location is hexadecimal, length is decimal)\n");
    fgets(input, sizeof(input), stdin);
    if (sscanf(input, "%x %d", &location, &length) != 2) {
        fprintf(stdout, "Invalid input\n");
        return;
    }
    if (s->debug_mode == '1') {
        printf("Debug:\n");
        printf("file name: %s\n", s->file_name);
        printf("location: %x\n", location);
        printf("length: %d\n", length);
    }
    fseek(file, location, SEEK_SET);
    s->mem_count = fread(s->mem_buf, s->unit_size, length, file);
    printf("Loaded %d units into memory\n", length);
    fclose(file);
}

void toggle_display_mode(state* s) {
    if (s->display_mode == '0') {
        s->display_mode = '1';
        printf("Display flag now on, hexadecimal representation\n");
    }
    else {
        s->display_mode = '0';
        printf("Display flag now off, decimal representation\n");
    }  
}

void print_buffer_units(char display_mode, int unit_size, unsigned char* buffer, int units) {
    unsigned char *end = buffer + (unit_size * units);
    unsigned char *location = buffer;
    int var;
    if (display_mode == '1') {
        printf("Hexadecimal\n===========\n");
    }
    else {
        printf("Decimal\n=======\n");
    }
    while (location < end) {
        var = *((int*)(location));
        if (display_mode == '1')
            printf(hex_formats[unit_size-1], var);
        else
            printf(dec_formats[unit_size-1], var);
        location = location + unit_size;
    }
}

void memory_display(state* s){
    char input[100];
    int address, length;
    printf("Enter address and length\n");
    printf("(Note that address is hexadecimal, length is decimal)\n");
    fgets(input, sizeof(input), stdin);
    if (sscanf(input, "%x %d", &address, &length) != 2) {
        fprintf(stdout, "Invalid input\n");
        return;
    }
    if (s->debug_mode == '1') {
        printf("Debug:\n");
        printf("file name: %s\n", s->file_name);
        printf("address: %x\n", address);
        printf("length: %d\n", length);
    }
    if (address == 0) {
        print_buffer_units(s->display_mode, s->unit_size, s->mem_buf, length);
    }
    else
        print_buffer_units(s->display_mode, s->unit_size, (unsigned char *)address, length);
}

void save_into_file(state* s) {
    FILE* file;
    char input[100];
    int units, source_address, target_location, end_file_pos;

    file = open_file(s, "r+");
    if (file == NULL)
        return;

    printf("Please enter <source-address> <target-location> <length>\n");
    printf("(Note that source-address is hexadecimal, target-location is hexadecimal, length is decimal)\n");
    fgets(input, sizeof(input), stdin);
    if (sscanf(input, "%x %x %d", &source_address, &target_location, &units) != 3) {
        fprintf(stdout, "Invalid input\n");
        return;
    }
    if (s->debug_mode == '1') {
        printf("Debug:\n");
        printf("file name: %s\n", s->file_name);
        printf("source address: %x\n", source_address);
        printf("target location: %x\n", target_location);
        printf("length: %d\n", units);
    }

    fseek(file, 0, SEEK_END);
    end_file_pos = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (target_location >= end_file_pos) {
        printf("Error: Invalid target-location\n");
        return;
    }

    fseek(file, target_location, SEEK_SET);
    
    if (source_address == 0){
        fwrite(s->mem_buf, s->unit_size, units, file); 
    }
    else {
        fwrite((void*)source_address, s->unit_size, units, file);
    }
    printf("Copyed %d units into memory\n", units);
    fclose(file);
}

void memory_modify(state* s) {
    int val, location;
    char input[50];

    printf("Please enter <location> <val> (in hexadecimal)\n");
    fgets(input, sizeof(input), stdin);
    if (sscanf(input, "%x %x", &location, &val) != 2) {
        fprintf(stdout, "Error: Invalid input\n");
        return;
    }
    if (s->debug_mode == '1') {
        printf("Debug:\n");
        printf("file name: %s\n", s->file_name);
        printf("location: %x\n", location);
        printf("val: %x\n", val);
    }
    
    if(location >= s->mem_count){
        printf("Error: Invalid location\n");
        return;
    }
    memcpy(&(s->mem_buf[location]), &(val), s->unit_size);
}

void quit(state* s) {
    if (s->debug_mode == '1') 
        fprintf(stderr, "quitting\n");
    exit(0);
}

void print_debug_mode(state* s) {
    fprintf(stderr,"Debug Print\n");
    fprintf(stderr,"unit size: %d\n" , s->unit_size);
    fprintf(stderr,"file name: %s\n" , s->file_name);
    fprintf(stderr,"memory count: %d\n" , s->mem_count);
}

int main(int argc, char **argv) {
    int i = -1, num = -1, bound, sscan;
    state the_state;
    char input[10];
    struct fun_desc menu[] = { 
        { "Toggle Debug Mode", toggle_debug_mode },
        { "Set File Name", set_file_name },
        { "Set Unit Size", set_unit_size }, 
        { "Load Into Memory", load_into_memory },
        { "Toggle Display Mode", toggle_display_mode },
        { "Memory Display", memory_display },
        { "Save Into File", save_into_file },
        { "Memory Modify", memory_modify },
        { "Quit", quit },
        { NULL, NULL } };
    bound = (sizeof(menu) - sizeof(menu[0])) / sizeof(menu[0]);
    // defult state
    the_state.unit_size = 1;
    the_state.debug_mode = '0';
    the_state.display_mode = '0';
    the_state.mem_count = 0;
    strcpy(the_state.file_name, "");

    do {
        if (i != -1) { // not the first time of the loop
            sscan = sscanf(input, "%d\n", &num);
            if (sscan == EOF || sscan == -1 || num < 0 || num >= bound) { // invalid input
                fprintf(stderr, "Not within bounds\n");
                exit(1);
            }
            else { // valid input
                printf("Within bounds\n\n");
                menu[num].fun_ptr(&the_state);
                printf("\n");
            }
        }

        if(the_state.debug_mode == 1)
            print_debug_mode(&the_state);
        printf("Choose action from the following menu:\n");
        i = 0;
        while (menu[i].name != NULL) { // print options
            printf("%d. %s\n", i, menu[i].name);
            i++;
        }
        printf("Select number: ");
    }
    while (fgets(input, 10, stdin) != NULL);
    return 0;
}