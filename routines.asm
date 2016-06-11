; Notes on System V ABI:
; pointer/int argument passing order:
;   rdi, rsi, rdx, rccx, r8, r9
; pointer/int returning order:
;   rax, rdx
; free registers (no need to push/pop):
;   rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11

;global _bimul       ; void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs)
;global _biadd       ; void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs)
;global _bisub       ; void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs)
global _biinc        ; void biinc(BigInt* op)

section .text

;_bimul:
; rdi = BigInt* dst
; rsi = BigInt* lhs
; rdx = BigInt* rhs
;ret

;_biadd:
; rdi = BigInt* dst
; rsi = BigInt* lhs
; rdx = BigInt* rhs
;ret

;_bisub:
; rdi = BigInt* dst
; rsi = BigInt* lhs
; rdx = BigInt* rhs
;ret

_biinc:
; rdi = BigInt* op
xor rcx, rcx
mov rdx, [rdi]              ; rdx = ptr to first word
mov ecx, [rdi + 8]          ; rcx = # of 64-bit words
dec rcx
shl rcx, 3                  ; ecx = byte offset of least sig word
mov rsi, [rdi]              ; rsi points to first word
add rsi, rcx                ; rsi poins to last word
add qword [rsi], 1          ; increment lowest word
.loop:
jnc .done                   ; only need to enter loop if carry is set
sub rsi, 8
add qword [rsi], 1
cmp rsi, rdx                ; did the most significant word just get ++?
je .done
jmp .loop
.done:
ret

