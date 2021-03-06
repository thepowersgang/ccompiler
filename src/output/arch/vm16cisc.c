/*
 * PHPOS2
 * By John Hodge (thePowersGang)
 *
 * See COPYING for licence
 *
 * output/vm16cisc.c
 * - Outputs the AST as my custom VM16CISC Assembley
 */
#include <global.h>
#include <stdio.h>
#include <string.h>
#include <ast.h>
#include <symbol.h>
#include <output.h>

#define DEF_REGISTERS	0xE000	// R15, R14, R13 used
#define	WORDSIZE	8

// === PROTOTYPES ===
 int	VM16CISC_GenerateFunction(FILE *OutFile, tFunction *Func);
void	VM16CISC_ProcessBlock(FILE *OutFile, int CurBPOfs, tAST_Node *Node);
void	VM16CISC_ProcessNode(FILE *OutFile, int CurBPOfs, tAST_Node *Node);
void	VM16CISC_DoAction(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint16_t Registers);
void	VM16CISC_GetAddress(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint16_t Registers);
void	VM16CISC_SaveTo(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint16_t Registers);
 int	VM16CISC_Int_AllocReg(uint16_t *Registers);

// === CODE ===
int VM16CISC_GenerateProlouge(FILE *OutFile)
{
	tSymbol	*sym;
	tFunction	*func;
	fprintf(OutFile, "; File Generated by Acess CC\n");
	fprintf(OutFile, "\n");
	fprintf(OutFile, "[section .data]\n");
	for(sym = gpGlobalSymbols;
		sym;
		sym = sym->Next
		)
	{
		if(sym->Type->Linkage == 2)	continue;
		if(sym->Type->Linkage != 1)
			fprintf(OutFile, "[global %s]\n", sym->Name);
		fprintf(
			OutFile,
			"%s:\ttimes %li d64 0\n",
			sym->Name,
			sym->Type->Size
			);
	}
	fprintf(OutFile, "\n");
	fprintf(OutFile, "[section .rodata]\n");
	fprintf(OutFile, "\n");
	fprintf(OutFile, "\n");
	fprintf(OutFile, "[section .text]\n");
	for(func = gpFunctions;
		func;
		func = func->Next)
	{
		if(func->Return->Linkage != 2)	continue;
		fprintf(OutFile, "[extern %s]\n", func->Name);
	}
	return 0;
}

int VM16CISC_GenerateFunction(FILE *OutFile, tFunction *Func)
{
	// int	curBP = 0;
	
	//printf("VM16CISC_GenerateFunction: (Func=%p{Name:'%s',Code:%p,...})\n",
	//	Func, Func->Name, Func->Code);
	
	// --- Function Prolouge
	fprintf(OutFile, "\n");
	fprintf(OutFile, "; Function '%s'\n", Func->Name);
	if(Func->Return->Linkage != 1)
		fprintf(OutFile, "[global %s]\n", Func->Name);
	fprintf(OutFile, "%s:\n", Func->Name);
	fprintf(OutFile, "\tPUSH R13\n");
	fprintf(OutFile, "\tMOV R13, R14\n");
	// F*** keeping SP unchanged throughout the function
	
	// --- Process function block
	VM16CISC_ProcessBlock(OutFile, 0, Func->Code);
	
	// --- Function Epilouge
	fprintf(OutFile, ".ret:\n");
	// --(lea esp, [R13])--
	fprintf(OutFile, "\tPOP R13\n");
	fprintf(OutFile, "\tRET\n");
	
	return 0;
}

void VM16CISC_ProcessBlock(FILE *OutFile, int CurBPOfs, tAST_Node *Node)
{
	tAST_Node	*tmp;
	
	if(Node->Type != NODETYPE_BLOCK)
	{
		return VM16CISC_ProcessNode(OutFile, CurBPOfs, Node);
	}
	
	
	// Prescan, determining the size of stack frame required
	// TODO
	
	// Parse Block
	for(tmp = Node->CodeBlock.FirstStatement;
		tmp;
		tmp = tmp->NextSibling)
	{
		VM16CISC_ProcessBlock(OutFile, CurBPOfs, tmp);
	}
}

/**
 * \brief Generates code from a root node (assign, return, for, while, function, ...)
 */
