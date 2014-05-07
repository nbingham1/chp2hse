/*
 * process.cpp
 *
 *  Created on: Oct 29, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "process.h"
#include "operator.h"
#include "composition.h"
#include "control.h"
#include "expression.h"
#include "../message.h"

process::process()
{
	_kind = "process";
	has_environment = false;
	chp = NULL;
	env_chp = NULL;
}

process::process(tokenizer &tokens, type_space &types)
{
	_kind = "process";
	has_environment = false;
	chp = NULL;
	env_chp = NULL;
	parse(tokens, types);
}

process::~process()
{
	for (vector<declaration*>::iterator d = args.begin(); d != args.end(); d++)
		delete (*d);
	args.clear();

	if (chp != NULL)
		delete chp;
	if (env_chp != NULL)
		delete env_chp;

	chp = NULL;
	env_chp = NULL;
}

bool process::is_next(tokenizer &tokens, size_t i)
{
	return (tokens.peek(i) == "process");
}

void process::parse(tokenizer &tokens, type_space &types)
{
	start_token = tokens.index+1;

	tokens.increment();
	tokens.push_expected("process");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	tokens.increment();
	tokens.push_bound("{");

	tokens.increment();
	tokens.push_expected("[instance]");
	tokens.push_bound("(");
	tokens.syntax(__FILE__, __LINE__);
	if (instance::is_next(tokens))
		name = instance(tokens, types, vars, NULL).value;
	tokens.decrement();

	tokens.increment();
	tokens.push_expected("(");
	tokens.push_bound("[declaration]");
	tokens.push_bound(")");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	name += "(";

	tokens.increment();
	tokens.push_expected(")");
	tokens.push_expected("[declaration]");
	string token = tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();
	while (token != ")")
	{
		tokens.increment();
		tokens.push_bound(",");
		tokens.push_bound(")");
		tokens.push_expected("[declaration]");
		tokens.syntax(__FILE__, __LINE__);
		if (declaration::is_next(tokens))
		{
			args.push_back(new declaration(tokens, types, vars, NULL));
			if (name[name.size()-1] != '(')
				name += ",";
			name += args.back()->type.name.value;
			if (args.back()->type.name.value == "node")
				name += "<" + to_string(args.back()->type.width.value) + ">";
		}
		tokens.decrement();

		tokens.increment();
		tokens.push_expected(",");
		tokens.push_expected(")");
		tokens.push_bound("[declaration]");
		token = tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();
	}

	name += ")";
	tokens.decrement();

	env_vars = vars;

	tokens.increment();
	tokens.push_expected("{");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	tokens.increment();
	tokens.push_bound("}");
	chp = new composition(tokens, types, vars, NULL);
	tokens.decrement();

	tokens.increment();
	tokens.push_expected("}");
	tokens.push_bound("{");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	if (tokens.peek_next() == "{")
	{
		has_environment = true;

		tokens.increment();
		tokens.push_expected("{");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		tokens.increment();
		tokens.push_bound("}");
		env_chp = new composition(tokens, types, env_vars, NULL);
		tokens.decrement();

		tokens.increment();
		tokens.push_expected("}");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();
	}

	if (env_chp == NULL)
	{
		env_chp = new composition();
		vector<size_t> operators;
		for (size_t i = 0; i < vars.labels.size(); i++)
		{
			if (vars.labels[i].type == NULL)
				internal(tokens, "found null variable type in variable space", __FILE__, __LINE__);
			else if (vars.labels[i].type->name.find("operator!") != string::npos ||
					 vars.labels[i].type->name.find("operator?") != string::npos)
				operators.push_back(i);
		}

		for (size_t i = 0; i < operators.size(); i++)
		{
			if (i != 0)
				((composition*)env_chp)->operators.push_back("||");

			operate *calltype = ((operate*)vars.labels[operators[i]].type);
			if (calltype->args.size() > 0 && calltype->args.front() != NULL)
			{
				((composition*)env_chp)->terms.push_back(new control());
				control *shell = (control*)(((composition*)env_chp)->terms.back());
				shell->parent = env_chp;
				shell->loop = true;
				shell->mutex = true;
				shell->terms.push_back(pair<expression*, composition*>(NULL, NULL));

				string  op;
				if (vars.labels[operators[i]].type->name.find("operator!") != string::npos)
					op = "operator?(" + calltype->args.front()->type.type_string(vars) + ")";
				else if (vars.labels[operators[i]].type->name.find("operator?") != string::npos)
				{
					op = "operator!(" + calltype->args.front()->type.type_string(vars);
					if (calltype->args.size() > 1 && calltype->args.back() != NULL)
						op += "," + calltype->args.back()->type.type_string(vars);
					op += ")";
				}

				keyword *optype = types.find(op);
				if (optype != NULL && optype->kind() == "operator")
				{
					vector<string> inputs;
					inputs.push_back(vars.labels[operators[i]].inputs.front());
					if (calltype->args.size() > 1 && calltype->args.back() != NULL)
					{
						inputs.push_back(env_vars.unique_name(calltype->args.back()->type.key));

						instruction *prepreface = env_vars.insert(tokens, variable(inputs.back(), calltype->args.back()->type.key, calltype->args.back()->type.width.value, false, vector<string>(), start_token));
						if (prepreface != NULL)
						{
							delete prepreface;
							prepreface = NULL;
						}
					}

					shell->terms.front().second = (composition*)env_vars.insert(tokens, variable(env_vars.unique_name(optype), optype, 0, false, inputs, start_token));

					if (calltype->args.size() > 1 && calltype->args.back() != NULL)
					{
						vector<variable_index> uids;
						for (size_t j = 0; j < env_vars.globals.size(); j++)
							if (env_vars.globals[j].name.substr(0, inputs.back().size()) == inputs.back())
								uids.push_back(variable_index(j, false));

						shell->terms.front().second->hide(env_vars, uids);
						tokenizer temp(env_vars.reset);
						env_vars.reset = hide(temp, env_vars, uids);
						if (env_vars.reset == "")
							env_vars.reset = "1";
					}
				}
			}
		}
	}

	end_token = tokens.index;
}

void process::build_hse()
{
	int num_places = 0;
	int num_transitions = 0;

	// Build the HSE for this process
	hse.strict = false;
	hse.type = "digraph";
	hse.id = "model";
	hse.stmt_list.stmts.push_back(dot_stmt());
	dot_stmt &s0 = hse.stmt_list.stmts.back();
	s0.stmt_type = "subgraph";
	s0.id = name.substr(0, name.find_first_of("()"));
	s0.stmt_list.stmts.push_back(dot_stmt());
	dot_stmt &s1 = s0.stmt_list.stmts.back();
	s1.stmt_type = "attr";
	s1.attr_type = "graph";
	s1.attr_list.attrs.push_back(dot_a_list());
	dot_a_list &a1 = s1.attr_list.attrs.back();
	a1.as.push_back(pair<string, string>("label", ""));
	a1.as.push_back(pair<string, string>("variables", vars.variable_list()));
	a1.as.push_back(pair<string, string>("type", "local"));
	a1.as.push_back(pair<string, string>("elaborate", "true"));
	if (vars.reset != "")
		a1.as.push_back(pair<string, string>("reset", vars.reset));
	if (vars.assumptions != "")
		a1.as.push_back(pair<string, string>("assume", vars.assumptions));
	if (vars.assertions != "")
		a1.as.push_back(pair<string, string>("assert", vars.assertions));

	s0.stmt_list.stmts.push_back(dot_stmt());
	dot_stmt &s2 = s0.stmt_list.stmts.back();
	s2.stmt_type = "node";
	dot_node_id start("S" + to_string(num_places++));
	s2.node_ids.push_back(start);
	s2.attr_list.attrs.push_back(dot_a_list());
	dot_a_list &a2 = s2.attr_list.attrs.back();
	a2.as.push_back(pair<string, string>("shape", "circle"));
	a2.as.push_back(pair<string, string>("width", "0.15"));
	a2.as.push_back(pair<string, string>("peripheries", "2"));
	a2.as.push_back(pair<string, string>("style", "filled"));
	a2.as.push_back(pair<string, string>("fillcolor", "#000000"));
	a2.as.push_back(pair<string, string>("label", ""));

	vector<dot_node_id> last(1, start);
	last = chp->build_hse(vars, s0.stmt_list.stmts, last, num_places, num_transitions);

	for (size_t i = 0; i < last.size(); i++)
	{
		s0.stmt_list.stmts.push_back(dot_stmt());
		s0.stmt_list.stmts.back().stmt_type = "edge";
		s0.stmt_list.stmts.back().node_ids.push_back(last[i]);
		s0.stmt_list.stmts.back().node_ids.push_back(start);
	}

	// Build the HSE for the Environment
	hse.stmt_list.stmts.push_back(dot_stmt());
	dot_stmt &s3 = hse.stmt_list.stmts.back();
	s3.stmt_type = "subgraph";
	s3.id = "environment";
	s3.stmt_list.stmts.push_back(dot_stmt());
	dot_stmt &s4 = s3.stmt_list.stmts.back();
	s4.stmt_type = "attr";
	s4.attr_type = "graph";
	s4.attr_list.attrs.push_back(dot_a_list());
	dot_a_list &a3 = s4.attr_list.attrs.back();
	a3.as.push_back(pair<string, string>("label", ""));
	a3.as.push_back(pair<string, string>("variables", env_vars.variable_list()));
	a3.as.push_back(pair<string, string>("type", "remote"));
	a3.as.push_back(pair<string, string>("elaborate", "false"));
	if (env_vars.reset != "")
		a3.as.push_back(pair<string, string>("reset", env_vars.reset));

	s3.stmt_list.stmts.push_back(dot_stmt());
	dot_stmt &s5 = s3.stmt_list.stmts.back();
	s5.stmt_type = "node";
	dot_node_id env_start("S" + to_string(num_places++));
	s5.node_ids.push_back(env_start);
	s5.attr_list.attrs.push_back(dot_a_list());
	dot_a_list &a4 = s5.attr_list.attrs.back();
	a4.as.push_back(pair<string, string>("shape", "circle"));
	a4.as.push_back(pair<string, string>("width", "0.15"));
	a4.as.push_back(pair<string, string>("peripheries", "2"));
	a4.as.push_back(pair<string, string>("style", "filled"));
	a4.as.push_back(pair<string, string>("fillcolor", "#000000"));
	a4.as.push_back(pair<string, string>("label", ""));

	vector<dot_node_id> env_last(1, env_start);
	env_last = env_chp->build_hse(env_vars, s3.stmt_list.stmts, env_last, num_places, num_transitions);

	for (size_t i = 0; i < env_last.size(); i++)
	{
		s3.stmt_list.stmts.push_back(dot_stmt());
		s3.stmt_list.stmts.back().stmt_type = "edge";
		s3.stmt_list.stmts.back().node_ids.push_back(env_last[i]);
		s3.stmt_list.stmts.back().node_ids.push_back(env_start);
	}
}

void process::print(ostream &os, string newl)
{
	os << name << newl;
	os << "{" << newl;
	if (chp != NULL)
	{
		os << "\t";
		chp->print(os, newl+"\t");
	}
	os << newl << "}" << newl;

	os << "{" << newl;
	if (env_chp != NULL)
	{
		os << "\t";
		env_chp->print(os, newl+"\t");
	}
	os << newl << "}" << newl;
}

ostream &operator<<(ostream &os, const process &p)
{
	os << p.name << endl;
	os << "{" << endl;
	if (p.chp != NULL)
		os << p.chp << endl;
	os << "}" << endl;

	os << "{" << endl;
	if (p.env_chp != NULL)
		os << p.env_chp << endl;
	os << "}" << endl;

	return os;
}

