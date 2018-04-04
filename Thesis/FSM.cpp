#include <algorithm>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <memory>
#include <fstream>

#include <llvm/Support/raw_ostream.h>

#include "FSM.h"
#include "BinarySMT.h"
#include "ExecSolver.h"
#include "DefectStorage.h"

using namespace std;

std::string PrintSMT(int iSat, const FormulaStorage& f)
{
	stringstream ss;
	ss << "form" << iSat << ".sat";
	ofstream outf(ss.str());
	outf << FormulaeToStringSat(f);
	outf.close();
	return ss.str();
}

FSM::FSM() 
    : iSat(0)
{
	CreateStart();
}

void FSM::CreateStart()
{
	StateFSM start;
	start.id = 0;
	states.push_back(start);
}

void FSM::Reset()
{
	SaveToXML();
	StatesStorage().swap(states);
	TransitionsStorage().swap(transitions);
	events.clear();
	iSat = 0;
	string().swap(functionName);
	CreateStart();
}

bool FSM::GetState(FSMID id, StateFSM& s) 
{
	int ss = states.size();
	if (id < ss && id >= 0)
	{
		if (states[id].id == id)
		{
			s = states[id];
			return true;
		}
		else
		{
			stringstream sstr;
			sstr << "id == " << id << " and index in vector states must be the same";
			throw logic_error(sstr.str());
		}
	}
	return false;
}

template<class T>
bool MoveVector(vector<T>& source, vector<T>& dst)
{
	vector<T> tmp;
	tmp.swap(source);
	tmp.insert(tmp.end(), dst.begin(), dst.end());
	dst.swap(tmp);
	return true;
}

int FSM::StateToLeaf(int leafId, const StateFSM& newState)
{
	return StateToLeaf(leafId, newState, TransitionFSM::EPSILON);
}

int FSM::StateToLeaf(int leafId, const StateFSM& newState, const std::string& pred)
{
	StateFSM s = newState;
	TransitionFSM t;
	t.start = leafId;
	s.id = GetNewStateID();
	t.end = s.id;
	t.id = GetNewTransitionID();
	t.evt = pred;
	StateFSM &leaf = states[leafId];
	leaf.outgoing.push_back(t.id);
	s.incoming.push_back(t.id);
	//Move all the formulae from old leaf to new leaf.
	s.formulae.insert(s.formulae.begin(), leaf.formulae.begin(), leaf.formulae.end());
	leaf.formulae.clear();

	MoveVector(leaf.allocArrays, s.allocArrays);
	MoveVector(leaf.allocPointers, s.allocPointers);
	MoveVector(leaf.delArrays, s.delArrays);
	MoveVector(leaf.delPointers, s.delPointers);

	states.push_back(s);
	transitions.push_back(t);
	//states.insert(pair<FSMID, StateFSM>(s.id, s));
	//transitions.insert(pair<FSMID, TransitionFSM>(t.id, t));
	return s.id;
}



void FSM::AddStateToLeaves(const StateFSM& s, LeafPredicate& pred)
{
	AddStateToLeaves(s, pred, TransitionFSM::EPSILON, true);
}

//TODO: Refactor this function!
void FSM::AddStateToLeaves(const StateFSM& s, LeafPredicate& pred, const std::string& cond, bool finCond)
{
	int size = states.size();
	for (int i = 0; i < size; ++i)
	{
		//if (states[i].IsLeaf())
		if (pred(states[i]))
		{
			if (MatchEvents(i)) //This is an expensive check, so we want perform it when other conditions are checked.
			{
				states[i].isBranchLeaf = !finCond;
				StateToLeaf(i, s, cond);
			}
		}
	}
	/*
	for (StatesIter iter = states.begin(); iter != states.end(); ++iter)
	{
		if (iter->outgoing.empty()) //leaf
		{
			TransitionFSM t;
			t.start = iter->id;
			s.id = GetNewStateID();
			t.end = s.id;
			t.id = GetNewTransitionID();
			t.evt = TransitionFSM::EPSILON;
			states.insert(pair<FSMID, StateFSM>(s.id, s));
			transitions.insert(pair<FSMID, TransitionFSM>(t.id, t));
		}
	}
	*/
}

/*
void FSM::AddBranchToLeaves(StateFSM s)
{
	for (StatesIter iter = states.begin(); iter != states.end(); ++iter)
	{
		if (iter->second.outgoing.empty()) //leaf
		{
			TransitionFSM t;
			t.start = iter->second.id;
			s.id = GetNewStateID();
			t.end = s.id;
			t.id = GetNewTransitionID();
			t.evt = TransitionFSM::EPSILON;
			states.insert(pair<FSMID, StateFSM>(s.id, s));
			transitions.insert(pair<FSMID, TransitionFSM>(t.id, t));
		}
	}
}
*/
void FSM::AddFormulaSMT(const std::shared_ptr<FormulaSMT>& f)
{
	int size = states.size();
	for (int i = 0; i < size; ++i)
	{
		if (states[i].outgoing.empty())
		{
			states[i].formulae.push_back(f);
		}
	}
}

