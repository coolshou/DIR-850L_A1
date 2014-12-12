#ifndef _BITS_SYSCALLS_H
#define _BITS_SYSCALLS_H
#ifndef _SYSCALL_H
# error "Never use <bits/syscalls.h> directly; include <sys/syscall.h> instead."
#endif

/*
   Some of the sneaky macros in the code were taken from
   glibc-2.3.2/sysdeps/unix/sysv/linux/arm/sysdep.h
*/

#define SYS_ify(syscall_name)  (__NR_##syscall_name)

#ifdef __ASSEMBLER__

/* Call a given syscall, with arguments loaded.  For EABI, we must
   save and restore r7 for the syscall number.  Unlike the DO_CALL
   macro in glibc, this macro does not load syscall arguments.  */
#undef DO_CALL
#if defined(__ARM_EABI__)
#define DO_CALL(syscall_name)			\
    mov ip, r7;					\
    ldr r7, =SYS_ify (syscall_name);		\
    swi 0x0;					\
    mov r7, ip;
#else
#define DO_CALL(syscall_name)			\
    swi SYS_ify (syscall_name);
#endif

#else

#include <errno.h>

#undef _syscall0
#define _syscall0(type,name) \
type name(void) \
{ \
return (type) (INLINE_SYSCALL(name, 0)); \
}

#undef _syscall1
#define _syscall1(type,name,type1,arg1) \
type name(type1 arg1) \
{ \
return (type) (INLINE_SYSCALL(name, 1, arg1)); \
}

#undef _syscall2
#define _syscall2(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1,type2 arg2) \
{ \
return (type) (INLINE_SYSCALL(name, 2, arg1, arg2)); \
}

#undef _syscall3
#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
type name(type1 arg1,type2 arg2,type3 arg3) \
{ \
return (type) (INLINE_SYSCALL(name, 3, arg1, arg2, arg3)); \
}

#undef _syscall4
#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4) \
type name (type1 arg1, type2 arg2, type3 arg3, type4 arg4) \
{ \
return (type) (INLINE_SYSCALL(name, 4, arg1, arg2, arg3, arg4)); \
}

#undef _syscall5
#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
	  type5,arg5) \
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5) \
{ \
return (type) (INLINE_SYSCALL(name, 5, arg1, arg2, arg3, arg4, arg5)); \
}

#undef _syscall6
#define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
	  type5,arg5,type6,arg6) \
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5, type6 arg6) \
{ \
return (type) (INLINE_SYSCALL(name, 6, arg1, arg2, arg3, arg4, arg5, arg6)); \
}

#undef _syscall7
#define _syscall7(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
	  type5,arg5,type6,arg6,type7,arg7) \
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5, type6 arg6,type7 arg7) \
{ \
return (type) (INLINE_SYSCALL(name, 7, arg1, arg2, arg3, arg4, arg5, arg6, arg7)); \
}


#undef INLINE_SYSCALL
#define INLINE_SYSCALL(name, nr, args...)				\
  ({ unsigned int _inline_sys_result = INTERNAL_SYSCALL (name, , nr, args);	\
     if (__builtin_expect (INTERNAL_SYSCALL_ERROR_P (_inline_sys_result, ), 0))	\
       {								\
	 __set_errno (INTERNAL_SYSCALL_ERRNO (_inline_sys_result, ));		\
	 _inline_sys_result = (unsigned int) -1;				\
       }								\
     (int) _inline_sys_result; })

#undef INTERNAL_SYSCALL_DECL
#define INTERNAL_SYSCALL_DECL(err) do { } while (0)

#undef INTERNAL_SYSCALL
#if !defined(__thumb__)
#if defined(__ARM_EABI__)
#define INTERNAL_SYSCALL(name, err, nr, args...)			\
  ({unsigned int __sys_result;						\
     {									\
       register int _a1 __asm__ ("r0"), _nr __asm__ ("r7");		\
       LOAD_ARGS_##nr (args)						\
       _nr = SYS_ify(name);						\
       __asm__ __volatile__ ("swi	0x0	@ syscall " #name	\
			     : "=r" (_a1)				\
			     : "r" (_nr) ASM_ARGS_##nr			\
			     : "memory");				\
	       __sys_result = _a1;					\
     }									\
     (int) __sys_result; })
#else /* defined(__ARM_EABI__) */

