/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 * 
 * This code is published under the terms of the BSD Licence. For more
 * information see the file COPYING.
 * 
 * expr.c - Token to AST Parser
 */
#define	DEBUG	0
#include "global.h"
#include <stdio.h>
#include <strings.h>
#include <memory.h>
#include <symbol.h>
#include <ast.h>
#include <stdbool.h>
#include <assert.h>

// === MACROS ===
#define CMPTOK(str)	(strlen((str))==giTokenLength&&strncmp((str),gsTokenStart,giTokenLength)==0)

// === IMPORTS ===
extern tAST_Node	*Optimiser_StaticOpt(tAST_Node *Node);

// === Prototypes ===
void	Parse_CodeRoot(tParser *Parser);
tType	*Parse_GetType(tParser *Parser);
 int	Parse_DoDefinition(tParser *Parser, tType *Type);
tAST_Node	*Parse_DoTopIdent(tParser *Parser, bool NoCode);
tAST_Node	*DoCodeBlock(tParser *Parser);
tAST_Node	*DoStatement(tParser *Parser);
tAST_Node	*DoIf(tParser *Parser);
tAST_Node	*DoWhile(tParser *Parser);
tAST_Node	*DoFor(tParser *Parser);
tAST_Node	*DoReturn(tParser *Parser);

tAST_Node	*DoExpr0(tParser *Parser);	// Assignment
tAST_Node	*DoExpr1(tParser *Parser);	// Ternary
tAST_Node	*DoExpr2(tParser *Parser);	// Boolean
tAST_Node	*DoExpr3(tParser *Parser);	// Bitwise
tAST_Node	*DoExpr4(tParser *Parser);	// Comparison
tAST_Node	*DoExpr5(tParser *Parser);	// Shifts
tAST_Node	*DoExpr6(tParser *Parser);	// Arithmatic
tAST_Node	*DoExpr7(tParser *Parser);	// Mult/Div
tAST_Node	*DoExpr8(tParser *Parser);	// Right Unaries
tAST_Node	*DoExpr9(tParser *Parser);	// Left Unaries
tAST_Node	*DoMember(tParser *Parser);	// Member dereference
tAST_Node	*DoParen(tParser *Parser);	// 2nd Last - Parens
tAST_Node	*DoValue(tParser *Parser);	// FINAL - Values
tAST_Node	*GetString(tParser *Parser);
tAST_Node	*GetCharConst(tParser *Parser);
tAST_Node	*GetIdent(tParser *Parser);
tAST_Node	*GetNumeric(tParser *Parser);
 int	GetReservedWord(void);

void	SyntaxError(tParser *Parser, const char *reason, ...);
void	SyntaxAssert(tParser *Parser, int tok, int expected);
//void	FatalError(const char *reason, ...);


// === CODE ===
/**
 * \brief Handle the root of the parse tree
 */
void Parse_CodeRoot(tParser *Parser)
{
	while( LookAhead(Parser) != TOK_NULL )
	{
		switch(GetToken(Parser))
		{
		case TOK_RWORD_TYPEDEF:
			// Type definition
			// - Expect a type and storage classes
			TODO("root typedef");
			// - Hand off to common code with global scope
			//Parse_DoTypedef(Parser, NULL);
			break;
		// - Storage classes
		case TOK_RWORD_STATIC:
		case TOK_RWORD_EXTERN:
		case TOK_RWORD_CONST:
		case TOK_RWORD_INLINE:
			// - Hand off to common code
			Parse_DoDefinition(Parser, NULL);
			break;
		// - Compound types
		case TOK_RWORD_STRUCT:
		case TOK_RWORD_UNION:
		case TOK_RWORD_ENUM:
			// Fetch compound type descriptor (and handle definition)
			TODO("root compound");
			break;
		// - Identifiers (should be a type)
		case TOK_IDENT:
			// Function/global definition
			Parse_DoTopIdent(Parser, false);
			break;
		default:
			// Error
			SyntaxError(Parser, "Unexpected %s in root", GetTokenStr(Parser->Cur.Token));
			break;
		}
	}
}

/**
 * \brief Handle storage classes, structs, unions, enums, primative types
 */
