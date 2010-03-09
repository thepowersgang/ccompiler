/*
 * Acess C Compiler
 * By John Hodge (thePowersGang)
 * 
 * This code is published under the terms of the BSD Licence. For more
 * information see the file COPYING.
 * 
 * expr.c - Token to AST Parser
 */
#define	DEBUG	3
#include "global.h"
#include <stdio.h>
#include <strings.h>
#include <memory.h>
#include <symbol.h>
#include <ast.h>

// === MACROS ===
#define CMPTOK(str)	(strncmp((str),gsTokenStart,giTokenLength)==0)

// === IMPORTS ===
extern int	giLine;
extern int	giStatement;
extern int	giToken;
extern int	giTokenLength;
extern int	giInPHPMode;
extern char	*gsTokenStart;
extern char	*gsNextChar;
extern int	giCurrentNamespace;
extern int	giCurrentClass;
extern int	giCurrentFunction;
extern char	*gsCurFile;

extern int	GetNamespaceFromString(int stringId);
extern int	GetClassFromString(int namespace, int stringId);
extern int	GetFunctionFromString(int namespace, int class, int stringId);
extern int	GetConstFromString(int namespace, int class, int stringId);
extern void SetNamespace(char *name);
extern void RestoreNamespace();
extern void SetClass(char *name);
extern void RestoreClass();
extern void SetFunction(char *name);
extern void SetFunctionArgs(int argc, int argt);
extern void RestoreFunction();
extern void	CreateOutput();

// === Prototypes ===
tType	*GetType();
tAST_Node	*DoCodeBlock(void);
tAST_Node	*DoStatement(void);
tAST_Node	*DoIf(void);
tAST_Node	*DoWhile(void);
tAST_Node	*DoFor(void);
tAST_Node	*DoReturn(void);

tAST_Node	*DoExpr0(void);	// Assignment
tAST_Node	*DoExpr1(void);	// Ternary
tAST_Node	*DoExpr2(void);	// Boolean
tAST_Node	*DoExpr3(void);	// Bitwise
tAST_Node	*DoExpr4(void);	// Comparison
tAST_Node	*DoExpr5(void);	// Shifts
tAST_Node	*DoExpr6(void);	// Arithmatic
tAST_Node	*DoExpr7(void);	// Mult/Div
tAST_Node	*DoExpr8(void);	// Right Unaries
tAST_Node	*DoExpr9(void);	// Left Unaries
tAST_Node	*DoParen(void);	// 2nd Last - Parens
tAST_Node	*DoValue(void);	// FINAL - Values
tAST_Node	*GetString(void);
tAST_Node	*GetCharConst(void);
tAST_Node	*GetIdent(void);
tAST_Node	*GetNumeric(void);
 int	GetReservedWord(void);

// === CODE ===
/**
 * \brief Parses a type definition
 */
tType *GetType()
{
	tType	*ret;
	 int	tok;
	 int	bSigned = 1;
	 int	bConst = 0;
	 int	linkage = 0;
	 int	size = 32;
	 int	depth = 0;
	
	for(;;)
	{
		tok = GetToken();
		if(tok == TOK_ASTERISK) {
			depth ++;
			continue;
		}
		if(tok != TOK_IDENT)	SyntaxError2(tok, TOK_IDENT);
		
		// Signed?
		if( CMPTOK("signed") ) {
			bSigned = 1;
			continue;
		}
		else if( CMPTOK("unsigned") ) {
			bSigned = 0;
			continue;
		}
		
		// Constant?
		if( CMPTOK("const") ) {
			bConst = 1;
			continue;
		}
		
		// Static/Extern?
		if( CMPTOK("static") ) {
			linkage = 1;
			continue;
		}
		else if( CMPTOK("extern") ) {
			linkage = 2;
			continue;
		}
		
		if( CMPTOK("char") )
			size = 8;	// 8-bit char
		else if( CMPTOK("short") )
			size = 16;	// 16-bit short
		else if( CMPTOK("int") )
			size = 32;	// 32-bit int
		else if( CMPTOK("void") )
			size = 0;
		else {
			char	*tmp = strndup(gsTokenStart, giTokenLength);
			ret = Symbol_ResolveTypedef(tmp, depth);
			if(!ret) {
				SyntaxErrorF("Unexpected '%s'", tmp);
				free(tmp);
				return NULL;
			}
			free(tmp);
			return ret;
		}
		
		ret = Symbol_CreateIntegralType(bSigned, bConst, linkage, size, depth);
		return ret;
	}
}

