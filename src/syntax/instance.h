/*
 * instance.h
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "../instruction.h"

#ifndef instance_h
#define instance_h

struct instance : instruction
{
	instance();
	instance(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	instance(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	instance(string instr, instruction *parent, map<string, string> rename);
	instance(string value);
	~instance();

	string value;

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const instance &i);

#endif
