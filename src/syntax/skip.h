/*
 * skip.h
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "../instruction.h"

#ifndef skip_h
#define skip_h

struct skip : instruction
{
	skip();
	skip(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	skip(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	~skip();

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	vector<dot_node_id> build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const skip &s);

#endif
