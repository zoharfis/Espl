section .data
    x_struct: db 5
    x_num: db 0xaa, 1,2,0x44,0x4f
    y_struct: db 6
    y_num: db 0xaa, 1,2,3,0x44,0x4f
    isOdd: db 0
    isLeadZero: db 1
    structP: dd 0
    STATE: dw 0xACE1
    MASK: dw 0x002d
section .rodata
    format: db "%02hhx", 0
    newline: db 10, 0
section .bss
    buffer resb 600
section .text
    extern printf
    extern puts
    extern fgets
    extern strlen
    extern malloc
    extern free
    extern stdin
    global main

main:
    push ebp
    mov ebp, esp
    mov ecx, [ebp+8] ; Get first argumens argc
    mov edx, [ebp+12] ; Get second arguments argv
    cmp ecx, 1
    jz xyStructs
    mov ebx, [edx+4]
    cmp word [ebx], "-I"
    jz getStructs
    cmp word [ebx], "-R"
    jz generateStructs
    jmp doneMain
xyStructs:
    push dword x_struct
    call print_multi
    add esp, 4
    push dword y_struct
    call print_multi
    add esp, 4
    push dword x_struct
    push dword y_struct
    call add_multi
    add esp, 8
    push eax
    push eax
    call print_multi
    add esp, 4
    ; free malloc
    pop eax
    push eax
    call free
    add esp, 4
    jmp doneMain
getStructs:
    call get_multi
    mov ebx, eax
    call get_multi
    mov ecx, eax
    push ebx
    call print_multi
    add esp, 4
    push ecx
    call print_multi
    add esp, 4
    push ebx
    push ecx
    call add_multi
    add esp, 8
    push eax
    push ebx
    push ecx
    push eax
    call print_multi
    add esp,4
    ; free malloc
    pop ecx
    push ecx
    call free
    add esp, 4
    pop ebx
    push ebx
    call free
    add esp, 4
    pop eax
    push eax
    call free
    add esp, 4
    jmp doneMain
generateStructs:
    call PRmulti
    mov ebx, eax
    call PRmulti
    mov ecx, eax
    push ebx
    call print_multi
    add esp, 4
    push ecx
    call print_multi
    add esp, 4
    push ebx
    push ecx
    call add_multi
    add esp, 8
    push eax
    push ebx
    push ecx
    push eax
    call print_multi
    add esp,4
    ; free malloc
    pop ecx
    push ecx
    call free
    add esp, 4
    pop ebx
    push ebx
    call free
    add esp, 4
    pop eax
    push eax
    call free
    add esp, 4
doneMain:
    mov esp, ebp
    pop ebp
    ret

print_multi:
    push ebp
    mov ebp, esp
    pushad
    mov byte [isLeadZero], 1
    mov ebx, [ebp+8] ; pointer to struct multi
    mov eax, 0
    mov al, byte [ebx] ; size
printNext:
    mov ecx, 0
    mov cl, [ebx+eax*1] ; curr element in array
    cmp byte [isLeadZero], 1
    jnz print
    cmp cl, 0
    jz afterPrint
    mov byte [isLeadZero], 0
print:
    pushad
    push ecx ; number to print
    push dword format ; string to print
    call printf
    add esp, 8
    popad
afterPrint:
    sub al, 1
    cmp al, 0
    jnz printNext
    cmp byte [isLeadZero], 1 ; number is zero
    jnz donePrint
    mov ecx, 0
    push ecx ; number to print
    push dword format ; string to print
    call printf
    add esp, 8
donePrint:
    push dword newline
    call printf
    add esp, 4
    popad
    mov esp, ebp
    pop ebp
    ret

get_multi:
    push ebp
    mov ebp, esp
    pushad
    mov byte [isOdd], 0
    push dword [stdin]
    push dword 600
    push dword buffer
    call fgets
    add esp, 12
    push buffer
    call strlen
    add esp, 4
    dec eax
    mov ebx, eax
    and ebx, 1
    cmp ebx, 1
    jnz initialStruct
    ; odd len
    mov byte [isOdd], 1
    add eax, 1
initialStruct:
    shr eax, 1
    mov ebx, eax
    inc eax
    push ebx
    push eax
    call malloc
    add esp, 4
    pop ebx
    mov [structP], eax
    mov byte [eax], bl
    mov esi, 0
    cmp byte [isOdd], 1
    jnz loopStruct
    mov ecx, 0
    mov cl, byte [buffer+esi]
    push ecx
    call digitToDec
    add esp, 4
    mov ecx, eax
    mov eax, dword [structP]
    mov byte [eax+ebx], cl
    dec ebx
    inc esi
