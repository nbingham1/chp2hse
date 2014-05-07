/*
 * slice.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: nbingham
 */

#include "slice.h"
#include "../message.h"

slice::slice()
{
	this->_kind = "slice";
}

slice::slice(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind = "slice";
	this->parent = parent;

	parse(tokens, types, vars);
}

slice::slice(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "slice";

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		slice *source = (slice*)instr;
		this->parent = parent;
		this->start = source->start;
		this->end = source->end;
	}
}

slice::~slice()
{

}

bool slice::is_next(tokenizer &tokens, size_t i)
{
	return (tokens.peek(i) == "[" && ((tokens.peek(i+2) == ".." && tokens.peek(i+4) == "]") || tokens.peek(i+2) == "]"));
}

void slice::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;

	tokens.increment();
	tokens.push_expected("[");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	start.parse(tokens, types, vars);

	if (tokens.peek(1) == "..")
	{
		tokens.next();
		end.parse(tokens, types, vars);
	}
	else
		end.value = start.value+1;

	tokens.increment();
	tokens.push_expected("]");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	end_token = tokens.index;
}

void slice::print(ostream &os, string newl)
{
	os << "[";
	start.print(os, newl);
	if (end.value - start.value != 1)
	{
		os << "..";
		end.print(os, newl);
	}
	os << "]";
}

ostream &operator<<(ostream &os, const slice &s)
{
	os << "[" << s.start;
	if (s.end.value - s.start.value != 1)
		os << ".." << s.end;
	os << "]";

	return os;
}
