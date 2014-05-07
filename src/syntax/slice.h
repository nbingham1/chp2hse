/*
 * slice.h
 *
 *  Created on: Apr 8, 2014
 *      Author: nbingham
 */

#include "../instruction.h"
#include "constant.h"

#ifndef slice_h
#define slice_h

struct slice : instruction
{
	slice();
	slice(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	slice(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	~slice();

	constant start;
	constant end;

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const slice &s);

#endif
