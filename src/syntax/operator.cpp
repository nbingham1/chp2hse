/*
 * process.cpp
 *
 *  Created on: Oct 29, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "operator.h"
#include "expression.h"
#include "../message.h"

operate::operate()
{
	_kind = "operator";
}

operate::operate(tokenizer &tokens, type_space &types)
{
	_kind = "operator";

	parse(tokens, types);
}

operate::~operate()
{
}

bool operate::is_next(tokenizer &tokens, size_t i)
{
	return (tokens.peek(i) == "operator");
}

void operate::parse(tokenizer &tokens, type_space &types)
{
	start_token = tokens.index+1;

	tokens.increment();
	tokens.push_expected("operator");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	tokens.increment();
	tokens.push_bound("{");

	tokens.increment();
	tokens.push_expected(":=");
	tokens.push_expected("|");
	tokens.push_expected("&");
	tokens.push_expected("^");
	tokens.push_expected("==");
	tokens.push_expected("~=");
	tokens.push_expected("<");
	tokens.push_expected(">");
	tokens.push_expected("<=");
	tokens.push_expected(">=");
	tokens.push_expected("<<");
	tokens.push_expected(">>");
	tokens.push_expected("+");
	tokens.push_expected("-");
	tokens.push_expected("*");
	tokens.push_expected("/");
	tokens.push_expected("%");
	tokens.push_expected("~");
	tokens.push_expected("#");
	tokens.push_expected("?");
	tokens.push_expected("!");
	tokens.push_bound("(");
	tokens.push_bound("[declaration]");
	tokens.push_bound(")");
	string op_string = tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	name = "operator" + op_string;

	tokens.increment();
	tokens.push_expected("(");
	tokens.push_bound("[declaration]");
	tokens.push_bound(")");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	name += "(";

	for (size_t i = 0; i < args.size(); i++)
	{
		if (name[name.size()-1] != '(')
			name += ",";

		name += args[i]->type.name.value;
		if (args[i]->type.name.value == "node")
			name += "<" + to_string(args[i]->type.width.value) + ">";
	}

	string last = "";
	tokens.increment();
	tokens.push_expected(")");
	tokens.push_expected("[declaration]");
	string token = tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();
	while (token != ")")
	{
		tokens.increment();
		tokens.push_bound(",");
		tokens.push_bound(")");
		tokens.push_expected("[declaration]");
		tokens.syntax(__FILE__, __LINE__);
		if (declaration::is_next(tokens))
		{
			args.push_back(new declaration(tokens, types, vars, NULL));

			if (last != "")
			{
				if (name[name.size()-1] != '(')
					name += ",";
				name += last;
			}

			last = args.back()->type.name.value;
			if (args.back()->type.name.value == "node")
				last += "<" + to_string(args.back()->type.width.value) + ">";
		}

		tokens.decrement();

		tokens.increment();
		tokens.push_expected(",");
		tokens.push_expected(")");
		tokens.push_bound("[declaration]");
		token = tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();
	}

	if ((op_string == "|" || op_string == "&" || op_string == "^" ||
		op_string == "==" || op_string == "~=" || op_string == "<" ||
		op_string == ">" || op_string == "<=" || op_string == ">=" ||
		op_string == "<<" || op_string == ">>" || op_string == "*" ||
		op_string == "/" || op_string == "%") && args.size() != 3)
		error(tokens, "expected 3 arguments, found " + to_string(args.size()), "binary operators have two operands and one output", __FILE__, __LINE__);
	else if ((op_string == "+" || op_string == "-") && args.size() != 2 && args.size() != 3)
		error(tokens, "expected 2 or 3 arguments, found " + to_string(args.size()), "+ and - operators can be either unary or binary, meaning one or two operands and one output", __FILE__, __LINE__);
	else if (op_string == "~" && args.size() != 2)
		error(tokens, "expected 2 arguments, found " + to_string(args.size()), "unary operators have one operand and one output", __FILE__, __LINE__);
	else if (op_string == "#" && args.size() != 2)
		error(tokens, "expected 1 argument, found " + to_string(args.size()-1), "for channel operators, the left operand is implicit and is named 'this', therefore probes only have one argument, an output", __FILE__, __LINE__);
	else if ((op_string == "?" || op_string == "!") && args.size() != 1 && args.size() != 2)
		error(tokens, "expected 0 or 1 arguments, found " + to_string(args.size()-1), "for channel operators, the left operand is implicit and is named 'this', therefore send and receive only have an argument if data is passed", __FILE__, __LINE__);
	else if (op_string == ":=" && args.size() != 2)
		error(tokens, "expected 2 arguments, found " + to_string(args.size()), "", __FILE__, __LINE__);

	if ((last != "" && name.find("operator!") != name.npos) || name.find("operator:=") != name.npos)
	{
		if (name[name.size()-1] != '(')
			name += ",";
		name += last;
	}

	name += ")";
	tokens.decrement();

	tokens.increment();
	tokens.push_expected("{");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	tokens.increment();
	tokens.push_bound("}");
	chp = new composition(tokens, types, vars, NULL);
	tokens.decrement();

	tokens.increment();
	tokens.push_expected("}");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	end_token = tokens.index;
}

void operate::print(ostream &os, string newl)
{
	os << name << "(";

	for (size_t i = 0; i < args.size(); i++)
	{
		if (i != 0)
			os << ",";
		args[i]->print(os, newl);
	}

	os << ")" << newl;
	os << "{" << newl;
	if (chp != NULL)
	{
		os << "\t";
		chp->print(os, newl+"\t");
		os << newl;
	}
	os << newl << "}" << newl;
}

ostream &operator<<(ostream &os, const operate &o)
{
	os << o.name << "(";

	for (size_t i = 0; i < o.args.size(); i++)
	{
		if (i != 0)
			os << ",";
		os << *(o.args[i]);
	}

	os << ")" << endl;
	os << "{" << endl;
	if (o.chp != NULL)
		os << o.chp << endl;
	os << "}" << endl;

	return os;
}
