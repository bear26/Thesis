#include "Variable.h"

using namespace std;

string Variable::TypeSMT() const
{
	if ((type.find("float") != string::npos) ||
		(type.find("double") != string::npos))
	{
		return "Real";
	}

	return "Int";
}
