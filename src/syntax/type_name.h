/*
 * type_name.h
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "../instruction.h"
#include "constant.h"
#include "instance.h"
#include "expression.h"

#ifndef type_name_h
#define type_name_h

struct type_name : instruction
{
	type_name();
	type_name(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	type_name(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	~type_name();

	keyword *key;
	instance name;
	constant width;
	vector<expression*> inputs;
	string kind_guess;
	instruction *preface;

	string type_string(variable_space &vars);
	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const type_name &t);

#endif
