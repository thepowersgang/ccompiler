/*
PHPOS Bytecode Compiler
Token Parser
TOKEN.C
*/
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

extern int	giLine;
extern char	*gsCurFile;

int	is_whitespace(char c)
{
	if(c == ' ' || c == '\r' || c == '\n' || c == '\t')
		return 1;
	return 0;
}

int	is_num(char c)
{
	if('0' <= c && c <= '9')
		return 1;
	return 0;
}

int	is_hex(char c)
{
	if('0' <= c && c <= '9')
		return 1;
	if('a' <= c && c <= 'f')
		return 1;
	if('A' <= c && c <= 'F')
		return 1;
	return 0;
}

int	is_ident(char c)
{
	if('0' <= c && c <= '9')
		return 1;
	if('a' <= c && c <= 'z')
		return 1;
	if('A' <= c && c <= 'Z')
		return 1;
	if(c == '_')
		return 1;
	return 0;
}

void ParseError(char *reason)
{
	printf("Parse Error on line #%i of '%s', %s\nAborting\n", giLine, gsCurFile, reason);
	exit(-1);
}

void ParseError2(int token, int expected)
{
	printf("Parse Error on line #%i of '%s'\n Unexpected %s", giLine, gsCurFile, GetTokenStr(token));
	if(expected)
		printf(", Expecting %s", GetTokenStr(expected));
	printf("\nCompile Aborting\n");
	exit(-1);
}

void SyntaxError(char *reason)
{
	printf("Syntax Error on line #%i of '%s', %s\nAborting\n", giLine, gsCurFile, reason);
	exit(-1);
}

void SyntaxError2(int token, int expected)
{
	printf("Syntax Error on line #%i of '%s'\n Unexpected %s (%i)", giLine, gsCurFile, GetTokenStr(token), token);
	if(expected)
		printf(", Expecting %s", GetTokenStr(expected));
	printf("\nCompile Aborting\n");
	exit(-1);
}

void SyntaxError3(int Token, ...)
{
	 int	expected;
	va_list	args;
	va_start(args, Token);
	
	printf("Syntax Error on line #%i of '%s'\n Unexpected %s (%i)", giLine, gsCurFile, GetTokenStr(Token), Token);
	
	expected = va_arg(args, int);
	if(expected)
	{
		 int	bFirst=1;
		printf(", Expecting ");
		while(expected)
		{
			if(!bFirst)	printf(", ");
			bFirst = 0;
			printf("%s", GetTokenStr(expected));
			expected = va_arg(args, int);
		}
	}
	printf("\nCompile Aborting\n");
	exit(-1);
}

void SyntaxErrorF(const char *reason, ...)
{
	va_list	args;
	va_start(args, reason);
	
	printf("Syntax Error on line #%i of '%s'\n", giLine, gsCurFile);
	vprintf(reason, args);
	printf("\nCompile Aborting\n");
	exit(-1);
}

void SyntaxAssert(int tok, int expected)
{
	if(tok != expected)
		SyntaxError2(tok, expected);
}

void FatalError(char *reason)
{
	printf("Fatal Error on line #%i of '%s', %s\nAborting\n", giLine, gsCurFile, reason);
	exit(-1);
}
