#define ASM			1

/* push all registers other than %eax */
#define push_reg_xa		\
    pushl   %fs;		\
    pushl   %es;		\
    pushl   %ds;		\
    pushl   %ebp;		\
    pushl   %edi;		\
    pushl   %esi;		\
    pushl   %edx;		\
    pushl   %ecx;		\
    pushl   %ebx;

/* restore all registers other than %eax */
#define pop_reg_xa		\
    popl    %ebx;		\
    popl    %ecx;		\
    popl    %edx;		\
    popl    %esi;		\
    popl    %edi;		\
    popl    %ebp;		\
    popl    %ds;		\
    popl    %es;		\
    popl    %fs;

/* system call linkage */
.global sys_Linkage
sys_Linkage:
	/* save all registers with eax excluded */
	push_reg_xa
	/* sanity check */
	cmpl	$12,	%eax
	jg		_mp3_sys_call_invalid
	cmpl	$0,		%eax
	jle		_mp3_sys_call_invalid
	/* jump to system calls */
	call 	*syst_call_dispatchee_functions(, %eax, 4)
	/* restore all registers other than %eax */
	pop_reg_xa
	iret

  _mp3_sys_call_invalid:
	movl 	$-1,	%eax
	/* restore all registers other than %eax */
	pop_reg_xa
	iret

.align	4
syst_call_dispatchee_functions:
.long 0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn
