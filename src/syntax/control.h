/*
 * control.h
 *
 *  Created on: Jul 16, 2013
 *      Author: nbingham
 */

#include "composition.h"
#include "expression.h"
#include "variable_name.h"

#ifndef control_h
#define control_h

struct control : instruction
{
	control();
	control(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	control(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	control(tokenizer &tokens, type_space &types, variable_space &vars, variable_name *left, expression *right);
	~control();

	vector<pair<expression*, composition*> > terms;
	bool loop;
	bool mutex;
	instruction *preface;

	bool is_shortcut(tokenizer &tokens);
	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	vector<dot_node_id> build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions);
	void hide(variable_space &vars, vector<variable_index> uids);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const control &c);

#endif
