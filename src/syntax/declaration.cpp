/*
 * declaration.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham
 *
 */

#include "declaration.h"
#include "assignment.h"
#include "composition.h"
#include "control.h"
#include "debug.h"
#include "skip.h"
#include "slice.h"
#include "variable_name.h"
#include "operator.h"
#include "../message.h"

declaration::declaration()
{
	this->_kind = "declaration";
	this->type.parent = this;
	this->name.parent = this;
	this->reset = NULL;
	this->preface = NULL;
}

declaration::declaration(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind = "declaration";
	this->parent = parent;
	this->type.parent = this;
	this->name.parent = this;
	this->reset = NULL;
	this->preface = NULL;

	parse(tokens, types, vars);
}

declaration::declaration(tokenizer &tokens, keyword *type, int width, string name, string reset, vector<string> inputs, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind = "declaration";
	this->parent = parent;
	this->type.parent = this;
	this->name.parent = this;
	this->type.width.parent = &this->type;
	this->type.name.parent = &this->type;
	this->type.kind_guess = type->name;
	this->type.key = type;
	this->type.width.value = width;
	this->type.name.value = type->name;

	for (size_t i = 0; i < inputs.size(); i++)
	{
		tokenizer temp(inputs[i]);
		this->type.inputs.push_back(new expression(temp, types, vars, this));
	}

	this->name.value = name;
	this->reset = NULL;

	if (reset != "")
	{
		tokenizer temp(reset);
		this->reset = new expression(temp, types, vars, this);
	}
	this->preface = NULL;

	inputs.clear();
	for (size_t i = 0; i < this->type.inputs.size(); i++)
	{
		if (this->type.inputs[i] != NULL)
			inputs.push_back(this->type.inputs[i]->expr_string(vars));
		else
			inputs.push_back("null");
	}

	if (this->type.key != NULL && this->name.value != "")
		preface = vars.insert(tokens, variable(this->name.value, this->type.key, this->type.width.value, this->parent == NULL, inputs, start_token));
}

declaration::declaration(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "declaration";
	this->type.parent = this;
	this->name.parent = this;
	this->reset = NULL;
	this->preface = NULL;

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		declaration *source = (declaration*)instr;
		this->parent = parent;
		this->name = instance(&source->name, tokens, vars, this, rename);
		this->type = type_name(&source->type, tokens, vars, this, rename);
		this->reset = new expression(source->reset, tokens, vars, this, rename);

		if (source->preface != NULL)
		{
			if (source->preface->kind() == "assignment")
				this->preface = new assignment(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "composition")
				this->preface = new composition(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "constant")
				this->preface = new constant(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "control")
				this->preface = new control(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "debug")
				this->preface = new debug(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "declaration")
				this->preface = new declaration(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "expression")
				this->preface = new expression(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "instance")
				this->preface = new instance(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "skip")
				this->preface = new skip(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "slice")
				this->preface = new slice(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "type_name")
				this->preface = new type_name(source->preface, tokens, vars, this, rename);
			else if (source->preface->kind() == "variable_name")
				this->preface = new variable_name(source->preface, tokens, vars, this, rename);
			else
				internal(tokens, "unrecognized kind '" + source->preface->kind() + "'", __FILE__, __LINE__);
		}
	}
}

declaration::~declaration()
{
	if (reset != NULL)
		delete reset;
	reset = NULL;

	if (preface != NULL)
		delete preface;
	preface = NULL;
}

bool declaration::is_next(tokenizer &tokens, size_t i)
{
	return type_name::is_next(tokens, i) && (tokens.peek(i+1) == "(" || tokens.peek(i+1) == "?" || tokens.peek(i+1) == "!" || instance::is_next(tokens, i+1) || (tokens.peek(i) == "node" && tokens.peek(i+1) == "<"));
}

