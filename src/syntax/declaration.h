/*
 * declaration.h
 *
 * This structure describes a declaration in the chp program. We need
 * to keep track of a declarations name, its type (record, keyword, process),
 * and its bit width.
 *
 */

#include "../instruction.h"
#include "expression.h"
#include "instance.h"
#include "type_name.h"

#ifndef declaration_h
#define declaration_h

struct declaration : instruction
{
	declaration();
	declaration(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent);
	declaration(tokenizer &tokens, keyword *type, int width, string name, string reset, vector<string> inputs, type_space &types, variable_space &vars, instruction *parent);
	declaration(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename);
	~declaration();

	type_name	type;
	instance	name;
	expression	*reset;
	instruction *preface;

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const declaration &d);

#endif
