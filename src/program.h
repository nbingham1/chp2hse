/*
 * program.h
 *
 *  Created on: Apr 25, 2014
 *      Author: Ned Bingham
 */

#include "common.h"
#include "tokenizer.h"
#include "keyword.h"

#ifndef program_h
#define program_h

/** This is the top level of the CHP parse tree, containing a record
 *  of all of the types described in the program.
 */

struct program
{
	program();
	~program();

	// The list of types (processes, operators, channels, and records)
	type_space	types;
	
	// This keeps track of what files we have been parsed for the include preprocessor command
	vector<string> loaded;
	vector<string> loading;

	bool parse(tokenizer &tokens);
};

#endif
