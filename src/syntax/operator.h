/*
 * process.h
 *
 *  Created on: Oct 28, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "process.h"

#ifndef operator_h
#define operator_h

/* This structure represents a process. Processes act in parallel
 * with each other and can only communicate with other processes using
 * channels. We need to keep track of the sequential that defines this process and
 * the input and output signals. The final element in this structure is
 * a list of production rules that are the result of the compilation.
 */
struct operate : process
{
	operate();
	operate(tokenizer &tokens, type_space &types);
	~operate();

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const operate &o);

#endif
