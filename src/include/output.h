/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 *
 * See COPYING for licence
 *
 * include/output.h
 * - Outputs common header
 */
#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <stdint.h>

#if 0
typedef struct sElf_x86_Reloc
{
	char	*Name;
	 int	Size;
	uint	Offset;
	uint	Addend;
}	tOutput_Reloc;

typedef struct sElf_x86_Function
{
	size_t	CodeLength;
	size_t	CodeSpace;
	uint8_t	*Code;
	
	 int	nRelocs;
	 int	RelocSpace;
	tOutput_Reloc	*Relocs;
}	tOutput_Function;

// === Functions ===
extern void	Output_AppendCode(tOutput_Function *Func, uint8_t Byte);
extern void	Output_AppendAbs16(tOutput_Function *Func, uint16_t Value);
extern void	Output_AppendAbs32(tOutput_Function *Func, uint32_t Value);
extern void Output_AppendReloc16(tOutput_Function *Func, int16_t Addend, char *SymName);
extern void Output_AppendReloc32(tOutput_Function *Func, int32_t Addend, char *SymName);
#endif

#endif
