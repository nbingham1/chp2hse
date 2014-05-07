/*
 * constant.cpp
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "constant.h"
#include "slice.h"
#include "../message.h"

constant::constant()
{
	this->_kind = "constant";
	this->value = 0;
	this->width = 0;
}

constant::constant(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind		= "constant";
	this->parent	= parent;
	this->value = 0;
	this->width = 0;

	parse(tokens, types, vars);
}

constant::constant(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "constant";
	this->parent = NULL;
	this->value = 0;
	this->width = 0;

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		constant *source = (constant*)instr;
		this->parent = parent;
		this->value = source->value;
		this->width = source->width;
	}
}

constant::constant(tokenizer &tokens, type_space &types, variable_space &vars, int value)
{
	this->_kind = "constant";
	this->parent = NULL;
	this->value = value;
	this->width = 0;
	while (value != 0)
	{
		this->width++;
		value = ((value >> 1) & 0x7FFFFFFF);
	}
}

constant::~constant()
{
}

void constant::restrict_bits(tokenizer &tokens, type_space &types, variable_space &vars)
{
	slice s(tokens, types, vars, this);
	value = (value >> s.start.value);
	value = value & ~(0xFFFFFFFF << (s.end.value - s.start.value));
	width = s.end.value - s.start.value;
}

bool constant::is_next(tokenizer &tokens, size_t i)
{
	string token = tokens.peek(i);
	bool hex = (token.size() > 1 && token[0] == '0' && token[1] == 'x');
	bool bin = (token.size() > 1 && token[0] == '0' && token[1] == 'b');
	bool dec = ((token.size() == 1 && token[0] >= '0' && token[0] <= '9') ||
				(token.size() >= 2 && token[0] >= '0' && token[0] <= '9' && token[1] >= '0' && token[1] <= '9'));

	if (!hex && !bin && !dec)
		return false;

	for (size_t j = 2; j < token.size(); j++)
		if ((dec && (token[j] < '0' || token[j] > '9')) ||
			(hex && (token[j] < '0' || (token[j] > '9' && token[j] < 'A') || (token[j] > 'Z' && token[j] < 'a') || token[j] > 'z')) ||
			(bin && (token[j] != '0' && token[j] != '1')))
			return false;

	return true;
}

void constant::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;

	string token = tokens.next();
	bool hex = (token.size() > 1 && token[0] == '0' && token[1] == 'x');
	bool bin = (token.size() > 1 && token[0] == '0' && token[1] == 'b');
	bool dec = ((token.size() == 1 && token[0] >= '0' && token[0] <= '9') ||
				(token.size() >= 2 && token[0] >= '0' && token[0] <= '9' && token[1] >= '0' && token[1] <= '9'));

	bool has_error = false;
	if (!hex && !bin && !dec)
		has_error = true;

	for (size_t j = 2; j < token.size(); j++)
		if ((dec && (token[j] < '0' || token[j] > '9')) ||
			(hex && (token[j] < '0' || (token[j] > '9' && token[j] < 'A') || (token[j] > 'Z' && token[j] < 'a') || token[j] > 'z')) ||
			(bin && (token[j] != '0' && token[j] != '1')))
			has_error = true;

	if (has_error)
		error(tokens, "malformed constant value", "constants may be specified in hexadecimal, decimal, or binary format", __FILE__, __LINE__);
	else if (hex)
		value = hex_to_int(token.substr(2));
	else if (bin)
		value = bin_to_int(token.substr(2));
	else if (dec)
		value = dec_to_int(token);

	int temp = value;

	while (temp != 0)
	{
		width++;
		temp = ((temp >> 1) & 0x7FFFFFFF);
	}

	while (slice::is_next(tokens))
		restrict_bits(tokens, types, vars);

	end_token = tokens.index;
}

void constant::print(ostream &os, string newl)
{
	os << value;
}

ostream &operator<<(ostream &os, const constant &c)
{
	os << c.value;
	return os;
}
