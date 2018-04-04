#ifndef FORMULASMT_H
#define FORMULASMT_H

#include <string>

class FormulaSMT
{
public:
	virtual std::string FormatSMTLIB() = 0;
};

#endif
