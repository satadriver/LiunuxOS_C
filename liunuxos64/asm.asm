




LiunuxOS64Entry PROTO 




.data


.code 

public LiunuxOS64Entry



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



public LiunuxOS64Leave



LiunuxOS64Leave proc 

cli

sub rsp,100h

mov word ptr [rsp],0ffh

mov rax,r8
mov dword ptr [rsp + 2], eax

lgdt fword ptr [rsp]

mov byte ptr [esp],0eah
mov dword ptr [rsp + 1],edx
mov word ptr [rsp + 5],cx

mov rax,rsp

mov rsp,r9

jmp  rax

LiunuxOS64Leave endp



END
