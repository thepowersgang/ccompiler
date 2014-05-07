/*
 * TODO
 */
#ifndef _IRM_H_
#define _IRM_H_

#include <symbol.h>

typedef struct sIRMState*	tIRMHandle;
typedef int	tIRMReg;

extern void	IRM_AppendConstant(tIRMHandle Handle, tIRMReg Register, uint64_t Value);
extern void	IRM_AppendCharacterConstant(tIRMHandle Handle, tIRMReg Register, size_t Length, const char *Data);
extern void	IRM_AppendSymbol(tIRMHandle Handle, tIRMReg Register, const tSymbol *Symbol);

#endif
