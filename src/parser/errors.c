/*
 */
#include <parser.h>
#include <stdio.h>
#include <stdarg.h>
#include <ast.h>

// === PROTOTYPES ===

// === CODE ===
static inline void message_header(const char *Filename, int Line, const char *sev, const char *type)
{
	fprintf(stderr, "%s:%s:%i: %s - ", sev, Filename, Line, type);
}
static inline void message_header_p(tParser *Parser, const char *sev, const char *type)
{
	message_header(Parser->Cur.Filename, Parser->Cur.Line, sev, type);
}

void CompileError(tAST_Node *Node, const char *format, ...)
{
	message_header("", Node->Line, "error", "compile");
	va_list	args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void CompileWarning(tAST_Node *Node, const char *format, ...)
{
	message_header("", Node->Line, "warning", "compile");
	va_list	args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void SyntaxError_T(tParser *Parser, enum eTokens Tok, const char *reason, ...)
{
	message_header_p(Parser, "error", "syntax");
	fprintf(stderr, "Unexpected %s, ", GetTokenStr(Tok));
	va_list	args;
	va_start(args, reason);
	vfprintf(stderr, reason, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void SyntaxError(tParser *Parser, const char *format, ...)
{
	message_header_p(Parser, "error", "syntax");
	va_list	args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

int SyntaxAssert(tParser *Parser, enum eTokens tok, enum eTokens expected)
{
	if( tok != expected )
	{
		message_header_p(Parser, "error", "syntax");
		fprintf(stderr, "Unexpected %s, expected %s", GetTokenStr(tok), GetTokenStr(expected));
		fprintf(stderr, "\n");
		return 1;
	}
	return 0;
}

void LexerError(tParser *Parser, const char *format, ...)
{
	message_header_p(Parser, "error", "lex");
	va_list	args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	//longjmp(Parser->ErrorTarget);
}

