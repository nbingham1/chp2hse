/*
 * guard.h
 *
 *  Created on: Jan 22, 2013
 *      Author: nbingham
 */

#include "common.h"
#include "state.h"
#include "variable.h"
#include "keyword.h"
#include "rule.h"
#include "instruction.h"
#include "graph.h"

#ifndef guard_h
#define guard_h

struct guard : instruction
{
	guard();
	guard(string chp, map<string, keyword*> types, map<string, variable> *globals, string tab, int verbosity);
	~guard();

	int uid;					// indexes into the state in the state space

	void expand_shortcuts();
	void parse(map<string, keyword*> types);
	int generate_states(state_space *space, graph *trans, int init);
	void generate_prs(map<string, variable> *globals);
};

state solve(string raw, map<string, variable> *vars, string tab, int verbosity);

#endif