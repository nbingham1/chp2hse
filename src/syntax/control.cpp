/*
 * control.cpp
 *
 *  Created on: Jul 16, 2013
 *      Author: nbingham
 */

#include "control.h"
#include "assignment.h"
#include "variable_name.h"
#include "skip.h"
#include "../message.h"

control::control()
{
	this->_kind = "control";
	this->loop = false;
	this->mutex = false;
	this->preface = NULL;
}

control::control(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent)
{
	this->_kind = "control";
	this->parent = parent;
	this->loop = false;
	this->mutex = false;
	this->preface = NULL;

	parse(tokens, types, vars);
}

control::control(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "control";
	this->loop = false;
	this->mutex = false;
	this->preface = NULL;

	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		control *source = (control*)instr;
		this->parent = parent;
		this->loop = source->loop;
		this->mutex = source->mutex;
		for (size_t i = 0; i < source->terms.size(); i++)
		{
			this->terms.push_back(pair<expression*, composition*>(NULL, NULL));
			if (source->terms[i].first != NULL)
				this->terms.back().first = new expression(source->terms[i].first, tokens, vars, this, rename);

			if (source->terms[i].second != NULL)
				this->terms.back().second = new composition(source->terms[i].second, tokens, vars, this, rename);
		}
	}
}

control::control(tokenizer &tokens, type_space &types, variable_space &vars, variable_name *left, expression *right)
{
	this->_kind = "control";
	this->parent = NULL;
	this->preface = NULL;

	if (right->var != NULL)
		internal(tokens, "attempting to create multibit guard", __FILE__, __LINE__);

	expression *left0 = right;
	expression *left1 = new expression(right, tokens, vars, this, map<string, string>());
	if (left1->val != NULL)
		left1->val->value = ~left1->val->value;
	else
		left1->expr = "~(" + left1->expr + ")";
	variable_name *right0aleft = left;
	variable_name *right1aleft = new variable_name(left, tokens, vars, NULL, map<string, string>());

	expression *right0aright = new expression(tokens, types, vars, 1);
	expression *right1aright = new expression(tokens, types, vars, 0);
	assignment *right0a = new assignment(tokens, types, vars, 1, right0aleft, right0aright);
	assignment *right1a = new assignment(tokens, types, vars, 1, right1aleft, right1aright);
	composition *right0 = new composition(tokens, types, vars, ";", 1, right0a);
	composition *right1 = new composition(tokens, types, vars, ";", 1, right1a);
	this->terms.push_back(pair<expression*, composition*>(left0, right0));
	this->terms.push_back(pair<expression*, composition*>(left1, right1));
	left0->parent = this;
	left1->parent = this;
	right0->parent = this;
	right1->parent = this;
	this->mutex = true;
	this->loop = false;
}

control::~control()
{
	for (vector<pair<expression*, composition*> >::iterator term = terms.begin(); term != terms.end(); term++)
	{
		if (term->first != NULL)
			delete term->first;
		if (term->second != NULL)
			delete term->second;
	}
	terms.clear();

	if (preface != NULL)
		delete preface;
	preface = NULL;
}

bool control::is_shortcut(tokenizer &tokens)
{
	size_t i = 1;
	size_t depth = 0;
	string token = tokens.peek(i);
	while ((depth != 0 || token != "]") && token != "")
	{
		if (token == "[" || token == "*[")
			depth++;
		else if (token == "]")
			depth--;
		else if (token == "->" && depth == 0)
			return false;

		i++;
		token = tokens.peek(i);
	}

	return true;
}

bool control::is_next(tokenizer &tokens, size_t i)
{
	string token = tokens.peek(i);
	return (token == "*[" || token == "[");
}

