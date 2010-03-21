/*
 * PHPOS Bytecode Compiler
 * Token Parser
 * TOKEN.C
 */
#define _TOKEN_C	1
#include <global.h>
#include <string.h>
#include <stdio.h>

#define	SUPPORT_CPP	0

#if DEBUG
# define RET_TOK(tok)	do{giToken=(tok);printf("GetToken: RETURN %s\n",GetTokenStr((tok)));return(tok);}while(0)
#else
# define RET_TOK(tok)	do{giToken=(tok);return (tok);}while(0)
#endif

// === IMPORTS ===
extern char	*gsNextChar;
extern int	giToken;
extern char	*gsTokenStart;
extern int	giTokenLength;
extern int	giLine;
extern char	*gsCurFile;
extern int	GetReservedWord();

// === GLOBALS ===
 int	giLastToken = 0;
char	*giLastStart = NULL;
char	*giLastEnd = NULL;
 int	giLastLength = 0;
 int	giLastLine = 0;

// === CODE ===
int GetToken()
{
	char*	sStart = gsNextChar;
	char	ch;

	giLastToken = giToken;
	giLastStart = gsTokenStart;
	giLastEnd = gsNextChar;
	giLastLength = giTokenLength;
	giLastLine = giLine;


	// Elminate Whitespace
checkWhitespace:
	while(is_whitespace(*gsNextChar))
	{
		if( (*gsNextChar == '\n' && gsNextChar[-1] != '\r') || *gsNextChar == '\r' )
			giLine ++;
		gsNextChar++;
	}

	// Check for EOF
	if(*gsNextChar == '\0')	{
		giTokenLength = 0;
		RET_TOK( TOK_EOF );
	}

	gsTokenStart = sStart = gsNextChar;
	giTokenLength = 1;
	ch = *gsNextChar++;
	switch(ch)
	{
	// C Preprocessor Output / Comment
	case '#':
		#if SUPPORT_CPP
		if(*gsNextChar == ' ' && is_num(gsNextChar[1]))
		{
			gsNextChar++;
			giLine = 0;
			while(is_num(*gsNextChar)) {
				giLine *= 10;
				giLine = *gsNextChar++ - '0';
			}
			// NOT Trailed by a string
			if(*gsNextChar != ' ' || gsNextChar[1] != '"')		goto doPPComment;
			// Eat ' '  and "
			gsNextChar++;	gsNextChar++;
			gsCurFile = gsNextChar;
			while(*gsNextChar != '"')
				gsNextChar ++;
			// HACK: Mutilate source string  (it doesn't matter because it's in a comment)
			*gsNextChar = '\0';
			gsNextChar ++;
			printf("Set '%s':%i\n", gsCurFile, giLine);
		}
doPPComment:
		#endif
		while(*gsNextChar != '\r' && *gsNextChar != '\n')	gsNextChar++;
		goto checkWhitespace;

	// Divide/Comments
	case '/':
		switch(*gsNextChar)
		{
		case '/':	// LINE COMMENT
			while(*gsNextChar != '\r' && *gsNextChar != '\n')	gsNextChar++;
			goto checkWhitespace;

		case '*':	// BLOCK COMMENT
			gsNextChar++;
			while( !(gsNextChar[0] == '*' && gsNextChar[1] == '/') ) {
				if( (*gsNextChar == '\n' && gsNextChar[-1] != '\r') || *gsNextChar == '\r' )
					giLine ++;
				gsNextChar++;
			}
			gsNextChar ++;
			gsNextChar ++;
			goto checkWhitespace;

		case '=':	// Divide Equal
			RET_TOK( TOK_DIV_EQU );

		default:
			RET_TOK( TOK_DIVIDE );
		}
		break;

	// Question Mark
	case '?':	RET_TOK( TOK_QMARK );

	// Equals
	case '=':
		if(*gsNextChar == '=')	// Compare Equals
		{
			gsNextChar++;
			RET_TOK( TOK_CMPEQU );
		}
		RET_TOK( TOK_ASSIGNEQU );

	// Multiply
	case '*':
		if(*gsNextChar == '=')	// Times-Equals
		{
			gsNextChar++;
			RET_TOK( TOK_MULT_EQU );
		}
		RET_TOK( TOK_ASTERISK );

	// Plus
	case '+':
		if(*gsNextChar == '=')	// Plus-Equals
		{
			gsNextChar++;
			RET_TOK( TOK_PLUS_EQU );
		}
		if(*gsNextChar == '+')	// Increment
		{
			gsNextChar++;
			RET_TOK( TOK_INC );
		}
		RET_TOK( TOK_PLUS );

	// Minus
	case '-':
		if(*gsNextChar == '=')	// Minus-Equals
		{
			gsNextChar++;
			RET_TOK( TOK_MINUS_EQU );
		}
		if(*gsNextChar == '>')	// Member of
		{
			gsNextChar++;
			RET_TOK( TOK_MEMBER );
		}
		if(*gsNextChar == '-')	// Increment
		{
			gsNextChar++;
			RET_TOK( TOK_DEC );
		}
		RET_TOK( TOK_MINUS );

	// OR
	case '|':
		if(*gsNextChar == '|')	// Boolean OR
		{
			gsNextChar++;
			return (giToken = TOK_LOGICOR);
		}
		if(*gsNextChar == '=')	// OR-Equals
		{
			gsNextChar++;
			RET_TOK( TOK_OR_EQU );
		}
		RET_TOK( TOK_OR );

	// XOR
	case '^':
		if(*gsNextChar == '=')	// XOR-Equals
		{
			gsNextChar++;
			RET_TOK( TOK_XOR_EQU );
		}
		RET_TOK( TOK_XOR );

	// AND/Address/Reference
	case '&':
		if(*gsNextChar == '&')	// Boolean AND
		{
			gsNextChar++;
			RET_TOK( TOK_LOGICAND );
		}
		if(*gsNextChar == '=')	// AND-Equals
		{
			gsNextChar++;
			RET_TOK( TOK_AND_EQU );
		}
		RET_TOK( TOK_AMP );

	// Bitwise NOT
	case '~':	RET_TOK( TOK_NOT );
	// Boolean NOT
	case '!':	RET_TOK( TOK_LOGICNOT );

	// String
	case '"':
		gsTokenStart = gsNextChar;
		while( !(gsNextChar[0] == '"' && gsNextChar[-1] != '\\') && *gsNextChar != 0 )	// Read String
			gsNextChar ++;
		if(*gsNextChar == 0)
			ParseError("Unexpected EOF in string");
		giTokenLength = gsNextChar - gsTokenStart;
		gsNextChar ++;	// Eat last Quote
		RET_TOK( TOK_STR );

	// Character
	case '\'':
		gsTokenStart = gsNextChar;
		while( !(gsNextChar[0] == '\'' && gsNextChar[-1] != '\\') && *gsNextChar != 0 )	// Read String
			gsNextChar ++;
		if(*gsNextChar == 0)
			ParseError("Unexpected EOF in character constant");
		giTokenLength = gsNextChar - gsTokenStart;
		gsNextChar ++;	// Eat last Quote
		RET_TOK( TOK_CHAR );

	// LT / Shift Left
	case '<':
		if(*gsNextChar == '<') {
			gsNextChar++;
			RET_TOK( TOK_SHL );
		}
		RET_TOK( TOK_LT );
	// GT / Shift Right
	case '>':
		if(*gsNextChar == '>') {
			gsNextChar++;
			RET_TOK( TOK_SHR );
		}
		RET_TOK( TOK_GT );

	// End of Statement (Semicolon)
	case ';':	RET_TOK( TOK_SEMICOLON );

	// Comma
	case ',':	RET_TOK( TOK_COMMA );

	// Scope / Label
	case ':':
		if( *gsNextChar == ':' )
		{
			gsNextChar++;
			RET_TOK( TOK_SCOPE );
		}
		RET_TOK( TOK_COLON );

	// Namespace Separator
	case '\\':	RET_TOK( TOK_SLASH );

	// Braces
	case '{':	RET_TOK( TOK_BRACE_OPEN );
	case '}':	RET_TOK( TOK_BRACE_CLOSE );
	// Parens
	case '(':	RET_TOK( TOK_PAREN_OPEN );
	case ')':	RET_TOK( TOK_PAREN_CLOSE );
	// Brackets
	case '[':	RET_TOK( TOK_SQUARE_OPEN );
	case ']':	RET_TOK( TOK_SQUARE_CLOSE );
	}

	// Numbers
	if(is_num(ch))
	{
		gsTokenStart = sStart;
		if(ch == '0' && *gsNextChar == 'x') {
			gsNextChar ++;
			while(is_hex(*gsNextChar))
				gsNextChar++;
		} else {
			while( is_num(*gsNextChar) )
				gsNextChar++;
		}
		giTokenLength = gsNextChar - sStart;
		RET_TOK( TOK_CONST_NUM );
	}

	// Identifier
	if(is_ident(ch))
	{
		 int	rword;
		while(is_ident(*gsNextChar))
			gsNextChar ++;
		giTokenLength = gsNextChar - sStart;

		#if DEBUG
		{
		char	*tmp = strndup(gsTokenStart,giTokenLength);
		printf(" GetToken: ident/rsvdwd = '%s'\n", tmp);
		free(tmp);
		}
		#endif
	
		rword = GetReservedWord();
		#if DEBUG
		printf(" GetToken: rword = %i\n", rword);
		#endif
		if(rword > 0)
			RET_TOK( TOK_RSVDWORD );
		else
			RET_TOK( TOK_IDENT );
	}

	fprintf(stderr, "Unknown symbol '%c' (0x%x)\n", ch, ch);
	ParseError("Unknown symbol");
	return 0;
}

void PutBack()
{
	giToken = giLastToken;
	gsTokenStart = giLastStart;
	gsNextChar = giLastEnd;
	giTokenLength = giLastLength;
	giLine = giLastLine;
}

int LookAhead()
{
	 int	newToken;
	 int	oldToken = giToken;
	 int	oldLength = giTokenLength;
	 int	oldLine = giLine;
	char	*oldStart = gsTokenStart;
	char	*oldEnd = gsNextChar;

	#if DEBUG
	printf("LookAhead: ()\n");
	#endif

	newToken = GetToken();

	giToken = oldToken;
	giTokenLength = oldLength;
	giLine = oldLine;
	gsTokenStart = oldStart;
	gsNextChar = oldEnd;

	return newToken;
}

char *GetTokenStr(int ID)
{
	return (char*)csaTOKEN_NAMES[ID];
}
