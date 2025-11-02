




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

cli
sub rsp,100h
mov word ptr [rsp],127
mov dword ptr [rsp + 2], r8
lgdt fword ptr [esp]

mov [esp],0eah

mov [esp + 1],edx
mov word ptr [esp + 5],cx
jmp esp

LiunuxOS64Leave endp


END