void control::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	start_token = tokens.index+1;

	tokens.increment();
	tokens.push_expected("[");
	tokens.push_expected("*[");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	if (tokens.curr == "*[")
		loop = true;

	bool shortcut = is_shortcut(tokens);

	tokens.increment();
	tokens.push_bound("->");
	tokens.push_bound(":");
	tokens.push_bound("[]");
	tokens.push_bound("]");
	if (!shortcut || !loop)
		terms.push_back(pair<expression*, composition*>(new expression(tokens, types, vars, this), NULL));
	else if (shortcut && loop)
		terms.push_back(pair<expression*, composition*>(NULL, new composition(tokens, types, vars, this)));
	tokens.decrement();

	if (!shortcut)
	{
		tokens.increment();
		tokens.push_expected("->");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		tokens.increment();
		tokens.push_bound(":");
		tokens.push_bound("[]");
		tokens.push_bound("]");
		terms.back().second = new composition(tokens, types, vars, this, 0);
		tokens.decrement();

		tokens.increment();
		tokens.push_expected(":");
		tokens.push_expected("[]");
		tokens.push_expected("]");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		if (tokens.curr == ":")
		{
			mutex = false;
			bool done = false;
			while (!done)
			{
				tokens.increment();
				tokens.push_bound("->");
				tokens.push_bound(":");
				tokens.push_bound("[]");
				tokens.push_bound("]");
				terms.push_back(pair<expression*, composition*>(new expression(tokens, types, vars, this), NULL));
				tokens.decrement();

				tokens.increment();
				tokens.push_expected("->");
				tokens.syntax(__FILE__, __LINE__);
				tokens.decrement();

				tokens.increment();
				tokens.push_bound("->");
				tokens.push_bound(":");
				tokens.push_bound("[]");
				tokens.push_bound("]");
				terms.back().second = new composition(tokens, types, vars, this);
				tokens.decrement();

				tokens.increment();
				tokens.push_expected("|");
				tokens.push_expected("]");
				tokens.syntax(__FILE__, __LINE__);
				tokens.decrement();

				if (tokens.curr == "]")
					done = true;
			}
		}
		else if (tokens.curr == "[]")
		{
			mutex = true;
			bool done = false;
			while (!done)
			{
				tokens.increment();
				tokens.push_bound("->");
				tokens.push_bound(":");
				tokens.push_bound("[]");
				tokens.push_bound("]");
				terms.push_back(pair<expression*, composition*>(new expression(tokens, types, vars, this), NULL));
				tokens.decrement();

				tokens.increment();
				tokens.push_expected("->");
				tokens.syntax(__FILE__, __LINE__);
				tokens.decrement();

				tokens.increment();
				tokens.push_bound("->");
				tokens.push_bound(":");
				tokens.push_bound("[]");
				tokens.push_bound("]");
				terms.back().second = new composition(tokens, types, vars, this);
				tokens.decrement();

				tokens.increment();
				tokens.push_expected("[]");
				tokens.push_expected("]");
				tokens.syntax(__FILE__, __LINE__);
				tokens.decrement();

				if (tokens.curr == "]")
					done = true;
			}
		}
	}
	else
	{
		tokens.increment();
		tokens.push_expected("]");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();
	}

	preface = new composition();
	for (size_t i = 0; i < terms.size(); i++)
	{
		if (terms[i].first != NULL && terms[i].first->preface != NULL)
		{
			if (((composition*)preface)->terms.size() != 0)
				((composition*)preface)->operators.push_back("||");
			((composition*)preface)->terms.push_back(terms[i].first->preface);
			terms[i].first->preface->parent = preface;
			terms[i].first->preface = NULL;
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

	if (preface != NULL)
		preface->parent = this;

	end_token = tokens.index;
}

vector<dot_node_id> control::build_hse(variable_space &vars, vector<dot_stmt> &stmts, vector<dot_node_id> last, int &num_places, int &num_transitions)
{
	string else_guard = "";
	vector<dot_node_id> result;
	for (size_t i = 0; i < terms.size(); i++)
	{
		if (else_guard != "0" && i != 0)
			else_guard += "&";

		if (else_guard != "0" && terms[i].first != NULL)
			else_guard += "~(" + terms[i].first->expr_string(vars) + ")";
		else
			else_guard = "0";

		dot_node_id trans("T" + to_string(num_transitions++));
		stmts.push_back(dot_stmt());
		stmts.back().stmt_type = "node";
		stmts.back().node_ids.push_back(trans);
		stmts.back().attr_list.attrs.push_back(dot_a_list());
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "plaintext"));
		if (terms[i].first != NULL)
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", "[ " + terms[i].first->expr_string(vars) + " ]"));
		else
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", "[ 1 ]"));
		for (size_t j = 0; j < last.size(); j++)
		{
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "edge";
			stmts.back().node_ids.push_back(last[j]);
			stmts.back().node_ids.push_back(trans);
		}

		vector<dot_node_id> next;
		if (terms[i].second != NULL)
		{
			dot_node_id semi("S" + to_string(num_places++));
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "node";
			stmts.back().node_ids.push_back(semi);
			stmts.back().attr_list.attrs.push_back(dot_a_list());
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "circle"));
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("width", "0.25"));
			stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", ""));
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "edge";
			stmts.back().node_ids.push_back(trans);
			stmts.back().node_ids.push_back(semi);
			next = terms[i].second->build_hse(vars, stmts, vector<dot_node_id>(1, semi), num_places, num_transitions);
		}
		else
			next.push_back(trans);

		result.insert(result.end(), next.begin(), next.end());
	}

	if (loop)
	{
		for (size_t j = 0; j < result.size(); j++)
		{
			for (size_t k = 0; k < last.size(); k++)
			{
				stmts.push_back(dot_stmt());
				stmts.back().stmt_type = "edge";
				stmts.back().node_ids.push_back(result[j]);
				stmts.back().node_ids.push_back(last[k]);
			}
		}

		dot_node_id trans("T" + to_string(num_transitions++));
		stmts.push_back(dot_stmt());
		stmts.back().stmt_type = "node";
		stmts.back().node_ids.push_back(trans);
		stmts.back().attr_list.attrs.push_back(dot_a_list());
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("shape", "plaintext"));
		stmts.back().attr_list.attrs.back().as.push_back(pair<string, string>("label", "[ " + else_guard + " ]"));
		for (size_t j = 0; j < last.size(); j++)
		{
			stmts.push_back(dot_stmt());
			stmts.back().stmt_type = "edge";
			stmts.back().node_ids.push_back(last[j]);
			stmts.back().node_ids.push_back(trans);
		}

		result = vector<dot_node_id>(1, trans);
	}

	return result;
}

