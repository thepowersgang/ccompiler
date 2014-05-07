/*
 */
#include <irm.h>

// === TYPES ===
struct sIRMState
{
};

// === CODE ===
tIRMHandle IRM_CreateFunction(const char *Name)
{
	return NULL;
}

void IRM_AppendConstant(tIRMHandle Handle, tIRMReg Register, uint64_t Value)
{
}
void IRM_AppendCharacterConstant(tIRMHandle Handle, tIRMReg Register, size_t Length, const char *Data)
{
}
void IRM_AppendSymbol(tIRMHandle Handle, tIRMReg Register, const tSymbol *Symbol)
{
}

