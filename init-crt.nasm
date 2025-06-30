;; must be linked with kernel32, shell32, and ucrtbase
extern GetCommandLineW, CommandLineToArgvW, _crt_atexit
global init_args, atexit

;; see ../../NASM/winapi/setup_argc_argv.nasm for the original unchanged file.

segment .text
	
%macro wstr_to_str_inplace_macro 0
	xor 	r8, r8
%%loop:
	mov 	ax, [rcx + 2*r8]

	test	ax, ax
	jz  	%%done	; null character

	test	ah, ah	; test if the upper byte is 0
	jz  	%%ascii
	mov 	al, '?'	; default to a question mark for non-ascii characters.
%%ascii:
	mov 	[rcx + r8], al
	inc 	r8
	jmp 	%%loop
%%done:
	mov 	byte [rcx + r8], 0
%endm

init_args:
	; moves argc into edi and argv into rsi.
	; ignores the ABI convention of rdi and rsi being non-volatile.
	push	rbp		; this counteracts the 8 bytes for the return address.
	mov 	rbp, rsp
	sub 	rsp, 48	; 32-bit shadow space + 4-byte integer + alignment. 32 total should also work.

	call	GetCommandLineW

	mov 	rcx, rax
	lea 	rdx, [rsp + 32]
	call	CommandLineToArgvW

	mov 	edi, dword [rsp + 32]	; rdi = argc
	mov 	rsi, rax				; rsi = argv

	xor 	r9d, r9d				; int i = 0;
.argloop:
	mov 	rcx, qword [rsi + 8*r9]	; rcx = argv[i]
	wstr_to_str_inplace_macro

	inc 	r9d
	cmp 	r9d, edi
	jb  	.argloop
	leave
	ret

;; this function is just so ld doesn't complain.
atexit:
	jmp 	_crt_atexit