void FSM::AddAllocPointer(const VersionedVariable& ap)
{
	int size = states.size();
	for (int i = 0; i < size; ++i)
	{
		if (states[i].outgoing.empty())
		{
			states[i].allocPointers.push_back(ap);
		}
	}
}

void FSM::HandleDeletePtr(const VersionedVariable& v, std::vector<VersionedVariable>& alloc, 
			std::vector<VersionedVariable>& del, StateFSM& delState)
{
	//write formulae.
	int size = alloc.size();
	//unsat => delete is correct
	FormulaStorage f = delState.formulae;
	for (int i = 0; i < size; ++i)
	{
		shared_ptr<BinarySMT> form(new BinarySMT(v, alloc[i], EqualSMT, true));
		f.push_back(form);
	}

	auto fileName = PrintSMT(iSat, f);

	auto solverResult = runSolver(fileName);

	if (solverResult.find("unsat") != -1) //unsat
	{
		llvm::errs() << "Correct delete\n";
		del.push_back(v);
	}
}

void FSM::AddDeleteState(const VersionedVariable& var, bool arrayForm)
{
	int size = states.size();
	for (int i = 0; i < size; ++i)
	{
		if (states[i].outgoing.empty())
		{
			StateFSM sdel;
			
			int sdelId = StateToLeaf(i, sdel);

			StateFSM &del = states[sdelId];
			VersionedVariable v(var);
			switch (var.MetaType())
			{
			case VAR_POINTER:
				HandleDeletePtr(v, del.allocPointers, del.delPointers, del);
				break;
			case VAR_ARRAY_POINTER:
				HandleDeletePtr(v, del.allocArrays, del.delArrays, del);
				break;
			default:
				break;
			}
			++iSat;
		}
	}
}

void FSM::ProcessReturnNone()
{
	//For each leaf
    for (StateFSM &s : states)
	{
		if (s.IsLeaf())
		{
			//Create a new end state
			CreateNewRetNoneState(s);
		}
	}
}

void FSM::SolveRet(bool isArray, const StateFSM& s) 
{
	VarStorage alloc, del;
	//FormulaStorage f = s.formulae; 
	if (isArray)
	{
		alloc = s.allocArrays;
		del = s.delArrays;
	}
	else
	{
		alloc = s.allocPointers;
		del = s.delPointers;
	}
	//for each allocated variable
	auto sAlloc = alloc.size();
	auto sDel = del.size();
	for (int i = 0; i < sAlloc; ++i)
	{
        if (alloc[i].Name() == returnVarName && !isAllocReturns) {
            // TODO: return pointer to allocated var
            isAllocReturns = true;
            continue;
        }

		//check whether this variable is del list.
		FormulaStorage f = s.formulae; //TODO: do this more effective.
		for (int j = 0; j < sDel; ++j)
		{
			//write formulae.
			shared_ptr<BinarySMT> bs(new BinarySMT(alloc[i], del[j], EqualSMT, true)); //unsat => in del list.
			f.push_back(bs);
		}
		auto fileName = PrintSMT(iSat++, f);
		//if this variable is not in del list then there is a memory leak.
		//auto fileName = PrintSMT(iSat, s.formulae);

		auto solverResult = runSolver(fileName);

		if (solverResult.find("unsat") != -1) //unsat
		{
			//No leak
		}
		else if (solverResult.find("sat") != -1)
		{
			//Leak
			llvm::errs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~LEAK~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
			llvm::errs() << "variable name: " << alloc[i].Name() << "\n";
			
			stringstream defect;
			defect << "Memory leak. Variable name: " << alloc[i].Name();
			DefectStorage::Instance().AddDefect(defect.str());
		}
	}

}

void FSM::CreateNewRetNoneState(StateFSM& leaf)
{
	StateFSM s;
	s.id = GetNewStateID();

	s.formulae.insert(s.formulae.begin(), leaf.formulae.begin(), leaf.formulae.end());
	leaf.formulae.clear();

	MoveVector(leaf.allocArrays, s.allocArrays);
	MoveVector(leaf.allocPointers, s.allocPointers);
	MoveVector(leaf.delArrays, s.delArrays);
	MoveVector(leaf.delPointers, s.delPointers);

	//Note:This may be parallelized
	SolveRet(true, s);
	SolveRet(false, s);

	//Add s to FSM.
	TransitionFSM tr;
	tr.id = GetNewTransitionID();
	tr.start = leaf.id;
	tr.end = s.id;
	leaf.outgoing.push_back(tr.id);
	transitions.push_back(tr);
	s.incoming.push_back(tr.id);
	s.isEnd = true;
	states.push_back(s);
	
}

