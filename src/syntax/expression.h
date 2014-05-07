/*
 * expression.h
 *
 *  Created on: Apr 8, 2014
 *      Author: nbingham
 */

#include "../instruction.h"
#include "instance.h"
#include "constant.h"
#include "variable_name.h"

#ifndef expression_h
#define expression_h

struct expression : instruction
{
	expression();
	expression(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent, size_t i = 0);
	expression(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	expression(tokenizer &tokens, type_space &types, variable_space &vars, string value);
	expression(tokenizer &tokens, type_space &types, variable_space &vars, int value);
	~expression();

	string last_operator;
	instruction *preface;
	variable_name *var;
	constant *val;
	string expr;

	int width(variable_space &vars);
	bool is_node(variable_space &vars);
	string type_string(variable_space &vars);
	string expr_string(variable_space &vars);

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars, size_t i);
	void hide(variable_space &vars, vector<variable_index> uids);
	void print(ostream &os = cout, string newl = "\n");
};

string hide(tokenizer &tokens, variable_space &vars, vector<variable_index> uids);

ostream &operator<<(ostream &os, const expression &e);

#endif
