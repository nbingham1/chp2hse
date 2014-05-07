/*
 * constant.h
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "../instruction.h"

#ifndef constant_h
#define constant_h

struct constant : instruction
{
	constant();
	constant(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	constant(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	constant(tokenizer &tokens, type_space &types, variable_space &vars, int value);
	~constant();

	int value;
	int width;

	void restrict_bits(tokenizer &tokens, type_space &types, variable_space &vars);
	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const constant &c);

#endif
