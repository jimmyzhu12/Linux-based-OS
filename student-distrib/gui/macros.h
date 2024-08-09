#ifndef MACROS_H
#define MACROS_H

static __inline__ void port_out(int value, int port)
{
    __asm__ volatile ("outb %0,%1"
	      ::"a" ((unsigned char) value), "d"((unsigned short) port));
}

static __inline__ void port_outw(int value, int port)
{
    __asm__ volatile ("outw %0,%1"
	     ::"a" ((unsigned short) value), "d"((unsigned short) port));
}

static __inline__ void port_outl(int value, int port)
{
    __asm__ volatile ("outl %0,%1"
             ::"a" ((unsigned long)value), "d" ((unsigned short) port));
}

static __inline__ int port_in(int port)
{
    unsigned char value;
    __asm__ volatile ("inb %1,%0"
		      :"=a" (value)
		      :"d"((unsigned short) port));
    return value;
}

static __inline__ int port_inw(int port)
{
    unsigned short value;
    __asm__ volatile ("inw %1,%0"
		      :"=a" (value)
		      :"d"((unsigned short) port));
    return value;
}

static __inline__ int port_inl(int port)
{
    unsigned int value;
    __asm__ volatile("inl %1,%0" :
               	     "=a" (value) :
                     "d" ((unsigned short)port));
    return value;
}

#define gr_readb(off)		(((volatile unsigned char *)GM)[(off)])
#define gr_readw(off)		(*(volatile unsigned short*)((GM)+(off)))
#define gr_readl(off)		(*(volatile unsigned long*)((GM)+(off)))
#define gr_writeb(v,off)	(GM[(off)] = (v))
#define gr_writew(v,off)	(*(unsigned short*)((GM)+(off)) = (v))
#define gr_writel(v,off)	(*(unsigned long*)((GM)+(off)) = (v))


/* Note that the arguments of outb/w are reversed compared with the */
/* kernel sources. The XFree86 drivers also use this format. */
#undef inb
#undef inw
#undef inl
#undef outb
#undef outw
#undef outl

#define inb port_in
#define inw port_inw
#define inl port_inl
#define outb(port, value) port_out(value, port)
#define outw(port, value) port_outw(value, port)
#define outl(port, value) port_outl(value, port)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                           \
do {                                    \
    asm volatile ("cli"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)             \
do {                                    \
    asm volatile ("                   \n\
            pushfl                    \n\
            popl %0                   \n\
            cli                       \n\
            "                           \
            : "=r"(flags)               \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                           \
do {                                    \
    asm volatile ("sti"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)            \
do {                                    \
    asm volatile ("                   \n\
            pushl %0                  \n\
            popfl                     \n\
            "                           \
            :                           \
            : "r"(flags)                \
            : "memory", "cc"            \
    );                                  \
} while (0)

#endif /* MACROS_H */
