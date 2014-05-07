/*
 * assignment.h
 *
 *  Created on: Apr 8, 2014
 *      Author: nbingham
 */

#include "../instruction.h"
#include "variable_name.h"
#include "expression.h"

#ifndef assignment_h
#define assignment_h

struct assignment : instruction
{
	assignment();
	assignment(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	assignment(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	assignment(tokenizer &tokens, type_space &types, variable_space &vars, int count, ...);
	~assignment();

	vector<pair<variable_name*, expression*> > expr;
	instruction *preface;

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	vector<dot_node_id> build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions);
	void hide(variable_space &vars, vector<variable_index> uids);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const assignment &a);

#endif
