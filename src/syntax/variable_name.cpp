/*
 * variable_name.cpp
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "variable_name.h"
#include "../message.h"

variable_name::variable_name()
{
	this->_kind = "variable_name";
	this->name.parent = this;
	this->bits = NULL;
	this->var = variable_index();
}

variable_name::variable_name(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind		= "variable_name";
	this->parent	= parent;
	this->name.parent = this;
	this->bits = NULL;
	this->var = variable_index();

	parse(tokens, types, vars);
}

variable_name::variable_name(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "variable_name";
	this->name.parent = this;
	this->bits = NULL;
	this->var = variable_index();

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		variable_name *source = (variable_name*)instr;
		this->parent = parent;
		this->name = instance(&source->name, tokens, vars, this, rename);
		if (source->bits != NULL)
			this->bits = new slice(source->bits, tokens, vars, this, rename);

		this->var = vars.find(this->name.value);
		if (!this->var.is_valid())
			error(tokens, "undefined variable '" + this->name.value + "'", "did you mean " + vars.closest_name(this->name.value), __FILE__, __LINE__);
	}
}

variable_name::variable_name(tokenizer &tokens, type_space &types, variable_space &vars, variable_index var, slice *bits)
{
	this->_kind = "variable_name";
	this->parent = NULL;
	this->name.parent = this;
	this->bits = bits;
	this->var = var;
	if (this->var.is_valid())
		this->name.value = vars.at(this->var).name;
}

variable_name::~variable_name()
{
}

bool variable_name::is_node(variable_space &vars)
{
	if (var.is_valid() && vars.at(var).type != NULL)
		return (vars.at(var).type->name == "node");
	else
		return true;
}

bool variable_name::in_bounds(variable_space &vars, int i)
{
	if (bits != NULL)
	{
		if (i + bits->start.value >= bits->end.value)
			return false;
	}
	else if (i >= vars.at(var).width)
			return false;

	return true;
}

int variable_name::width(variable_space &vars)
{
	if (bits != NULL)
		return bits->end.value - bits->start.value;
	else if (var.is_valid())
		return vars.at(var).width;
	else
		return 0;
}

string variable_name::type_string(variable_space &vars)
{
	if (var.is_valid())
	{
		string result;
		if (vars.at(var).type != NULL)
			result = vars.at(var).type->name;
		else
			result += "null";

		if (result == "node")
		{
			if (bits != NULL)
				result += "<" + to_string(bits->end.value - bits->start.value) + ">";
			else
				result += "<" + to_string(vars.at(var).width) + ">";
		}

		return result;
	}
	else
		return "null";
}

string variable_name::expr_string(variable_space &vars)
{
	if (var.is_valid())
	{
		string result = vars.at(var).name;
		if (bits != NULL)
		{
			result += "[" + to_string(bits->start.value);
			if (bits->end.value != bits->start.value)
				result += ".." + to_string(bits->end.value);
			result += "]";
		}
		return result;
	}
	else
		return "null";
}

void variable_name::restrict_bits(tokenizer &tokens, type_space &types, variable_space &vars)
{
	slice *s = new slice(tokens, types, vars, this);

	if (bits != NULL)
	{
		s->start.value += bits->start.value;
		s->end.value += bits->start.value;

		if (s->start.value >= bits->end.value || s->end.value > bits->end.value)
			error(tokens, "index out of bounds", "", __FILE__, __LINE__);

		bits->start.value = s->start.value;
		bits->end.value = s->end.value;

		delete s;
		s = NULL;
	}
	else
	{
		if (s->start.value >= vars.at(var).width || s->end.value > vars.at(var).width)
			error(tokens, "index out of bounds", "", __FILE__, __LINE__);

		bits = s;
		s = NULL;
	}

	if (var.is_valid() && bits->start.value == 0 && bits->end.value == vars.at(var).width)
	{
		delete bits;
		bits = NULL;
	}
	else if (bits->end.value - bits->start.value == 1)
	{
		name.value += "[" + to_string(bits->start.value) + "]";
		delete bits;
		bits = NULL;

		var = vars.find(name.value);
		if (var.is_valid())
			name.value = vars.at(var).name;
		else
			error(tokens, "undefined variable '" + name.value + "'", "did you mean " + vars.closest_name(name.value), __FILE__, __LINE__);
	}
}

bool variable_name::is_next(tokenizer &tokens, size_t i)
{
	return instance::is_next(tokens, i);
}

void variable_name::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;

	tokens.increment();
	tokens.push_bound("[slice]");
	tokens.push_expected("[instance]");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	if (instance::is_next(tokens))
	{
		name.parse(tokens, types, vars);
		var = vars.find(name.value);
		if (var.is_valid())
			name.value = vars.at(var).name;
		else
			error(tokens, "undefined variable '" + name.value + "'", "did you mean " + vars.closest_name(name.value), __FILE__, __LINE__);
	}

	while (slice::is_next(tokens))
		restrict_bits(tokens, types, vars);

	end_token = tokens.index;
}

void variable_name::print(ostream &os, string newl)
{
	name.print(os, newl);
	if (bits != NULL)
		bits->print(os, newl);
}

ostream &operator<<(ostream &os, const variable_name &v)
{
	os << v.name;
	if (v.bits != NULL)
		os << v.bits;

	return os;
}
