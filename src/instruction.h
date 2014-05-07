/*
 * instruction.h
 *
 *  Created on: Oct 28, 2012
 *      Author: Ned Bingham
 */

#include "common.h"
#include "keyword.h"
#include "variable_space.h"
#include "tokenizer.h"
#include "syntax/dot.h"

#ifndef instruction_h
#define instruction_h

/** This structure is the abstract base class for all CHP syntaxes.
 *  Its primarily used for two things: polymorphism (having a list
 *  of generic instruction is nice), and distributing function calls
 *  to the correct child by checking the value of the _kind field.
 */
struct instruction
{
protected:
	/** Used for type checking during runtime. All classes that inherit
	 *  from instruction must define their _kind field as their name in
	 *  all of their constructors.
	 */
	string _kind;

public:
	instruction();
	virtual ~instruction();

	/** The parent instruction in the syntax hierarchy. I don't think that
	 *  this is actually used anywhere, but it is always correctly set up
	 *  in the event that someone ever needs it.
	 */
	instruction *parent;

	/** The range of tokens from which this instruction was parsed.
	 */
	int start_token;
	int end_token;

	string kind();

	void parse(tokenizer &tokens, type_space &types, variable_space &vars);
	vector<dot_node_id> build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions);
	void hide(variable_space &vars, vector<variable_index> uids);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, instruction* i);

#endif
