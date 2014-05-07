/*
 * skip.cpp
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "skip.h"
#include "../message.h"

skip::skip()
{
	this->_kind = "skip";
}

skip::skip(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind		= "skip";
	this->parent	= parent;

	parse(tokens, types, vars);
}

skip::skip(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "skip";

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{

	}
}

skip::~skip()
{
}

bool skip::is_next(tokenizer &tokens, size_t i)
{
	return (tokens.peek(i) == "skip");
}

void skip::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;

	tokens.increment();
	tokens.push_expected("skip");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	end_token = tokens.index;
}

vector<dot_node_id> skip::build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions)
{
	dot_node_id trans("T" + to_string(num_transitions++));
	stmts.push_back(dot_stmt());
	stmts.back().stmt_type = "node";
	stmts.back().node_ids.push_back(trans);
	stmts.back().attr_list.attrs.push_back(dot_a_list());
	stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "plaintext"));
	stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", "1"));
	for (size_t j = 0; j < last.size(); j++)
	{
		stmts.push_back(dot_stmt());
		stmts.back().stmt_type = "edge";
		stmts.back().node_ids.push_back(last[j]);
		stmts.back().node_ids.push_back(trans);
	}

	return vector<dot_node_id>(1, trans);
}

void skip::print(ostream &os, string newl)
{
	os << "skip";
}

ostream &operator<<(ostream &os, const skip &s)
{
	os << "skip";
	return os;
}
