                section         .text

                global          _start

%define st_pointer rsp
%define size rcx

%define i_iter r8
%define j_iter r9
%define ij_offset r10
%define _carry r11

; qword size
%define chunk_size 8
%define max_size 128

_start:
                sub	st_pointer, 4 * max_size * chunk_size
                lea	rdi, [st_pointer + 3 * max_size * chunk_size]   ; place for first number

                mov	size, max_size

                call	read_long
                lea	rdi, [st_pointer + 2 * max_size * chunk_size]   ; place for second number
                call	read_long

                lea	rsi, [st_pointer + 3 * max_size * chunk_size]   ; rsi -- indicates first number
                lea	rbx, [st_pointer + 2 * max_size * chunk_size]   ; rbx -- indicates second number

                mov	rdi, st_pointer                                 ; rdi -- indicates result
                mov	size, 2 * max_size

                call	set_zero

                call	mul_long_long

                call	write_long
                mov	al, 0x0a
                call	write_char

                jmp	exit

; mul two long numbers
;    rsi -- address of summand #1 (long number)
;    rbx -- address of summand #2 (long number)
;    size -- length of long numbers in qwords
; result:
;    mul is written to rdi
mul_long_long:
                push	rsi
                push	rbx
                push	size
                clc

                mov	i_iter, 0
.for_first:
                mov	j_iter, 0
.for_second:
                mov	rax, [rsi + i_iter * chunk_size]        ; rax <- rsi[i]
                mov	rdx, [rbx + j_iter * chunk_size]        ; rdx <- rbx[j]
                mul	rdx                                     ; rdx:rax = rax * rdx <=> rsi[i] * rbx[j]
                lea	ij_offset, [i_iter + j_iter]

                clc                                             ; clear the carry flag
                add	[rdi + ij_offset * chunk_size], rax     ; rdi[i + j] += rax, set carry

                inc	ij_offset                               ; offset to next chunk
                adc	[rdi + ij_offset * chunk_size], rdx     ; rdi[i + j + 1] += rdx + carry
                mov	_carry, 0                               ;
                adc	_carry, 0                               ; _carry from last addition

                jc      .next

                inc	ij_offset                               ; offset to next chunk
                add	[rdi + ij_offset * chunk_size], _carry  ; rdi[i + j + 2] += carry
.next:
                inc	j_iter
                cmp 	j_iter, max_size
                jnz	.for_second
; for_second's end
                inc	i_iter
                cmp	i_iter, max_size
                jnz	.for_first
; for_first's end
                pop	size
                pop	rbx
                pop	rsi
                ret

; adds 64-bit number to long number
;    rdi -- address of summand #1 (long number)
;    rax -- summand #2 (64-bit unsigned)
;    size -- length of long number in qwords
; result:
;    sum is written to rdi
add_long_short:
                push            rdi
                push            size
                push            rdx

                xor             rdx,rdx
.loop:
                add             [rdi], rax
                adc             rdx, 0
                mov             rax, rdx
                xor             rdx, rdx
                add             rdi, 8

                dec             size
                jnz             .loop

                pop             rdx
                pop             size
                pop             rdi
                ret

; multiplies long number by a short
;    rdi -- address of multiplier #1 (long number)
;    rbx -- multiplier #2 (64-bit unsigned)
;    size -- length of long number in qwords
; result:
;    product is written to rdi
mul_long_short:
                push            rax
                push            rdi
                push            size

                xor             rsi, rsi
.loop:
                mov             rax, [rdi]
                mul             rbx
                add             rax, rsi
                adc             rdx, 0
                mov             [rdi], rax
                add             rdi, 8
                mov             rsi, rdx
                dec             size
                jnz             .loop

                pop             size
                pop             rdi
                pop             rax
                ret