loopStruct: ; ebx-counter down, esi - counter plus, 
    cmp ebx, 0
    jz doneLoopStruct
    mov edx, 0
    mov dl, byte [buffer+esi]
    inc esi
    push edx 
    call digitToDec
    add esp, 4
    mov edx, eax
    mov ecx, 0
    mov cl, byte [buffer+esi]
    inc esi
    push ecx 
    call digitToDec
    add esp, 4
    mov ecx, eax
    shl edx, 4
    add ecx, edx
    mov eax, [structP]
    mov byte [eax + ebx], cl
    dec ebx
    jmp loopStruct
doneLoopStruct:
    popad
    mov eax, [structP]
    mov esp, ebp
    pop ebp
    ret

digitToDec:
    push ebp
    mov ebp, esp
    push ecx
    mov cl, [ebp+8] ; digit
    sub cl, '0'
    cmp cl, 9
    jle doneDigitToDec
    add cl, '0'
    sub cl, 'a'
    add cl, 10
doneDigitToDec:
    mov eax, 0
    mov al, cl
    pop ecx
    mov esp, ebp
    pop ebp
    ret

getMaxMin:
    push ecx
    push edx
    mov ecx, 0
    mov cl, byte [eax]
    mov edx, 0
    mov dl, byte [ebx]
    cmp dl, cl
    jle doneGetMaxMin
    mov ecx, eax
    mov eax, ebx
    mov ebx, ecx
doneGetMaxMin:
    pop edx
    pop ecx
    ret

add_multi:
    push ebp
    mov ebp, esp
    pushad
    mov eax, [ebp+8] ; first pointer 
    mov ebx, [ebp+12] ; second pointer
    call getMaxMin
    mov ecx, 0
    mov cl, byte [eax] 
    add cl, 2
    pushad
    push ecx
    call malloc
    add esp, 4
    mov [structP], eax
    popad
    dec cl
    mov edx, [structP]
    mov byte [edx], cl
    dec cl ; size of big array
    mov esi, 1 ; index in array
    mov edx, 0
    mov dl, byte [ebx] ; size of small array
    and al, al ; clear CF
add2Loop: ; eax-big array, ebx-lit array, cl- big size, dl - lit size. esi=index
    push ecx
    push edx
    mov ecx, 0
    mov cl, byte [ebx+esi]
    adc cl, byte [eax+esi]
    mov edx, [structP]
    mov byte [edx+esi], cl
    inc esi
    pop edx
    pop ecx
    dec cl
    jz finishAddLoop
    dec dl
    jz add1Loop
    jmp add2Loop
add1Loop:
    push ecx
    mov ecx, 0
    adc cl, byte [eax+esi]
    mov edx, [structP]
    mov byte [edx+esi], cl
    inc esi
    pop ecx
    dec cl
    jz finishAddLoop
    jmp add1Loop
finishAddLoop:
    mov ecx, 0 
    adc cl, cl
    mov edx, [structP]
    mov byte [edx+esi], cl
    popad
    mov eax, [structP]
    mov esp, ebp
    pop ebp
    ret

rand_num:
    push ebx
    push ecx
    mov ecx, 0
    mov ax, [MASK]
    mov bx, [STATE]
    mov cx, bx
    and cx, 1 ; cx is output bit
    and bx, ax
    jp even
    ; odd
    mov bx, [STATE]
    or bx, 1
    ror bx, 1
    jmp doneRand
even:
    mov bx, [STATE]
    shr bx, 1
doneRand:
    mov word [STATE], bx
    mov eax, ecx ; return value
    pop ecx
    pop ebx
    ret

PRmulti:
    pushad
generateN:
    call generate8bits
    cmp eax, 0
    jz generateN
    ; initial struct
    mov ebx, eax
    push ebx
    inc ebx
    push ebx
    call malloc
    add esp, 4
    pop ebx
    mov [structP], eax
    mov byte [eax], bl
loopGenerateNum:
    cmp ebx, 0
    jz doneGenerateStruct
    call generate8bits
    mov edx, [structP]
    mov byte [edx+ebx], al
    dec ebx
    jmp loopGenerateNum
doneGenerateStruct:
    popad
    mov eax, [structP]
    ret

generate8bits:
    push edx
    push ebx
    mov edx, 8 ; counter
    mov ebx, 0 ; accumulator
loopGenerate8:
    cmp edx, 0
    jz doneGenerate8
    shl ebx, 1
    call rand_num
    ; eax is the new bit
    or ebx, eax
    dec edx
    jmp loopGenerate8
doneGenerate8:
    mov eax, ebx
    pop ebx
    pop edx
    ret