void VM16CISC_ProcessNode(FILE *OutFile, int CurBPOfs, tAST_Node *Node)
{
	switch(Node->Type)
	{
	case NODETYPE_IF:
		VM16CISC_DoAction(OutFile, CurBPOfs, Node->If.Test, DEF_REGISTERS);
		fprintf(OutFile, "\tTEST R1, R1\n");
		fprintf(OutFile, "\tJZ .if_%p_false\n", Node);
		VM16CISC_ProcessBlock(OutFile, CurBPOfs, Node->If.True);
		fprintf(OutFile, "\tJMP .if_%p_end\n", Node);
		fprintf(OutFile, ".if_%p_false:\n", Node);
		VM16CISC_ProcessBlock(OutFile, CurBPOfs, Node->If.False);
		fprintf(OutFile, ".if_%p_end:\n", Node);
		fprintf(OutFile, "\n");
		break;
	
	case NODETYPE_RETURN:
		VM16CISC_DoAction(OutFile, CurBPOfs, Node->UniOp.Value, DEF_REGISTERS);
		fprintf(OutFile, "\tJMP .ret\n");
		break;
	default:
		VM16CISC_DoAction(OutFile, CurBPOfs, Node, DEF_REGISTERS);
		//fprintf(stderr, "VM16CISC_ProcessNode: Unexpected 0x%03x\n", Node->Type);
		return;
	}
}

void VM16CISC_DoAction(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint16_t Registers)
{
	tAST_Node	*child;
	 int	reg, ofs, i;
	switch( Node->Type )
	{
	case NODETYPE_INTEGER:
		fprintf(OutFile, "\tMOV R1, 0x%lx\n", Node->Integer.Value);
		break;
	
	case NODETYPE_STRING:
		{
		 int	inString = 0;
		fprintf(OutFile, "[section .rodata]\n_str_%p:\td8 ", Node->String.Data);
		for( i = 0; i < Node->String.Length; i++) {
			uint8_t	ch = ((char*)Node->String.Data)[i];
			if( ' ' <= ch && ch < 0x7F )
			{
				if(inString == 0)	fprintf(OutFile, "\"");
				inString = 1;
				if(ch == '"')
					fprintf(OutFile, "\\\"");
				else
					fprintf(OutFile, "%c", ch);
			}
			else {
				if(inString == 1)	fprintf(OutFile, "\", ");
				inString = 0;
				fprintf(OutFile, "%i, ", ch);
			}
		}
		if(inString == 1)	fprintf(OutFile, "\", ");
		fprintf(OutFile, "0\n[section .text]\n");
		fprintf(OutFile, "\tMOV R1, _str_%p\n", Node->String.Data);
		}
		break;
	
	case NODETYPE_LOCALVAR:
		fprintf(OutFile, "\t; NODETYPE_LOCALVAR ('%s')\n", Node->LocalVariable.Sym->Name);
		if( Node->LocalVariable.Sym->Offset < 0 ) {
			ofs = -Node->LocalVariable.Sym->Offset + 1;	// [R13] == OldR13, [R13+8] == RetAddr
		}
		else {
			ofs = Node->LocalVariable.Sym->Offset - 1;
		}
		ofs *= WORDSIZE;
		
		if(ofs < 0)
			fprintf(OutFile, "\tMOV R1, WORD [R13-0x%x]\n", -ofs);
		else
			fprintf(OutFile, "\tmov R1, WORD [R13+0x%x]\n", ofs);
		break;
	
	case NODETYPE_SYMBOL:
		fprintf(OutFile, "\t; NODETYPE_SYMBOL ('%s')\n", Node->Symbol.Name);
		fprintf(OutFile, "\tMOV R1, WORD [$%s]\n", Node->Symbol.Name);
		break;
	
	
	// Function Call
	case NODETYPE_FUNCTIONCALL:
		i = 0;
		for(child = Node->FunctionCall.FirstArgument;
			child;
			child = child->NextSibling
			)
		{
			i ++;
		}
		ofs = i;
		fprintf(OutFile, "\tSUB R14, 0x%x\n", ofs*WORDSIZE);
		i = 0;
		for(child = Node->FunctionCall.FirstArgument;
			child;
			child = child->NextSibling
			)
		{
			VM16CISC_ProcessNode(OutFile, CurBPOfs, child);
			fprintf(OutFile, "\tMOV WORD [R14+0x%x], R1\t; Argument %i for %p\n", i*WORDSIZE, i, Node);
			i ++;
		}
		VM16CISC_GetAddress(OutFile, CurBPOfs, Node->FunctionCall.Function, DEF_REGISTERS);
		fprintf(OutFile, "\tCALL R1\n");
		fprintf(OutFile, "\tADD R14, 0x%x\n", ofs*WORDSIZE);
		break;
	
	// Assignment
	case NODETYPE_ASSIGN:
		fprintf(OutFile, "\t; NODETYPE_ASSIGN (%p = %p)\n", Node->Assign.To, Node->Assign.From);
		VM16CISC_DoAction(OutFile, CurBPOfs, Node->Assign.From, Registers);
		VM16CISC_SaveTo(OutFile, CurBPOfs, Node->Assign.To, Registers);
		break;
	
	// Simple Binary Operations
	case NODETYPE_ADD:
	case NODETYPE_SUBTRACT:
		fprintf(OutFile, "\t; 0x%03x (%p, %p)\n", Node->Type, Node->BinOp.Left, Node->BinOp.Right);
		// Get the left
		VM16CISC_DoAction(OutFile, CurBPOfs, Node->BinOp.Left, Registers);
		
		// Save it to a register
		reg = VM16CISC_Int_AllocReg(&Registers);
		fprintf(OutFile, "\tMOV R%i, R1\n", reg);
		
		// Get the right
		VM16CISC_DoAction(OutFile, CurBPOfs, Node->BinOp.Right, Registers);
		
		// Do operation
		switch(Node->Type)
		{
		case NODETYPE_ADD:
			fprintf(OutFile, "\tADD R1, R%i\n", reg);
			break;
		case NODETYPE_SUBTRACT:
			fprintf(OutFile, "\tSUB R1, R%i\n", reg);
			break;
		}
		
		Registers &= ~(1<<reg);
		break;
	
	default:
		fprintf(stderr, "VM16CISC_DoAction: Unknown node 0x%03x\n", Node->Type);
		exit(1);
		break;
	}
}

