/*
 * record.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "record.h"
#include "../message.h"

record::record()
{
	name = "";
	_kind = "record";
}

record::record(tokenizer &tokens, type_space &types)
{
	_kind = "record";

	parse(tokens, types);
}

record::~record()
{
	for (vector<instruction*>::iterator term = terms.begin(); term != terms.end(); term++)
		delete (*term);
	terms.clear();
}

bool record::is_next(tokenizer &tokens, size_t i)
{
	return (tokens.peek(i) == "record");
}

void record::parse(tokenizer &tokens, type_space &types)
{
	start_token = tokens.index+1;

	if (tokens.next() != "record")
		error(tokens, "expected record definition", "", __FILE__, __LINE__);

	tokens.increment();
	tokens.push_expected("[instance]");
	tokens.push_bound("{");
	tokens.syntax(__FILE__, __LINE__);
	if (instance::is_next(tokens))
		name = instance(tokens, types, vars, NULL).value;
	tokens.decrement();

	tokens.increment();
	tokens.push_expected("{");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	tokens.increment();
	tokens.push_bound("}");
	string token = tokens.peek_next();
	while (token != "}")
	{
		tokens.increment();
		tokens.push_bound(";");
		bool error = false;
		if (token == "enforce" || token == "require")
			terms.push_back(new debug(tokens, types, vars, NULL));
		else if (declaration::is_next(tokens))
			terms.push_back(new declaration(tokens, types, vars, NULL));
		else
			error = true;
		tokens.decrement();

		if (!error)
		{
			tokens.increment();
			tokens.push_expected(";");
			tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();
		}
		else if (tokens.peek_next() != "}")
			tokens.next();

		token = tokens.peek_next();
	}
	tokens.decrement();

	tokens.increment();
	tokens.push_expected("}");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	end_token = tokens.index;
}

void record::print(ostream &os, string newl)
{
	os << name << "{" << newl;
	for (size_t i = 0; i < terms.size(); i++)
	{
		os << "\t";
		terms[i]->print(os, newl+"\t");
		os << ";" << newl;
	}
	os << "}" << newl;
}

ostream &operator<<(ostream &os, const record &r)
{
    os << r.name << "{" << endl;
    for (size_t i = 0; i < r.terms.size(); i++)
    	os << r.terms[i] << ";" << endl;
    os << "}" << endl;

    return os;
}