#define INTERNAL_SYSCALL(name, err, nr, args...)			\
  ({ unsigned int __sys_result;						\
     {									\
       register int _a1 __asm__ ("a1");					\
       LOAD_ARGS_##nr (args)						\
       __asm__ __volatile__ ("swi	%1	@ syscall " #name	\
		     : "=r" (_a1)					\
		     : "i" (SYS_ify(name)) ASM_ARGS_##nr		\
		     : "memory");					\
       __sys_result = _a1;						\
     }									\
     (int) __sys_result; })
#endif
#else /* !defined(__thumb__) */
/* We can't use push/pop inside the asm because that breaks
   unwinding (ie. thread cancellation).
 */
#define INTERNAL_SYSCALL(name, err, nr, args...)			\
  ({ unsigned int __sys_result;						\
    {									\
      int _sys_buf[2];							\
      register int _a1 __asm__ ("a1");					\
      register int *_v3 __asm__ ("v3") = _sys_buf;			\
      *_v3 = (int) (SYS_ify(name));					\
      LOAD_ARGS_##nr (args)						\
      __asm__ __volatile__ ("str	r7, [v3, #4]\n"			\
		    "\tldr	r7, [v3]\n"				\
		    "\tswi	0	@ syscall " #name "\n"		\
		    "\tldr	r7, [v3, #4]"				\
		    : "=r" (_a1)					\
		    : "r" (_v3) ASM_ARGS_##nr				\
                    : "memory");					\
	__sys_result = _a1;						\
    }									\
    (int) __sys_result; })
#endif /*!defined(__thumb__)*/

#undef INTERNAL_SYSCALL_ERROR_P
#define INTERNAL_SYSCALL_ERROR_P(val, err) \
  ((unsigned int) (val) >= 0xfffff001u)

#undef INTERNAL_SYSCALL_ERRNO
#define INTERNAL_SYSCALL_ERRNO(val, err)	(-(val))

#define LOAD_ARGS_0()
#define ASM_ARGS_0
#define LOAD_ARGS_1(a1)				\
  int _a1tmp = (int) (a1);			\
  LOAD_ARGS_0 ()				\
  _a1 = _a1tmp;
#define ASM_ARGS_1	ASM_ARGS_0, "r" (_a1)
#define LOAD_ARGS_2(a1, a2)			\
  int _a2tmp = (int) (a2);			\
  LOAD_ARGS_1 (a1)				\
  register int _a2 __asm__ ("a2") = _a2tmp;
#define ASM_ARGS_2	ASM_ARGS_1, "r" (_a2)
#define LOAD_ARGS_3(a1, a2, a3)			\
  int _a3tmp = (int) (a3);			\
  LOAD_ARGS_2 (a1, a2)				\
  register int _a3 __asm__ ("a3") = _a3tmp;
#define ASM_ARGS_3	ASM_ARGS_2, "r" (_a3)
#define LOAD_ARGS_4(a1, a2, a3, a4)		\
  int _a4tmp = (int) (a4);			\
  LOAD_ARGS_3 (a1, a2, a3)			\
  register int _a4 __asm__ ("a4") = _a4tmp;
#define ASM_ARGS_4	ASM_ARGS_3, "r" (_a4)
#define LOAD_ARGS_5(a1, a2, a3, a4, a5)		\
  int _v1tmp = (int) (a5);			\
  LOAD_ARGS_4 (a1, a2, a3, a4)			\
  register int _v1 __asm__ ("v1") = _v1tmp;
#define ASM_ARGS_5	ASM_ARGS_4, "r" (_v1)
#define LOAD_ARGS_6(a1, a2, a3, a4, a5, a6)	\
  int _v2tmp = (int) (a6);			\
  LOAD_ARGS_5 (a1, a2, a3, a4, a5)		\
  register int _v2 __asm__ ("v2") = _v2tmp;
#define ASM_ARGS_6	ASM_ARGS_5, "r" (_v2)
#define LOAD_ARGS_7(a1, a2, a3, a4, a5, a6, a7)	\
  int _v3tmp = (int) (a7);			\
  LOAD_ARGS_6 (a1, a2, a3, a4, a5, a6)		\
  register int _v3 __asm__ ("v3") = _v3tmp;
#define ASM_ARGS_7	ASM_ARGS_6, "r" (_v3)


#endif /* __ASSEMBLER__ */
#endif /* _BITS_SYSCALLS_H */
