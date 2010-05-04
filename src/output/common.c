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
#include <string.h>

#define _CONCAT(a,b)	a##b
#define CONCAT(a,b)	_CONCAT(a,b)

// === CONSTANTS ===
#define CODE_STEP	1024
#define RELOC_STEP	16

//#define OUTPUT_FORMAT	VM16CISC
#define OUTPUT_FORMAT	X86

// === IMPORTS ===
extern tFunction	*gpFunctions;
extern tSymbol	*gpGlobalSymbols;
extern int	X86_GenerateFunction(FILE *OutFile, tFunction *Func);
extern int	X86_GenerateProlouge(FILE *OutFile);
extern int	VM16CISC_GenerateFunction(FILE *OutFile, tFunction *Func);
extern int	VM16CISC_GenerateProlouge(FILE *OutFile);

// === PROTOTYPES ===
 int	SetOutputArch(char *Name);
void	GenerateOutput(char *File);
#if 0
void	Output_AppendCode(tOutput_Function *Func, uint8_t Byte);
void	Output_AppendReloc16(tOutput_Function *Func, int16_t Addend, char *SymName);
void	Output_AppendReloc32(tOutput_Function *Func, int32_t Addend, char *SymName);
void	Output_Int_AddReloc(tOutput_Function *Func, int Bits, uint Offset, uint Addend, char *Name);
#endif

// === GLOBALS ===
const tOutputFormat	caOutputFormats[] = {
	{"X86", X86_GenerateProlouge, X86_GenerateFunction},
	{"VM16CISC", VM16CISC_GenerateProlouge, VM16CISC_GenerateFunction}
};
#define NUM_OUTPUT_FORMATS	(sizeof(caOutputFormats)/sizeof(caOutputFormats[0]))

const tOutputFormat	*gpOutputFormat = &caOutputFormats[0];

// === CODE ===
int SetOutputArch(char *Name)
{
	 int	i;
	for( i = 0; i < NUM_OUTPUT_FORMATS; i++ )
	{
		if(strcmp(caOutputFormats[i].Name, Name) == 0) {
			gpOutputFormat = &caOutputFormats[i];
			return 0;
		}
	}
	
	fprintf(stderr, "Unknown output architecture '%s'\n", Name);
	fprintf(stderr, "Valid architectures: ");
	for( i = 0; i < NUM_OUTPUT_FORMATS; i++ )
	{
		if(i)	fprintf(stderr, ", ");
		fprintf(stderr, "%s", caOutputFormats[i].Name);
	}
	fprintf(stderr, "\n");
	
	return -1;
}

void GenerateOutput(char *File)
{
	tFunction	*func;
	FILE	*fp = stdout;
	
	if( File && (fp = fopen(File, "w")) == NULL )
	{
		fprintf(stderr, "ERROR: Unable to open '%s' for writing\n", File);
		perror("GenerateOutput()");
	}
	
	//CONCAT(OUTPUT_FORMAT, _GenerateProlouge)(fp);
	gpOutputFormat->GenProlouge(fp);
	
	for(func = gpFunctions;
		func;
		func = func->Next
		)
	{
		if( func->Code == NULL )
			continue;
		
		//CONCAT(OUTPUT_FORMAT,_GenerateFunction)(fp, func);
		gpOutputFormat->GenFunction(fp, func);
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
