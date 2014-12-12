/* vi: set sw=8 ts=8: */

/*
 * Various assembly language/system dependent hacks that are required
 * so that we can minimize the amount of platform specific code.
 */

/* Define this if the system uses RELOCA.  */
#undef ELF_USES_RELOCA
#include <elf.h>
#include <link.h>

#define ARCH_NUM 4
#define DT_MIPS_GOTSYM_IDX	(DT_NUM + OS_NUM)
#define DT_MIPS_LOCAL_GOTNO_IDX	(DT_NUM + OS_NUM +1)
#define DT_MIPS_SYMTABNO_IDX	(DT_NUM + OS_NUM +2)
#define DT_MIPS_PLTGOT_IDX	(DT_NUM + OS_NUM +3)

#define ARCH_DYNAMIC_INFO(dpnt,  dynamic, debug_addr) \
do { \
if (dpnt->d_tag == DT_MIPS_GOTSYM) \
     dynamic[DT_MIPS_GOTSYM_IDX] = dpnt->d_un.d_val; \
else if (dpnt->d_tag == DT_MIPS_LOCAL_GOTNO) \
     dynamic[DT_MIPS_LOCAL_GOTNO_IDX] = dpnt->d_un.d_val; \
else if (dpnt->d_tag == DT_MIPS_SYMTABNO) \
     dynamic[DT_MIPS_SYMTABNO_IDX] = dpnt->d_un.d_val; \
else if (dpnt->d_tag == DT_MIPS_PLTGOT) \
     dynamic[DT_MIPS_PLTGOT_IDX] = dpnt->d_un.d_val; \
else if (dpnt->d_tag == DT_MIPS_RLD_MAP) \
     *(ElfW(Addr) *)(dpnt->d_un.d_ptr) =  (ElfW(Addr)) debug_addr; \
} while (0)

/* Initialization sequence for the application/library GOT.  */
#define INIT_GOT(GOT_BASE,MODULE)						\
do {										\
	unsigned long idx;							\
	unsigned long *pltgot;							\
										\
	/* Check if this is the dynamic linker itself */			\
	if (MODULE->libtype == program_interpreter)				\
		continue;							\
										\
	/* Fill in first two GOT entries according to the ABI */		\
	GOT_BASE[0] = (unsigned long) _dl_runtime_resolve;			\
	GOT_BASE[1] = (unsigned long) MODULE;					\
										\
	pltgot = MODULE->dynamic_info[DT_MIPS_PLTGOT_IDX];			\
	if (pltgot) {								\
		pltgot[0] = (unsigned long) _dl_runtime_pltresolve;		\
		pltgot[1] = (unsigned long) MODULE;				\
	}									\
										\
	/* Add load address displacement to all local GOT entries */		\
	idx = 2;									\
	while (idx < MODULE->dynamic_info[DT_MIPS_LOCAL_GOTNO_IDX])		\
		GOT_BASE[idx++] += (unsigned long) MODULE->loadaddr;		\
										\
} while (0)


/* Here we define the magic numbers that this dynamic loader should accept */
#define MAGIC1 EM_MIPS
#define MAGIC2 EM_MIPS_RS3_LE


/* Used for error messages */
#define ELF_TARGET "MIPS"


unsigned long __dl_runtime_resolve(unsigned long sym_index,
	unsigned long old_gpreg);

struct elf_resolve;
void _dl_perform_mips_global_got_relocations(struct elf_resolve *tpnt, int lazy);

#define elf_machine_type_class(type) \
  ((((type) == R_MIPS_JUMP_SLOT) * ELF_RTYPE_CLASS_PLT)	\
   | (((type) == R_MIPS_COPY) * ELF_RTYPE_CLASS_COPY))

#define OFFSET_GP_GOT 0x7ff0

static __always_inline ElfW(Addr) *
elf_mips_got_from_gpreg (ElfW(Addr) gpreg)
{
	/* FIXME: the offset of gp from GOT may be system-dependent. */
	return (ElfW(Addr) *) (gpreg - OFFSET_GP_GOT);
}

/* Return the link-time address of _DYNAMIC.  Conveniently, this is the
   first element of the GOT.  This must be inlined in a function which
   uses global data.  We assume its $gp points to the primary GOT.  */
static __always_inline ElfW(Addr)
elf_machine_dynamic (void)
{
	register ElfW(Addr) gp __asm__ ("$28");
	return *elf_mips_got_from_gpreg (gp);
}

#define STRINGXP(X) __STRING(X)
#define STRINGXV(X) STRINGV_(X)
#define STRINGV_(...) # __VA_ARGS__
#define PTR_LA               la
#define PTR_SUBU     subu

/* Return the run-time load address of the shared object.  */
static __always_inline ElfW(Addr)
elf_machine_load_address (void)
{
	ElfW(Addr) addr;
	__asm__ ("        .set noreorder\n"
	     "        " STRINGXP (PTR_LA) " %0, 0f\n"
	     "        bltzal $0, 0f\n"
	     "        nop\n"
	     "0:      " STRINGXP (PTR_SUBU) " %0, $31, %0\n"
	     "        .set reorder\n"
	     :        "=r" (addr)
	     :        /* No inputs */
	     :        "$31");
	return addr;
}

static __always_inline void
elf_machine_relative (ElfW(Addr) load_off, const ElfW(Addr) rel_addr,
		      ElfW(Word) relative_count)
{
	/* No RELATIVE relocs in MIPS? */
}
