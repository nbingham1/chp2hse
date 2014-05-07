/*
 * variable_name.h
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "../instruction.h"
#include "instance.h"
#include "slice.h"

#ifndef variable_name_h
#define variable_name_h

struct variable_name : instruction
{
	variable_name();
	variable_name(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	variable_name(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	variable_name(tokenizer &tokens, type_space &types, variable_space &vars, variable_index var, slice *bits);
	~variable_name();

	variable_index var;
	instance name;
	slice *bits;

	bool is_node(variable_space &vars);
	bool in_bounds(variable_space &vars, int i);
	int width(variable_space &vars);
	void restrict_bits(tokenizer &tokens, type_space &types, variable_space &vars);
	string type_string(variable_space &vars);
	string expr_string(variable_space &vars);

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	void print(ostream &os = cout, string newl = "\n");

};

ostream &operator<<(ostream &os, const variable_name &v);

#endif
