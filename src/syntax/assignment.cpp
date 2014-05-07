/*
 * assignment.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: nbingham
 */

#include "assignment.h"
#include "composition.h"
#include "control.h"
#include "operator.h"
#include "../message.h"

assignment::assignment()
{
	this->_kind = "assignment";
	this->preface = NULL;
}

assignment::assignment(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind = "assignment";
	this->parent = parent;
	this->preface = NULL;

	parse(tokens, types, vars);
}

assignment::assignment(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "assignment";
	this->preface = NULL;

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		assignment *source = (assignment*)instr;
		this->parent = parent;
		for (size_t i = 0; i < source->expr.size(); i++)
		{
			this->expr.push_back(pair<variable_name*, expression*>(NULL, NULL));
			if (source->expr[i].first != NULL)
				this->expr.back().first = new variable_name(source->expr[i].first, tokens, vars, this, rename);

			if (source->expr[i].second != NULL)
				this->expr.back().second = new expression(source->expr[i].second, tokens, vars, this, rename);
		}
	}
}

assignment::assignment(tokenizer &tokens, type_space &types, variable_space &vars, int count, ...)
{
	this->_kind = "assignment";
	this->preface = NULL;
	this->parent = parent;

	va_list args;

	va_start(args, count);
	for (int i = 0; i < count; i++)
	{
		instruction *left = (instruction*)va_arg(args, instruction*);
		if (left == NULL || left->kind() != "variable_name")
			internal(tokens, "expected a variable name in manual instantiation", __FILE__, __LINE__);
		else
			left->parent = this;

		instruction *right = (instruction*)va_arg(args, instruction*);
		if (right == NULL || right->kind() != "expression")
			internal(tokens, "expected an expression in manual instantiation", __FILE__, __LINE__);
		else
			right->parent = this;

		this->expr.push_back(pair<variable_name*, expression*>((variable_name*)left, (expression*)right));
	}
	va_end(args);
}

assignment::~assignment()
{
	for (vector<pair<variable_name*, expression*> >::iterator i = expr.begin(); i != expr.end(); i++)
	{
		delete i->first;
		delete i->second;
	}
	expr.clear();

	if (preface != NULL)
		delete preface;
	preface = NULL;
}

bool assignment::is_next(tokenizer &tokens, size_t i)
{
	return variable_name::is_next(tokens, i) && tokens.peek(i) != "skip" && (tokens.peek(i+1) == "+" || tokens.peek(i+1) == "-" || tokens.peek(i+1) == ":=" || tokens.peek(i+1) == "," || slice::is_next(tokens, i+1));
}