/**
 * \brief Gets the definion of a variable or function
 */
void GetDefinition(void)
{
	tType	*type;
	char	*name;
	void	*ptr;
	 int	tok;
 	 int	i;
	
	type = GetType();
	SyntaxAssert(GetToken(), TOK_IDENT);
	name = strndup(gsTokenStart, giTokenLength);
	DEBUG1("Creating '%s'\n", name);
	
	switch(GetToken())
	{
	case TOK_SEMICOLON:
		// Create variable in .bss / Define extern var
		break;
	case TOK_ASSIGNEQU:
		// Create variable in .data
		break;
	case TOK_PAREN_OPEN:
		ptr = Symbol_GetFunction(type, strdup(name));
		i = 0;
		// Create function
		while(LookAhead() == TOK_IDENT)
		{
			tType	*type;
			char	*name = NULL;
			
			type = GetType();
			DEBUG1("type = {Type:%i,PtrDepth:%i,Size:%i,...}\n",
				type->Type, type->PtrDepth, type->Integer.Bits);
			
			if( i == 0 && type->Type == 0 ) {
				SyntaxAssert( GetToken(), TOK_PAREN_CLOSE );
				break;
			}
			if( GetToken() == TOK_IDENT )
			{
				name = strndup(gsTokenStart, giTokenLength);
				Symbol_SetArgument(ptr, i, type, name);
				DEBUG2(" Argument %i - '%s'\n", i, name);
				GetToken();
			}
			
			if( giToken == TOK_PAREN_CLOSE )
				break;
			if( giToken != TOK_COMMA )
				SyntaxError2(giToken, TOK_COMMA);
			i ++;
		}
		switch( (tok = LookAhead()) )
		{
		case TOK_BRACE_OPEN:
			Symbol_SetFunction( ptr );
			Symbol_SetFunctionCode( ptr, DoCodeBlock() );
			break;
		case TOK_SEMICOLON:
			GetToken();
			break;
		default:
			printf("gsTokenStart = \"%s\"\n", strndup(gsTokenStart, giTokenLength));
			SyntaxError3(tok, TOK_BRACE_OPEN, TOK_SEMICOLON, 0);
			break;
		}
		break;
	default:
		SyntaxError2(giToken, 0);
		break;
	}
	free(name);
}

/**
 * \fn tAST_Node *DoCodeBlock()
 * \brief Parses a code block, or a single statement
 */
tAST_Node *DoCodeBlock()
{
	if(LookAhead() == TOK_BRACE_OPEN)
	{
		tAST_Node	*ret = AST_NewCodeBlock();
		Symbol_EnterBlock();
		GetToken();
		// Parse Block
		while(LookAhead() != TOK_BRACE_CLOSE)
		{
			AST_AppendNode( ret, DoStatement());
		}
		GetToken();
		Symbol_LeaveBlock();
		return ret;
	}
	else
		return DoStatement();
}

/**
 * \brief Parses a statement _within_ a function
 */
