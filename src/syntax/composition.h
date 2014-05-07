/*
 * composition.h
 *
 *  Created on: Nov 1, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "../instruction.h"
#include "dot.h"

#ifndef composition_h
#define composition_h

struct composition : instruction
{
	composition();
	composition(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent, size_t i = 0);
	composition(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	composition(tokenizer &tokens, type_space &types, variable_space &vars, string op, int count, ...);
	composition(tokenizer &tokens, type_space &types, variable_space &vars, string op, vector<instruction*> terms);
	~composition();

	vector<instruction*> terms;
	vector<string> operators;

	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars, size_t i);

	vector<dot_node_id> build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions);
	void hide(variable_space &vars, vector<variable_index> uids);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const composition &a);

#endif