void assignment::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;
	string token;
	do
	{
		tokens.increment();
		tokens.push_expected("[variable name]");
		tokens.push_bound(",");
		tokens.push_bound(":=");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		if (variable_name::is_next(tokens))
			expr.push_back(pair<variable_name*, expression*>(new variable_name(tokens, types, vars, this), NULL));

		tokens.increment();
		tokens.push_expected(",");
		tokens.push_expected(":=");
		if (token == "")
		{
			tokens.push_expected("+");
			tokens.push_expected("-");
		}
		token = tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();
	} while (token == ",");

	if (token == "+" && expr.size() > 0)
	{
		expr[0].second = new expression(tokens, types, vars, 1);
		expr[0].second->parent = this;

		if (expr[0].first != NULL && expr[0].first->var.is_valid() && vars.at(expr[0].first->var).type_string() != "node<1>")
			error(tokens, "operand must be of type 'node<1>', not '" + vars.at(expr[0].first->var).type_string() + "'", "", __FILE__, __LINE__);
	}
	else if (token == "-" && expr.size() > 0)
	{
		expr[0].second = new expression(tokens, types, vars, 0);
		expr[0].second->parent = this;

		if (expr[0].first != NULL && expr[0].first->var.is_valid() && vars.at(expr[0].first->var).type_string() != "node<1>")
			error(tokens, "operand must be of type 'node<1>', not '" + vars.at(expr[0].first->var).type_string() + "'", "", __FILE__, __LINE__);
	}
	else if (token == ":=" && expr.size() > 0)
	{
		size_t i = 0;
		do
		{
			tokens.increment();
			tokens.push_expected("[expression]");
			tokens.push_bound(",");
			tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();

			if (expression::is_next(tokens))
			{
				expr[i].second = new expression(tokens, types, vars, this);

				if (expr[i].first != NULL && expr[i].second != NULL && expr[i].second->type_string(vars) != expr[i].first->type_string(vars) && (expr[i].first->type_string(vars).find("node") == string::npos || expr[i].second->val == NULL))
					error(tokens, "type mismatch '" + expr[i].first->type_string(vars) + "' and '" + expr[i].second->type_string(vars) + "'", "", __FILE__, __LINE__);
			}

			i++;
			if (i < expr.size())
			{
				tokens.increment();
				tokens.push_expected(",");
				token = tokens.syntax(__FILE__, __LINE__);
				tokens.decrement();
			}
		} while (token == "," && i < expr.size());

		if (i < expr.size())
			error(tokens, "expected " + to_string(expr.size() - i) + " more expressions", "the number of expressions on the right must match the number of variables on the left", __FILE__, __LINE__);
	}

	preface = new composition();
	for (size_t i = 0; i < expr.size(); i++)
	{
		if (expr[i].second != NULL && expr[i].second->preface != NULL)
		{
			if (((composition*)preface)->terms.size() != 0)
				((composition*)preface)->operators.push_back("||");
			((composition*)preface)->terms.push_back(expr[i].second->preface);
			expr[i].second->preface->parent = preface;
			expr[i].second->preface = NULL;
		}
	}

	if (((composition*)preface)->terms.size() == 0)
	{
		delete preface;
		preface = NULL;
	}
	else if (((composition*)preface)->terms.size() == 1)
	{
		instruction *temp = ((composition*)preface)->terms[0];
		((composition*)preface)->terms.clear();
		delete preface;
		preface = temp;
	}

	vector<instruction*> postface;

	/* At this point, we have a collection of multi-bit flat
	 * assignments (var := var1 or var := 0 or var := 1) that
	 * we need to expand into a collection of single bit
	 * constant assignments (var := 0 or var := 1). In doing
	 * so, we will need to place conditionals to deal with the
	 * var := var case.
	 */
	for (size_t i = 0; i < expr.size(); )
	{
		if (expr[i].first != NULL && expr[i].first->var.is_valid() && expr[i].second != NULL)
		{
			int width = 0;
			keyword *type = types.find("node");
			if (expr[i].first->var.is_valid())
			{
				width = vars.at(expr[i].first->var).width;
				type = vars.at(expr[i].first->var).type;
			}

			if (expr[i].first->bits != NULL)
				width = expr[i].first->bits->end.value - expr[i].first->bits->start.value;

			// Bake in all node assignments
			if (type != NULL && type->name == "node")
			{
				if (width == 1 && expr[i].second->val != NULL)
					postface.push_back(new assignment(tokens, types, vars, 1, expr[i].first, expr[i].second));
				else if (width == 1 && (expr[i].second->var != NULL || expr[i].second->expr != "null"))
					postface.push_back(new control(tokens, types, vars, expr[i].first, expr[i].second));
				else if (width > 1 && expr[i].second->val != NULL)
				{
					for (size_t j = 0; j < vars.globals.size(); j++)
					{
						size_t idx_start = vars.globals[j].name.find_last_of("[")+1;
						size_t idx_end = vars.globals[j].name.find_last_of("]");

						int index = 0;
						if (idx_start != string::npos && idx_end != string::npos)
							index = atoi(vars.globals[j].name.substr(idx_start, idx_end - idx_start).c_str());
						if (vars.globals[j].name.substr(0, expr[i].first->name.value.size()) == expr[i].first->name.value && (expr[i].first->bits == NULL || (index >= expr[i].first->bits->start.value && index < expr[i].first->bits->end.value)))
						{
							variable_name *left = new variable_name(tokens, types, vars, variable_index(j, false), NULL);
							expression *right = new expression(tokens, types, vars, (expr[i].second->val->value >> index) & 0x01);
							postface.push_back(new assignment(tokens, types, vars, 1, left, right));
						}
					}

					delete expr[i].first;
					delete expr[i].second;
				}
				else if (width > 1 && expr[i].second->var != NULL)
				{
					for (size_t j = 0; j < vars.globals.size(); j++)
					{
						size_t idx_start = vars.globals[j].name.find_last_of("[")+1;
						size_t idx_end = vars.globals[j].name.find_last_of("]");

						int index = 0;
						if (idx_start != string::npos && idx_end != string::npos)
							index = atoi(vars.globals[j].name.substr(idx_start, idx_end - idx_start).c_str());

						if (vars.globals[j].name.substr(0, expr[i].first->name.value.size()) == expr[i].first->name.value && ((expr[i].second->var == NULL && (expr[i].second->val != NULL || index == 0)) || (expr[i].second->var != NULL && expr[i].second->var->in_bounds(vars, index))))
						{
							string set_name = expr[i].second->expr_string(vars) + vars.globals[j].name.substr(expr[i].first->name.value.size());
							variable_name *left = new variable_name(tokens, types, vars, variable_index(j, false), NULL);
							expression *right = new expression(tokens, types, vars, set_name);
							postface.push_back(new control(tokens, types, vars, left, right));
						}
					}

					delete expr[i].first;
					delete expr[i].second;
				}
			}
			else
			{
				string op = "operator:=(" + vars.at(expr[i].first->var).type_string() + "," + expr[i].second->type_string(vars) + ")";

				operate *optype = ((operate*)types.find(op));
				if (optype == NULL || optype->kind() != "operator")
					error(tokens, "undefined operator '" + op + "'", "did you mean " + types.closest(op), __FILE__, __LINE__);
				else if (optype->args.size() > 0)
				{
					vector<string> inputs;
					inputs.push_back(expr[i].first->name.value);
					inputs.push_back(expr[i].second->expr_string(vars));

					instruction *post = vars.insert(tokens, variable(vars.unique_name(optype), optype, 0, false, inputs, start_token));

					if (post != NULL)
						postface.push_back(post);
				}

				delete expr[i].first;
				delete expr[i].second;
			}
		}
		else if (expr[i].first != NULL)
			delete expr[i].first;
		else if (expr[i].second != NULL)
			delete expr[i].second;

		expr.erase(expr.begin() + i);
	}

	if (preface != NULL && postface.size() > 0)
	{
		composition *new_preface = new composition();
		new_preface->parent = this;
		new_preface->terms.push_back(preface);
		preface->parent = this;
		if (postface.size() > 1)
			new_preface->terms.push_back(new composition(tokens, types, vars, ",", postface));
		else
		{
			new_preface->terms.push_back(postface[0]);
			postface[0]->parent = new_preface;
		}
		new_preface->operators.push_back(";");
		preface = new_preface;
	}
	else if (postface.size() > 1)
		preface = new composition(tokens, types, vars, ",", postface);
	else if (postface.size() > 0)
		preface = postface[0];

	postface.clear();

	if (preface != NULL)
		preface->parent = this;

	end_token = tokens.index;
}

