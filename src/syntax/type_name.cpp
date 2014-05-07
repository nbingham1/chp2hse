/*
 * type_name.cpp
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "type_name.h"
#include "process.h"
#include "operator.h"
#include "channel.h"
#include "record.h"
#include "../message.h"

type_name::type_name()
{
	this->_kind = "type_name";
	this->name.parent = this;
	this->width.parent = this;
	this->preface = NULL;
	this->key = NULL;
}

type_name::type_name(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind		= "type_name";
	this->parent	= parent;
	this->name.parent = this;
	this->width.parent = this;
	this->preface = NULL;
	this->key = NULL;

	parse(tokens, types, vars);
}

type_name::type_name(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "type_name";
	this->name.parent = this;
	this->width.parent = this;
	this->preface = NULL;
	this->key = NULL;

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		type_name *source = (type_name*)instr;
		this->parent = parent;
		this->key = source->key;
		this->kind_guess = source->kind_guess;
		this->name = instance(&source->name, tokens, vars, this, map<string, string>());
		this->width = constant(&source->width, tokens, vars, this, map<string, string>());
		for (size_t i = 0; i < inputs.size(); i++)
			this->inputs.push_back(new expression(source->inputs.back(), tokens, vars, this, rename));
	}
}

type_name::~type_name()
{
}

string type_name::type_string(variable_space &vars)
{
	string result = name.value;
	if ((key != NULL && key->kind() == "keyword") || (key == NULL && width.value != 0))
		result += "<" + to_string(width.value) + ">";
	else if ((key != NULL && (key->kind() == "process" || key->kind() == "operator")) || (key == NULL && inputs.size() > 0))
	{
		result += "(";
		for (size_t i = 0; i < inputs.size(); i++)
		{
			if (inputs[i] != NULL)
				result += inputs[i]->type_string(vars);
			else
				result += "null";
		}
		result += ")";
	}
	return result;
}

bool type_name::is_next(tokenizer &tokens, size_t i)
{
	return instance::is_next(tokens, i);
}

void type_name::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;

	tokens.increment();
	tokens.push_expected("[instance]");
	tokens.push_bound("<");
	tokens.push_bound("(");
	tokens.push_bound("?");
	tokens.push_bound("!");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	if (instance::is_next(tokens))
		name.parse(tokens, types, vars);

	bool has_error = false;
	string token = tokens.peek_next();
	if (token == "<")
	{
		kind_guess = "keyword";
		if (name.value != "" && name.value != "node")
			error(tokens, "only nodes may have a specified bit width", "", __FILE__, __LINE__);

		tokens.increment();
		tokens.push_expected("<");
		tokens.push_bound(">");
		tokens.push_bound("[constant]");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		tokens.increment();
		tokens.push_expected("[constant]");
		tokens.push_bound(">");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		if (constant::is_next(tokens))
			width.parse(tokens, types, vars);

		tokens.increment();
		tokens.push_expected(">");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();
	}
	else if (token == "(")
	{
		kind_guess = "process";
		tokens.next();
		name.value += "(";
		string token = tokens.peek_next();
		while (token != ")")
		{
			tokens.increment();
			tokens.push_bound(",");
			tokens.push_bound(")");
			tokens.push_expected("[expression]");
			if (expression::is_next(tokens))
			{
				inputs.push_back(new expression(tokens, types, vars, this));
				expression *e = (expression*)inputs.back();
				if (name.value[name.value.size()-1] != '(')
					name.value.push_back(',');

				name.value += e->type_string(vars);

				if (e->type_string(vars) == "null")
					has_error = true;
			}
			tokens.decrement();

			tokens.increment();
			tokens.push_expected(",");
			tokens.push_expected(")");
			token = tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();
		}
		name.value += ")";
	}
	else if (token == "?" || token == "!")
	{
		kind_guess = "operator";
		variable_index chan = vars.find(name.value);
		if (!chan.is_valid())
		{
			error(tokens, "undefined variable '" + name.value + "'", "did you mean " + vars.closest_name(name.value), __FILE__, __LINE__);
			has_error = true;
		}

		token = tokens.next();

		inputs.push_back(new expression());
		inputs.front()->parent = this;
		inputs.front()->var = new variable_name(tokens, types, vars, chan, NULL);
		inputs.front()->var->parent = inputs.front();

		if (expression::is_next(tokens))
		{
			inputs.push_back(new expression(tokens, types, vars, this, tokens.operations.size()));

			if (token == "?" && inputs.back()->val != NULL)
				error(tokens, "cannot write to a constant", "", __FILE__, __LINE__);
		}

		if (chan.is_valid() && vars.at(chan).type != NULL)
		{
			name.value = "operator" + token + "(" + inputs.front()->type_string(vars);
			if (inputs.size() > 1 && token == "!")
				name.value += "," + inputs.back()->type_string(vars);
			name.value += ")";

			if (inputs.back()->type_string(vars) == "null")
				has_error = true;
		}
		else
		{
			has_error = true;
			name.value = "operator" + token + "(null";
			if (inputs.size() > 1 && token == "!")
				name.value += "," + inputs.back()->type_string(vars);
			name.value += ")";

			if (inputs.back()->type_string(vars) == "null")
				has_error = true;
		}
	}
	else if (instance::is_next(tokens))
		kind_guess = "record";

	key = types.find(name.value);
	if (!has_error && key == NULL)
		error(tokens, "unrecognized type name '" + name.value + "'", "did you mean " + types.closest(name.value) + "?", __FILE__, __LINE__);
	else if (!has_error && ((key->kind() == "operator" || key->kind() == "process") && ((process*)key)->args.size() != inputs.size()))
		error(tokens, "process expected " + to_string(((process*)key)->args.size()) + " arguments, but found " + to_string(inputs.size()) + ".", "", __FILE__, __LINE__);

	if (inputs.size() > 0)
	{
		preface = new composition();
		for (size_t i = 0; i < inputs.size(); i++)
		{
			if (inputs[i] != NULL && inputs[i]->preface != NULL)
			{
				if (((composition*)preface)->terms.size() != 0)
					((composition*)preface)->operators.push_back("||");
				((composition*)preface)->terms.push_back(inputs[i]->preface);
				inputs[i]->preface->parent = preface;
				inputs[i]->preface = NULL;
			}
		}

		if (((composition*)preface)->terms.size() == 1)
		{
			instruction *temp = ((composition*)preface)->terms.front();
			((composition*)preface)->terms.clear();
			delete preface;
			preface = temp;
		}
		else if (((composition*)preface)->terms.size() == 0)
		{
			delete preface;
			preface = NULL;
		}
	}

	end_token = tokens.index;
}

void type_name::print(ostream &os, string newl)
{
	os << name;

	if (width.value != 0)
	{
		os << "<";
		width.print(os, newl);
		os << ">";
	}

	if (inputs.size() > 0)
	{
		os << "(";

		for (size_t i = 0; i < inputs.size(); i++)
		{
			if (i != 0)
				os << ",";
			inputs[i]->print(os, newl);
		}

		os << ")";
	}
}

ostream &operator<<(ostream &os, const type_name &t)
{
	os << t.name;

	if (t.width.value != 0)
		os << "<" << t.width << ">";

	if (t.inputs.size() > 0)
	{
		os << "(";

		for (size_t i = 0; i < t.inputs.size(); i++)
		{
			if (i != 0)
				os << ",";
			os << *t.inputs[i];
		}

		os << ")";
	}

	return os;
}
