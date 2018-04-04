#ifndef VARIABLESMT_H
#define VARIABLESMT_H

#include "FormulaSMT.h"
#include "VersionedVariable.h"

class VariableSMT : public FormulaSMT
{
public:
	virtual std::string FormatSMTLIB() override;

	inline void Var(const VersionedVariable& v) { var = v; }
	inline VersionedVariable Var() { return var; }
private:
	VersionedVariable var;
};

#endif