vector<dot_node_id> assignment::build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions)
{
	vector<dot_node_id> result;
	for (size_t i = 0; i < expr.size(); i++)
	{
		dot_node_id trans("T" + to_string(num_transitions++));
		string trans_string = expr[i].first->expr_string(vars);
		if ((expr[i].second->val != NULL && expr[i].second->val->value == 0) ||
			(expr[i].second->val == NULL && expr[i].second->expr == "0"))
			trans_string = "~" + trans_string;

		stmts.push_back(dot_stmt());
		stmts.back().stmt_type = "node";
		stmts.back().node_ids.push_back(trans);
		stmts.back().attr_list.attrs.push_back(dot_a_list());
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "plaintext"));
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", trans_string));
		for (size_t j = 0; j < last.size(); j++)
		{
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "edge";
			stmts.back().node_ids.push_back(last[j]);
			stmts.back().node_ids.push_back(trans);
		}

		result.push_back(trans);
	}

	return result;
}

void assignment::hide(variable_space &vars, vector<variable_index> uids)
{
	for (size_t i = 0; i < expr.size();)
	{
		if (expr[i].first != NULL && find(uids.begin(), uids.end(), expr[i].first->var) != uids.end())
			expr.erase(expr.begin() + i);
		else
		{
			if (expr[i].second != NULL)
				expr[i].second->hide(vars, uids);
			i++;
		}
	}
}

void assignment::print(ostream &os, string newl)
{
	for (size_t i = 0; i < expr.size(); i++)
	{
		if (i != 0)
			os << ",";

		if (expr[i].first != NULL)
			expr[i].first->print(os, newl);
		else
			os << "null";
	}

	if (expr.size() == 1 && expr[0].second != NULL && expr[0].second->val != NULL && expr[0].second->val->value == 0)
		os << "-";
	else if (expr.size() == 1 && expr[0].second != NULL && expr[0].second->val != NULL && expr[0].second->val->value == 1)
		os << "+";
	else
	{
		os << ":=";

		for (size_t i = 0; i < expr.size(); i++)
		{
			if (i != 0)
				os << ",";

			if (expr[i].second != NULL)
				expr[i].second->print(os, newl);
			else
				os << "null";
		}
	}
}

ostream &operator<<(ostream &os, const assignment &a)
{
	for (size_t i = 0; i < a.expr.size(); i++)
	{
		if (i != 0)
			os << ",";

		if (a.expr[i].first != NULL)
			os << (*a.expr[i].first);
		else
			os << "null";
	}

	if (a.expr.size() == 1 && a.expr[0].second != NULL && a.expr[0].second->val != NULL && a.expr[0].second->val->value == 0)
		os << "-";
	else if (a.expr.size() == 1 && a.expr[0].second != NULL && a.expr[0].second->val != NULL && a.expr[0].second->val->value == 1)
		os << "+";
	else
	{
		os << ":=";

		for (size_t i = 0; i < a.expr.size(); i++)
		{
			if (i != 0)
				os << ",";

			if (a.expr[i].second != NULL)
				os << (*a.expr[i].second);
			else
				os << "null";
		}
	}

	return os;
}
