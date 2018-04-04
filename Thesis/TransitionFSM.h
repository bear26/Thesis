#ifndef TRANSITIONFSM_H
#define TRANSITIONFSM_H

#include <string>

#include "TypesFSM.h"

class TransitionFSM
{
public:
	FSMID id;
	FSMID start;
	FSMID end;

	std::string evt;
	static const std::string EPSILON;
};

bool operator ==(const TransitionFSM& lhs, const TransitionFSM& rhs);
bool operator <(const TransitionFSM& lhs, const TransitionFSM& rhs);

#endif
