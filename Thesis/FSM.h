#ifndef FSM_H
#define FSM_H

#include <vector>
#include <deque>
#include <map>
#include <memory>
/*
#ifdef _MSC_VER
#if _MSC_VER == 1600
#elif _MSC_VER == 1800
#endif
#endif
*/


#include "StateFSM.h"
#include "TransitionFSM.h"
#include "LeafPredicate.h"
#include "VersionedVariable.h"
#include "FormulaSMT.h"

/*
typedef std::map<FSMID, StateFSM> StatesStorage;
typedef std::map<FSMID, TransitionFSM> TransitionsStorage;
typedef std::map<FSMID, StateFSM>::iterator StatesIter;
typedef std::map<FSMID, TransitionFSM>::iterator TransitionsIter;
*/
typedef std::vector<StateFSM> StatesStorage;
typedef std::vector<TransitionFSM> TransitionsStorage;
typedef std::vector<StateFSM>::iterator StatesIter;
typedef std::vector<TransitionFSM>::iterator TransitionsIter;
typedef std::deque<ConditionEvent> ConditionStorage;

#define Cout llvm::errs()

std::string PrintSMT(int iSat, const FormulaStorage& f);

class FSM
{
public:
	FSM();
	void Reset();
	std::string FunctionName() const { return functionName; }
	void FunctionName(const std::string& val) { functionName = val; }

	inline void AddState(const StateFSM& s) { states[s.id] = s; }
	inline void AddTransition(const TransitionFSM& trans) { transitions[trans.id] = trans; }
	bool GetState(FSMID id, StateFSM& s);
	bool GetTransition(FSMID id, TransitionFSM& t);

	inline FSMID GetNewStateID() const { return states.size(); }
	inline FSMID GetNewTransitionID() const { return transitions.size(); }

	inline void PushCondition(const std::string& cond) { events.push_back(cond); }
	inline void PopCondition() { if (!events.empty()) events.pop_back(); }

	int StateToLeaf(int leafId, const StateFSM& newState);
	int StateToLeaf(int leafId, const StateFSM& newState, const std::string& pred);
	void AddStateToLeaves(const StateFSM& s, LeafPredicate& pred);
	void AddStateToLeaves(const StateFSM& s, LeafPredicate& pred, const std::string& cond, bool finCond);
	//void AddBranchToLeaves(StateFSM s);
	void AddFormulaSMT(const std::shared_ptr<FormulaSMT>& f); //Add SMT-formula to all leaves;
	void AddAllocPointer(const VersionedVariable& ap);
	//void AddTransition(shared_ptr<StateFSM> begin, shared_ptr<StateFSM> end, shared_ptr<Tra)
	void AddDeleteState(const VersionedVariable& var, bool arrayForm);
	void ProcessReturnNone();


public:
    void SetReturnVarName(const std::string& varName) { returnVarName = varName; isAllocReturns = false; }
    void ClearReturnVarName() { returnVarName = ""; }

    bool IsAllocReturns() { return isAllocReturns; }

private:
    std::string returnVarName;
    bool isAllocReturns = false;

public: //debug
	void SaveToXML();
	void SaveToXML(const std::string& path);
private://internal functions
	void CreateStart();
	void HandleDeletePtr(const VersionedVariable& v, std::vector<VersionedVariable>& alloc, 
		std::vector<VersionedVariable>& del, StateFSM& delState);
	void CreateNewRetNoneState(StateFSM& leaf);
	void SolveRet(bool isArray, const StateFSM& s);
	bool MatchEvents(FSMID stateID);
private: //member variables
	std::string functionName;
	StatesStorage states;
	TransitionsStorage transitions;
	int iSat;		
	ConditionStorage events;
};

#endif