void control::hide(variable_space &vars, vector<variable_index> uids)
{
	for (size_t i = 0; i < terms.size(); i++)
	{
		if (terms[i].first != NULL)
			terms[i].first->hide(vars, uids);
		if (terms[i].second != NULL)
		{
			terms[i].second->hide(vars, uids);
			if (terms[i].second->terms.size() == 0)
			{
				terms[i].second->terms.push_back(new skip());
				terms[i].second->terms.back()->parent = terms[i].second;
			}
		}
	}
}

void control::print(ostream &os, string newl)
{
	if (terms.size() > 1)
		os << newl;

	if (loop)
		os << "*";

	os << "[";
	if (terms.size() > 1)
		os << "  ";
	for (size_t i = 0; i < terms.size(); i++)
	{
		if (i != 0 && mutex)
			os << "[] ";
		else if (i != 0 && !mutex)
			os << ":  ";

		if (terms[i].first != NULL)
			terms[i].first->print(os, newl+"\t");
		else
			os << newl << "\t";

		if (terms[i].first != NULL && terms[i].second != NULL)
			os << "->";

		if (terms[i].second != NULL)
			terms[i].second->print(os, newl+"\t");

		if (terms.size() > 1)
			os << newl;
	}
	if (terms.size() > 0 && terms.front().first == NULL)
		os << newl;

	os << "]";
}

ostream &operator<<(ostream &os, const control &c)
{
	if (c.loop)
		os << "*";

	os << "[";
	for (size_t i = 0; i < c.terms.size(); i++)
	{
		if (i != 0 && c.mutex)
			os << "[]";
		else if (i != 0 && !c.mutex)
			os << ":";

		if (c.terms[i].first != NULL)
			os << *c.terms[i].first;

		if (c.terms[i].first != NULL && c.terms[i].second != NULL)
			os << "->";

		if (c.terms[i].second != NULL)
			os << *c.terms[i].second;

		if (c.terms.size() > 1)
			os << endl;
	}
	os << "]";

	return os;
}