tType *Parse_GetType(tParser *Parser)
{
	tType	*type = NULL;
	unsigned int	storage_classes = 0;
	// Consume storage classes until an identifier is hit
	// - Asterisk creates a pointer using current classes
	while( !type || Parser->Cur.Token != TOK_IDENT )
	{
		switch( Parser->Cur.Token )
		{
		case TOK_IDENT:
			// Identifier, should only happen if Type is unset
			assert(!type);
			type = Types_GetTypeFromName(Parser->Cur.TokenStart, Parser->Cur.TokenLen);
			if( !type ) {
				SyntaxError(Parser, "'%.*s' does not describe a type",
					Parser->Cur.TokenLen, Parser->Cur.TokenStart);
				return NULL;
			}
			break;
		case TOK_ASTERISK:
			// Pointer
			// - Determine type
			//type = Types_ApplyStorageClasses(type, storage_classes);
			//type = Types_CreatePointerType(type);
			// - Clear
			storage_classes = 0;
			// - Continue on
			break;
		case TOK_RWORD_STATIC:	storage_classes |= 1;	break;
		case TOK_RWORD_INLINE:	storage_classes |= 2;	break;
		case TOK_RWORD_EXTERN:	storage_classes |= 4;	break;
		case TOK_RWORD_CONST:	storage_classes	|= 8;	break;
		default:
			SyntaxError_T(Parser, Parser->Cur.Token, "Expected * or storage class");
			Types_DerefType(type);
			return NULL;
		}
		GetToken(Parser);
	}
	return type;
}

/**
 * \brief Handle variable/function definition
 */
int Parse_DoDefinition(tParser *Parser, tType *Type)
{
	SyntaxAssert(Parser, Parser->Cur.Token, TOK_IDENT);
	
	// Current token is variable/function name
	
	// - next '(' = Declare function
	if( LookAhead(Parser) == TOK_PAREN_OPEN )
	{
		// Parse function arguments
		TODO("Function definition arguments");
	}
	// - next * = Declare variable
	else
	{
		TODO("Variable definition");
	}
	return 1;
}

/*
 * \brief Handle an identifier at the top of the parse tree (first token in a statement)
 * 
 * \param NoCode	Set if outside of a function (errors if not a definition)
 */
tAST_Node *Parse_DoTopIdent(tParser *Parser, bool NoCode)
{
	// 1. Must be an identifier
	SyntaxAssert(Parser, Parser->Cur.Token, TOK_IDENT);
	
	// 2. If next token is:
	switch(LookAhead(Parser))
	{
	// - An identifier = Definition (prev must be a type)
	case TOK_IDENT:
	// - Storage class = Definition (prev must be a type)
	case TOK_RWORD_STATIC:
	case TOK_RWORD_EXTERN:
	case TOK_RWORD_INLINE:
	case TOK_RWORD_CONST:
		{
			// Get type
			tType *type = Parse_GetType(Parser);
			if( !type ) {
				return NULL;
			}
			
			if( Parse_DoDefinition(Parser, type) )
				return NULL;
		}
		return ACC_ERRPTR;
	// - * = Variable reference (or function call)
	default:
		if( NoCode ) {
			// Error!
			SyntaxError(Parser, "Unexpected %s", GetTokenStr(Parser->Cur.Token));
			return NULL;
		}
		// Descend into expression code
		return DoExpr0(Parser);
	}

	return NULL;
}

/**
 * \fn tAST_Node *DoCodeBlock()
 * \brief Parses a code block, or a single statement
 */
tAST_Node *DoCodeBlock(tParser *Parser)
{
	if(LookAhead(Parser) == TOK_BRACE_OPEN)
	{
		GetToken(Parser);
		tAST_Node	*ret = AST_NewCodeBlock();
		// Parse Block
		while(LookAhead(Parser) != TOK_BRACE_CLOSE)
		{
			AST_AppendNode( ret, DoStatement(Parser) );
		}
		GetToken(Parser);
		return ret;
	}

	return DoStatement(Parser);
}

/**
 * \brief Parses a statement _within_ a function
 */
tAST_Node *DoStatement(tParser *Parser)
{
	tAST_Node	*ret;

	switch(LookAhead(Parser))
	{
	case TOK_RWORD_IF:
		GetToken(Parser);
		return DoIf(Parser);
	case TOK_EOF:
		return NULL;

	case TOK_RWORD_RETURN:
		ret = DoReturn(Parser);
		SyntaxAssert( Parser, GetToken(Parser), TOK_SEMICOLON );
		break;
	case TOK_IDENT:
		ret = Parse_DoTopIdent(Parser, false);
		SyntaxAssert( Parser, GetToken(Parser), TOK_SEMICOLON );
		break;
	default:
		ret = DoExpr0(Parser);
		SyntaxAssert( Parser, GetToken(Parser), TOK_SEMICOLON );
		break;
	}
	return ret;
}

