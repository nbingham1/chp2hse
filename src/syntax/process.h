/*
 * process.h
 *
 *  Created on: Oct 28, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "../common.h"
#include "../keyword.h"
#include "../tokenizer.h"
#include "../variable_space.h"
#include "composition.h"
#include "declaration.h"
#include "dot.h"

#ifndef process_h
#define process_h

/* This structure represents a process. Processes act in parallel
 * with each other and can only communicate with other processes using
 * channels. We need to keep track of the sequential that defines this process and
 * the input and output signals. The final element in this structure is
 * a list of production rules that are the result of the compilation.
 */
struct process : keyword
{
	process();
	process(tokenizer &tokens, type_space &types);
	~process();

	variable_space			vars;
	variable_space			env_vars;
	vector<declaration*>	args;

	instruction*			chp;
	instruction*			env_chp;

	bool					has_environment;

	dot_graph				hse;

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types);
	void build_hse();
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const process &p);

#endif
