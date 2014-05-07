/*
 * variable.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "variable.h"
#include "variable_space.h"

variable::variable()
{
	name = "";
	type = NULL;
	width = 0;
	arg = false;
	definition_location = -1;
}

variable::variable(string name, keyword *type, uint16_t width, bool arg, vector<string> inputs, int definition_location)
{
	this->name = name;
	this->type = type;
	this->width = width;
	this->arg = arg;
	this->inputs = inputs;
	this->definition_location = definition_location;
}

variable::~variable()
{
}

string variable::type_string() const
{
	string result;
	if (type != NULL)
		result += type->name;
	else
		result += "null";

	if ((type != NULL && type->kind() == "keyword" && width != 0) || (type == NULL && width != 0))
		result += "<" + to_string(width) + ">";

	return result;
}

variable &variable::operator=(variable v)
{
	name = v.name;
	type = v.type;
	width = v.width;
	arg = v.arg;
	inputs = v.inputs;
	definition_location = v.definition_location;
	return *this;
}

ostream &operator<<(ostream &os, const variable &v)
{
	os << v.type_string() << " ";

	os << v.name;
	if ((v.type != NULL && (v.type->kind() == "process" || v.type->kind() == "operator")) || (v.type == NULL && v.inputs.size() > 0))
	{
		os << "(";
		for (size_t i = 0; i < v.inputs.size(); i++)
		{
			if (i != 0)
				os << ",";
			os << v.inputs[i];
		}
		os << ")";
	}

	os << " " << (v.arg ? "argument " : "");
    return os;
}