void declaration::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;

	// Read in the type name
	tokens.increment();
	tokens.push_expected("[type name]");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	if (type_name::is_next(tokens))
	{
		type.parse(tokens, types, vars);
		preface = type.preface;
	}

	
	if ((type.key != NULL && type.key->kind() != "process" && type.key->kind() != "operator") || (type.key == NULL && type.kind_guess != "process" && type.kind_guess != "operator"))
	{
		// This isn't a process instantiation so the next thing we read is the name of the variable being declared.
		tokens.increment();
		tokens.push_expected("[instance]");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		if (instance::is_next(tokens))
			name.parse(tokens, types, vars);

		string token = tokens.peek_next();
		if (token == ":=")
		{
			// This declaration also has a reset expression so read that next
			tokens.next();
			if (parent == NULL && type.key != NULL && (type.key->kind() != "keyword" || type.key->name != "node"))
				error(tokens, "only nodes can take be globally reset", "", __FILE__, __LINE__);

			tokens.increment();
			tokens.push_expected("[expression]");
			tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();

			if (expression::is_next(tokens))
			{
				reset = new expression(tokens, types, vars, this);
				if (parent == NULL && reset->val == NULL && reset->type_string(vars) != "null")
					error(tokens, "global reset expressions must be constant", "", __FILE__, __LINE__);
				else if (reset != NULL && reset->type_string(vars) != "null" && reset->type_string(vars) != type.type_string(vars) && (type.type_string(vars).find("node") == string::npos || reset->val == NULL))
					error(tokens, "type mismatch '" + type.type_string(vars) + "' and '" + reset->type_string(vars) + "'", "", __FILE__, __LINE__);
			}
		}
	}
	else if (type.key != NULL)
	{
		// This is a process instantiation so we need to generate a name for it
		// Ultimately this name is hidden and is only used to separate the new processes variables from the current set of declared variables
		if (parent == NULL)
			error(tokens, "cannot declare a process or operator as an argument", "", __FILE__, __LINE__);

		name.value = vars.unique_name(type.key);
	}

	vector<string> inputs;
	for (size_t i = 0; i < this->type.inputs.size(); i++)
	{
		if (this->type.inputs[i] != NULL)
			inputs.push_back(this->type.inputs[i]->expr_string(vars));
		else
			inputs.push_back("null");
	}

	if (type.key != NULL && name.value != "")
	{
		instruction *prepreface = vars.insert(tokens, variable(name.value, type.key, type.width.value, parent == NULL, inputs, start_token));

		if (prepreface != NULL && preface != NULL)
		{
			composition *new_preface = new composition();
			new_preface->terms.push_back(preface);
			new_preface->terms.push_back(prepreface);
			preface->parent = new_preface;
			prepreface->parent = new_preface;
			new_preface->operators.push_back(";");
			preface = new_preface;
		}
		else if (prepreface != NULL)
			preface = prepreface;
	}

	if (parent == NULL && reset != NULL && reset->val != NULL && reset->type_string(vars) != "null")
	{
		if (type.width.value == 1)
		{
			if (vars.reset != "")
				vars.reset += "&";

			if (reset->val->value == 0)
				vars.reset += "~";

			vars.reset += name.value;
		}
		/*else if (type.width.value > 1)
		{
			for (size_t j = 0; j < vars.globals.size(); j++)
			{
				size_t idx_start = vars.globals[j].name.find_last_of("[")+1;
				size_t idx_end = vars.globals[j].name.find_last_of("]");

				int index = 0;
				if (idx_start != string::npos && idx_end != string::npos)
					index = atoi(vars.globals[j].name.substr(idx_start, idx_end - idx_start).c_str());
				if (vars.globals[j].name.substr(0, name.value.size()) == name.value)
				{
					if (vars.reset != "")
						vars.reset += "&";

					if (((reset->val->value >> index) & 0x01) == 0)
						vars.reset += "~";

					vars.reset += vars.globals[j].name;
				}
			}
		}*/

		delete reset;
		reset = NULL;
	}
	else if (parent != NULL && reset != NULL)
	{
		if (reset->preface != NULL && preface != NULL)
		{
			composition *new_preface = new composition();
			new_preface->terms.push_back(preface);
			new_preface->terms.push_back(reset->preface);
			preface->parent = new_preface;
			reset->preface->parent = new_preface;
			new_preface->operators.push_back(";");
			preface = new_preface;
		}
		else if (reset->preface != NULL)
			preface = reset->preface;

		reset->preface = NULL;

		vector<instruction*> postface;

		// Bake in all node assignments
		if (type.key != NULL && type.key->name == "node")
		{
			if (type.width.value == 1 && reset->val != NULL)
				postface.push_back(new assignment(tokens, types, vars, 1, new variable_name(tokens, types, vars, vars.find(name.value), NULL), reset));
			else if (type.width.value == 1 && reset->var != NULL)
				postface.push_back(new control(tokens, types, vars, new variable_name(tokens, types, vars, vars.find(name.value), NULL), reset));
			else if (type.width.value > 1 && reset->val != NULL)
			{
				for (size_t j = 0; j < vars.globals.size(); j++)
				{
					size_t idx_start = vars.globals[j].name.find_last_of("[")+1;
					size_t idx_end = vars.globals[j].name.find_last_of("]");

					int index = 0;
					if (idx_start != string::npos && idx_end != string::npos)
						index = atoi(vars.globals[j].name.substr(idx_start, idx_end - idx_start).c_str());
					if (vars.globals[j].name.substr(0, name.value.size()) == name.value)
					{
						variable_name *left = new variable_name(tokens, types, vars, variable_index(j, false), NULL);
						expression *right = new expression(tokens, types, vars, (reset->val->value >> index) & 0x01);
						postface.push_back(new assignment(tokens, types, vars, 1, left, right));
					}
				}

				delete reset;
				reset = NULL;
			}
			else if (type.width.value > 1 && reset->var != NULL)
			{
				for (size_t j = 0; j < vars.globals.size(); j++)
				{
					size_t idx_start = vars.globals[j].name.find_last_of("[")+1;
					size_t idx_end = vars.globals[j].name.find_last_of("]");

					int index = 0;
					if (idx_start != string::npos && idx_end != string::npos)
						index = atoi(vars.globals[j].name.substr(idx_start, idx_end - idx_start).c_str());

					if (vars.globals[j].name.substr(0, name.value.size()) == name.value && ((reset->var == NULL && (reset->val != NULL || index == 0)) || (reset->var != NULL && reset->var->in_bounds(vars, index))))
					{
						string set_name = reset->expr_string(vars) + vars.globals[j].name.substr(name.value.size());
						cout << set_name << " " << index << endl;
						variable_name *left = new variable_name(tokens, types, vars, variable_index(j, false), NULL);
						expression *right = new expression(tokens, types, vars, set_name);
						cout << right->expr_string(vars) << endl;
						postface.push_back(new control(tokens, types, vars, left, right));
					}
				}

				delete reset;
				reset = NULL;
			}
		}
		else if (reset->type_string(vars) != "null")
		{
			string op = "operator:=(" + vars.at(vars.find(name.value)).type_string() + "," + reset->type_string(vars) + ")";

			operate *optype = ((operate*)types.find(op));
			if (optype == NULL || optype->kind() != "operator")
				error(tokens, "undefined operator '" + op + "'", "did you mean " + types.closest(op), __FILE__, __LINE__);
			else if (optype->args.size() > 0)
			{
				vector<string> inputs;
				inputs.push_back(name.value);
				inputs.push_back(reset->expr_string(vars));

				instruction *post = vars.insert(tokens, variable(vars.unique_name(optype), optype, 0, false, inputs, start_token));

				if (post != NULL)
					postface.push_back(post);
			}

			delete reset;
			reset = NULL;
		}

		if (postface.size() > 1 && preface != NULL)
			preface = new composition(tokens, types, vars, ";", 2, preface, new composition(tokens, types, vars, ",", postface));
		else if (postface.size() == 1 && preface != NULL)
			preface = new composition(tokens, types, vars, ";", 2, preface, postface[0]);
		else if (postface.size() > 1)
			preface = new composition(tokens, types, vars, ",", postface);
		else if (postface.size() == 1)
			preface = postface[0];

		postface.clear();

		reset = NULL;
	}

	if (preface != NULL)
		preface->parent = this;

	end_token = tokens.index;
}

void declaration::print(ostream &os, string newl)
{
	if (type.inputs.size() > 0)
	{
		os << name;
		os << "(";

		for (size_t i = 0; i < type.inputs.size(); i++)
		{
			if (i != 0)
				os << ",";
			type.inputs[i]->print(os, newl);
		}

		os << ")";
	}
	else if (reset != NULL)
	{
		os << name << ":=";
		reset->print(os, newl);
	}
	else
		os << "skip";
}

ostream &operator<<(ostream &os, const declaration &d)
{
	if (d.type.inputs.size() > 0)
	{
		os << d.name;
		os << "(";

		for (size_t i = 0; i < d.type.inputs.size(); i++)
		{
			if (i != 0)
				os << ",";
			os << *d.type.inputs[i];
		}

		os << ")";
	}
	else if (d.reset != NULL)
		os << d.name << ":=" << *d.reset;
	else
		os << "skip";

	return os;
}
