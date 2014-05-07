/*
 * instance.cpp
 *
 *  Created on: Aug 21, 2013
 *      Author: nbingham
 */

#include "instance.h"
#include "../message.h"

instance::instance()
{
	this->_kind = "instance";
}

instance::instance(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind		= "instance";
	this->parent	= parent;

	parse(tokens, types, vars);
}

instance::instance(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "instance";

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		instance *source = (instance*)instr;
		string id = "";
		for (size_t i = 0; i < source->value.size(); i++)
		{
			if (!nc(source->value[i]))
			{
				if (id != "")
				{
					for (map<string, string>::iterator match = rename.begin(); match != rename.end(); match++)
					{
						if (id.find(match->first) == 0)
						{
							id = match->second + id.substr(match->first.size());
							break;
						}
					}

					this->value += id;
				}

				id = "";
				this->value += source->value[i];
			}
			else
				id += source->value[i];
		}
		if (id != "")
		{
			for (map<string, string>::iterator match = rename.begin(); match != rename.end(); match++)
			{
				if (id.find(match->first) == 0)
				{
					id = match->second + id.substr(match->first.size());
					break;
				}
			}

			this->value += id;
		}
	}
}

instance::instance(string instr, instruction *parent, map<string, string> rename)
{
	this->_kind = "instance";
	this->parent = parent;

	string id = "";
	for (size_t i = 0; i < instr.size(); i++)
	{
		if (!nc(instr[i]))
		{
			if (id != "")
			{
				for (map<string, string>::iterator match = rename.begin(); match != rename.end(); match++)
				{
					if (id.find(match->first) == 0)
					{
						id = match->second + id.substr(match->first.size());
						break;
					}
				}

				this->value += id;
			}

			id = "";
			this->value += instr[i];
		}
		else
			id += instr[i];
	}
	if (id != "")
	{
		for (map<string, string>::iterator match = rename.begin(); match != rename.end(); match++)
		{
			if (id.find(match->first) == 0)
			{
				id = match->second + id.substr(match->first.size());
				break;
			}
		}

		this->value += id;
	}
}

instance::instance(string value)
{
	this->_kind = "instance";
	this->value = value;
	this->parent = NULL;
}

instance::~instance()
{
}

bool instance::is_next(tokenizer &tokens, size_t i)
{
	string token = tokens.peek(i);
	if (token.size() == 0 || token[0] < 'A' || (token[0] > 'Z' && token[0] != '_' && token[0] < 'a') || token[0] > 'z')
		return false;

	for (size_t j = 1; j < token.size(); j++)
		if (!nc(token[j]))
			return false;

	return true;
}

void instance::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;

	string token = tokens.next();

	bool has_error = false;
	if (token.size() == 0 || token[0] < 'A' || (token[0] > 'Z' && token[0] != '_' && token[0] < 'a') || token[0] > 'z')
		has_error = true;

	for (size_t j = 1; j < token.size(); j++)
		if (!nc(token[j]))
			has_error = true;

	if (!has_error)
		value = token;
	else
		error(tokens, "malformed name", "names must start with either a letter or an underscore and may only contain letters, numbers, or underscores", __FILE__, __LINE__);

	end_token = tokens.index;
}

void instance::print(ostream &os, string newl)
{
	os << value;
}

ostream &operator<<(ostream &os, const instance &i)
{
	os << i.value;

	return os;
}
