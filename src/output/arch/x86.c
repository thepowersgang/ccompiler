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
tOutput_Function	*X86_GenerateFunction(tFunction *Func);
void	X86_ProcessBlock(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node);
void	X86_ProcessNode(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node);
void	X86_DoAction(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node);

// === CODE ===
tOutput_Function *X86_GenerateFunction(tFunction *Func)
{
	tOutput_Function	*ret = calloc(1, sizeof(tOutput_Function));
	// int	curBP = 0;
	
	printf("X86_GenerateFunction: (Func=%p{Name:'%s',Code:%p,...})\n",
		Func, Func->Name, Func->Code);
	
	// --- Function Prolouge
	// push ebp
	Output_AppendCode(ret, 0x54);	// PUSH_BP
	// mov ebp, esp
	Output_AppendCode(ret, 0x8B);	// MOV_RMX
	Output_AppendCode(ret, 0xE5);	// ModRM (mod=0b11,r=4,m=5)
	
	// --- Process function block
	X86_ProcessBlock(ret, 0, Func->Code);
	
	// --- Function Epilouge
	// --(lea esp, [ebp])--
	// pop ebp
	Output_AppendCode(ret, 0x5C);	// POP_BP
	// ret
	Output_AppendCode(ret, 0xC3);	// RET_N
	
	return ret;
}

void X86_ProcessBlock(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node)
{
	tAST_Node	*tmp;
	
	printf("X86_ProcessBlock: (..., Node={Type:0x%03x,...})\n", Node->Type);
	
	// Prescan, determining the size of stack frame required
	// Convert (using X86_ProcessNode)
	if(Node->Type != NODETYPE_BLOCK)
	{
		return X86_ProcessNode(Func, CurBPOfs, Node);
	}
	
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
		X86_DoAction(Func, CurBPOfs, Node->UniOp.Value);
		// jmp .end
		Output_AppendCode(Func, 0xEB);	// JMP S (16 signed)
		Output_AppendReloc16(Func, -Func->CodeLength, ".ret");
		break;
	default:
		fprintf(stderr, "X86_ProcessNode: Unexpected 0x%03x\n", Node->Type);
		return;
	}
}

void X86_DoAction(tOutput_Function *Func, int CurBPOfs, tAST_Node *Node)
{
	
}