tAST_Node *DoStatement()
{
	 int	tok, id;
	tAST_Node	*ret;

	DEBUG2("%s:%i - Statement %i\n", gsCurFile, giLine, giStatement++);
	tok = LookAhead();
	DEBUG3(" DoStatement: LookAhead() = %s\n", GetTokenStr(tok));
	switch(tok)
	{
	case TOK_RSVDWORD:
		GetToken();
		id = GetReservedWord();
		switch(id)
		{
		// Code Blocks
		case RWORD_IF:
			return DoIf();
			break;
		case RWORD_WHILE:
			return DoWhile();
			break;
		case RWORD_FOR:
			return DoFor();
			break;
		
		// Operations
		case RWORD_RETURN:
			ret = DoReturn();
			SyntaxAssert( GetToken(), TOK_SEMICOLON );
			break;

		// TODO - Variables

		default:
			SyntaxError("Unimplemented Reserved word");
			exit(-1);
		}
		break;

	case TOK_EOF:
		return NULL;

#if 0
	case TOK_IDENT:
		char	*str;
		GetToken();
		str = strndup(gsTokenStart, giTokenLength);
		if( IsTypeDecl(str) )
		{
			break;
		}
#endif
	default:
		ret = DoExpr0();
			SyntaxAssert( GetToken(), TOK_SEMICOLON );
	}
	return ret;
}

/**
 * \fn tAST_Node *DoIf()
 * \brief Parses an if block
 */
tAST_Node *DoIf()
{
	tAST_Node	*test, *true;
	tAST_Node	*false = NULL;

	GetToken();	// Get (
	SyntaxAssert(giToken, TOK_PAREN_OPEN);
	test = DoExpr0();
	GetToken();	// Get )
	SyntaxAssert(giToken, TOK_PAREN_CLOSE);

	// Contents
	true = DoCodeBlock();

	if(GetToken() == TOK_RSVDWORD)
	{
		 int	id;
		// Check the the reserved word is else
		id = GetReservedWord();
		if( id != RWORD_ELSE )
			PutBack();
		else
			false = DoCodeBlock();
	}

	return AST_NewIf(test, true, false);
}

/**
 * \fn tAST_Node *DoWhile()
 * \brief Parses a while block
 */
tAST_Node *DoWhile()
{
	tAST_Node	*test, *code;

	GetToken();	// Get (
	SyntaxAssert(giToken, TOK_PAREN_OPEN);
	test = DoExpr0();
	GetToken();	// Get )
	SyntaxAssert(giToken, TOK_PAREN_CLOSE);

	// Contents
	code = DoCodeBlock();

	return AST_NewWhile(test, code);
}

/**
 * \fn tAST_Node *DoFor()
 * \brief Handle FOR loops
 */
tAST_Node *DoFor()
{
	tAST_Node	*init, *test, *post, *code;

	GetToken();	SyntaxAssert(giToken, TOK_PAREN_OPEN);	// Eat (

	// Initialiser
	init = DoExpr0();
	GetToken();	SyntaxAssert(giToken, TOK_SEMICOLON);	// Eat ;

	// Condition
	test = DoExpr0();
	GetToken();	SyntaxAssert(giToken, TOK_SEMICOLON);	// Eat ;

	// Postscript
	post = DoExpr0();

	GetToken();	SyntaxAssert(giToken, TOK_PAREN_CLOSE);	// Eat )

	// Contents
	code = DoCodeBlock();

	return AST_NewFor(init, test, post, code);
}

/**
 * \brief Handle a return statement
 */
tAST_Node *DoReturn(void)
{
	return AST_NewUniOp( NODETYPE_RETURN, DoExpr0() );
}

// --------------------
// Expression 0 - Assignments
// --------------------
tAST_Node *DoExpr0()
{
	tAST_Node	*ret = DoExpr1();

	// Check Assignment
	switch(LookAhead())
	{
	case TOK_ASSIGNEQU:
		GetToken();		// Eat Token
		ret = AST_NewAssign(ret, DoExpr0());
		break;
	case TOK_DIV_EQU:
		GetToken();		// Eat Token
		ret = AST_NewAssignOp(ret, NODETYPE_DIVIDE, DoExpr0());
		break;
	case TOK_MULT_EQU:
		GetToken();		// Eat Token
		ret = AST_NewAssignOp(ret, NODETYPE_MULTIPLY, DoExpr0());
		break;
	default:	break;
	}
	return ret;
}

