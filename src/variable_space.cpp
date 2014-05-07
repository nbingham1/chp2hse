/*
 * variable_space.cpp
 *
 *  Created on: Feb 9, 2013
 *      Author: nbingham
 */

#include "variable_space.h"
#include "syntax/record.h"
#include "syntax/channel.h"
#include "syntax/process.h"
#include "syntax/operator.h"
#include "instruction.h"
#include "syntax/expression.h"
#include "syntax/assignment.h"
#include "syntax/composition.h"
#include "syntax/constant.h"
#include "syntax/control.h"
#include "syntax/skip.h"
#include "syntax/slice.h"
#include "syntax/variable_name.h"
#include "message.h"

variable_index::variable_index()
{
	this->uid = 0xFFFFFFFF;
	this->label = false;
}

variable_index::variable_index(size_t uid, bool label)
{
	this->uid = uid;
	this->label = label;
}

variable_index::~variable_index()
{

}

bool variable_index::is_valid()
{
	return (this->uid != 0xFFFFFFFF);
}

variable_index &variable_index::operator=(variable_index i)
{
	this->uid = i.uid;
	this->label = i.label;
	return *this;
}

bool operator==(variable_index v1, variable_index v2)
{
	return (v1.label == v2.label && v1.uid == v2.uid);
}

variable_space::variable_space()
{
}

variable_space::~variable_space()
{
}

instruction *variable_space::insert(tokenizer &tokens, variable v)
{
	variable_index check = find(v.name);
	if (check.is_valid())
	{
		int temp = tokens.index;
		tokens.index = v.definition_location;
		error(tokens, "conflicting definition of '" + v.name + "'", "", __FILE__, __LINE__);
		tokens.index = at(check).definition_location;
		error(tokens, "previously defined here", "", __FILE__, __LINE__);
		tokens.index = temp;
	}
	else if (v.type != NULL)
	{
		if (v.type->kind() == "keyword")
		{
			if (v.width == 1)
				globals.push_back(v);
			else
			{
				labels.push_back(v);
				for (size_t i = 0; i < v.width; i++)
				{
					variable v1 = v;
					v1.width = 1;
					v1.name += "[" + to_string(i) + "]";
					globals.push_back(v1);
				}
			}

			return NULL;
		}
		else
		{
			labels.push_back(v);
			variable_space *vars = NULL;
			if (v.type->kind() == "record")
				vars = &(((record*)v.type)->vars);
			else if (v.type->kind() == "channel")
				vars = &(((channel*)v.type)->vars);
			else if (v.type->kind() == "process")
				vars = &(((process*)v.type)->vars);
			else if (v.type->kind() == "operator")
				vars = &(((operate*)v.type)->vars);
			else
			{
				error(tokens, "trying to instantiate variable with type of kind '" + v.type->kind() + "'", "", __FILE__, __LINE__);
				return NULL;
			}

			map<string, string> rename;
			for (size_t i = 0; i < vars->globals.size(); i++)
			{
				if (vars->globals[i].arg == (v.type->kind() == "record" || v.type->kind() == "channel"))
				{
					variable v1 = vars->globals[i];
					size_t idx;
					if ((idx = v1.name.find("this.")) != v1.name.npos)
						v1.name = v1.name.substr(0, idx) + v1.name.substr(idx+5);

					rename.insert(pair<string, string>(v1.name, v.name + "." + v1.name));
					v1.name = v.name + "." + v1.name;

					v1.arg = v.arg;
					globals.push_back(v1);
				}
			}

			for (size_t i = 0; i < vars->labels.size(); i++)
			{
				if (vars->labels[i].arg == (v.type->kind() == "record" || v.type->kind() == "channel"))
				{
					variable v1 = vars->labels[i];
					size_t idx;
					if ((idx = v1.name.find(".this")) != v1.name.npos)
						v1.name = v1.name.substr(0, idx) + v1.name.substr(idx+5);

					rename.insert(pair<string, string>(v1.name, v.name + "." + v1.name));
					v1.name = v.name + "." + v1.name;

					v1.arg = v.arg;
					labels.push_back(v1);
				}
			}

			if (v.type->kind() == "process" || v.type->kind() == "operator")
			{
				process *p = (process*)v.type;
				for (size_t i = 0; i < v.inputs.size() && i < p->args.size(); i++)
					rename.insert(pair<string, string>(p->args[i]->name.value, v.inputs[i]));

				if (p->chp != NULL)
				{
					if (p->chp->kind() == "assignment")
						return new assignment(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "composition")
						return new composition(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "constant")
						return new constant(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "control")
						return new control(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "debug")
						return new debug(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "declaration")
						return new declaration(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "expression")
						return new expression(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "instance")
						return new instance(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "skip")
						return new skip(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "slice")
						return new slice(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "type_name")
						return new type_name(p->chp, tokens, *this, NULL, rename);
					else if (p->chp->kind() == "variable_name")
						return new variable_name(p->chp, tokens, *this, NULL, rename);
					else
						internal(tokens, "unrecognized kind '" + p->chp->kind() + "'", __FILE__, __LINE__);
				}
			}

			if (vars != NULL && vars->reset != "")
			{
				if (reset != "")
					reset += "&";

				reset += instance(vars->reset, NULL, rename).value;
			}
		}
	}
	else
		error(tokens, "attempting to create variable without type", "", __FILE__, __LINE__);

	return NULL;
}

