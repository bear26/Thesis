#include "TransitionFSM.h"

const std::string TransitionFSM::EPSILON = "";

bool operator ==(const TransitionFSM& lhs, const TransitionFSM& rhs)
{
	return lhs.id == rhs.id;
}

bool operator <(const TransitionFSM& lhs, const TransitionFSM& rhs)
{
	return lhs.id < rhs.id;
}
