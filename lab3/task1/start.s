EXIT EQU 1
READ EQU 3
WRITE EQU 4
OPEN EQU 5
CLOSE EQU 6
STDIN EQU 0
STDOUT EQU 1
STDERR EQU 2
O_RDONLY EQU 0
O_WRONLY EQU 1
O_CREATE EQU 64
O_RDRW EQU 2
O_TRUNC EQU 512

section .data
    newline db 10 
    Infile dd STDIN
    Outfile dd STDOUT
section .bss
    buffer resb 1

section .text
    global _start
    global system_call
extern strlen
_start:
    pop    dword ecx    ; ecx = argc
    mov    esi,esp      ; esi = argv
    ;; lea eax, [esi+4*ecx+4] ; eax = envp = (4*ecx)+esi+4
    mov     eax,ecx     ; put the number of arguments into eax
    shl     eax,2       ; compute the size of argv in bytes
    add     eax,esi     ; add the size to the address of argv 
    add     eax,4       ; skip NULL at the end of argv
    push    dword eax   ; char *envp[]
    push    dword esi   ; char* argv[]
    push    dword ecx   ; int argc

    call    main        ; int main( int argc, char *argv[], char *envp[] )

    mov     ebx,eax
    mov     eax,EXIT
    int     0x80
    nop
        
system_call:
    push    ebp             ; Save caller state
    mov     ebp, esp
    sub     esp, 4          ; Leave space for local var on stack
    pushad                  ; Save some more caller state

    mov     eax, [ebp+8]    ; Copy function args to registers: leftmost...        
    mov     ebx, [ebp+12]   ; Next argument...
    mov     ecx, [ebp+16]   ; Next argument...
    mov     edx, [ebp+20]   ; Next argument...
    int     0x80            ; Transfer control to operating system
    mov     [ebp-4], eax    ; Save returned value...
    popad                   ; Restore caller state (registers)
    mov     eax, [ebp-4]    ; place returned value where caller can see it
    add     esp, 4          ; Restore caller state
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller

main: 
    push ebp
    mov ebp, esp
argvs:
    mov ecx, [ebp+8] ; Get first argumens argc
    mov edx, [ebp+12] ; Get second arguments argv
    mov ebx, 0 ; ebx is counter
nextArg:
    push ebx
    push ecx
    push edx
    mov ecx, [edx+ebx*4] ; curr argument
    push ecx
checkInRedirect:
    cmp word [ecx], "-i"
    jnz checkOutRedirect
    add ecx, 2
    ; open input
    mov eax, OPEN
    mov ebx, ecx        ; file to open
    mov ecx, O_RDONLY   ; flag of read
    int 0x80
    mov [Infile], eax
    jmp printArg
checkOutRedirect:
    cmp word [ecx], "-o"
    jnz printArg
    add ecx, 2
    ; open output
    mov eax, OPEN
    mov ebx, ecx        ; file to open
    mov ecx, O_WRONLY | O_CREATE | O_TRUNC
    mov edx, 0q700 ; rwx permission for user
    int 0x80
    mov [Outfile], eax
printArg:
    pop ecx
    push ecx             ; parameter for strlen
    call strlen          ; len in eax
    add esp, 4           ; instead pop
    ; prepare for Write
    mov edx, eax         ; strlen
    mov eax, WRITE 
    mov ebx, STDERR
    int 0x80             ; print arg
    mov eax, WRITE
    mov ecx, newline
    mov edx, 1           ; len of newline
    int 0x80             ; print newline
    pop edx
    pop ecx
    pop ebx
    inc ebx              ; counter++
    cmp ebx, ecx         ; check if its end of argv
    jnz nextArg
; encoder
read:
    mov eax, READ
    mov ebx, [Infile]
    mov ecx, buffer
    mov edx, 1
    int 0x80
    cmp byte [buffer], 0 ; check if buffer is null
    jz finish
    cmp eax, 0
    jz finish
encode:
    mov al, [buffer]
    cmp al, 'A'
    jl print
    cmp al, 'Z'
    jle upper
    cmp al, 'a'
    jl print
    cmp al, 'z'
    jle lower
    jmp print
upper:
    cmp al, 'Z'
    jz ZCase
    inc al
    jmp print
ZCase: 
    mov al, 'A'
    jmp print
lower:
    cmp al, 'z'
    jz zCase
    inc al
    jmp print
zCase: 
    mov al, 'a'
    jmp print
print:
    mov [buffer], al
    mov eax, WRITE
    mov ebx, [Outfile]
    mov ecx, buffer
    mov edx, 1
    int 0x80 ; print
    jmp read
finish:
close:
    mov eax, CLOSE
    mov ebx, [Infile]
    int 0x80
    mov eax, CLOSE
    mov ebx, [Outfile]
    int 0x80
done:
    mov esp, ebp
    pop ebp
    ret 