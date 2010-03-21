/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 *
 * See COPYING for licence
 *
 * output/common.c
 * - Output common methods
 */
#include <global.h>
#include <symbol.h>
#include <output.h>

// === CONSTANTS ===
#define CODE_STEP	1024
#define RELOC_STEP	16

// === IMPORTS ===
extern tFunction	*gpFunctions;
extern tSymbol	*gpGlobalSymbols;
extern int	X86_GenerateFunction(FILE *OutFile, tFunction *Func);
extern int	X86_GenerateProlouge(FILE *OutFile);

// === PROTOTYPES ===
void	GenerateOutput(char *File);
#if 0
void	Output_AppendCode(tOutput_Function *Func, uint8_t Byte);
void	Output_AppendReloc16(tOutput_Function *Func, int16_t Addend, char *SymName);
void	Output_AppendReloc32(tOutput_Function *Func, int32_t Addend, char *SymName);
void	Output_Int_AddReloc(tOutput_Function *Func, int Bits, uint Offset, uint Addend, char *Name);
#endif

// === CODE ===
void GenerateOutput(char *File)
{
	tFunction	*func;
	FILE	*fp = stdout;
	
	X86_GenerateProlouge(fp);
	
	for(func = gpFunctions;
		func;
		func = func->Next
		)
	{
		//printf("func->Name = '%s'\n", func->Name);
		printf("\n");
		printf("; Function '%s'\n", func->Name);
		X86_GenerateFunction(fp, func);
	}
}

#if 0
void Output_AppendCode(tOutput_Function *Func, uint8_t Byte)
{
	printf("Output_AppendCode: (Func=%p, Byte=0x%02x)\n", Func, Byte);
	if( Func->CodeLength + 1 > Func->CodeSpace )
	{
		Func->CodeSpace += CODE_STEP;
		Func->Code = realloc(Func->Code, Func->CodeSpace);
		if(!Func->Code)	perror("Unable to allocate space");
	}
	
	Func->Code[Func->CodeLength++] = Byte;
}

void Output_AppendAbs16(tOutput_Function *Func, uint16_t Value)
{
	printf("Output_AppendAbs16: (Func=%p, Value=0x%04x)\n", Func, Value);
	if( Func->CodeLength + 2 > Func->CodeSpace )
	{
		Func->CodeSpace += CODE_STEP;
		Func->Code = realloc(Func->Code, Func->CodeSpace);
		if(!Func->Code)	perror("Unable to allocate space");
	}
	
	//if( Func->LittleEndian ) {
		Func->Code[Func->CodeLength + 0] = Value&0xFF;
		Func->Code[Func->CodeLength + 1] = Value>>8;
	//}
	
	Func->CodeLength += 2;
}

void Output_AppendAbs32(tOutput_Function *Func, uint32_t Value)
{
	printf("Output_AppendAbs32: (Func=%p, Value=0x%08x)\n", Func, Value);
	if( Func->CodeLength + 4 > Func->CodeSpace )
	{
		Func->CodeSpace += CODE_STEP;
		Func->Code = realloc(Func->Code, Func->CodeSpace);
		if(!Func->Code)	perror("Unable to allocate space");
	}
	
	//if( Func->LittleEndian ) {
		Func->Code[Func->CodeLength + 0] = Value & 0xFF;
		Func->Code[Func->CodeLength + 1] = (Value >>  8) & 0xFF;
		Func->Code[Func->CodeLength + 2] = (Value >> 16) & 0xFF;
		Func->Code[Func->CodeLength + 3] = (Value >> 24) & 0xFF;
	//}
	
	Func->CodeLength += 4;
}

void Output_AppendReloc16(tOutput_Function *Func, int16_t Addend, char *SymName)
{
	printf("Output_AppendReloc16: (Func=%p, Addend=0x%04x, SymName='%s')\n", Func, Addend, SymName);
	if( Func->CodeLength + 2 > Func->CodeSpace )
	{
		Func->CodeSpace += CODE_STEP;
		Func->Code = realloc(Func->Code, Func->CodeSpace);
		if(!Func->Code)	perror("Unable to allocate space");
	}
	
	Func->Code[Func->CodeLength + 0] = 0;
	Func->Code[Func->CodeLength + 1] = 0;
	
	Output_Int_AddReloc(Func, 16, Func->CodeLength, Addend, SymName);
	Func->CodeLength += 2;
}

void Output_AppendReloc32(tOutput_Function *Func, int32_t Addend, char *SymName)
{
	printf("Output_AppendReloc32: (Func=%p, Addend=0x%08x, SymName='%s')\n", Func, Addend, SymName);
	if( Func->CodeLength + 4 > Func->CodeSpace )
	{
		Func->CodeSpace += CODE_STEP;
		Func->Code = realloc(Func->Code, Func->CodeSpace);
		if(!Func->Code)	perror("Unable to allocate space");
	}
	
	Func->Code[Func->CodeLength + 0] = 0;
	Func->Code[Func->CodeLength + 1] = 0;
	Func->Code[Func->CodeLength + 2] = 0;
	Func->Code[Func->CodeLength + 3] = 0;
	
	Output_Int_AddReloc(Func, 32, Func->CodeLength, Addend, SymName);
	Func->CodeLength += 4;
}

void Output_Int_AddReloc(tOutput_Function *Func, int Bits, uint Offset, uint Addend, char *Name)
{
	if( Func->nRelocs + 1 > Func->RelocSpace )
	{
		Func->RelocSpace += RELOC_STEP;
		Func->Relocs = realloc(Func->Relocs, Func->RelocSpace);
		if(!Func->Relocs)	perror("Unable to allocate space");
	}
	
	Func->Relocs[Func->nRelocs].Name = Name;
	Func->Relocs[Func->nRelocs].Size = Bits;
	Func->Relocs[Func->nRelocs].Addend = Addend;
	Func->Relocs[Func->nRelocs].Offset = Offset;
	Func->nRelocs ++;
}
#endif
