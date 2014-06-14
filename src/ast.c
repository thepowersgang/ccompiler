/*
 * 
 */
#include <global.h>
#include <ast.h>
#include <stdio.h>
#include <string.h>

// === PROTOTYPES ===
void	AST_DumpTree(tAST_Node *Node, int Depth);
tAST_Node	*AST_NewNode(int Type);
tAST_Node	*AST_AppendNode(tAST_Node *Parent, tAST_Node *Child);
tAST_Node	*AST_NewNoOp(void);
tAST_Node	*AST_NewCodeBlock(void);
tAST_Node	*AST_NewIf(tAST_Node *Test, tAST_Node *True, tAST_Node *False);
tAST_Node	*AST_NewWhile(tAST_Node *Test, tAST_Node *Code);
tAST_Node	*AST_NewFor(tAST_Node *Init, tAST_Node *Test, tAST_Node *Inc, tAST_Node *Code);
tAST_Node	*AST_NewFunctionCall(tAST_Node *Function);
tAST_Node	*AST_NewAssign(tAST_Node *To, tAST_Node *From);
tAST_Node	*AST_NewAssignOp(tAST_Node *To, int Op, tAST_Node *From);
tAST_Node	*AST_NewBinOp(int Op, tAST_Node *Left, tAST_Node *Right);
tAST_Node	*AST_NewUniOp(int Op, tAST_Node *Value);
tAST_Node	*AST_NewLocalVar(tSymbol *Sym);
tAST_Node	*AST_NewString(void *Data, size_t Length);
tAST_Node	*AST_NewInteger(uint64_t Value);
tAST_Node	*AST_NewArrayIndex(tAST_Node *Var, tAST_Node *Index);
void	AST_DeleteNode(tAST_Node *Node);

// === CODE ===
void AST_DumpTree(tAST_Node *Node, int Depth)
{
	 int	i;
	tAST_Node	*tmp;
	char	pad[Depth+1];
	
	for(i=0;i<Depth;i++)	pad[i] = '\t';
	pad[i] = '\0';
	printf("%s", pad);
	
	switch(Node->Type)
	{
	case NODETYPE_NULL:
		printf("NULL\n");
		break;
	
	// --- Leaves ---
	case NODETYPE_INTEGER:
		printf("Constant - 0x%lx\n", Node->Integer.Value);
		break;
	case NODETYPE_SYMBOL:
		printf("Symbol - '%s' %p\n", Node->Symbol.Name, Node->Symbol.Sym);
		break;
	
	// --- Branches ---
	case NODETYPE_FUNCTIONCALL:
		printf("Function Call: {\n");
		printf("%s-Name: {\n", pad);
		AST_DumpTree(Node->FunctionCall.Function, Depth+1);
		printf("%s}\n", pad);
		printf("%s-Arguments: {\n", pad);
		tmp = Node->FunctionCall.FirstArgument;
		while(tmp)
		{
			AST_DumpTree(tmp, Depth+1);
			tmp = tmp->NextSibling;
		}
		printf("%s}\n", pad);
		break;
	
	case NODETYPE_BLOCK:
		printf("Code Block: {\n");
		tmp = Node->UniOp.Value;
		while(tmp)
		{
			AST_DumpTree(tmp, Depth+1);
			tmp = tmp->NextSibling;
		}
		printf("%s}\n", pad);
		break;
	case NODETYPE_IF:
		printf("If\n");
		printf("%s-Condition {\n", pad);
		AST_DumpTree(Node->If.Test, Depth+1);
		printf("%s}\n", pad);
		printf("%s-Action {\n", pad);
		AST_DumpTree(Node->If.True, Depth+1);
		printf("%s}\n", pad);
		printf("%s-Else {\n", pad);
		AST_DumpTree(Node->If.False, Depth+1);
		printf("%s}\n", pad);
		break;
	
	// Statements
	case NODETYPE_RETURN:
		printf("Return {\n");
		AST_DumpTree(Node->UniOp.Value, Depth+1);
		printf("%s}\n", pad);
		break;
	
	// Binary Operations
	case NODETYPE_ASSIGN:
		printf("Assign\n");
		printf("%s-Left {\n", pad);
		AST_DumpTree(Node->Assign.To, Depth+1);
		printf("%s}\n", pad);
		printf("%s-Right {\n", pad);
		AST_DumpTree(Node->Assign.From, Depth+1);
		printf("%s}\n", pad);
		break;
	
	case NODETYPE_ADD:
		printf("Add\n");
		goto binCommon;
	case NODETYPE_SUBTRACT:
		printf("Subtract\n");
		goto binCommon;
	case NODETYPE_MULTIPLY:
		printf("Multiply\n");
		goto binCommon;
	case NODETYPE_DIVIDE:
		printf("Divide\n");
		goto binCommon;
	binCommon:
		printf("%s-Left {\n", pad);
		AST_DumpTree(Node->Assign.To, Depth+1);
		printf("%s}\n", pad);
		printf("%s-Right {\n", pad);
		AST_DumpTree(Node->Assign.From, Depth+1);
		printf("%s}\n", pad);
		break;
	
	default:
		printf("%i:%i\n", Node->Type>>8, Node->Type&0xFF);
		break;
	}
}

