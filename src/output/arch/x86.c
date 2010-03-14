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

#define ASM_OUTPUT	1

// === PROTOTYPES ===
tOutput_Function	*X86_GenerateFunction(tFunction *Func);
void	X86_ProcessBlock(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node);
void	X86_ProcessNode(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node);
void	X86_DoAction(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node, uint8_t Registers);
 int	X86_Int_AllocReg(uint8_t *Registers);

const char * const csaRegEX[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "edi", "esi"};

// === CODE ===
tOutput_Function *X86_GenerateFunction(tFunction *Func)
{
	tOutput_Function	*ret = calloc(1, sizeof(tOutput_Function));
	// int	curBP = 0;
	
	printf("X86_GenerateFunction: (Func=%p{Name:'%s',Code:%p,...})\n",
		Func, Func->Name, Func->Code);
	
	// --- Function Prolouge
	#if ASM_OUTPUT
	printf("%s:\n", Func->Name);
	printf("\tpush ebp\n");
	printf("\tmov ebp, esp\n");
	// F*** keeping SP unchanged throughout the function
	#else
	Output_AppendCode(ret, 0x54);	// PUSH_BP
	Output_AppendCode(ret, 0x8B);	// MOV_RMX
	Output_AppendCode(ret, 0xE5);	// ModRM (mod=0b11,r=4,m=5)
	#endif
	
	// --- Process function block
	X86_ProcessBlock(ret, 0, Func->Code);
	
	// --- Function Epilouge
	#if ASM_OUTPUT
	printf(".ret:\n");
	// --(lea esp, [ebp])--
	printf("\tpop ebp\n");
	printf("\tret\n");
	#else
	Output_AppendCode(ret, 0x5C);	// POP_BP
	Output_AppendCode(ret, 0xC3);	// RET_N
	#endif
	
	return ret;
}

void X86_ProcessBlock(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node)
{
	tAST_Node	*tmp;
	
	if(Node->Type != NODETYPE_BLOCK)
	{
		return X86_ProcessNode(Func, CurBPOfs, Node);
	}
	
	
	// Prescan, determining the size of stack frame required
	// TODO
	
	// Parse Block
	for(tmp = Node->CodeBlock.FirstStatement;
		tmp;
		tmp = tmp->NextSibling)
	{
		X86_ProcessBlock(Func, CurBPOfs, tmp);
	}
}

/**
 * \brief Generates code from a root node (assign, return, for, while, function, ...)
 */
void X86_ProcessNode(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node)
{
	switch(Node->Type)
	{
	case NODETYPE_RETURN:
		X86_DoAction(Func, CurBPOfs, Node->UniOp.Value, 0x18);
		#if ASM_OUTPUT
		printf("\tjmp .ret\n");
		#else
		// jmp .end
		Output_AppendCode(Func, 0xEB);	// JMP S (16 signed)
		Output_AppendReloc16(Func, -Func->CodeLength, ".ret");
		#endif
		break;
	default:
		fprintf(stderr, "X86_ProcessNode: Unexpected 0x%03x\n", Node->Type);
		return;
	}
}

void X86_DoAction(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node, uint8_t Registers)
{
	 int	reg, ofs;
	switch( Node->Type )
	{
	case NODETYPE_INTEGER:
		#if SUPPORT_LLINT
		if(Node->Integer.Value > ((uint64_t)1<<32)) {
			// mov edx, value >> 32
			Output_AppendCode(Func, 0xC7);	// MOV RIX
			Output_AppendCode(Func, 0x10);	// (mod=0, r=edx(2), m=0)
			Output_AppendAbs32(Func, Node->Integer.Value >> 32);
		}
		#else
		if(Node->Integer.Value > ((uint64_t)1<<32)) {
			fprintf(stderr, "ERROR: double native integers are unsupported\n");
			exit(-1);
		}
		#endif
		// mov eax, value
		#if ASM_OUTPUT
		printf("\tmov eax, 0x%08x", (uint32_t)Node->Integer.Value & 0xFFFFFFFF);
		#else
		Output_AppendCode(Func, 0xC7);	// MOV RIX
		Output_AppendCode(Func, 0x00);	// (mod=0, r=eax(0), m=0)
		Output_AppendAbs32(Func, Node->Integer.Value & 0xFFFFFFFF);
		#endif
		break;
	
	case NODETYPE_LOCALVAR:
		printf("\t; NODETYPE_LOCALVAR ('%s')\n", Node->LocalVariable.Sym->Name);
		if( Node->LocalVariable.Sym->Offset < 0 ) {
			ofs = -Node->LocalVariable.Sym->Offset + 1;	// [EBP] == OldEBP, [EBP+4] == RetAddr
		}
		else {
			ofs = Node->LocalVariable.Sym->Offset - 1;
		}
		ofs *= 4;
		#if ASM_OUTPUT
		if(ofs < 0)
			printf("\tmov eax, [ebp-0x%x]\n", -ofs);
		else
			printf("\tmov eax, [ebp+0x%x]\n", ofs);
		#else
		#endif
		break;
	
	case NODETYPE_SYMBOL:
		printf("\t; NODETYPE_SYMBOL ('%s')\n", Node->Symbol.Name);
		#if ASM_OUTPUT
		printf("\tmov eax, [%s]\n", Node->Symbol.Name);
		#else
		#endif
		break;
	
	// Simple Binary Operations
	case NODETYPE_ADD:
	case NODETYPE_SUBTRACT:
		printf("\t; 0x%03x (%p, %p)\n", Node->Type, Node->BinOp.Left, Node->BinOp.Right);
		// Get the left
		X86_DoAction(Func, CurBPOfs, Node->BinOp.Left, Registers);
		
		// Save it to a register
		reg = X86_Int_AllocReg(&Registers);
		#if ASM_OUTPUT
		printf("\tmov %s, eax\n", csaRegEX[reg]);
		#else
		Output_AppendCode(Func, 0x8B);	// MOV RMX
		Output_AppendCode(Func, 0xC0|(reg<<3));	// (mod=3, r=reg, m=eax(0))
		#endif
		
		// Get the right
		X86_DoAction(Func, CurBPOfs, Node->BinOp.Right, Registers);
		
		// Do operation
		switch(Node->Type)
		{
		case NODETYPE_ADD:
			#if ASM_OUTPUT
			printf("\tadd eax, %s\n", csaRegEX[reg]);
			#else
			Output_AppendCode(Func, 0x03);	// ADD RMX
			Output_AppendCode(Func, 0xC0|(reg));	// (mod=3, r=eax(0), m=?)
			#endif
			break;
		case NODETYPE_SUBTRACT:
			#if ASM_OUTPUT
			printf("\tsub eax, %s\n", csaRegEX[reg]);
			#else
			Output_AppendCode(Func, 0x2B);	// ADD RMX
			Output_AppendCode(Func, 0xC0|(reg));	// (mod=3, r=eax(0), m=?)
			#endif
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
