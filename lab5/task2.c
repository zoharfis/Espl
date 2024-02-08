#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <elf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

extern int startup(int argc, char **argv, void (*start)());

int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *,int), int arg) {
    Elf32_Ehdr *elfH =  (Elf32_Ehdr *)map_start;
    Elf32_Phdr *elfPH = (Elf32_Phdr *)(map_start + elfH->e_phoff); 
    int phNum = elfH->e_phnum;
    int i;
    for (i = 0; i < phNum; i++) {
        func(&elfPH[i], arg);
    }
    return 0;
}

void printPHAddress(Elf32_Phdr *ph, int i) {
    printf("Program header number %d at address 0x%x\n", i, ph->p_vaddr);
}

char *typeToString(Elf32_Word phType) {
    switch (phType) {
        case PT_NULL: return "NULL";
        case PT_LOAD: return "LOAD";
        case PT_DYNAMIC: return "DYNAMIC";
        case PT_INTERP: return "INTERP";
        case PT_NOTE: return "NOTE";
        case PT_SHLIB: return "SHLIB";
        case PT_PHDR: return "PHDR";
        case PT_TLS: return "TLS";
        default: return "Unknown";
    }
}

char *flagsToString(Elf32_Word phFlags) {
    static char flagsStr[4];
    int index = 0;
    if (phFlags & PF_R)
        flagsStr[index++] = 'R';
    else flagsStr[index++] = ' ';
    if (phFlags & PF_W)
        flagsStr[index++] = 'W';
    else flagsStr[index++] = ' ';
    if (phFlags & PF_X)
        flagsStr[index++] = 'X';
    else flagsStr[index++] = ' ';
    flagsStr[index] = '\0';  // Null-terminate the string
    return flagsStr;
}

void printPH(Elf32_Phdr *ph, int i) {
    Elf32_Word phFlags = ph->p_flags;
    char *flagsStr = flagsToString(phFlags);
    printf("%s           0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %s 0x%04x\n",
    typeToString(ph->p_type), ph->p_offset, ph->p_vaddr, ph->p_paddr, ph->p_filesz, ph->p_memsz, flagsStr, ph->p_align);
}

int flagsToProtFlgMmap(int phFlags) {
    int output = 0;
    if (phFlags & PF_R)
        output = output | PROT_READ;
    if (phFlags & PF_W)
        output = output | PROT_WRITE;
    if (phFlags & PF_X)
        output = output | PROT_EXEC;
    return output;
}

char *protFlgMmapToString(int protFlgMmap) {
    switch (protFlgMmap)
    {
    case 7: return "PROT_READ|PROT_WRITE|PROT_EXEC";
    case 6: return "PROT_WRITE|PROT_EXEC";
    case 5: return "PROT_READ|PROT_EXEC";
    case 4: return "PROT_EXEC";
    case 3: return "PROT_READ|PROT_WRITE";
    case 2: return "PROT_WRITE";
    case 1: return "PROT_READ";
    case 0: return "";
    default:
        return "";
    }
}

int flagsToMappingFlg(int phFlags) {
    int output = 0;
    if ((phFlags & PF_R))
        output = output | MAP_PRIVATE;
    // if (phFlags & PF_W) {
    //     // output = 0;
    //     // output = output | MAP_SHARED;
    // }
    if ((phFlags & PF_X) | (phFlags & PF_W))
        output = output | MAP_PRIVATE | MAP_FIXED;
    return output;
}

char *mappingFlgToString(int mappingFlg) {
    switch (mappingFlg)
    {
    case 19: return "MAP_PRIVATE|MAP_SHARED|MAP_FIXED";
    case 18: return "MAP_PRIVATE|MAP_FIXED";
    case 17: return "MAP_SHARED|MAP_FIXED";
    case 16: return "MAP_FIXED";
    case 3: return "MAP_PRIVATE|MAP_SHARED";
    case 2: return "MAP_PRIVATE";
    case 1: return "MAP_SHARED";
    case 0: return "";
    
    default:
        return "";
    }
}

void printPHwithMmap(Elf32_Phdr *ph, int i) {
    Elf32_Word phFlags = ph->p_flags;
    int protFlgMmap = flagsToProtFlgMmap(phFlags), mappingFlg = flagsToMappingFlg(phFlags);
    char *protMapStr = protFlgMmapToString(protFlgMmap), *mappingStr = mappingFlgToString(mappingFlg);
    char *flagsStr = flagsToString(phFlags);

    printf("%s           0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %s 0x%04x ",
    typeToString(ph->p_type), ph->p_offset, ph->p_vaddr, ph->p_paddr, ph->p_filesz, ph->p_memsz, flagsStr, ph->p_align);

    printf("%s %s\n", protMapStr, mappingStr);
}

void printAllPH(void *map_start) {
    printf("Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align\n");
    foreach_phdr(map_start, printPH, 0);
}

void printAllWithMMap(void *map_start) {
    printf("Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align   ProtMP  MPFlg\n");
    foreach_phdr(map_start, printPHwithMmap, 0);
}

void load_phdr(Elf32_Phdr *phdr, int fd) {
    void *map_start;
    Elf32_Off offset;
    Elf32_Addr padding, vaddr;
    if (phdr->p_type == PT_LOAD) {
        vaddr = phdr->p_vaddr & 0xfffff000;
        offset = phdr->p_offset & 0xfffff000;
        padding = phdr->p_vaddr & 0xfff;
        map_start = mmap((void *)vaddr, 
                            phdr->p_memsz + padding, 
                            flagsToProtFlgMmap(phdr->p_flags),
                            MAP_FIXED | MAP_PRIVATE, //flagsToMappingFlg(phdr->p_flags), 
                            fd, offset);
        if (map_start == MAP_FAILED) {
            perror("mmap failed");
            return;
        }
        printf("phdr mmapped.\nDetails:\n");
        printf("pointer: %p\n", map_start);
        printf("Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align\n");
        printPH(phdr, 0);
    }
}

void loadPHs(void *map_start, int fd) {
    foreach_phdr(map_start, load_phdr, fd);
}

int main(int argc, char **argv) {
    int fd, fileSize;
    void *map_start;
    Elf32_Ehdr *elfH;
    if (argc < 2) {
        fprintf(stderr, "ELF file name should be first argument\n");
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }
    fileSize = lseek(fd, 0, SEEK_END);
    map_start = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    printAllWithMMap(map_start);
    loadPHs(map_start, fd);
    elfH =  (Elf32_Ehdr *)map_start;
    startup(argc-1, argv+1, (void *)(elfH->e_entry));

    munmap(map_start, fileSize);
    close(fd);
    return 0;
}