// --------------------
// Expression 1 - Ternary
// TODO: Implement Ternary Operator
// --------------------
tAST_Node *DoExpr1()
{
	return DoExpr2();
}

// --------------------
// Expression 2 - Boolean AND/OR
// --------------------
tAST_Node *DoExpr2()
{
	tAST_Node	*ret, *right;
	ret = DoExpr3();

	switch(LookAhead())
	{
	case TOK_LOGICOR:
		GetToken();		// Eat Token
		right = DoExpr2();		// Recurse
		ret = AST_NewBinOp(NODETYPE_BOOLOR, ret, right);
		break;
	case TOK_LOGICAND:
		GetToken();		// Eat Token
		right = DoExpr2();		// Recurse
		ret = AST_NewBinOp(NODETYPE_BOOLOR, ret, right);
		break;
	default:	break;
	}
	return ret;
}

// --------------------
// Expression 3 - Bitwise Operators
// --------------------
tAST_Node *DoExpr3()
{
	tAST_Node	*ret = DoExpr4();

	// Check Token
	switch(LookAhead())
	{
	case TOK_OR:
		GetToken();		// Eat Token
		ret = AST_NewBinOp(NODETYPE_BWOR, ret, DoExpr3());
		break;
	case TOK_AMP:
		GetToken();		// Eat Token
		ret = AST_NewBinOp(NODETYPE_BWAND, ret, DoExpr3());
		break;
	case TOK_XOR:
		GetToken();		// Eat Token
		ret = AST_NewBinOp(NODETYPE_BWXOR, ret, DoExpr3());
		break;
	default:	break;
	}
	return ret;
}

// --------------------
// Expression 4 - Comparison Operators
// --------------------
tAST_Node *DoExpr4()
{
	tAST_Node	*ret = DoExpr5();

	// Check token
	switch(LookAhead())
	{
	case TOK_CMPEQU:
		GetToken();		// Eat Token
		ret = AST_NewBinOp(NODETYPE_EQUALS, ret, DoExpr4());
		break;
	case TOK_LT:
		GetToken();	// Eat <
		ret = AST_NewBinOp(NODETYPE_LESSTHAN, ret, DoExpr4());
		break;
	case TOK_GT:
		GetToken();	// Eat >
		ret = AST_NewBinOp(NODETYPE_GREATERTHAN, ret, DoExpr4());
		break;
	default:	break;
	}
	return ret;
}

// --------------------
// Expression 5 - Shifts
// --------------------
tAST_Node *DoExpr5()
{
	tAST_Node *ret = DoExpr6();

	switch(LookAhead())
	{
	case TOK_SHL:
		GetToken();
		ret = AST_NewBinOp(NODETYPE_BITSHIFTLEFT, ret, DoExpr5());
		break;
	case TOK_SHR:
		GetToken();
		ret = AST_NewBinOp(NODETYPE_BITSHIFTRIGHT, ret, DoExpr5());
		break;
	default:	break;
	}

	return ret;
}

// --------------------
// Expression 6 - Arithmatic
// --------------------
tAST_Node *DoExpr6()
{
	tAST_Node	*ret = DoExpr7();

	switch(LookAhead())
	{
	case TOK_PLUS:
		GetToken();
		ret = AST_NewBinOp(NODETYPE_ADD, ret, DoExpr6());
		break;
	case TOK_MINUS:
		GetToken();
		ret = AST_NewBinOp(NODETYPE_SUBTRACT, ret, DoExpr6());
		break;
	default:	break;
	}

	return ret;
}

// --------------------
// Expression 7 - Multiplcation / Division
// --------------------
tAST_Node *DoExpr7()
{
	tAST_Node	*ret = DoExpr8();
	switch(LookAhead())
	{
	case TOK_ASTERISK:
		GetToken();
		ret = AST_NewBinOp(NODETYPE_MULTIPLY, ret, DoExpr7());
		break;
	case TOK_DIVIDE:
		GetToken();
		ret = AST_NewBinOp(NODETYPE_DIVIDE, ret, DoExpr7());
		break;
	default:	break;
	}

	return ret;
}

