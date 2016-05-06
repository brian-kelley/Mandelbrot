; void longmul(u64 f1, u64 f2, u64* phi, u64* plo);

global _longmul

section .text

_longmul:
; according to 64 bit System V:
; f1 in rdi
; f2 in rsi
; phi in rdx
; plo in rcx
mov r8, rdx        ; save phi because mul overwrites rdx
mov rax, rdi
mul qword rsi
; high qword of result in rdx, low in rax
mov [r8], rdx
mov [rcx], rax
ret
