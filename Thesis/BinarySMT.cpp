#include <sstream>

#include "BinarySMT.h"

using namespace std;

BinarySMT::BinarySMT(const VersionedVariable& l, const VersionedVariable& r, OperatorSMT o, bool n) 
    : FormulaSMT()
    , lhs(l)
    , rhs(r)
    , op(o)
    , negation(n)
{
}

string BinarySMT::FormatSMTLIB()
{
	string sop;
	switch (op)
	{
	case EqualSMT:
		sop = "= ";
		break;
	case LessSMT:
		sop = "< ";
		break;
	case GreaterSMT:
		sop = "> ";
		break;
	default:
		sop = "= ";
	}

	string negBeg = "";// = 
	string negEnd = "";// = ")";
	if (negation)
	{
		negBeg = "not (";
		negEnd = ")";
	}

	stringstream ss;
	ss << "(assert( " << negBeg << sop << lhs.VersionedName() << " " << rhs.VersionedName() << ")" << negEnd << ")\n";
	return ss.str();
}
