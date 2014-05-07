/*
 * instruction.cpp
 *
 *  Created on: Oct 28, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 */

#include "instruction.h"
#include "syntax/assignment.h"
#include "syntax/composition.h"
#include "syntax/constant.h"
#include "syntax/control.h"
#include "syntax/debug.h"
#include "syntax/declaration.h"
#include "syntax/expression.h"
#include "syntax/instance.h"
#include "syntax/skip.h"
#include "syntax/slice.h"
#include "syntax/type_name.h"
#include "syntax/variable_name.h"
#include "message.h"

instruction::instruction()
{
	parent = NULL;
	_kind = "instruction";
	start_token = 0;
	end_token = 0;
}

instruction::~instruction()
{
	_kind = "instruction";
	parent = NULL;
}

/** For type checking during runtime.
 */
string instruction::kind()
{
	return _kind;
}

/** This calls the parse function of the class given in _kind.
 *  It does NOT call the parse function dependent upon what is
 *  next in the token stream.
 *
 *  Classes inheriting from instruction should define a parse
 *  function with this interface that consumes some tokens from
 *  the token stream in order to fill data within the class. The
 *  function must start with:	start_token = tokens.index+1;
 *  and end with:				end_token = tokens.index;
 *  So that higher order errors can be flagged.
 */
void instruction::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	if (this == NULL)
		internal(tokens, "attempting to call the parse function of a null pointer", __FILE__, __LINE__);
	else if (this->kind() == "assignment")
		((assignment*)this)->parse(tokens, types, vars);
	else if (this->kind() == "composition")
		((composition*)this)->parse(tokens, types, vars);
	else if (this->kind() == "constant")
		((constant*)this)->parse(tokens, types, vars);
	else if (this->kind() == "control")
		((control*)this)->parse(tokens, types, vars);
	else if (this->kind() == "debug")
		((debug*)this)->parse(tokens, types, vars);
	else if (this->kind() == "declaration")
		((declaration*)this)->parse(tokens, types, vars);
	else if (this->kind() == "expression")
		((expression*)this)->parse(tokens, types, vars);
	else if (this->kind() == "instance")
		((instance*)this)->parse(tokens, types, vars);
	else if (this->kind() == "skip")
		((skip*)this)->parse(tokens, types, vars);
	else if (this->kind() == "slice")
		((slice*)this)->parse(tokens, types, vars);
	else if (this->kind() == "type_name")
		((type_name*)this)->parse(tokens, types, vars);
	else if (this->kind() == "variable_name")
		((variable_name*)this)->parse(tokens, types, vars);
	else
		internal(tokens, "unrecognized instruction of kind '" + this->kind() + "'", __FILE__, __LINE__);
}

/** This calls the build_hse function of the class given in _kind.
 *
 *  Classes inheriting from instruction should define a build_hse
 *  function with this interface that dumps the necessary class data
 *  into the dot statement list so that it may be printed as a graphical
 *  representation of HSE. A graphical representation is being used so
 *  that this program may later support non-properly nested reshufflings.
 */
vector<dot_node_id> instruction::build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions)
{
	if (this == NULL)
		return last;
	else if (this->kind() == "assignment")
		return ((assignment*)this)->build_hse(vars, stmts, last, num_places, num_transitions);
	else if (this->kind() == "composition")
		return ((composition*)this)->build_hse(vars, stmts, last, num_places, num_transitions);
	else if (this->kind() == "control")
		return ((control*)this)->build_hse(vars, stmts, last, num_places, num_transitions);
	else if (this->kind() == "debug")
		return ((debug*)this)->build_hse(vars, stmts, last, num_places, num_transitions);
	else if (this->kind() == "skip")
		return ((skip*)this)->build_hse(vars, stmts, last, num_places, num_transitions);
	else
		return last;
}

void instruction::hide(variable_space &vars, vector<variable_index> uids)
{
	if (this == NULL)
	{

	}
	else if (this->kind() == "assignment")
		((assignment*)this)->hide(vars, uids);
	else if (this->kind() == "composition")
		((composition*)this)->hide(vars, uids);
	else if (this->kind() == "control")
		((control*)this)->hide(vars, uids);
	else if (this->kind() == "debug")
		((debug*)this)->hide(vars, uids);
	else if (this->kind() == "expression")
		((expression*)this)->hide(vars, uids);
}

/** This calls the print function of the class given in _kind.
 *
 *  Classes inheriting from instruction should define a print
 *  function with this interface that prints the HSE in a normal
 *  human readable format. This is defined so that people may
 *  check to make sure the parser didn't screw something up.
 */
void instruction::print(ostream &os, string newl)
{
	if (this == NULL)
		os << "null";
	else if (this->kind() == "assignment")
		((assignment*)this)->print(os, newl);
	else if (this->kind() == "composition")
		((composition*)this)->print(os, newl);
	else if (this->kind() == "constant")
		((constant*)this)->print(os, newl);
	else if (this->kind() == "control")
		((control*)this)->print(os, newl);
	else if (this->kind() == "debug")
		((debug*)this)->print(os, newl);
	else if (this->kind() == "declaration")
		((declaration*)this)->print(os, newl);
	else if (this->kind() == "expression")
		((expression*)this)->print(os, newl);
	else if (this->kind() == "instance")
		((instance*)this)->print(os, newl);
	else if (this->kind() == "skip")
		((skip*)this)->print(os, newl);
	else if (this->kind() == "slice")
		((slice*)this)->print(os, newl);
	else if (this->kind() == "type_name")
		((type_name*)this)->print(os, newl);
	else if (this->kind() == "variable_name")
		((variable_name*)this)->print(os, newl);
}

/** In all cases, this is the same as print, however
 *  the spacing is not as good. This is just used if
 *  you want to do some quick debugging.
 */
ostream &operator<<(ostream &os, instruction* i)
{
	if (i == NULL)
		os << "null";
	else if (i->kind() == "assignment")
		os << *((assignment*)i);
	else if (i->kind() == "composition")
		os << *((composition*)i);
	else if (i->kind() == "constant")
		os << *((constant*)i);
	else if (i->kind() == "control")
		os << *((control*)i);
	else if (i->kind() == "debug")
		os << *((debug*)i);
	else if (i->kind() == "declaration")
		os << *((declaration*)i);
	else if (i->kind() == "expression")
		os << *((expression*)i);
	else if (i->kind() == "instance")
		os << *((instance*)i);
	else if (i->kind() == "skip")
		os << *((skip*)i);
	else if (i->kind() == "slice")
		os << *((slice*)i);
	else if (i->kind() == "type_name")
		os << *((type_name*)i);
	else if (i->kind() == "variable_name")
		os << *((variable_name*)i);

	return os;
}
