/*
 */
#include <parser.h>
#include <stdio.h>
#include <stdarg.h>

// === PROTOTYPES ===

// === CODE ===
static void message_header(tParser *Parser, const char *sev, const char *type)
{
	fprintf(stderr, "%s:%s:%i: %s - ", sev, Parser->Cur.Filename, Parser->Cur.Line, type);
}

void SyntaxError_T(tParser *Parser, enum eTokens Tok, const char *reason, ...)
{
	message_header(Parser, "error", "syntax");
	fprintf(stderr, "Unexpected %s, ", GetTokenStr(Tok));
	va_list	args;
	va_start(args, reason);
	vfprintf(stderr, reason, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void SyntaxError(tParser *Parser, const char *format, ...)
{
	message_header(Parser, "error", "syntax");
	va_list	args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void SyntaxAssert(tParser *Parser, enum eTokens tok, enum eTokens expected)
{
	if( tok != expected )
	{
		message_header(Parser, "error", "syntax");
		fprintf(stderr, "Unexpected %s, expected %s", GetTokenStr(tok), GetTokenStr(expected));
		fprintf(stderr, "\n");
	}
}

void LexerError(tParser *Parser, const char *format, ...)
{
	message_header(Parser, "error", "lex");
	va_list	args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	//longjmp(Parser->ErrorTarget);
}