tAST_Node *AST_NewNode(int Type)
{
	tAST_Node	*ret = malloc(sizeof(tAST_Node));
	ret->Type = Type;
	ret->NextSibling = NULL;
	return ret;
}

tAST_Node *AST_AppendNode(tAST_Node *Parent, tAST_Node *Child)
{
	tAST_Node	*node;
	switch(Parent->Type)
	{
	case NODETYPE_BLOCK:
		if( !Parent->CodeBlock.FirstStatement )
		{
			Parent->CodeBlock.FirstStatement = Child;
			Parent->CodeBlock.LastStatement = Child;
		}
		else
		{
			Parent->CodeBlock.LastStatement->NextSibling = Child;
			Parent->CodeBlock.LastStatement = Child;
		}
		break;
	case NODETYPE_FUNCTIONCALL:
		node = Parent->FunctionCall.FirstArgument;
		if(node)
		{
			while(node->NextSibling)	node = node->NextSibling;
			node->NextSibling = Child;
		}
		else
			Parent->FunctionCall.FirstArgument = Child;
		break;

	default:
		// Fatal Internal Error
		break;
	}
	return Parent;
}

tAST_Node *AST_NewNoOp(void)
{
	return AST_NewNode(NODETYPE_NOOP);
}

tAST_Node *AST_NewCodeBlock(void)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_BLOCK);
	ret->CodeBlock.FirstStatement = NULL;
	ret->CodeBlock.LastStatement = NULL;
	return ret;
}

tAST_Node *AST_NewIf(tAST_Node *Test, tAST_Node *True, tAST_Node *False)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_IF);
	ret->If.Test = Test;
	ret->If.True = True;
	ret->If.False = False;
	return ret;
}

tAST_Node *AST_NewWhile(tAST_Node *Test, tAST_Node *Code)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_WHILE);
	ret->While.Test = Test;
	ret->While.Action = Code;
	return ret;
}

tAST_Node *AST_NewFor(tAST_Node *Init, tAST_Node *Test, tAST_Node *Inc, tAST_Node *Code)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_FOR);
	ret->For.Init = Init;
	ret->For.Test = Test;
	ret->For.Next = Inc;
	ret->For.Action = Code;
	return ret;
}

tAST_Node *AST_NewFunctionCall(tAST_Node *Function)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_FUNCTIONCALL);
	ret->FunctionCall.Function = Function;
	ret->FunctionCall.FirstArgument = NULL;
	return ret;
}

tAST_Node *AST_NewAssign(tAST_Node *To, tAST_Node *From)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_ASSIGN);
	ret->Assign.To = To;
	ret->Assign.From = From;
	return ret;
}

tAST_Node *AST_NewAssignOp(tAST_Node *To, int Op, tAST_Node *From)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_ASSIGNOP);
	ret->AssignOp.Op = Op;
	ret->AssignOp.To = To;
	ret->AssignOp.From = From;
	return ret;
}

tAST_Node *AST_NewBinOp(int Op, tAST_Node *Left, tAST_Node *Right)
{
	tAST_Node	*ret = AST_NewNode(Op);
	ret->BinOp.Left = Left;
	ret->BinOp.Right = Right;
	return ret;
}

tAST_Node *AST_NewUniOp(int Op, tAST_Node *Value)
{
	tAST_Node	*ret = AST_NewNode(Op);
	ret->UniOp.Value = Value;
	return ret;
}

tAST_Node *AST_NewCast(const tType *Type, tAST_Node *Value)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_CAST);
	ret->Cast.Value = Value;
	ret->Cast.Type = Type;
	return ret;
}

tAST_Node *AST_NewSymbol(const char *Name)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_SYMBOL);
	ret->Symbol.Sym = NULL;
	ret->Symbol.Name = strdup(Name);
	return ret;
}

tAST_Node *AST_NewLocalVar(tSymbol *Sym)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_LOCALVAR);
	ret->Symbol.Sym = Sym;
	return ret;
}

tAST_Node	*AST_NewString(void *Data, size_t Length)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_STRING);
	ret->String.Data = Data;
	ret->String.Length = Length;
	return ret;
}

tAST_Node *AST_NewInteger(uint64_t Value)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_INTEGER);
	ret->Integer.Value = Value;
	return ret;
}

tAST_Node *AST_NewArrayIndex(tAST_Node *Var, tAST_Node *Index)
{
	tAST_Node	*ret = AST_NewNode(NODETYPE_INDEX);
	ret->BinOp.Left = Var;
	ret->BinOp.Right = Index;
	return ret;
}

void AST_DeleteNode(tAST_Node *Node)
{
	switch(Node->Type)
	{
	case NODETYPE_INTEGER:
		free(Node);
		break;
	
	default:
		fprintf(stderr, "Why are you deleting me? (%p)\n", Node);
		exit(-1);
	}
}
