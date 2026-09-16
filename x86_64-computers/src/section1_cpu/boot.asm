
; Force the section to be Allocatable, Executable, and Progbits
section .multiboot alloc exec progbits
align 8
header_start:
    dd 0xe85250d6                ; Magic: multiboot 2
    dd 0                         ; Architecture: i386 (protected mode)
    dd header_end - header_start ; Header length
    ; Checksum math
    dd -(0xe85250d6 + 0 + (header_end - header_start))

    ; End tag (required by Multiboot 2)
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
header_end:

[bits 32]
section .text
global _start
extern kernel_main

_start:
    cli
    ; Use EAX to load the 32-bit physical address of the GDT
    mov eax, gdt32_ptr          
    lgdt [eax]                  ; This avoids the 16-bit truncation error
    
    mov eax, cr0
    or eax, 1                   ; Set PE bit
    mov cr0, eax
    
    ; Use a 32-bit jump to reach the 1MB range
    jmp dword 0x08:init_32      

[bits 32]
init_32:
    mov ax, 0x10                ; 0x10 is 32-bit Data Segment
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, stack_top          ; Point to the stack we reserved below

    ; --- Paging Setup (Identity Mapping first 10MB) ---
    mov edi, pml4
    xor eax, eax
    mov ecx, 3072                
    rep stosd                   ; Clear PML4, PDPT, and PDT

    mov eax, pdpt
    or eax, 0x3                 ; Present + Write
    mov [pml4], eax              

    mov eax, pdt
    or eax, 0x3
    mov [pdpt], eax              
    
    ; --- Dynamic 2MB Huge Pages Mapping ---
    ; Let's map 32 pages (64MB total RAM) using a tiny loop so my kernel has room to breathe!
    mov edi, pdt
    mov eax, 0x00000083         ; Starting physical address 0x00000000 + Present + Writable + HugePage
    mov ecx, 32                 ; 32 pages * 2MB per page = 64MB total mapping area
.map_kernel_pages:
    mov [edi], eax              ; Write low 32-bits of the entry
    mov dword [edi + 4], 0      ; Clear high 32-bits of the entry
    add eax, 0x00200000         ; Add 2MB to physical address pointer
    add edi, 8                  ; Jump to next 64-bit page entry in the table
    loop .map_kernel_pages


    ; --- Enter Long Mode ---
    mov eax, cr4
    or eax, 1 << 5              ; Enable PAE
    mov cr4, eax

    mov eax, pml4
    mov cr3, eax                ; Load PML4

    mov ecx, 0xC0000080         ; EFER MSR
    rdmsr
    or eax, 1 << 8              ; LME bit
    wrmsr

    mov eax, cr0
    or eax, 1 << 31             ; Enable Paging
    mov cr0, eax

    lgdt [gdt64_ptr]            ; Load 64-bit GDT
    jmp 0x18:init_64            ; Leap to 64-bit!

[bits 64]
init_64:
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov es, ax
    
    call kernel_main            ; Welcome to ScribbleOS 4.0 64-bit

.halt:
    hlt
    jmp .halt

; --- Data & Tables ---
section .rodata
align 16
gdt32:
    dq 0
    dq 0x00CF9A000000FFFF       ; 32-bit Code
    dq 0x00CF92000000FFFF       ; 32-bit Data
gdt32_ptr:
    dw $ - gdt32 - 1
    dd gdt32

align 16
gdt64:
    dq 0                        ; 0x00 Null
    dq 0                        ; 0x08 Unused
    dq 0                        ; 0x10 Unused
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)       ; 0x18 Kernel Code
    dq (1<<44) | (1<<47) | (3<<45)                 ; 0x20 Kernel Data
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) | (3<<45) ; 0x28 User Code
    dq (1<<44) | (1<<47) | (3<<45)                 ; 0x30 User Data

    ; --- 16-Byte TSS Descriptor (Index 0x38) ---
    ; Low 8 bytes
    dw 0x67                         ; Limit: 103 bytes
    global tss_base_low
    tss_base_low:   dw 0            ; Base bits 0-15
    global tss_base_mid
    tss_base_mid:   db 0            ; Base bits 16-23
    db 0x89                         ; Access: Present, Ring 0, TSS
    db 0x00                         ; Flags
    global tss_base_high
    tss_base_high:  db 0            ; Base bits 24-31
    ; High 8 bytes
    global tss_base_upper
    tss_base_upper: dd 0            ; Base bits 32-63
    dd 0                            ; Reserved


gdt64_ptr:
    dw $ - gdt64 - 1
    dq gdt64

section .bss
align 4096
pml4: resb 4096
pdpt: resb 4096
pdt:  resb 4096
stack_bottom:
    resb 16384                  ; 16KB Stack
global stack_top
stack_top:
