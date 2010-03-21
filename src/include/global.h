/*
PHPOS Bytecode Compiler
Core Include File
GLOBAL.H
*/
#ifndef _GLOBAL_H
#define _GLOBAL_H

#if DEBUG
# define DEBUG_S(...)	printf(__VA_ARGS__)
#else
# define DEBUG_S(...)
#endif

// ---
#define STR(x...) #x
#define EXPAND_STR(x...) STR(x)
// ---

#include <stdlib.h>
#include <stdio.h>
#include <parser.h>

extern int	GetVariableId();
extern int	GetCodeOffset();
extern int	RegisterString(char *str, int length);

extern void	*SaveCodeBuf(int start, int *length);
extern void	AppendCodeBuf(void *buffer, int length);
extern void	SetArgument1(int Offset, uint Value);
extern void	SetArgument2(int Offset, uint Value);

typedef struct {
	char	*Data;
	 int	Offset;
	 int	Length;
} tConstantString;

extern tConstantString	*gaStrings;
extern int	giStringCount;


#ifdef DEBUG
# if DEBUG >= 3
#  define	DEBUG3(v...)	printf(v)
# else
#  define	DEBUG3(v...)
# endif
# if DEBUG >= 2
#  define	DEBUG2(v...)	printf(v)
# else
#  define	DEBUG2(v...)
# endif
# if DEBUG
#  define	DEBUG1(v...)	printf(v)
# else
#  define	DEBUG1(v...)
# endif
#else
# define	DEBUG1(v...)
# define	DEBUG2(v...)
# define	DEBUG3(v...)
#endif


#endif