size_t variable_space::find_global(string name)
{
	for (size_t i = 0; i < globals.size(); i++)
		if (globals[i].name == name || globals[i].name == "this." + name)
			return i;

	return globals.size();
}

size_t variable_space::find_label(string name)
{
	for (size_t i = 0; i < labels.size(); i++)
		if (labels[i].name == name || labels[i].name == "this." + name)
			return i;

	return labels.size();
}

variable_index variable_space::find(string name)
{
	for (size_t i = 0; i < globals.size(); i++)
		if (globals[i].name == name || globals[i].name == "this." + name)
			return variable_index(i, false);

	for (size_t i = 0; i < labels.size(); i++)
		if (labels[i].name == name || labels[i].name == "this." + name)
			return variable_index(i, true);

	return variable_index();
}

bool variable_space::contains(string name)
{
	return (find_global(name) != globals.size() || find_label(name) != labels.size());
}

string variable_space::closest_name(string name)
{
	int dist = 999999999;
	string result = "";
	size_t count = 0;

	for (size_t i = 0; i < globals.size(); i++)
	{
		int temp = 0;
		size_t j = 0;
		for (j = 0; j < name.size() && j < globals[i].name.size(); j++)
			if (name[j] != globals[i].name[j])
				temp += name.size() - j;

		for (; j < globals[i].name.size(); j++)
			temp += j - name.size();

		for (; j < name.size(); j++)
			temp += name.size() - j;

		if (temp < dist)
		{
			result = "'" + globals[i].name + "'";
			dist = temp;
			count = 1;
		}
		else if (temp == dist)
		{
			if (count == 1)
				result = "or " + result;
			result = "'" + globals[i].name + "' ";
			count++;
		}
	}

	for (size_t i = 0; i < labels.size(); i++)
	{
		int temp = 0;
		size_t j = 0;
		for (j = 0; j < name.size() && j < labels[i].name.size(); j++)
			if (name[j] != labels[i].name[j])
				temp += name.size() - j;

		for (; j < labels[i].name.size(); j++)
			temp += j - name.size();

		for (; j < name.size(); j++)
			temp += name.size() - j;

		if (temp < dist)
		{
			result = "'" + labels[i].name + "'";
			dist = temp;
			count = 1;
		}
		else if (temp == dist)
		{
			if (count == 1)
				result = "or " + result;
			result = "'" + labels[i].name + "' ";
			count++;
		}
	}

	return result;
}

string variable_space::unique_name(keyword *type)
{
	string type_name;
	if (type == NULL)
		type_name = "null";
	else if (type->kind() == "process" || type->kind() == "operator")
		type_name = "_fn";
	else
		type_name = "_var";

	size_t i = 0;
	while (contains(type_name + to_string(i)))
		i++;

	return type_name + to_string(i);
}

variable_space &variable_space::operator=(variable_space s)
{
	this->globals = s.globals;
	this->labels = s.labels;
	this->reset = s.reset;
	this->assumptions = s.assumptions;
	this->assertions = s.assertions;
	return *this;
}

variable &variable_space::at(variable_index i)
{
	if (i.label)
		return labels[i.uid];
	else
		return globals[i.uid];
}

string variable_space::variable_list()
{
	string result;
	for (size_t i = 0; i < globals.size(); i++)
	{
		if (i != 0)
			result += ",";
		result += globals[i].name;
	}
	return result;
}

ostream &operator<<(ostream &os, const variable_space &s)
{
	for (size_t i = 0; i < s.globals.size(); i++)
		os << i << " " << s.globals[i] << endl;

	for (size_t i = 0; i < s.labels.size(); i++)
		os << i << " " << s.labels[i] << endl;

	return os;
}
