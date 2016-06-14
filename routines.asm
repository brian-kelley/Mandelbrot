; Notes on System V ABI:
; pointer/int argument passing order:
;   rdi, rsi, rdx, rccx, r8, r9
; pointer/int returning order:
;   rax, rdx
; free registers (no need to push/pop):
;   rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11

global _bimul        ; void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs)
global _biadd        ; void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs)
global _bisub        ; void bimul(BigInt* dst, BigInt* lhs, BigInt* rhs)
global _biinc        ; void biinc(BigInt* op)

section .text

_bimul:
push r12
; rdi = BigInt* dst
; rsi = BigInt* lhs
; rdx = BigInt* rhs
mov r11, rdi
; r11 = &dst
mov r10, rdx
; r10 = &rhs
mov rdi, [r11]              ; rdi points to dst words
mov ecx, [r10 + 8]          ; ecx = # of words
mov edx, ecx                ; save a copy of # of words to r9 (ecx gets lost)
mov r9, rdx
shl rcx, 1                  ; rcx needs the # of dwords to zero out
xor eax, eax                ; eax has the 0 to write out
rep stosd                   ; writes eax to ecx dwords starting at edi
; now dst words are all 0 
mov r11, [r11]
mov rsi, [rsi]
mov r10, [r10]
; r11 = dst->val
; rsi = lhs->val
; r10 = rhs->val
; r9 = # of qwords to operate on
; rax, rcx, rdx, rdi, r8 free to use

; vars needed here:
;   dst starting word (ptr) ; r11
;   lhs starting word (ptr) ; rsi
;   rhs starting word (ptr) ; r10
;   dst word index          ; rbx
;   lhs word index          ; r12
;   rhs word index          ; r8
;   eax free for mulq       ; rax
;   edx free for mulq       ; rdx
;   number of words         ; r9
;   lhs word address        ; rcx
;   rhs word address        ; rdi

xor r12, r12                ; zero out loop i
xor r8, r8
.iloop:                     ; loops from original bimul implementation
    xor r8, r8              ; zero out loop j
    .jloop:
        ; get actual address of operands
        lea rcx, [rsi + 8 * r12]
        lea rdi, [r10 + 8 * r8]
        mov rax, [rcx]      ; load current lhs qword
        mul qword [rdi]     ; multiply with current rhs qword
        ; rdx = high part
        ; rax = lo part
        ; get the dst word position (reuse rcx)
        mov rcx, r12
        add rcx, r8
        lea rcx, [r11 + 8 * rcx]
        ; rcx has the high word dst
        add [rcx + 8], rax
        adc [rcx], rdx
        sub rcx, 16
        jnc .carryDone
        .handleCarry:
            sub rcx, 8
            adc qword [rcx], 1
            jc .handleCarry
        .carryDone
        inc r8
        cmp r8, r9
        je .jdone
        jmp .jloop          ; todo: unroll a few times
    .jdone:
    inc r12
    cmp r12, r9
    je .idone
    jmp .iloop              ; todo: unroll
.idone:
pop r12
ret

;_biadd:
;rdi = BigInt* dst
;rsi = BigInt* lhs
;rdx = BigInt* rhs
;xor rcx, rcx
;mov rdx, [rdi]
;mov ecx, [rdi + 8]
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
jc .loop
.done:
ret
.loop:
sub rsi, 8
add qword [rsi], 1
cmp rsi, rdx                ; did the most significant word just get ++?
jne .check
ret
.check:
jc .loop
ret
