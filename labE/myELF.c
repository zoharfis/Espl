#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <elf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct ELF {
    int fd;
    void *map_start;
    int file_size;
    char file_name[50];
    void *strSHTab;
    void *strSymTab;
} ELF;

struct fun_desc {
    char *name;
    void (*fun_ptr)();
}; 

ELF elf1;
ELF elf2;
char debug_mode = '0';

void toggle_debug_mode() {  
    if (debug_mode == '1') {
        debug_mode = '0';
        fprintf(stdout, "Debug flag now off\n");
    }
    else {
        debug_mode = '1';
        fprintf(stdout, "Debug flag now on\n");
    }
}

void quit() {
    if (debug_mode == '1') 
        fprintf(stderr, "**debug mode: quitting\n");
    if (elf1.fd != -1) {
        munmap(elf1.map_start, elf1.file_size);
        close(elf1.fd);
    }
    if (elf2.fd != -1) {
        munmap(elf2.map_start, elf2.file_size);
        close(elf2.fd);
    }
    exit(0);
}

void update_strSHTab(ELF *elf) {
    Elf32_Ehdr *elfH =  (Elf32_Ehdr *)elf->map_start;
    Elf32_Shdr *elfSH = (Elf32_Shdr *)(elf->map_start + elfH->e_shoff);
    Elf32_Half shstrndx = elfH->e_shstrndx;
    Elf32_Shdr shst_h = elfSH[shstrndx];
    Elf32_Off shst_off = shst_h.sh_offset;
    elf->strSHTab = (elf->map_start + shst_off);
}

void update_strSymTab(ELF *elf) {
    Elf32_Ehdr *elfH =  (Elf32_Ehdr *)elf->map_start;
    Elf32_Shdr *elfSH = (Elf32_Shdr *)(elf->map_start + elfH->e_shoff);
    int shnum = elfH->e_shnum;
    int i;
    char *sh_name;
    if (elf->strSHTab == NULL) update_strSHTab(elf);
    for (i = 0; i < shnum; i++) {
        sh_name = (char *)(elf->strSHTab + elfSH[i].sh_name);
        if (strcmp(sh_name, ".strtab") == 0) {
            elf->strSymTab = (void *)(elf->map_start + elfSH[i].sh_offset);
            break;
        }
    }
}

int foreach_shdr(ELF *elf, void (*func)(Elf32_Shdr *, int, void *)) {
    Elf32_Ehdr *elfH =  (Elf32_Ehdr *)elf->map_start;
    Elf32_Shdr *elfSH = (Elf32_Shdr *)(elf->map_start + elfH->e_shoff);
    int shnum = elfH->e_shnum;
    int i;
    if (elf->strSHTab == NULL) update_strSHTab(elf);
    if (debug_mode == '1') {
        fprintf(stderr, "**debug mode: shstrndx: %d\n", elfH->e_shstrndx);
        fprintf(stderr, "**debug mode: string table offset: %06x\n", elfSH[elfH->e_shstrndx].sh_offset);
    }
    for (i = 0; i < shnum; i++) {
        func(&elfSH[i], i, elf->strSHTab);
    }
    return 0;
}

char *shtypeToString(Elf32_Word shType) {
    switch (shType) {
        case SHT_NULL:return "NULL";
        case SHT_PROGBITS:return "PROGBITS";
        case SHT_SYMTAB:return "SYMTAB";
        case SHT_STRTAB:return "STRTAB";
        case SHT_RELA:return "RELA";
        case SHT_HASH:return "HASH";
        case SHT_DYNAMIC:return "DYNAMIC";
        case SHT_NOTE:return "NOTE";
        case SHT_NOBITS:return "NOBITS";
        case SHT_REL:return "REL";
        case SHT_SHLIB:return "SHLIB";
        case SHT_DYNSYM:return "DYNSYM";
        default:return "Unknown";
    }
}

void print_shdr(Elf32_Shdr *shdr ,int i, void *stringTable) {
    char *name = (char *)(stringTable + shdr->sh_name);
    char *type = shtypeToString(shdr->sh_type);
    printf("%d %s %08x %06x %06x %s\n", i, name, shdr->sh_addr, shdr->sh_offset, shdr->sh_size, type);
    // if (debug_mode == '1') {
    //     fprintf(stderr, "**debug mode: section name offset: %06x\n", shdr->sh_name);
    // }
}

