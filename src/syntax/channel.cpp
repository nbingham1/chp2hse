/*
 * channel.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "channel.h"
#include "../message.h"

channel::channel()
{
	name = "";
	_kind = "channel";
	probe = NULL;
}

channel::channel(tokenizer &tokens, type_space &types)
{
	_kind = "channel";
	probe = NULL;

	parse(tokens, types);
}

channel::~channel()
{
}

bool channel::is_next(tokenizer &tokens, size_t i)
{
	return (tokens.peek(i) == "channel");
}

void channel::parse(tokenizer &tokens, type_space &types)
{
	start_token = tokens.index+1;

	tokens.increment();
	tokens.push_expected("channel");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

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
		if (token == "enforce" || token == "require")
		{
			tokens.increment();
			tokens.push_bound(";");
			terms.push_back(new debug(tokens, types, vars, NULL));
			tokens.decrement();

			tokens.increment();
			tokens.push_expected(";");
			tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();
		}
		else if (token == "operator")
		{
			operate *channel_operator = new operate();
			channel_operator->args.push_back(new declaration(tokens, this, 0, "this", "", vector<string>(), types, channel_operator->vars, NULL));
			channel_operator->parse(tokens, types);
			if (channel_operator->name.find_first_of("!") != string::npos)
				send.push_back(channel_operator);
			else if (channel_operator->name.find_first_of("?") != string::npos)
				recv.push_back(channel_operator);
			else if (channel_operator->name.find_first_of("#") != string::npos)
				probe = channel_operator;
			else
			{
				error(tokens, "expected channel operator definition 'operator!' 'operator?' or 'operator#'", "found '" + channel_operator->name + "'", __FILE__, __LINE__);
				delete channel_operator;
			}
		}
		else if (declaration::is_next(tokens))
		{
			tokens.increment();
			tokens.push_bound(";");
			terms.push_back(new declaration(tokens, types, vars, NULL));
			tokens.decrement();

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

void channel::print(ostream &os, string newl)
{
	os << "channel " << name << "{" << newl;
	for (size_t i = 0; i < terms.size(); i++)
	{
		os << "\t";
		terms[i]->print(os, newl+"\t");
		cout << ";" << newl;
	}

	for (size_t i = 0; i < send.size(); i++)
		if (send[i] != NULL)
		{
			os << "\t";
			send[i]->print(os, newl+"\t");
			os << newl;
		}

	for (size_t i = 0; i < recv.size(); i++)
		if (recv[i] != NULL)
		{
			os << "\t";
			recv[i]->print(os, newl+"\t");
			os << newl;
		}

	if (probe != NULL)
	{
		os << "\t";
		probe->print(os, newl+"\t");
		os << newl;
	}

	os << "}";
}

ostream &operator<<(ostream &os, const channel &c)
{
    os << "channel " << c.name << "{" << endl;
    for (size_t i = 0; i < c.terms.size(); i++)
    	cout << c.terms[i] << ";" << endl;

    for (size_t i = 0; i < c.send.size(); i++)
    	if (c.send[i] != NULL)
    		os << *c.send[i] << endl;

    for (size_t i = 0; i < c.recv.size(); i++)
       	if (c.recv[i] != NULL)
       		os << *c.recv[i] << endl;

    if (c.probe != NULL)
    	os << *c.probe << endl;

    os << "}" << endl;

    return os;
}



