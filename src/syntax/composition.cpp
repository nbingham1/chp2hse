/*
 * composition.cpp
 *
 *  Created on: Nov 1, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "composition.h"
#include "skip.h"
#include "control.h"
#include "debug.h"
#include "assignment.h"
#include "declaration.h"
#include "../message.h"

composition::composition()
{
	this->_kind = "composition";
}

composition::composition(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent, size_t i)
{
	this->_kind = "composition";
	this->parent = parent;

	parse(tokens, types, vars, i);
}

composition::composition(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "composition";

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		composition *source = (composition*)instr;
		this->parent = parent;
		this->operators = source->operators;
		for (size_t i = 0; i < source->terms.size(); i++)
		{
			if (source->terms[i] != NULL)
			{
				if (source->terms[i]->kind() == "composition")
					this->terms.push_back(new composition(source->terms[i], tokens, vars, this, rename));
				else if (source->terms[i]->kind() == "skip")
					this->terms.push_back(new skip(source->terms[i], tokens, vars, this, rename));
				else if (source->terms[i]->kind() == "control")
					this->terms.push_back(new control(source->terms[i], tokens, vars, this, rename));
				else if (source->terms[i]->kind() == "debug")
					this->terms.push_back(new debug(source->terms[i], tokens, vars, this, rename));
				else if (source->terms[i]->kind() == "declaration")
					this->terms.push_back(new declaration(source->terms[i], tokens, vars, this, rename));
				else if (source->terms[i]->kind() == "assignment")
					this->terms.push_back(new assignment(source->terms[i], tokens, vars, this, rename));
			}
			else
				this->terms.push_back(NULL);
		}
	}
}

composition::composition(tokenizer &tokens, type_space &types, variable_space &vars, string op, int count, ...)
{
	this->_kind = "composition";
	this->parent = NULL;

	va_list args;
	va_start(args, count);
	for (int i = 0; i < count; i++)
	{
		if (i != 0)
			operators.push_back(op);
		terms.push_back((instruction*)va_arg(args, instruction*));
	}
	va_end(args);
}

composition::composition(tokenizer &tokens, type_space &types, variable_space &vars, string op, vector<instruction*> terms)
{
	this->_kind = "composition";
	this->parent = NULL;
	this->terms = terms;

	for (size_t i = 0; i < terms.size()-1; i++)
		operators.push_back(op);
}

composition::~composition()
{
	for (vector<instruction*>::iterator term = terms.begin(); term != terms.end(); term++)
		delete (*term);
	terms.clear();
}

void composition::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	parse(tokens, types, vars, 0);
}

void composition::parse(tokenizer &tokens, type_space &types, variable_space &vars, size_t i)
{
	start_token = tokens.index+1;

	bool done = false;
	while (!done)
	{
		tokens.increment();
		tokens.push_bound(tokens.compositions[i]);
		if (i < tokens.compositions.size()-1)
			terms.push_back(new composition(tokens, types, vars, this, i+1));
		else
		{
			tokens.increment();
			tokens.push_expected("(");
			tokens.push_expected("[skip]");
			tokens.push_expected("[conditional]");
			tokens.push_expected("[debug]");
			tokens.push_expected("[declaration]");
			tokens.push_expected("[assignment]");
			string token = tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();

			if (token == "(")
			{
				tokens.increment();
				tokens.push_bound(")");
				terms.push_back(new composition(tokens, types, vars, this));
				tokens.decrement();

				tokens.increment();
				tokens.push_expected(")");
				tokens.syntax(__FILE__, __LINE__);
				tokens.decrement();
			}
			else if (skip::is_next(tokens))
				terms.push_back(new skip(tokens, types, vars, this));
			else if (control::is_next(tokens))
			{
				control *temp = new control(tokens, types, vars, this);
				if (temp->preface == NULL)
					terms.push_back(temp);
				else
				{
					terms.push_back(new composition());
					terms.back()->parent = this;
					((composition*)terms.back())->operators.push_back(";");
					((composition*)terms.back())->terms.push_back(temp->preface);
					temp->preface->parent = terms.back();
					temp->preface = NULL;
					((composition*)terms.back())->terms.push_back(temp);
				}

			}
			else if (debug::is_next(tokens))
				terms.push_back(new debug(tokens, types, vars, this));
			else if (declaration::is_next(tokens))
			{
				declaration temp(tokens, types, vars, this);
				if (temp.preface != NULL)
				{
					temp.preface->parent = this;
					terms.push_back(temp.preface);
					temp.preface = NULL;
				}
				else
				{
					terms.push_back(new skip());
					terms.back()->parent = this;
				}
			}
			else if (assignment::is_next(tokens))
			{
				assignment temp(tokens, types, vars, this);
				if (temp.preface != NULL)
				{
					temp.preface->parent = this;
					terms.push_back(temp.preface);
					temp.preface = NULL;
				}
				else
				{
					terms.push_back(new skip());
					terms.back()->parent = this;
				}
			}
		}
		tokens.decrement();

		if (find(tokens.compositions[i].begin(), tokens.compositions[i].end(), tokens.peek_next()) != tokens.compositions[i].end())
			operators.push_back(tokens.next());
		else if (tokens.in_bound(1) != tokens.bound.end())
			done = true;
		else
		{
			tokens.increment();
			tokens.push_expected(tokens.compositions[i]);
			string token = tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();

			if (find(tokens.compositions[i].begin(), tokens.compositions[i].end(), token) != tokens.compositions[i].end())
				operators.push_back(token);
			else
				done = true;
		}
	}

	end_token = tokens.index;
}

vector<dot_node_id> composition::build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions)
{
	vector<dot_node_id> result;
	vector<dot_node_id> prev = last;
	vector<dot_node_id> next;
	if (operators.size() > 0 && (operators.front() == "||" || operators.front() == ","))
	{
		dot_node_id trans("T" + to_string(num_transitions++));
		stmts.push_back(dot_stmt());
		stmts.back().stmt_type = "node";
		stmts.back().node_ids.push_back(trans);
		stmts.back().attr_list.attrs.push_back(dot_a_list());
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "plaintext"));
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", "1"));
		for (size_t j = 0; j < prev.size(); j++)
		{
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "edge";
			stmts.back().node_ids.push_back(prev[j]);
			stmts.back().node_ids.push_back(trans);
		}

		last = vector<dot_node_id>(1, trans);
	}

	for (size_t i = 0; (i == 0 || i-1 < operators.size()) && i < terms.size(); i++)
	{
		if (i != 0 && operators[i-1] == ";")
		{
			dot_node_id semi("S" + to_string(num_places++));
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "node";
			stmts.back().node_ids.push_back(semi);
			stmts.back().attr_list.attrs.push_back(dot_a_list());
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "circle"));
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("width", "0.25"));
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", ""));
			for (size_t j = 0; j < next.size(); j++)
			{
				stmts.push_back(dot_stmt());
				stmts.back().stmt_type = "edge";
				stmts.back().node_ids.push_back(next[j]);
				stmts.back().node_ids.push_back(semi);
			}

			prev = vector<dot_node_id>(1, semi);
		}
		else if (i != 0 && (operators[i-1] == "||" || operators[i-1] == ","))
		{
			dot_node_id semi("S" + to_string(num_places++));
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "node";
			stmts.back().node_ids.push_back(semi);
			stmts.back().attr_list.attrs.push_back(dot_a_list());
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "circle"));
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("width", "0.25"));
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", ""));
			for (size_t j = 0; j < next.size(); j++)
			{
				stmts.push_back(dot_stmt());
				stmts.back().stmt_type = "edge";
				stmts.back().node_ids.push_back(next[j]);
				stmts.back().node_ids.push_back(semi);
			}
			result.push_back(semi);
		}

		if (operators.size() > 0 && (operators.front() == "||" || operators.front() == ","))
		{
			dot_node_id semi("S" + to_string(num_places++));
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "node";
			stmts.back().node_ids.push_back(semi);
			stmts.back().attr_list.attrs.push_back(dot_a_list());
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "circle"));
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("width", "0.25"));
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", ""));
			for (size_t j = 0; j < last.size(); j++)
			{
				stmts.push_back(dot_stmt());
				stmts.back().stmt_type = "edge";
				stmts.back().node_ids.push_back(last[j]);
				stmts.back().node_ids.push_back(semi);
			}
			prev = vector<dot_node_id>(1, semi);
		}

		next = terms[i]->build_hse(vars, stmts, prev, num_places, num_transitions);
	}

	if (operators.size() == 0 || operators.back() == ";")
		result.insert(result.end(), next.begin(), next.end());
	else
	{
		dot_node_id semi("S" + to_string(num_places++));
		stmts.push_back(dot_stmt());
		stmts.back().stmt_type = "node";
		stmts.back().node_ids.push_back(semi);
		stmts.back().attr_list.attrs.push_back(dot_a_list());
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "circle"));
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("width", "0.25"));
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", ""));
		for (size_t j = 0; j < next.size(); j++)
		{
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "edge";
			stmts.back().node_ids.push_back(next[j]);
			stmts.back().node_ids.push_back(semi);
		}
		result.push_back(semi);

		dot_node_id trans("T" + to_string(num_transitions++));
		stmts.push_back(dot_stmt());
		stmts.back().stmt_type = "node";
		stmts.back().node_ids.push_back(trans);
		stmts.back().attr_list.attrs.push_back(dot_a_list());
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "plaintext"));
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", "1"));
		for (size_t j = 0; j < result.size(); j++)
		{
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "edge";
			stmts.back().node_ids.push_back(result[j]);
			stmts.back().node_ids.push_back(trans);
		}
		result = vector<dot_node_id>(1, trans);
	}

	return result;
}

void composition::hide(variable_space &vars, vector<variable_index> uids)
{
	for (size_t i = 0; i < terms.size();)
	{
		terms[i]->hide(vars, uids);
		if ((terms[i]->kind() == "assignment" && ((assignment*)terms[i])->expr.size() == 0) ||
			(terms[i]->kind() == "composition" && ((composition*)terms[i])->terms.size() == 0))
			terms.erase(terms.begin() + i);
		else
			i++;
	}
}

void composition::print(ostream &os, string newl)
{
	if (operators.size() > 0 && operators.back() == "||")
		os << "(" << newl;

	size_t i = 0;
	for (i = 0; (i == 0 || i-1 < operators.size()) && i < terms.size(); i++)
	{
		if (operators.size() > 0 && operators.back() == "||")
		{
			os << "\t";
			terms[i]->print(os, (terms.size() > 1 ? newl+"\t" : newl));
		}
		else
			terms[i]->print(os, newl);

		if (i < operators.size())
		{
			os << operators[i];
			if (operators.size() > 0 && operators.back() == "||")
				os << newl;
		}
	}

	if (operators.size() > 0 && operators.back() == "||")
		os << newl << ")";
}

ostream &operator<<(ostream &os, const composition &a)
{
	if (a.operators.size() > 0 && a.operators.back() == "||")
		os << "(";

	size_t i = 0;
	for (i = 0; i < a.operators.size() && i < a.terms.size(); i++)
		cout << a.terms[i] << a.operators[i];

	if (i < a.terms.size())
		cout << a.terms[i];

	if (a.operators.size() > 0 && a.operators.back() == "||")
		os << ")";

	return os;
}