/**
 * \brief Parses an if block
 */
tAST_Node *DoIf(tParser *Parser)
{
	tAST_Node	*test, *true_code;
	tAST_Node	*false_code = NULL;

	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN);		// Eat (
	test = DoExpr0(Parser);
	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);	// Eat )

	// Contents
	true_code = DoCodeBlock(Parser);

	if(GetToken(Parser) == TOK_RWORD_ELSE)
	{
		false_code = DoCodeBlock(Parser);
	}

	return AST_NewIf(test, true_code, false_code);
}

/**
 * \brief Parses a while block
 */
tAST_Node *DoWhile(tParser *Parser)
{
	tAST_Node	*test, *code;

	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN);
	test = DoExpr0(Parser);
	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);

	// Contents
	code = DoCodeBlock(Parser);

	return AST_NewWhile(test, code);
}

/**
 * \fn tAST_Node *DoFor()
 * \brief Handle FOR loops
 */
tAST_Node *DoFor(tParser *Parser)
{
	tAST_Node	*init, *test, *post, *code;

	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN);	// Eat (

	// Initialiser (definition valid)
	// TODO: Support defining variables in initialiser
	init = DoExpr0(Parser);
	SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON);	// Eat ;

	// Condition
	test = DoExpr0(Parser);
	SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON);	// Eat ;

	// Postscript
	post = DoExpr0(Parser);

	SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);	// Eat )

	// Contents
	code = DoCodeBlock(Parser);

	return AST_NewFor(init, test, post, code);
}

/**
 * \brief Handle a return statement
 */
tAST_Node *DoReturn(tParser *Parser)
{
	return AST_NewUniOp( NODETYPE_RETURN, DoExpr0(Parser) );
}