// --------------------
// Expression 8 - Unaries Right
// --------------------
tAST_Node *DoExpr8()
{
	tAST_Node	*ret = DoExpr9();

	switch(LookAhead())
	{
	case TOK_INC:
		GetToken();
		ret = AST_NewUniOp(NODETYPE_POSTINC, ret);
		break;
	case TOK_DEC:
		GetToken();
		ret = AST_NewUniOp(NODETYPE_POSTDEC, ret);
		break;
	default:	break;
	}

	return ret;
}

// --------------------
// Expression 9 - Unaries Left
// --------------------
tAST_Node *DoExpr9()
{
	tAST_Node	*ret;
	switch(LookAhead())
	{
	case TOK_INC:
		GetToken();
		ret = AST_NewUniOp(NODETYPE_PREINC, DoParen());
		break;
	case TOK_DEC:
		GetToken();
		ret = AST_NewUniOp(NODETYPE_PREDEC, DoParen());
		break;
	case TOK_MINUS:
		GetToken();
		ret = AST_NewUniOp(NODETYPE_NEGATE, DoParen());
		break;
	case TOK_NOT:
		GetToken();
		ret = AST_NewUniOp(NODETYPE_BWNOT, DoParen());
		break;

	case TOK_RSVDWORD:
		GetToken();
		SyntaxError("Unexpected reserved word");
		return NULL;

	default:
		ret = DoParen();
		break;
	}
	return ret;
}

// --------------------
// 2nd Last Expression - Parens
// --------------------
tAST_Node *DoParen()
{
	if(LookAhead() == TOK_PAREN_OPEN)
	{
		tAST_Node	*ret;
		GetToken();
		ret = DoExpr0();
		GetToken();
		SyntaxAssert(giToken, TOK_PAREN_CLOSE);
		return ret;
	}
	else
		return DoValue();
}

// --------------------
// Last Expression - Value
// --------------------
tAST_Node *DoValue()
{
	 int	tok = LookAhead();

	switch(tok)
	{
	case TOK_STR:	return GetString();
	case TOK_CHAR:	return GetCharConst();
	case TOK_CONST_NUM:	return GetNumeric();
	case TOK_IDENT:	return GetIdent();

	default:
		ParseError2( tok, TOK_T_VALUE );
		return NULL;
	}
}

/**
 * \fn tAST_Node *GetSimpleString()
 * \brief Parses a simple string
 */
tAST_Node *GetString()
{
	char	*buf;
	 int	i, j;

	GetToken();
	// Check token
	if(giToken != TOK_STR)	ParseError2( giToken, TOK_STR );

	// Parse String
	buf = malloc(giTokenLength+1);
	for(i=0,j=0;i<giTokenLength;j++,i++)
	{
		if(gsTokenStart[i] == '\\')
		{
			i ++;
			switch(gsTokenStart[i])
			{
			case '"':	buf[j] = '"';	break;
			case 'n':	buf[j] = '\n';	break;
			case 'r':	buf[j] = '\r';	break;
			case 't':	buf[j] = '\t';	break;
			}
		}
		else
			buf[j] = gsTokenStart[i];
	}
	buf[j] = '\0';

	return AST_NewString( buf, j );
}

/**
 * \fn tAST_Node *GetCharConst()
 * \brief Parses a character constant
 */
tAST_Node *GetCharConst()
{
	 int	i;
	uint64_t	val;

	GetToken();
	// Check token
	if(giToken != TOK_STR)	ParseError2( giToken, TOK_STR );

	if( giTokenLength > 8 )
		SyntaxError("Character constant is over 8-bytes");

//	if( giTokenLength > 1 )
//		SyntaxWarning("Multi-byte character constants are not recommended");

	// Parse String
	for( i = 0; i < giTokenLength; i ++ )
	{
		val <<= 8;
		if(gsTokenStart[i] == '\\')
		{
			i ++;
			switch(gsTokenStart[i])
			{
			case '\'':	val |= '\'';	break;
			case 'n':	val |= '\n';	break;
			case 'r':	val |= '\r';	break;
			default:
				SyntaxError("Unkown escape sequence in character constant");
			}
		}
		else
			val |= gsTokenStart[i];
	}

	return AST_NewInteger(val);
}

