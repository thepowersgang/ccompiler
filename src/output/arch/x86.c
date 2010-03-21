/*
 * PHPOS2
 * By John Hodge (thePowersGang)
 *
 * See COPYING for licence
 *
 * output/x86.c
 * - Outputs the AST as x86
 */
#include <global.h>
#include <stdio.h>
#include <string.h>
#include <ast.h>
#include <symbol.h>
#include <output.h>

// === PROTOTYPES ===
 int	X86_GenerateFunction(FILE *OutFile, tFunction *Func);
void	X86_ProcessBlock(FILE *OutFile, int CurBPOfs, tAST_Node *Node);
void	X86_ProcessNode(FILE *OutFile, int CurBPOfs, tAST_Node *Node);
void	X86_DoAction(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint8_t Registers);
void	X86_GetAddress(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint8_t Registers);
void	X86_SaveTo(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint8_t Registers);
 int	X86_Int_AllocReg(uint8_t *Registers);

const char * const csaRegB[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
const char * const csaRegX[] = {"ax", "cx", "dx", "bx", "sp", "bp", "di", "si"};
const char * const csaRegEX[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "edi", "esi"};

// === CODE ===
int X86_GenerateProlouge(FILE *OutFile)
{
	tSymbol	*sym;
	fprintf(OutFile, "; File Generated by Acess CC\n");
	fprintf(OutFile, "[bits 32]\n");
	fprintf(OutFile, "\n");
	fprintf(OutFile, "[section .data]\n");
	for(sym = gpGlobalSymbols;
		sym;
		sym = sym->Next
		)
	{
		fprintf(
			OutFile,
			"%s:\ttimes %i dd 0\n",
			sym->Name,
			sym->Type->Size
			);
	}
	fprintf(OutFile, "\n");
	fprintf(OutFile, "[section .rodata]\n");
	fprintf(OutFile, "\n");
	fprintf(OutFile, "\n");
	fprintf(OutFile, "[section .text]\n");
	return 0;
}

int X86_GenerateFunction(FILE *OutFile, tFunction *Func)
{
	// int	curBP = 0;
	
	//printf("X86_GenerateFunction: (Func=%p{Name:'%s',Code:%p,...})\n",
	//	Func, Func->Name, Func->Code);
	
	// --- Function Prolouge
	fprintf(OutFile, "[global %s]\n", Func->Name);
	fprintf(OutFile, "%s:\n", Func->Name);
	fprintf(OutFile, "\tpush ebp\n");
	fprintf(OutFile, "\tmov ebp, esp\n");
	// F*** keeping SP unchanged throughout the function
	
	// --- Process function block
	X86_ProcessBlock(OutFile, 0, Func->Code);
	
	// --- Function Epilouge
	fprintf(OutFile, ".ret:\n");
	// --(lea esp, [ebp])--
	fprintf(OutFile, "\tpop ebp\n");
	fprintf(OutFile, "\tret\n");
	
	return 0;
}

void X86_ProcessBlock(FILE *OutFile, int CurBPOfs, tAST_Node *Node)
{
	tAST_Node	*tmp;
	
	if(Node->Type != NODETYPE_BLOCK)
	{
		return X86_ProcessNode(OutFile, CurBPOfs, Node);
	}
	
	
	// Prescan, determining the size of stack frame required
	// TODO
	
	// Parse Block
	for(tmp = Node->CodeBlock.FirstStatement;
		tmp;
		tmp = tmp->NextSibling)
	{
		X86_ProcessBlock(OutFile, CurBPOfs, tmp);
	}
}

/**
 * \brief Generates code from a root node (assign, return, for, while, function, ...)
 */
void X86_ProcessNode(FILE *OutFile, int CurBPOfs, tAST_Node *Node)
{
	switch(Node->Type)
	{
	case NODETYPE_IF:
		X86_DoAction(OutFile, CurBPOfs, Node->If.Test, 0x18);
		printf("\tcmp eax, 0\n");
		printf("\tjz .if_%p_false\n", Node);
		X86_ProcessBlock(OutFile, CurBPOfs, Node->If.True);
		printf("\tjmp .if_%p_end\n", Node);
		printf(".if_%p_false:\n", Node);
		X86_ProcessBlock(OutFile, CurBPOfs, Node->If.False);
		printf(".if_%p_end:\n", Node);
		printf("\n");
		break;
	
	case NODETYPE_RETURN:
		X86_DoAction(OutFile, CurBPOfs, Node->UniOp.Value, 0x18);
		printf("\tjmp .ret\n");
		break;
	default:
		X86_DoAction(OutFile, CurBPOfs, Node, 0x18);
		//fprintf(stderr, "X86_ProcessNode: Unexpected 0x%03x\n", Node->Type);
		return;
	}
}

void X86_DoAction(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint8_t Registers)
{
	tAST_Node	*child;
	 int	reg, ofs;
	switch( Node->Type )
	{
	case NODETYPE_INTEGER:
		#if SUPPORT_LLINT
		if(Node->Integer.Value > ((uint64_t)1<<32)) {
			// mov edx, value >> 32
			fprintf(OutFile, "\tmov edx, 0x%08x", (uint32_t)(Node->Integer.Value >> 32));
		}
		#else
		if(Node->Integer.Value > ((uint64_t)1<<32)) {
			fprintf(stderr, "ERROR: double native integers are unsupported\n");
			exit(-1);
		}
		#endif
		fprintf(OutFile, "\tmov eax, 0x%08x\n", (uint32_t)Node->Integer.Value & 0xFFFFFFFF);
		break;
	
	case NODETYPE_LOCALVAR:
		fprintf(OutFile, "\t; NODETYPE_LOCALVAR ('%s')\n", Node->LocalVariable.Sym->Name);
		if( Node->LocalVariable.Sym->Offset < 0 ) {
			ofs = -Node->LocalVariable.Sym->Offset + 1;	// [EBP] == OldEBP, [EBP+4] == RetAddr
		}
		else {
			ofs = Node->LocalVariable.Sym->Offset - 1;
		}
		ofs *= 4;
		
		if(ofs < 0)
			fprintf(OutFile, "\tmov eax, [ebp-0x%x]\n", -ofs);
		else
			fprintf(OutFile, "\tmov eax, [ebp+0x%x]\n", ofs);
		break;
	
	case NODETYPE_SYMBOL:
		fprintf(OutFile, "\t; NODETYPE_SYMBOL ('%s')\n", Node->Symbol.Name);
		fprintf(OutFile, "\tmov eax, [%s]\n", Node->Symbol.Name);
		break;
	
	
	// Function Call
	case NODETYPE_FUNCTIONCALL:
		ofs = 0;
		for(child = Node->FunctionCall.FirstArgument;
			child;
			child = child->NextSibling
			)
		{
			X86_ProcessNode(OutFile, CurBPOfs, child);
			fprintf(OutFile, "\tpush eax\t; Argument %i for %p\n", ofs, Node);
			ofs ++;
		}
		X86_GetAddress(OutFile, CurBPOfs, Node->FunctionCall.Function, 0x18);
		fprintf(OutFile, "\tcall eax\n");
		fprintf(OutFile, "\tadd esp, 0x%x\n", ofs*4);
		break;
	
	// Assignment
	case NODETYPE_ASSIGN:
		fprintf(OutFile, "\t; NODETYPE_ASSIGN (%p = %p)\n", Node->Assign.To, Node->Assign.From);
		X86_DoAction(OutFile, CurBPOfs, Node->Assign.From, Registers);
		X86_SaveTo(OutFile, CurBPOfs, Node->Assign.To, Registers);
		break;
	
	// Simple Binary Operations
	case NODETYPE_ADD:
	case NODETYPE_SUBTRACT:
		fprintf(OutFile, "\t; 0x%03x (%p, %p)\n", Node->Type, Node->BinOp.Left, Node->BinOp.Right);
		// Get the left
		X86_DoAction(OutFile, CurBPOfs, Node->BinOp.Left, Registers);
		
		// Save it to a register
		reg = X86_Int_AllocReg(&Registers);
		fprintf(OutFile, "\tmov %s, eax\n", csaRegEX[reg]);
		
		// Get the right
		X86_DoAction(OutFile, CurBPOfs, Node->BinOp.Right, Registers);
		
		// Do operation
		switch(Node->Type)
		{
		case NODETYPE_ADD:
			fprintf(OutFile, "\tadd eax, %s\n", csaRegEX[reg]);
			break;
		case NODETYPE_SUBTRACT:
			fprintf(OutFile, "\tsub eax, %s\n", csaRegEX[reg]);
			break;
		}
		
		Registers &= ~(1<<reg);
		break;
	
	default:
		fprintf(stderr, "X86_DoAction: Unknown node 0x%03x\n", Node->Type);
		exit(1);
		break;
	}
}

void X86_GetAddress(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint8_t Registers)
{
	 int	ofs;
	switch( Node->Type )
	{
	case NODETYPE_DEREF:		
		// Get Address
		X86_DoAction(OutFile, CurBPOfs, Node->UniOp.Value, Registers);
		break;
	
	case NODETYPE_SYMBOL:
		fprintf(OutFile, "\tmov eax, $%s\n", Node->Symbol.Name);
		break;
	
	case NODETYPE_LOCALVAR:
		fprintf(OutFile, "\t; NODETYPE_LOCALVAR ('%s')\n", Node->LocalVariable.Sym->Name);
		if( Node->LocalVariable.Sym->Offset < 0 ) {
			ofs = -Node->LocalVariable.Sym->Offset + 1;	// [EBP] == OldEBP, [EBP+4] == RetAddr
		}
		else {
			ofs = Node->LocalVariable.Sym->Offset - 1;
		}
		ofs *= 4;
		
		if(ofs < 0)
			fprintf(OutFile, "\tlea eax, [ebp-0x%x]\n", -ofs);
		else
			fprintf(OutFile, "\tlea eax, [ebp+0x%x]\n", ofs);
		break;
	
	default:
		fprintf(stderr, "Uhoh! Non pointer value in X86_GetAddress\n");
		exit(1);
	}
}

/**
 * 
 */
void X86_SaveTo(FILE *OutFile, int CurBPOfs, tAST_Node *Node, uint8_t Registers)
{
	 int	reg, ofs;
	switch( Node->Type )
	{
	case NODETYPE_DEREF:
		// Save EAX somewhere
		reg = X86_Int_AllocReg(&Registers);
		fprintf(OutFile, "\tmov %s, eax\n", csaRegEX[reg]);
		
		// Get Address
		X86_DoAction(OutFile, CurBPOfs, Node->UniOp.Value, Registers);
		#if 0
		switch( GetPtrSize( Node->UniOp.Value ) )
		{
		//case  8: fprintf(OutFile, "mov BYTE [eax], %s\n", csaRegB[reg]);	break;
		case 16: fprintf(OutFile, "mov WORD [eax], %s\n", csaRegX[reg]);	break;
		case 32: fprintf(OutFile, "mov DWORD [eax], %s\n", csaRegEX[reg]);	break;
		default: fprintf(stderr, "ARRRGH!!!!\n");	exit(1);
		}
		#else
		fprintf(OutFile, "\tmov DWORD [eax], %s\n", csaRegEX[reg]);
		#endif
		break;
	
	case NODETYPE_SYMBOL:
		fprintf(OutFile, "\tmov [$%s], eax\n", Node->Symbol.Name);
		break;
	
	case NODETYPE_LOCALVAR:
		fprintf(OutFile, "\t; NODETYPE_LOCALVAR ('%s')\n", Node->LocalVariable.Sym->Name);
		if( Node->LocalVariable.Sym->Offset < 0 ) {
			ofs = -Node->LocalVariable.Sym->Offset + 1;	// [EBP] == OldEBP, [EBP+4] == RetAddr
		}
		else {
			ofs = Node->LocalVariable.Sym->Offset - 1;
		}
		ofs *= 4;
		
		if(ofs < 0)
			fprintf(OutFile, "\tmov [ebp-0x%x], eax\n", -ofs);
		else
			fprintf(OutFile, "\tmov [ebp+0x%x], eax\n", ofs);
		break;
	
	default:
		fprintf(stderr, "Uhoh! Non pointer value in X86_SaveTo\n");
		exit(1);
	}
}

/**
 * \brief Allocate a register
 */
int X86_Int_AllocReg(uint8_t *Registers)
{
	 int	i;
	
	for(i = 0; i < 8; i++)
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
