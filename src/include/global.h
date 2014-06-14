/*
PHPOS Bytecode Compiler
Core Include File
GLOBAL.H
*/
#ifndef _GLOBAL_H
#define _GLOBAL_H

#ifdef DEBUG_ENABLED
#define IF_DEBUG(code)	do{code;}while(0)
#define DEBUG_S(str, v...)	printf(str ,## v )
#else
#define IF_DEBUG(code)	do{}while(0)
#define DEBUG_S(...)	do{}while(0)
#endif
#define DEBUG_NL(str, v...)	DEBUG_S("DEBUG %s:%i: %s "str, __FILE__, __LINE__, __func__ ,## v )
#define DEBUG(str, v...)	DEBUG_NL(str"\n" ,## v)

#define ACC_ERRPTR	((void*)-1)

// ---
#define STR(x...) #x
#define EXPAND_STR(x...) STR(x)
// ---

#include <stdlib.h>
#include <stdio.h>

extern void	*CreateRef(const void *Data, size_t Len);
extern void	Ref(void *RefedData);
extern void	Deref(void *RefedData);

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

#define TODO(str, v...)	do{ fprintf(stderr, "%s:%i - TODO: "str"\n", __FILE__,__LINE__,##v); exit(1); }while(0)

#endif
