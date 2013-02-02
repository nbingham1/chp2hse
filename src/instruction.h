/*
 * instruction.h
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "common.h"
#include "state.h"
#include "variable.h"
#include "keyword.h"
#include "rule.h"
#include "graph.h"

#ifndef instruction_h
#define instruction_h

/* This structure describes an instruction in the chp program, namely what lies between
 * two semicolons in a block of. This has not been expanded to ;S1||S2; type of composition.
 */
struct instruction
{
protected:
	string _kind;

public:
	instruction();
	virtual ~instruction();


	// The raw CHP of this instruction.
	string chp;

	// This is the list of production rules that defines this instruction
	list<rule> rules;

	// Some pointers for good use
	map<string, variable> *global;
	map<string, variable> *label;

	// For outputting debugging messages
	string tab;
	int verbosity;

	string kind();

	void print_prs();

	virtual instruction *duplicate(map<string, variable> *globals, map<string, variable> *labels, map<string, string> convert) = 0;

	virtual void expand_shortcuts() = 0;
	virtual void parse(map<string, keyword*> types) = 0;
	virtual int generate_states(state_space *space, graph *trans, int init) = 0;
	virtual void generate_prs() = 0;
};

#endif
