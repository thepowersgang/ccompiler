/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 *
 * This code is published under the terms of the FreeBSD licence.
 * See the file COPYING for details
 *
 * symbol.c - Global and Local symbol manipulation
 */
#define DEBUG	0
#include <global.h>
#include <symbol.h>
#include <string.h>
#include <ast.h>

// === IMPORTS ===
extern int	giLine;
extern tType	*GetType(void);

// === PROTOTYPES ===
void	Symbol_EnterBlock(void);
void	Symbol_LeaveBlock(void);
tType	*Symbol_ResolveTypedef(char *Name, int Depth);
tType	*Symbol_CreateIntegralType(int bSigned, int bConst, int Linkage, int Size, int Depth);
tSymbol	*Symbol_GetLocalVariable(char *Name);
tSymbol	*Symbol_ResolveSymbol(char *Name);
 int	Symbol_GetSymClass(tSymbol *Symbol);
void	Symbol_AddGlobalVariable(tType *Type, char *Name, uint64_t InitValue);
void	Symbol_SetFunction(tFunction *Fcn);
void	Symbol_SetFunctionCode(tFunction *Fcn, void *Block);
void	Symbol_DumpTree(void);
tType	*Symbol_ParseStruct(char *Name);
tType	*Symbol_GetStruct(char *Name);
tType	*Symbol_ParseUnion(char *Name);
tType	*Symbol_GetUnion(char *Name);
tType	*Symbol_ParseEnum(char *Name);
tType	*Symbol_GetEnum(char *Name);

// === GLOBALS ===
tFunction	*gpFunctions = NULL;
tFunction	*gpCurrentFunction = NULL;
tCodeBlock	*gpCurrentBlock = NULL;
tSymbol	*gpGlobalSymbols = NULL;
tStruct	*gpStructures;
tStruct	*gpUnions;

// === CODE ===
void Symbol_EnterBlock(void)
{
	tCodeBlock	*new = malloc( sizeof(tCodeBlock) );
	new->Parent = gpCurrentBlock;
	new->LocalVariables = NULL;
	gpCurrentBlock = new;
}

void Symbol_LeaveBlock(void)
{
	tCodeBlock	*old = gpCurrentBlock;
	gpCurrentBlock = old->Parent;
	free(old);
}


tType *Symbol_ResolveTypedef(char *Name, int Depth)
{
	//! \todo Typedefs
	return NULL;
}

tType *Symbol_CreateIntegralType(int bSigned, int bConst, int Linkage, int Size, int Depth)
{
	tType	*ret;
	
	ret = malloc( sizeof(tType) );
	if(Size == 0)
		ret->Type = 0;	// Void
	else
		ret->Type = 1;	// Integer
	ret->Linkage = Linkage;
	ret->bConst = bConst;
	ret->PtrDepth = Depth;
	ret->Integer.bSigned = bSigned;
	ret->Integer.Bits = Size;
	ret->Size = Size/32;
	
	return ret;
}

/**
 * \brief Gets a local variable
 */
tSymbol *Symbol_GetLocalVariable(char *Name)
{
	tCodeBlock	*block;
	tSymbol	*sym;
	
	DEBUG_S("Symbol_GetLocalVariable: (Name='%s')\n", Name);
	
	for( block = gpCurrentBlock; block; block = block->Parent )
	{
		DEBUG_S(" block = %p\n", block);
		for( sym = block->LocalVariables; sym; sym = sym->Next )
		{
			DEBUG_S(" strcmp(Name, '%s')\n", sym->Name);
			if(strcmp(Name, sym->Name) == 0)
				return sym;
		}
	}

	DEBUG_S(" gpCurrentFunction = %p\n", gpCurrentFunction);
	DEBUG_S(" gpCurrentFunction->Arguments = %p\n", gpCurrentFunction->Arguments);
	for( sym = gpCurrentFunction->Arguments; sym; sym = sym->Next )
	{
		DEBUG_S(" sym = %p\n", sym);
		DEBUG_S(" strcmp(Name, '%s')\n", sym->Name);
		if(strcmp(Name, sym->Name) == 0)
			return sym;
	}
	return NULL;
}

tSymbol *Symbol_ResolveSymbol(char *Name)
{
	DEBUG_S("Symbol_ResolveSymbol: (Name='%s')\n", Name);
	{
		tSymbol	*sym = gpGlobalSymbols;
		for(; sym; sym = sym->Next)
		{
			DEBUG_S(" Symbol_ResolveSymbol: sym = %p\n", sym);
			if(strcmp(Name, sym->Name) == 0)
				return sym;
		}
	}
	{
		tFunction	*fcn;
		for(fcn=gpFunctions;fcn;fcn=fcn->Next)
		{
			DEBUG_S(" Symbol_ResolveSymbol: fcn = %p\n", fcn);
			if(strcmp(Name, fcn->Name) == 0)
				return &fcn->Sym;
		}
	}
	return NULL;
}

int Symbol_CompareTypes(tType *T1, tType *T2)
{
	if(T1->Type != T2->Type)	return 1;
	if(T1->PtrDepth != T2->PtrDepth)	return 2;
	switch(T1->Type)
	{
	case 1:
		if(T1->Integer.bSigned != T2->Integer.bSigned)	return 3;
		if(T1->Integer.Bits != T2->Integer.Bits)	return 4;
		break;
	}
	return 0;
}

int Symbol_GetSymClass(tSymbol *Symbol)
{
	return Symbol->Class;
}


void Symbol_SetFunctionVariableArgs(tFunction *Func)
{
	Func->bVaArgs = 1;
}

void Symbol_SetFunctionCode(tFunction *Fcn, void *Block)
{
	Fcn->Code = Block;
}

void Symbol_DumpTree(void)
{
	#if DEBUG
	tSymbol *sym;
	tFunction	*fcn;
	
	printf("Global Symbols:\n");
	for(sym = gpGlobalSymbols;
		sym;
		sym = sym->Next)
	{
		printf(" '%s'\n", sym->Name);
	}
	
	printf("Functions:\n");
	for(fcn = gpFunctions;
		fcn;
		fcn = fcn->Next)
	{
		printf(" %s()\n", fcn->Name);
		AST_DumpTree(fcn->Code, 1);
	}
	#endif
}

// --- Structures, Unions and Enums ---
tType *Symbol_GetStruct(char *Name)
{
	for( tStruct *ele = gpStructures; ele; ele = ele->Next )
	{
		if(strcmp(ele->Name, Name) == 0) {
			tType *ret = calloc(1, sizeof(tType));
			ret->Type = 2;
			ret->StructUnion = ele;
			return ret;
		}
	}
	return NULL;
}

tType *Symbol_GetUnion(char *Name)
{
	for( tStruct *ele = gpUnions; ele; ele = ele->Next )
	{
		if(strcmp(ele->Name, Name) == 0) {
			tType	*ret = calloc(1, sizeof(tType));
			ret->Type = 3;
			ret->StructUnion = ele;
			return ret;
		}
	}
	return NULL;
}

tType *Symbol_GetEnum(char *Name)
{
	return NULL;
}