int foreach_symtab(ELF *elf, void (*func)(Elf32_Sym *, int, void *, void *, Elf32_Shdr *)) {
    int i, symtab_len, symtab_size;
    Elf32_Ehdr *elfH =  (Elf32_Ehdr *)elf->map_start;
    Elf32_Shdr *elfSH = (Elf32_Shdr *)(elf->map_start + elfH->e_shoff);
    Elf32_Sym *symtab = NULL;
    int shnum = elfH->e_shnum;
    char *sh_name;
    if (elf->strSHTab == NULL) update_strSHTab(elf);
    if (elf->strSymTab == NULL) update_strSymTab(elf);
    // find symbole table
    for (i = 0; i < shnum; i++) {
        sh_name = (char *)elf->strSHTab + elfSH[i].sh_name;
        if (strcmp(sh_name, ".symtab") == 0) {
            symtab = (Elf32_Sym *)(elf->map_start + elfSH[i].sh_offset);
            symtab_size = elfSH[i].sh_size;
            symtab_len = symtab_size / sizeof(Elf32_Sym);
            break;
        }
    }
    if (symtab == NULL) {
        printf("Error: there is no symbol table\n");
        return 1;
    }
    if (debug_mode == '1') {
        fprintf(stderr, "**debug mode: size of symbol table: 0x%x\n", symtab_size);
        fprintf(stderr, "**debug mode: number of symbols: %d\n", symtab_len);
    }
    for (i = 0; i < symtab_len; i++) {
        func(&symtab[i], i, elf->strSymTab, elf->strSHTab, elfSH);
    }
    return 0;
}

void print_symtab(Elf32_Sym *sym ,int i, void *strSymTab, void *strSHTab, Elf32_Shdr *elfSH) {
    char *sym_name = (char *)(strSymTab + sym->st_name);
    Elf32_Section shndx = sym->st_shndx;
    char *section_name, *section_name0 = NULL;
    int index = 0;
    switch (shndx)
    {
    case SHN_ABS:
        section_name = "ABS";
        break;
    case SHN_COMMON:
        section_name = "COM";
        break;
    case SHN_XINDEX:
        section_name0 = (char *)(strSHTab + elfSH[sym->st_value].sh_name);
        section_name = "COM";
        break;
    case SHN_UNDEF:
        section_name = "UND";
        break;
    default:
        index = (int)sym->st_shndx;
        section_name = (char *)(strSHTab + elfSH[sym->st_shndx].sh_name);
        break;
    }
    if (index != 0) 
        printf("%d %08x %d %s %s\n", i, sym->st_value, index, section_name, sym_name);
    else if (section_name0 == NULL)
        printf("%d %08x %s %s\n", i, sym->st_value, section_name, sym_name);
    else
        printf("%d %08x %s[%s] %s\n", i, sym->st_value, section_name, section_name0, sym_name);
}

void print_Elf(Elf32_Ehdr *eh) {
    printf("Magic number: %c%c%c\n", eh->e_ident[EI_MAG1], eh->e_ident[EI_MAG2], eh->e_ident[EI_MAG3]);
    printf("Data encoding scheme: %s\n", (eh->e_ident[EI_DATA] == ELFDATA2LSB) ? "little endian" : "big endian");
    printf("Entry point adress: 0x%08x\n", eh->e_entry);
    printf("Start of section headers: %d (bytes into file)\n", eh->e_shoff);
    printf("Number of section headers: %d\n", eh->e_shnum);
    printf("Size of section headers: %d (bytes)\n", eh->e_shentsize);
    printf("Start of program headers: %d (bytes into file)\n", eh->e_phoff);
    printf("Number of program headers: %d\n", eh->e_phnum);
    printf("Size of program headers: %d (bytes)\n", eh->e_phentsize);
}

