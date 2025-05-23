




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
;mov rdx,0
mov rbx,0

mov rsi,0
mov rdi,0

mov rax,0

call rdx

LiunuxOS64Entry endp


LiunuxOS64Leave proc 

ret

LiunuxOS64Leave endp


END