/**
 * \fn tAST_Node *GetNumeric()
 * \brief Reads a numeric value
 */
tAST_Node *GetNumeric()
{
	 int	value;
	char	temp[60];
	GetToken();
	// Check token
	if(giToken != TOK_CONST_NUM)
		ParseError2( giToken, TOK_CONST_NUM );

	// Create Temporary String
	for( value = 0; value < giTokenLength; value ++ )
		temp[ value ] = gsTokenStart[ value ];
	temp[ value ] = '\0';

	// Get value
	value = strtol( temp, NULL, 0 );

	return AST_NewInteger( value );
}

/**
 * \fn tAST_Node *GetIdent()
 * \brief Read an identifier
 */
tAST_Node *GetIdent()
{
	tSymbol	*sym = NULL;
	char	*str;
	DEBUG3(" GetIdent: ()\n");
	GetToken();

	// Get ID
	str = strndup(gsTokenStart, giTokenLength);

	// Function?
	if(LookAhead() == TOK_PAREN_OPEN)
	{
		tAST_Node	*fcn, *ret;

		if( (sym = Symbol_GetLocalVariable(str)) )
		{
			if( Symbol_GetSymClass(sym) != SYMCLASS_FCNPTR ) {
				SyntaxError("Unexpected '('");
			}
			fcn = AST_NewUniOp( NODETYPE_DEREF, AST_NewLocalVar( sym ) );
		}
		else
		{
			sym = Symbol_ResolveSymbol(str);
			if(!sym) {
				SyntaxErrorF("Function '%s' is undefined", str);
			}
		
			switch( Symbol_GetSymClass(sym) )
			{
			case SYMCLASS_FCNPTR:
				fcn = AST_NewUniOp( NODETYPE_DEREF, AST_NewSymbol( sym, str ) );
				break;
			case SYMCLASS_FCN:
				fcn = AST_NewSymbol( sym, str );
				break;
			default:
				SyntaxError("Unexpected '('");
				break;
			}
		}

		ret = AST_NewFunctionCall( fcn );

		// Get Arguments
		GetToken();	// Eat (
		if( LookAhead() != TOK_PAREN_CLOSE )
		{
			do {
				AST_AppendNode(ret, DoExpr0());
				GetToken();
				DEBUG3("  giToken = %s\n", GetTokenStr(giToken));
			}	while( giToken == TOK_COMMA );
			if(giToken != TOK_PAREN_CLOSE)
				SyntaxError2(giToken, TOK_PAREN_CLOSE);
		} else
			GetToken();

		return ret;
	}

	if( (sym = Symbol_GetLocalVariable(str)) != NULL )
		return AST_NewLocalVar( sym );
	else
		return AST_NewSymbol( Symbol_ResolveSymbol(str), str );
}

/**
 * \brief int GetReservedWord()
 * \brief Gets a symbolic representation of a reserved word
 */
int GetReservedWord()
{
	 int	ret = 0;
	char	*tok = strndup( gsTokenStart, giTokenLength );
	if(strcmp("if", tok) == 0)
		ret = RWORD_IF;
	else if(strcmp("else", tok) == 0)
		ret = RWORD_ELSE;
	else if(strcmp("for", tok) == 0)
		ret = RWORD_FOR;
	else if(strcmp("while", tok) == 0)
		ret = RWORD_WHILE;
	else if(strcmp("do", tok) == 0)
		ret = RWORD_DO;

	else if(strcmp("return", tok) == 0)
		ret = RWORD_RETURN;
	else if(strcmp("continue", tok) == 0)
		ret = RWORD_CONTINUE;
	else if(strcmp("break", tok) == 0)
		ret = RWORD_BREAK;

	free(tok);
	return ret;
}