void examine_ELF_file() {
    char file_name[50];
    int fd, file_size;
    void *map_start;
    Elf32_Ehdr *elfH;
    char magic_number[4];
    if (elf1.fd != -1 && elf2.fd != -1) {
        printf("Error: Cannot examine more than 2 files\n");
        return;
    }
    printf("Please enter ELF file name\n");
    fgets(file_name, 50, stdin);
    file_name[strlen(file_name)-1] = '\0';
    fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    file_size = lseek(fd, 0, SEEK_END);
    map_start = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return;
    }
    elfH = (Elf32_Ehdr *)map_start;
    magic_number[0] = elfH->e_ident[EI_MAG1];
    magic_number[1] = elfH->e_ident[EI_MAG2];
    magic_number[2] = elfH->e_ident[EI_MAG3];
    magic_number[3] = '\0';
    if (strcmp(magic_number, "ELF") != 0) {
        printf("File is not ELF file!\n");
        munmap(map_start, file_size);
        close(fd);
        return;
    }
    if (elf1.fd == -1) {
        elf1.map_start = map_start;
        elf1.file_size = file_size;
        elf1.fd = fd;
        strcpy(elf1.file_name, file_name);
    }
    else {
        elf2.map_start = map_start;
        elf2.file_size = file_size;
        elf2.fd = fd;
        strcpy(elf2.file_name, file_name);
    }
    print_Elf(elfH);
}

void print_section_names() {
    int i;
    ELF *elf;
    for (i = 1; i <= 2; i++) {
        if (i == 1 && elf1.fd != -1) {
            elf = &elf1;
        }
        else if (i == 2 && elf2.fd != -1) {
            elf = &elf2;
        }
        else { continue; }
        printf("File %s\n", elf->file_name);
        foreach_shdr(elf, print_shdr);
    }
}

void print_symbols() {
    int i;
    ELF *elf;
    for (i = 1; i <= 2; i++) {
        if (i == 1 && elf1.fd != -1) {
            elf = &elf1;
        }
        else if (i == 2 && elf2.fd != -1) {
            elf = &elf2;
        }
        else { continue; }
        printf("File %s\n", elf->file_name);
        foreach_symtab(elf, print_symtab);
    }
}

// return 0 for not in symtab, 1 for defined and -1 for undefined
int find_sym(Elf32_Sym *symtab, int symtab_len, void *strSymTab, char *sym_name) {
    int i;
    char *curr_name;
    for (i = 1; i < symtab_len; i++) {
        curr_name = (char *)strSymTab + symtab[i].st_name;
        if (strcmp(curr_name, sym_name) == 0) {
            if (symtab[i].st_shndx == SHN_UNDEF) return -1;
            else return 1; 
        }
    }
    return 0;
}

int compare_symtabs(Elf32_Sym *symtab1, Elf32_Sym *symtab2, int symtab_len1, int symtab_len2, void *strSymTab1, void *strSymTab2) {
    int i, isDef1, isDef2;
    char *sym_name;
    for (i = 1; i < symtab_len1; i++) {
        sym_name = (char *)strSymTab1 + symtab1[i].st_name;
        if (strcmp(sym_name, "") == 0) continue;
        if (symtab1[i].st_shndx == SHN_UNDEF) isDef1 = -1;
        else isDef1 = 1; 
        isDef2 = find_sym(symtab2, symtab_len2, strSymTab2, sym_name);
        if (isDef1 == -1 && isDef2 == -1) printf("Error: Symbol %s undefined.\n", sym_name);
        else if (isDef1 == -1 && isDef2 == 0) printf("Error: Symbol %s undefined.\n", sym_name);
        else if (isDef1 == 1 && isDef2 == 1) printf("Error: Symbol %s multiply defined.\n", sym_name);
    }
    return 0;
}

void check_files_for_merge() {
    ELF *elf;
    int i, j, counter;
    Elf32_Sym *symtab1, *symtab2;
    int symtab_len1, symtab_len2;
    Elf32_Ehdr *elfH;
    Elf32_Shdr *elfSH;
    int shnum;
    int symtab_len;
    Elf32_Sym *symtab = NULL;
    char *sh_name;

    if (elf1.fd == -1 || elf2.fd == -1) {
        printf("Error: should load 2 ELF files first\n");
        return;
    }
    // check each elf has exactly one symbol table and find it
    for (i = 1; i <= 2; i++) {
        if (i == 1) elf = &elf1;
        else elf = &elf2;

        elfH = (Elf32_Ehdr *)elf->map_start;
        elfSH = (Elf32_Shdr *)(elf->map_start + elfH->e_shoff);
        shnum = elfH->e_shnum;
        counter = 0;

        if (elf->strSHTab == NULL) update_strSHTab(elf);
        if (elf->strSymTab == NULL) update_strSymTab(elf);
        
        for (j = 0; j < shnum; j++) {
            sh_name = (char *)(elf->strSHTab + elfSH[j].sh_name);
            if (strcmp(sh_name, ".symtab") == 0) {
                symtab = (Elf32_Sym *)(elf->map_start + elfSH[j].sh_offset);
                symtab_len = elfSH[j].sh_size / sizeof(Elf32_Sym);
                counter++;
            }
        }

        if (counter != 1 || elf->strSymTab == NULL) {
            printf("feature not supported");
            return;
        }
        if (i == 1) {
            symtab1 = symtab;
            symtab_len1 = symtab_len;
        }
        else {
            symtab2 = symtab;
            symtab_len2 = symtab_len;
        }
    }
    compare_symtabs(symtab1, symtab2, symtab_len1, symtab_len2, elf1.strSymTab, elf2.strSymTab);
    compare_symtabs(symtab2, symtab1, symtab_len2, symtab_len1, elf2.strSymTab, elf1.strSymTab);
    printf("Finish check\n");
}