; divides long number by a short
;    rdi -- address of dividend (long number)
;    rbx -- divisor (64-bit unsigned)
;    size -- length of long number in qwords
; result:
;    quotient is written to rdi
;    rdx -- remainder
div_long_short:
                push            rdi
                push            rax
                push            size

                lea             rdi, [rdi + chunk_size * size - 8]
                xor             rdx, rdx

.loop:
                mov             rax, [rdi]
                div             rbx
                mov             [rdi], rax
                sub             rdi, 8

                dec             size
                jnz             .loop

                pop             size
                pop             rax
                pop             rdi
                ret

; assigns a zero to long number
;    rdi -- argument (long number)
;    size -- length of long number in qwords
set_zero:
                push            rax
                push            rdi
                push            size

                xor             rax, rax
                rep stosq

                pop             size
                pop             rdi
                pop             rax
                ret

; checks if a long number is a zero
;    rdi -- argument (long number)
;    size -- length of long number in qwords
; result:
;    ZF=1 if zero
is_zero:
                push            rax
                push            rdi
                push            size

                xor             rax, rax
                rep scasq

                pop             size
                pop             rdi
                pop             rax
                ret

; read long number from stdin
;    rdi -- location for output (long number)
;    size -- length of long number in qwords
read_long:
                push            size
                push            rdi

                call            set_zero
.loop:
                call            read_char
                or              rax, rax
                js              exit
                cmp             rax, 0x0a
                je              .done
                cmp             rax, '0'
                jb              .invalid_char
                cmp             rax, '9'
                ja              .invalid_char

                sub             rax, '0'
                mov             rbx, 10
                call            mul_long_short
                call            add_long_short
                jmp             .loop

.done:
                pop             rdi
                pop             size
                ret

.invalid_char:
                mov             rsi, invalid_char_msg
                mov             rdx, invalid_char_msg_size
                call            print_string
                call            write_char
                mov             al, 0x0a
                call            write_char

.skip_loop:
                call            read_char
                or              rax, rax
                js              exit
                cmp             rax, 0x0a
                je              exit
                jmp             .skip_loop

; write long number to stdout
;    rdi -- argument (long number)
;    size -- length of long number in qwords
write_long:
                push            rax
                push            size

                mov             rax, 20
                mul             size
                mov             rbp, st_pointer
                sub             st_pointer, rax

                mov             rsi, rbp

.loop:
                mov             rbx, 10
                call            div_long_short
                add             rdx, '0'
                dec             rsi
                mov             [rsi], dl
                call            is_zero
                jnz             .loop

                mov             rdx, rbp
                sub             rdx, rsi
                call            print_string

                mov             st_pointer, rbp
                pop             size
                pop             rax
                ret

; read one char from stdin
; result:
;    rax == -1 if error occurs
;    rax \in [0; 255] if OK
read_char:
                push            size
                push            rdi

                sub             st_pointer, 1
                xor             rax, rax
                xor             rdi, rdi
                mov             rsi, st_pointer
                mov             rdx, 1
                syscall

                cmp             rax, 1
                jne             .error
                xor             rax, rax
                mov             al, [st_pointer]
                add             st_pointer, 1

                pop             rdi
                pop             size
                ret
.error:
                mov             rax, -1
                add             st_pointer, 1
                pop             rdi
                pop             size
                ret

; write one char to stdout, errors are ignored
;    al -- char
write_char:
                sub             st_pointer, 1
                mov             [st_pointer], al

                mov             rax, 1
                mov             rdi, 1
                mov             rsi, st_pointer
                mov             rdx, 1
                syscall
                add             st_pointer, 1
                ret

exit:
                mov             rax, 60
                xor             rdi, rdi
                syscall

; print string to stdout
;    rsi -- string
;    rdx -- size
print_string:
                push            rax

                mov             rax, 1
                mov             rdi, 1
                syscall

                pop             rax
                ret


                section         .rodata
invalid_char_msg:
                db              "Invalid character: "
invalid_char_msg_size: equ             $ - invalid_char_msg
