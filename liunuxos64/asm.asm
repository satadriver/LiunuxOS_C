




LiunuxOS64Entry PROTO 




.data


.code 



LiunuxOS64Entry proc 

mov rsp,rcx
mov rbp,rsp

mov ax,16
mov ds,ax
mov es,ax
mov fs,ax
mov gs,ax
mov ss,ax

mov rax,0
mov rcx,0

mov rbx,0

mov rsi,0
mov rdi,0

mov rax,0

call rdx

LiunuxOS64Entry endp


LiunuxOS64Leave proc 

mov rax,r8
mov dword ptr [__mode32_address_entry],eax

mov rax, cr0
and eax, 7fffffffh
mov cr0, rax

mov rcx, 0C0000080h
rdmsr
and eax, 0fffffeffh
wrmsr

mov eax,edx
shl rax,16
or rax,0ffh
push rax
lgdt fword ptr [rsp]

mov rax, rcx
mov cr3, rax

mov rax, cr0
or eax, 80000000h
mov cr0, rax

db 0eah
__mode32_address_entry:
dd 0
dw 8

LiunuxOS64Leave endp


END
