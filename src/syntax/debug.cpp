/*
 * debug.cpp
 *
 *  Created on: Aug 13, 2013
 *      Author: nbingham
 */

#include "debug.h"
#include "../message.h"

debug::debug()
{
	this->_kind = "debug";
	this->parent = NULL;
	this->expr = NULL;
}

debug::debug(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind = "debug";
	this->parent = parent;
	this->expr = NULL;

	parse(tokens, types, vars);
}

debug::debug(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "debug";
	this->parent = NULL;
	this->expr = NULL;

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		debug *source = (debug*)instr;
		this->parent = parent;
		this->type = source->type;
		this->expr = new expression(source->expr, tokens, vars, this, rename);
	}
}

debug::~debug()
{
	if (expr != NULL)
		delete expr;
	expr = NULL;
}

bool debug::is_next(tokenizer &tokens, size_t i)
{
	string token = tokens.peek(i);
	if (token == "{" || token == "assert" || token == "assume" || token == "enforce" || token == "require")
		return true;

	return false;
}

void debug::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;

	if (tokens.peek_next() == "{")
		type = "assert";
	else
	{
		tokens.increment();
		tokens.push_expected("assert");
		tokens.push_expected("assume");
		tokens.push_expected("enforce");
		tokens.push_expected("require");
		tokens.push_bound("{");
		type = tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();
	}

	tokens.increment();
	tokens.push_expected("{");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	tokens.increment();
	tokens.push_bound("}");
	expr = new expression(tokens, types, vars, this);
	tokens.decrement();

	tokens.increment();
	tokens.push_expected("}");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	end_token = tokens.index;
}

vector<dot_node_id> debug::build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions)
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

void debug::hide(variable_space &vars, vector<variable_index> uids)
{
	if (expr != NULL)
		expr->hide(vars, uids);
}

void debug::print(ostream &os, string newl)
{
	os << type << "{";
	if (expr != NULL)
		expr->print(os, newl);
	else
		os << "null";
	os << "}";
}

ostream &operator<<(ostream &os, const debug &d)
{
	os << d.type << "{";
	if (d.expr != NULL)
		os << *d.expr;
	else
		os << "null";
	os << "}";

	return os;
}
