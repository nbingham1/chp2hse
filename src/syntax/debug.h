/*
 * debug.h
 *
 *  Created on: Aug 13, 2013
 *      Author: nbingham
 */

#include "expression.h"

#ifndef debug_h
#define debug_h

/**
 * assert	- Throws an error if the statement isn't true at a particular point in time
 * require	- Throws an error if the statement isn't true at any point in time
 * assume	- Tries to make the statement true at a particular time. Throws an error if the statement
 * 			  conflicts with known information about the state space.
 * enforce	- Tries to make the statement true at all points in time. Throws an error if the statement
 * 			  conflicts with known information about the state space.
 *
 * TODO Add an allow debug statement that modifies the timing assumptions used for a particular process
 * allow	- Modifies the timing assumptions used for a particular process.
 * 			  __di__		Force a strict delay insensitive circuit
 * 			  __qdi__		Allow isochronic forks.
 * 			  __bounded__	Allow delay lines with a bounded delay.
 * 			  __mesosync__	Allow mesosynchronoous architecture.
 * 			  __sync__		Allow synchronous architecture.
 *
 * TODO Fix debug so that it is an attribute of an instruction instead of standing alone.
 */
struct debug : instruction
{
	debug();
	debug(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	debug(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	~debug();

	string type;
	expression *expr;

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	vector<dot_node_id> build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions);
	void hide(variable_space &vars, vector<variable_index> uids);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const debug &d);

#endif