void add_section(char *section_name, int mergedELF, int section_size_1, Elf32_Shdr *dynamicElfSH, int indexInShTable) {
    int j, section_size_2;
    char *sh_name;
    Elf32_Ehdr *elfH = (Elf32_Ehdr *)elf2.map_start;
    Elf32_Shdr *elfSH = (Elf32_Shdr *)(elf2.map_start + elfH->e_shoff);
    Elf32_Shdr *found_section = NULL;
    int shnum = elfH->e_shnum;
    for (j = 0; j < shnum; j++) {
        sh_name = (char *)(elf2.strSHTab + elfSH[j].sh_name);
        if (strcmp(sh_name, section_name) == 0) {
            found_section = elf2.map_start + elfSH[j].sh_offset;
            section_size_2 = elfSH[j].sh_size;
            break;
        }
    }
    if (found_section != NULL) {
        dynamicElfSH[indexInShTable].sh_size = section_size_2 + section_size_1;
        lseek(mergedELF, 0, SEEK_END);
        write(mergedELF, found_section, section_size_2);
    }
}

void handleSymbols(Elf32_Sym *symtab1, int symtab_len1) {
    int i, j;
    Elf32_Sym *symtab2 = NULL;
    int symtab_len2;
    Elf32_Ehdr *elfH = (Elf32_Ehdr *)elf2.map_start;
    Elf32_Shdr *elfSH = (Elf32_Shdr *)(elf2.map_start + elfH->e_shoff);
    int shnum = elfH->e_shnum;
    char *sh_name, *sym_name, *sym_name2;
    // find symtab2
    for (j = 0; j < shnum; j++) {
        sh_name = (char *)(elf2.strSHTab + elfSH[j].sh_name);
        if (strcmp(sh_name, ".symtab") == 0) {
            symtab2 = (Elf32_Sym *)(elf2.map_start + elfSH[j].sh_offset);
            symtab_len2 = elfSH[j].sh_size / sizeof(Elf32_Sym);
            break;
        }
    }
    if (symtab2 == NULL) return;
    // merge symbols
    for (i = 1; i < symtab_len1; i++) {
        sym_name = (char *)elf1.strSymTab + symtab1[i].st_name;
        if (strcmp(sym_name, "") == 0) continue;
        if (symtab1[i].st_shndx == SHN_UNDEF) {
            // find sym in symtab2
            for (j = 1; j < symtab_len2; j++) {
                sym_name2 = (char *)elf2.strSymTab + symtab2[j].st_name;
                if (strcmp(sym_name, sym_name2) == 0) {
                    symtab1[i].st_value = symtab2[j].st_value;
                    symtab1[i].st_info = symtab2[j].st_info;
                    symtab1[i].st_other = symtab2[j].st_other;
                    symtab1[i].st_shndx = symtab2[j].st_shndx;
                    break;
                }
            }
        }
    }
}

void updateAllElfs() {
    if (elf1.strSHTab == NULL) update_strSHTab(&elf1);
    if (elf1.strSymTab == NULL) update_strSymTab(&elf1);
    if (elf2.strSHTab == NULL) update_strSHTab(&elf2);
    if (elf2.strSymTab == NULL) update_strSymTab(&elf2);
}

