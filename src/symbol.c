/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 *
 * This code is published under the terms of the FreeBSD licence.
 * See the file COPYING for details
 *
 * symbol.c - Global and Local symbol manipulation
 */
#include <global.h>
#include <symbol.h>
#include <string.h>
#include <ast.h>
#include <assert.h>

// === IMPORTS ===
extern int	giLine;
extern tType	*GetType(void);

// === PROTOTYPES ===
void	Symbol_EnterBlock(void);
void	Symbol_LeaveBlock(void);
tType	*Symbol_ResolveTypedef(char *Name, int Depth);
tType	*Symbol_CreateIntegralType(int bSigned, int bConst, int Linkage, int Size, int Depth);
//tSymbol	*Symbol_ResolveSymbol(const char *Name);
 int	Symbol_GetSymClass(tSymbol *Symbol);
// int	Symbol_AddGlobalVariable(tType *Type, enum eLinkage Linkage, const char *Name, tAST_Node *InitValue);
// int	Symbol_AddFunction(tType *Type, enum eLinkage Linkage, const char *Name, tAST_Node *Code);
void	Symbol_SetFunction(tFunction *Fcn);
void	Symbol_SetFunctionCode(tFunction *Fcn, void *Block);
void	Symbol_DumpTree(void);
tType	*Symbol_ParseStruct(char *Name);
tType	*Symbol_GetStruct(char *Name);
tType	*Symbol_ParseUnion(char *Name);
tType	*Symbol_GetUnion(char *Name);
tType	*Symbol_ParseEnum(char *Name);

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

bool Symbol_DefineVariable(const tType *Type, const char *Name)
{
	return false;
}

/**
 * \brief Gets a local variable
 */
tSymbol *Symbol_GetLocalVariable(char *Name)
{
	DEBUG_S("Symbol_GetLocalVariable: (Name='%s')\n", Name);
	
	for( tCodeBlock *block = gpCurrentBlock; block; block = block->Parent )
	{
		DEBUG_S(" block = %p\n", block);
		for( tSymbol *sym = block->LocalVariables; sym; sym = sym->Next )
		{
			DEBUG_S(" strcmp(Name, '%s')\n", sym->Name);
			if(strcmp(Name, sym->Name) == 0)
				return sym;
		}
	}

	DEBUG_S(" gpCurrentFunction = %p\n", gpCurrentFunction);
	DEBUG_S(" gpCurrentFunction->Arguments = %p\n", gpCurrentFunction->Arguments);
	//for( tSymbol *sym = gpCurrentFunction->Arguments; sym; sym = sym->Next )
	//{
	//	DEBUG_S(" sym = %p\n", sym);
	//	DEBUG_S(" strcmp(Name, '%s')\n", sym->Name);
	//	if(strcmp(Name, sym->Name) == 0)
	//		return sym;
	//}
	return NULL;
}

tSymbol *Symbol_ResolveSymbol(const char *Name)
{
	DEBUG_S("Symbol_ResolveSymbol: (Name='%s')\n", Name);
	for(tSymbol *sym = gpGlobalSymbols; sym; sym = sym->Next)
	{
		DEBUG_S(" Symbol_ResolveSymbol: sym = %p\n", sym);
		if(strcmp(Name, sym->Name) == 0)
			return sym;
	}
	return NULL;
}

int Symbol_AddGlobalVariable(const tType *Type, enum eLinkage Linkage, const char *Name, tAST_Node *InitValue)
{
	for( tSymbol *sym = gpGlobalSymbols; sym; sym = sym->Next )
	{
		// TODO: Linkage checks?
		if( strcmp(Name, sym->Name) == 0 ) {
			return 1;
		}
	}
	tSymbol *new_sym = malloc( sizeof(tSymbol) + strlen(Name) + 1 );
	new_sym->Linkage = Linkage;
	new_sym->Name = (void*)(new_sym+1);
	strcpy( (void*)(new_sym+1), Name );
	new_sym->Type = Type;
	new_sym->Line = 0;	// TODO: Get line
	new_sym->Offset = 0;	// not used yet
	new_sym->Value = InitValue;

	new_sym->Next = gpGlobalSymbols;
	gpGlobalSymbols = new_sym;
	return 0;
}

int Symbol_AddFunction(const tType *Type, enum eLinkage Linkage, const char *Name, tAST_Node *Code)
{
	for( tSymbol *sym = gpGlobalSymbols; sym; sym = sym->Next )
	{
		if( strcmp(Name, sym->Name) == 0 )
		{
			if( Types_Compare(Type, sym->Type) ) {
				//SyntaxError(NULL, "Redefinition of %s as incomatible type", Name);
				return 1;
			}
			// Ok to redefine if
			// 1. Linkage doesn't mismatch badly (i.e. extern+static)
			// TODO: Check linkage
			// 2. Existing doesn't have code (and adding code)
			if( Code ) {
				if( sym->Value ) {
					//SyntaxError(NULL, "Redefinition of %s", Name);
					return 1;
				}
				sym->Value = Code;
			}
			return 1;
		}
	}
	
	tSymbol *new_sym = malloc( sizeof(tSymbol) + strlen(Name) + 1 );
	new_sym->Linkage = Linkage;
	new_sym->Name = (void*)(new_sym+1);
	strcpy( (void*)(new_sym+1), Name );
	new_sym->Type = Type;
	new_sym->Line = 0;	// TODO: Get line
	new_sym->Offset = 0;	// not used yet
	new_sym->Value = Code;

	new_sym->Next = gpGlobalSymbols;
	gpGlobalSymbols = new_sym;
	return 0;
}

void Symbol_SetFunctionVariableArgs(tFunction *Func)
{
	//Func->bVaArgs = 1;
}

void Symbol_SetFunctionCode(tFunction *Fcn, void *Block)
{
	Fcn->Sym.Value = Block;
}

void Symbol_DumpTree(void)
{
	#if DEBUG
	printf("Global Symbols:\n");
	for(tSymbol *sym = gpGlobalSymbols; sym; sym = sym->Next)
	{
		printf(" '%s'\n", sym->Name);
	}
	
	printf("Functions:\n");
	for(tFunction *fcn = gpFunctions; fcn; fcn = fcn->Next)
	{
		printf(" %s()\n", fcn->Name);
		AST_DumpTree(fcn->Code, 1);
	}
	#endif
}