bool FSM::MatchEvents(FSMID stateID)
{
	//ConditionStorage events = states[stateID].events;
	int cur = stateID;
	auto localEvents = events;
	while (!localEvents.empty())
	{
		if (states[cur].incoming.empty()) //start state.
		{
			break;
		}
		auto trID = states[cur].incoming[0]; //in the current model there is one parent of each state.
		TransitionFSM &tr = transitions[trID];
		auto parent = tr.start;
		if (!localEvents.empty())
		{
			if (tr.evt != TransitionFSM::EPSILON)
			{
				if (tr.evt != localEvents.back())
				{
					return false;
				}
			}
		}
		cur = parent;
		localEvents.pop_back();
	}
	return true;
}

//debug
void FSM::SaveToXML()
{
	string path = functionName;
	if (path.empty())
	{
		path = "defaultFunction";
	}
	path += ".xstd";
	SaveToXML(path);
}

namespace
{
	int indent;
	string DoIndent()
	{
		string res = "";
		for (int i = 0; i < indent; ++i)
		{
			res.push_back('\t');
		}
		return res;
	}

	string CnvStateId(FSMID id)
	{
		stringstream res;
		res << (id * 2);
		return res.str();
	}

	string CnvTransitionId(FSMID id)
	{
		stringstream res;
		res << (id * 2 + 1);
		return res.str();
	}
}

void FSM::SaveToXML(const std::string &path)
{
	ofstream outf(path);
	if (!outf.good())
	{
		return;
	}

	outf << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl;
	outf << "<diagram>" << endl;

	++indent;
	outf << DoIndent() << "<name>" << functionName << "</name>" << endl;

	outf << DoIndent() << "<data>" << endl;
	++indent;
	outf << DoIndent() << "<Statemachine>" << endl;

	++indent;
	for (auto iter = events.begin(); iter != events.end(); ++iter)
	{
		outf << DoIndent() << "<event name=\"" << *iter << "\" comment=\"\"/>" << endl;
	}
	outf << DoIndent() << "<autoreject>False</autoreject>" << endl;
	--indent;

	outf << DoIndent() << "</Statemachine>" << endl;
	--indent;
	outf << DoIndent() << "</data>" << endl;
	--indent;

	llvm::errs() << "States: ";

	for (auto iter = states.begin(); iter != states.end(); ++iter)
	{
		outf << DoIndent() << "<widget id=\"" << CnvStateId(iter->id) << "\" type=\"State\">" << endl;
		++indent;
		outf << DoIndent() << "<attributes>" << endl;
		++indent;
		outf << DoIndent() << "<name>state" << iter->id << "</name>" << endl;
		outf << DoIndent() << "<type>";
		if (iter->id)
		{
			//llvm::errs() << ((iter->isEnd) ? "0 " : "2 ");
			outf << ((!iter->isEnd) ? "0 " : "2 ");
		} 
		else
		{
			llvm::errs() << "1 ";

			outf << "1";
		}
		outf << "</type>" << endl;

		for (auto income = iter->incoming.begin(); income != iter->incoming.end(); ++income)
		{
			outf << DoIndent() << "<incoming id=\"" << CnvTransitionId(*income) << "\"/>" << endl;
		}

		for (auto outg = iter->outgoing.begin(); outg != iter->outgoing.end(); ++outg)
		{
			outf << DoIndent() << "<outgoing id=\"" << CnvTransitionId(*outg) << "\"/>" << endl;
		}

		--indent;
		outf << DoIndent() << "</attributes>" << endl;
		--indent;
		outf << DoIndent() << "</widget>" << endl;
	}
	llvm::errs() << "\n";


	for (auto iter = transitions.begin(); iter != transitions.end(); ++iter)
	{
		outf << "<widget id=\"" << CnvTransitionId(iter->id) << "\" type=\"Transition\">" << endl;
		++indent;
		outf << DoIndent() << "<attributes>" << endl;
		++indent;
		outf << DoIndent() << "<event name=\"" << iter->evt << "\" comment=\"\" />" << endl;
		--indent;
		outf << DoIndent() << "</attributes>" << endl;
		--indent;
		outf << DoIndent() << "</widget>" << endl;
	}

	outf << "</diagram>" << endl;

	outf.close();
}
