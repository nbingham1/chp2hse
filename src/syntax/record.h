/*
 * record.h
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "../common.h"
#include "../keyword.h"
#include "../tokenizer.h"
#include "../variable_space.h"
#include "debug.h"
#include "declaration.h"

#ifndef record_h
#define record_h

/* This structure represents a structure or record. A record
 * contains a bunch of member variables that help you index
 * segments of bits within the multibit signal.
 */
struct record : keyword
{
	record();
	record(tokenizer &tokens, type_space &types);
	~record();

	vector<instruction*> terms;

	variable_space vars;

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const record &r);

#endif