// --------------------
// Expression 0 - Assignments
// --------------------
tAST_Node *DoExpr0(tParser *Parser)
{
	#define _next	DoExpr1
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;
	while(cont)
	{
		switch(GetToken(Parser))
		{
		case TOK_ASSIGNEQU:
			ret = AST_NewAssign(ret, _next(Parser));
			break;
		case TOK_DIV_EQU:
			ret = AST_NewAssignOp(ret, NODETYPE_DIVIDE, _next(Parser));
			break;
		case TOK_MULT_EQU:
			ret = AST_NewAssignOp(ret, NODETYPE_MULTIPLY, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 1 - Ternary
// TODO: Implement Ternary Operator
// --------------------
tAST_Node *DoExpr1(tParser *Parser)
{
	#define _next	DoExpr2
	tAST_Node *ret = _next(Parser);
	if( LookAhead(Parser) == TOK_QMARK )
	{
		TODO("Ternary operator");
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 2 - Boolean AND/OR
// --------------------
tAST_Node *DoExpr2(tParser *Parser)
{
	#define _next	DoExpr3
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;
	while(cont)
	{
		switch(GetToken(Parser))
		{
		case TOK_LOGICOR:
			ret = AST_NewBinOp(NODETYPE_BOOLOR, ret, _next(Parser));
			break;
		case TOK_LOGICAND:
			ret = AST_NewBinOp(NODETYPE_BOOLOR, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 3 - Bitwise Operators
// --------------------
tAST_Node *DoExpr3(tParser *Parser)
{
	#define _next	DoExpr4
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;
	while(cont)
	{
		switch(GetToken(Parser))
		{
		case TOK_OR:
			ret = AST_NewBinOp(NODETYPE_BWOR, ret, _next(Parser));
			break;
		case TOK_AMP:
			ret = AST_NewBinOp(NODETYPE_BWAND, ret, _next(Parser));
			break;
		case TOK_XOR:
			ret = AST_NewBinOp(NODETYPE_BWXOR, ret, _next(Parser));
			break;
		default:
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 4 - Comparison Operators
// --------------------
tAST_Node *DoExpr4(tParser *Parser)
{
	#define _next	DoExpr5
	tAST_Node	*ret = _next(Parser);

	// Check token
	bool cont = true;
	while( cont )
	{
		switch(GetToken(Parser))
		{
		case TOK_CMPEQU:
			ret = AST_NewBinOp(NODETYPE_EQUALS, ret, _next(Parser));
			break;
		case TOK_LT:
			ret = AST_NewBinOp(NODETYPE_LESSTHAN, ret, _next(Parser));
			break;
		case TOK_GT:
			ret = AST_NewBinOp(NODETYPE_GREATERTHAN, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 5 - Shifts
// --------------------
tAST_Node *DoExpr5(tParser *Parser)
{
	#define _next	DoExpr6
	tAST_Node *ret = _next(Parser);
	bool	cont = true;
	while( cont )
	{
		switch(GetToken(Parser))
		{
		case TOK_SHL:
			ret = AST_NewBinOp(NODETYPE_BITSHIFTLEFT, ret, _next(Parser));
			break;
		case TOK_SHR:
			ret = AST_NewBinOp(NODETYPE_BITSHIFTRIGHT, ret, _next(Parser));
			break;
		default:
			cont = false;
			PutBack(Parser);
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 6 - Arithmatic
// --------------------
tAST_Node *DoExpr6(tParser *Parser)
{
	#define _next	DoExpr7
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;

	while(cont)
	{
		switch(LookAhead(Parser))
		{
		case TOK_PLUS:
			ret = AST_NewBinOp(NODETYPE_ADD, ret, _next(Parser));
			break;
		case TOK_MINUS:
			ret = AST_NewBinOp(NODETYPE_SUBTRACT, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
	#undef _next
}

// --------------------
// Expression 7 - Multiplcation / Division
// --------------------
tAST_Node *DoExpr7(tParser *Parser)
{
	#define _next	DoExpr8
	tAST_Node	*ret = _next(Parser);
	bool	cont = true;
	while( cont )
	{
		switch(GetToken(Parser))
		{
		case TOK_ASTERISK:
			ret = AST_NewBinOp(NODETYPE_MULTIPLY, ret, _next(Parser));
			break;
		case TOK_DIVIDE:
			ret = AST_NewBinOp(NODETYPE_DIVIDE, ret, _next(Parser));
			break;
		default:
			PutBack(Parser);
			cont = false;
			break;
		}
	}
	return ret;
}

// --------------------
// Expression 8 - Unaries Right
// --------------------
tAST_Node *DoExpr8(tParser *Parser)
{
	tAST_Node	*ret = DoExpr9(Parser);

	switch(GetToken(Parser))
	{
	case TOK_INC:
		ret = AST_NewUniOp(NODETYPE_POSTINC, ret);
		break;
	case TOK_DEC:
		ret = AST_NewUniOp(NODETYPE_POSTDEC, ret);
		break;
	default:
		PutBack(Parser);
		break;
	}

	return ret;
}

// --------------------
// Expression 9 - Unaries Left
// --------------------
tAST_Node *DoExpr9(tParser *Parser)
{
	tAST_Node	*ret = DoMember(Parser);
	switch(GetToken(Parser))
	{
	case TOK_INC:
		ret = AST_NewUniOp(NODETYPE_PREINC, ret);
		break;
	case TOK_DEC:
		ret = AST_NewUniOp(NODETYPE_PREDEC, ret);
		break;
	case TOK_MINUS:
		ret = AST_NewUniOp(NODETYPE_NEGATE, ret);
		break;
	case TOK_NOT:
		ret = AST_NewUniOp(NODETYPE_BWNOT, ret);
		break;
	case TOK_AMP:
		ret = AST_NewUniOp(NODETYPE_ADDROF, ret);
		break;
	case TOK_ASTERISK:
		ret = AST_NewUniOp(NODETYPE_DEREF, ret);
		break;

	default:
		PutBack(Parser);
		break;
	}
	return ret;
}

// ---
// Member dereferences (. and ->)
// TODO: Fix
// ---
tAST_Node *DoMember(tParser *Parser)
{
	tAST_Node	*ret = DoParen(Parser);
	switch(GetToken(Parser))
	{
	case TOK_DOT:	// .
		ret = AST_NewBinOp(NODETYPE_MEMBER, ret, DoMember(Parser));
		break;
	case TOK_MEMBER:	// ->
		ret = AST_NewBinOp(NODETYPE_MEMBER, AST_NewUniOp(NODETYPE_DEREF, ret), DoMember(Parser));
		break;
	default:
		PutBack(Parser);
		break;
	}
	return ret;
}

// --------------------
// 2nd Last Expression - Parens
// --------------------
tAST_Node *DoParen(tParser *Parser)
{
	if(LookAhead(Parser) == TOK_PAREN_OPEN)
	{
		tAST_Node	*ret;
		GetToken(Parser);
		ret = DoExpr0(Parser);
		SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);
		return ret;
	}
	else
		return DoValue(Parser);
}

// --------------------
// Last Expression - Value
// --------------------
tAST_Node *DoValue(tParser *Parser)
{
	switch(LookAhead(Parser))
	{
	case TOK_STR:	return GetString(Parser);
	case TOK_CHAR:	return GetCharConst(Parser);
	case TOK_CONST_NUM:	return GetNumeric(Parser);
	case TOK_IDENT:	return GetIdent(Parser);

	default:
		SyntaxAssert(Parser, GetToken(Parser), TOK_T_VALUE);
		return NULL;
	}
}

size_t Parse_ConvertString(tParser *Parser, char *Output, const char *Src, size_t SrcLen)
{
	size_t	ret = 0;
	for( int i = 0; i < SrcLen; i++ )
	{
		if( Src[i] == '\\' )
		{
			i ++;
			assert(i != SrcLen);	// Lexer shouldn't allow, as it would have escaped closing "
			char ch = 0;
			switch(Src[i])
			{
			case '"':	ch = '"';	break;
			case 'n':	ch = '\n';	break;
			case 'r':	ch = '\r';	break;
			case 't':	ch = '\t';	break;
			case 'b':	ch = '\b';	break;
			case '0' ... '7':
				// Octal constant
				TODO("Octal constants in strings");
				break;
			case 'x':
				// Hex constant
				TODO("Hex constants in strings");
				break;
			default:
				// oopsie
				break;
			}
			if(Output)
				Output[ret] = ch;
			ret ++;
		}
		else
		{
			if(Output)
				Output[ret] = Src[i];
			ret ++;
		}
	}
	return ret;
}

/**
 * \fn tAST_Node *GetSimpleString()
 * \brief Parses a simple string
 */
tAST_Node *GetString(tParser *Parser)
{
	SyntaxAssert(Parser, GetToken(Parser), TOK_STR);

	// Parse String
	size_t	len = Parse_ConvertString(Parser, NULL, Parser->Cur.TokenStart+1, Parser->Cur.TokenLen-2);
	char *buf = malloc( len+1 );
	assert(buf);
	Parse_ConvertString(Parser, buf, Parser->Cur.TokenStart+1, Parser->Cur.TokenLen-2);
	buf[len] = '\0';

	return AST_NewString( buf, len );
}

/**
 * \fn tAST_Node *GetCharConst()
 * \brief Parses a character constant
 */
tAST_Node *GetCharConst(tParser *Parser)
{
	uint64_t	val;

	SyntaxAssert(Parser, GetToken(Parser), TOK_STR);

//	if( giTokenLength > 1 )
//		SyntaxWarning("Multi-byte character constants are not recommended");

	// Parse constant (local machine endian?)
	size_t len = Parse_ConvertString(Parser, NULL, Parser->Cur.TokenStart+1, Parser->Cur.TokenLen-2);
	if( len > 8 )
		SyntaxError(Parser, "Character constant is over 8-bytes");
	
	Parse_ConvertString(Parser, (void*)&val, Parser->Cur.TokenStart+1, Parser->Cur.TokenLen-2);

	return AST_NewInteger(val);
}

/**
 * \fn tAST_Node *GetNumeric()
 * \brief Reads a numeric value
 */
tAST_Node *GetNumeric(tParser *Parser)
{
	// Check token
	SyntaxAssert(Parser, GetToken(Parser), TOK_CONST_NUM);

	// Create Temporary String
	char	temp[Parser->Cur.TokenLen+1];
	for( int i = 0; i < Parser->Cur.TokenLen; i ++ )
		temp[ i ] = Parser->Cur.TokenStart[ i ];
	temp[ Parser->Cur.TokenLen ] = '\0';

	// Get value
	long value = strtol( temp, NULL, 0 );
	return AST_NewInteger( value );
}