void merge_ELF_files() {
    int mergedELF;
    Elf32_Ehdr *elfH = (Elf32_Ehdr *)elf1.map_start;
    Elf32_Shdr *elfSH = (Elf32_Shdr *)(elf1.map_start + elfH->e_shoff);
    int shnum = elfH->e_shnum, i;
    Elf32_Shdr *dynamicElfSH, *curr_sh;
    char *sh_name;
    int section_size;
    Elf32_Sym *symtab;
    int symtab_len;
    Elf32_Off fileP;
    int bytesOfWrite;
    if (elf1.fd == -1 || elf2.fd == -1) {
        printf("Error: should load 2 ELF files first\n");
        return;
    }
    updateAllElfs();
    mergedELF = open("out.ro", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (mergedELF == -1) {
        perror("Error opening file");
        return;
    }
    // copy headers to the new elf
    bytesOfWrite = write(mergedELF, elfH, sizeof(Elf32_Ehdr));
    if (bytesOfWrite != sizeof(Elf32_Ehdr)) {
        perror("Failed to write ELF header to output file");
        close(mergedELF);
        return;
    }
    // creat a copy of section headers in memory
    dynamicElfSH = (Elf32_Shdr *)malloc(sizeof(Elf32_Shdr) * shnum);
    memcpy(dynamicElfSH, elfSH, sizeof(Elf32_Shdr) * shnum);
    for(i = 0; i < shnum; i++) {
        sh_name = (char *)(elf1.strSHTab + elfSH[i].sh_name);
        if (strcmp(sh_name, ".symtab") == 0) { // handle sybol tabale
            symtab = (Elf32_Sym *)(elf1.map_start + elfSH[i].sh_offset);
            symtab_len = elfSH[i].sh_size / sizeof(Elf32_Sym);
            handleSymbols(symtab, symtab_len);
        }
        // each section is copyed to the mergedELF 
        fileP = lseek(mergedELF, 0, SEEK_END);
        dynamicElfSH[i].sh_offset = fileP;
        section_size = elfSH[i].sh_size;
        curr_sh = elf1.map_start + elfSH[i].sh_offset;
        write(mergedELF, curr_sh, section_size);
        if(strcmp(sh_name, ".text") == 0 || strcmp(sh_name, ".data") == 0 || strcmp(sh_name, ".rodata") == 0) {
            add_section(sh_name, mergedELF, section_size, dynamicElfSH, i);
        }
    }
    // write the merged (and modified) section header
    fileP = lseek(mergedELF, 0, SEEK_END);
    write(mergedELF, dynamicElfSH, sizeof(Elf32_Shdr)*shnum);
    // fix the "e_shoff" field in ELF header 
    lseek(mergedELF, 0, SEEK_SET);
    lseek(mergedELF, 0x20, SEEK_SET);
    write(mergedELF, &fileP, sizeof(Elf32_Off));
    // done
    close(mergedELF);
    free(dynamicElfSH);
    printf("Done\n");
}

void initialELFs() {
    elf1.fd = -1;
    elf2.fd = -1;
    elf1.strSHTab = NULL;
    elf1.strSymTab = NULL;
    elf2.strSHTab = NULL;
    elf2.strSymTab = NULL;
}

int main(int argc, char **argv) {
    int i = -1, num = -1, bound, sscan;
    char input[10];
    // 0-Toggle Debug Mode
    // 1-Examine ELF File
    // 2-Print Section Names
    // 3-Print Symbols
    // 4-Check Files for Merge
    // 5-Merge ELF Files
    // 6-Quit
    struct fun_desc menu[] = { 
        { "Toggle Debug Mode", toggle_debug_mode },
        { "Examine ELF File", examine_ELF_file },
        { "Print Section Names", print_section_names }, 
        { "Print Symbols", print_symbols },
        { "Check Files for Merge", check_files_for_merge },
        { "Merge ELF Files", merge_ELF_files },
        { "Quit", quit },
        { NULL, NULL } };
    bound = (sizeof(menu) - sizeof(menu[0])) / sizeof(menu[0]);
    initialELFs();

    do {
        if (i != -1) { // not the first time of the loop
            sscan = sscanf(input, "%d\n", &num);
            if (sscan == EOF || sscan == -1 || num < 0 || num >= bound) { // invalid input
                fprintf(stderr, "Not within bounds\n");
                exit(1);
            }
            else { // valid input
                printf("Within bounds\n\n");
                menu[num].fun_ptr();
                printf("\n");
            }
        }
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