void VM16CISC_GetAddress(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint16_t Registers)
{
	 int	ofs;
	switch( Node->Type )
	{
	case NODETYPE_DEREF:		
		// Get Address
		VM16CISC_DoAction(OutFile, CurBPOfs, Node->UniOp.Value, Registers);
		break;
	
	case NODETYPE_SYMBOL:
		fprintf(OutFile, "\tMOV R1, $%s\n", Node->Symbol.Name);
		break;
	
	case NODETYPE_LOCALVAR:
		fprintf(OutFile, "\t; NODETYPE_LOCALVAR ('%s')\n", Node->LocalVariable.Sym->Name);
		if( Node->LocalVariable.Sym->Offset < 0 ) {
			ofs = -Node->LocalVariable.Sym->Offset + 1;	// [R13] == OldR13, [R13+4] == RetAddr
		}
		else {
			ofs = Node->LocalVariable.Sym->Offset - 1;
		}
		ofs *= WORDSIZE;
		
		if(ofs < 0)
			fprintf(OutFile, "\tLEA R1, [R13-0x%x]\n", -ofs);
		else
			fprintf(OutFile, "\tLEA R1, [R13+0x%x]\n", ofs);
		break;
	
	default:
		fprintf(stderr, "Uhoh! Non pointer value in VM16CISC_GetAddress\n");
		exit(1);
	}
}

/**
 * 
 */
void VM16CISC_SaveTo(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint16_t Registers)
{
	 int	reg, ofs;
	switch( Node->Type )
	{
	case NODETYPE_DEREF:
		// Save R1 somewhere
		reg = VM16CISC_Int_AllocReg(&Registers);
		fprintf(OutFile, "\tMOV R%i, R1\n", reg);
		
		// Get Address
		VM16CISC_DoAction(OutFile, CurBPOfs, Node->UniOp.Value, Registers);
		#if 0
		switch( GetPtrSize( Node->UniOp.Value ) )
		{
		//case  8: fprintf(OutFile, "mov BYTE [R1], R%i\n", reg);	break;
		case 16: fprintf(OutFile, "mov DBYTE [R1], R%i\n", reg);	break;
		case 32: fprintf(OutFile, "mov HWORD [R1], R%i\n", reg);	break;
		default: fprintf(stderr, "ARRRGH!!!!\n");	exit(1);
		}
		#else
		fprintf(OutFile, "\tmov WORD [R1], R%i\n", reg);
		#endif
		break;
	
	case NODETYPE_SYMBOL:
		fprintf(OutFile, "\tmov WORD [$%s], R1\n", Node->Symbol.Name);
		break;
	
	case NODETYPE_LOCALVAR:
		fprintf(OutFile, "\t; NODETYPE_LOCALVAR ('%s')\n", Node->LocalVariable.Sym->Name);
		if( Node->LocalVariable.Sym->Offset < 0 ) {
			ofs = -Node->LocalVariable.Sym->Offset + 1;	// [R13] == OldR13, [R13+4] == RetAddr
		}
		else {
			ofs = Node->LocalVariable.Sym->Offset - 1;
		}
		ofs *= 4;
		
		if(ofs < 0)
			fprintf(OutFile, "\tmov WORD [R13-0x%x], R1\n", -ofs);
		else
			fprintf(OutFile, "\tmov WORD [R13+0x%x], R1\n", ofs);
		break;
	
	default:
		fprintf(stderr, "Uhoh! Non pointer value in VM16CISC_SaveTo\n");
		exit(1);
	}
}

/**
 * \brief Allocate a register
 */
int VM16CISC_Int_AllocReg(uint16_t *Registers)
{
	 int	i;
	
	for(i = 0; i < 16; i++)
	{
		if(*Registers & (1 << i)) {
			*Registers |= (1 << i);
			return i;
		}
	}
	fprintf(stderr, "ERROR: Out of registers\n");
	exit(1);
	return 0;
}
