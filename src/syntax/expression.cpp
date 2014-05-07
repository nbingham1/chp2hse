/*
 * expression.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: nbingham
 */

#include "expression.h"
#include "slice.h"
#include "record.h"
#include "channel.h"
#include "process.h"
#include "operator.h"
#include "type_name.h"
#include "variable_name.h"
#include "assignment.h"
#include "control.h"
#include "skip.h"
#include "../message.h"

expression::expression()
{
	this->_kind = "expression";
	this->parent = NULL;
	this->val = NULL;
	this->var = NULL;
	this->preface = NULL;
	this->expr = "null";
}

/** Create a new expression by parsing the token steam.
 */
expression::expression(tokenizer &tokens, type_space &types, variable_space &vars, instruction *parent, size_t i)
{
	this->_kind = "expression";
	this->parent = parent;
	this->val = NULL;
	this->var = NULL;
	this->preface = NULL;
	this->expr = "null";

	parse(tokens, types, vars, i);
}

/** Duplicate this expression while doing a search and replace of certain variable names. This is ultimately used for process instantiation.
 */
expression::expression(instruction *instr, tokenizer &tokens, variable_space &vars, instruction *parent, map<string, string> rename)
{
	this->_kind = "expression";
	this->parent = NULL;
	this->val = NULL;
	this->var = NULL;
	this->preface = NULL;
	this->expr = "null";

	// Make sure that our inputs are correct
	if (instr == NULL)
		internal(tokens, "attempting to copy a '" + kind() + "' from a null instruction pointer", __FILE__, __LINE__);
	else if (instr->kind() != kind())
		internal(tokens, "attempting to copy a '" + kind() + "' from a '" + instr->kind() + "' pointer", __FILE__, __LINE__);
	else
	{
		expression *source = (expression*)instr;
		this->parent = parent;
		this->last_operator = source->last_operator;

		if (source->val != NULL)
			this->val = new constant(source->val, tokens, vars, this, rename);

		if (source->var != NULL)
			this->var = new variable_name(source->var, tokens, vars, this, rename);

		// Since the preface can technically be any type, we need to check all of them.
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

		this->expr = instance(source->expr, NULL, rename).value;
	}
}

/** Parse an expression from a string by creating a temporary tokenizer and then pushing the error count on to the perminant one.
 */
expression::expression(tokenizer &tokens, type_space &types, variable_space &vars, string value)
{
	this->_kind = "expression";
	this->parent = parent;
	this->val = NULL;
	this->var = NULL;
	this->preface = NULL;
	this->expr = "null";
	tokenizer temp(value);
	parse(temp, types, vars, 0);
}

/** Initialize this expression with a constant value.
 */
expression::expression(tokenizer &tokens, type_space &types, variable_space &vars, int value)
{
	this->_kind = "expression";
	this->parent = NULL;
	this->var = NULL;
	this->preface = NULL;
	this->val = new constant(tokens, types, vars, value);
	this->val->parent = this;
	this->expr = to_string(val->value & 0x01);
}

expression::~expression()
{
	if (preface != NULL)
		delete preface;
	preface = NULL;

	if (val != NULL)
		delete val;
	val = NULL;

	if (var != NULL)
		delete var;
	var = NULL;

	expr = "null";
}

/** Get the bit width of the expression. If the expression is not of type node then this returns 0.
 */
int expression::width(variable_space &vars)
{
	if (var != NULL)
		return var->width(vars);
	else if (val != NULL)
		return val->width;
	else if (expr != "null")
		return 1;
	else
		return 0;
}

/** Returns true if the type of this expression is a node.
 */
bool expression::is_node(variable_space &vars)
{
	if (var != NULL)
		return var->is_node(vars);
	else if (val != NULL || expr != "null")
		return true;
	else
		return false;
}

/** Return the type string of the expression including the width if the expression has a type of node.
 */
string expression::type_string(variable_space &vars)
{
	string result;
	if (var != NULL)
		result += var->type_string(vars);
	else if (val != NULL)
		result += "node<" + to_string(val->width) + ">";
	else if (expr != "null")
		result += "node<1>";
	else
		result += "null";

	return result;
}

/** Get a string representation of this expression.
 */
string expression::expr_string(variable_space &vars)
{
	if (val != NULL)
		return to_string(val->value);
	else if (var != NULL)
		return var->expr_string(vars);
	else if (expr != "null")
		return expr;
	else
		return "null";
}

/** Check to see if the next ith item in the token stream is an expression.
 */
bool expression::is_next(tokenizer &tokens, size_t i)
{
	string token = tokens.peek(i);
	return (instance::is_next(tokens, i) || constant::is_next(tokens, i) || string("(+-~#").find(token) != string::npos);
}

/** This is just to maintain consistency with the abstract base class.
 */
void expression::parse(tokenizer &tokens, type_space &types, variable_space &vars)
{
	parse(tokens, types, vars, 0);
}

/** Parse an expression from the token steam, creating the preface instruction tree as we go.
 *  By the end, one of var, val, or expr will not be null and the preface will be filled as is
 *  necessary. The value of i tells us the current operator presidence level.
 */
void expression::parse(tokenizer &tokens, type_space &types, variable_space &vars, size_t i)
{
	start_token = tokens.index+1;

	// Parse all of the binary operators
	if (i < tokens.operations.size())
	{
		bool done = false;
		while (!done)
		{
			/* add the operators at the current presidence level to the bound
			 * so that we don't pass by them as we parse lower level expressions.
			 */
			tokens.increment();
			tokens.push_bound(tokens.operations[i]);

			// Look for the next order expression in the token stream
			tokens.increment();
			tokens.push_expected("[expression]");
			tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();

			// If there wasn't a syntax error, then parse and handle the next order expression.
			if (expression::is_next(tokens))
			{
				expression e(tokens, types, vars, this, i+1);
				// If this the left most operand, we can just copy the result of the lower pesidence expression parsing
				if (last_operator == "")
				{
					var = e.var;
					if (var != NULL)
						var->parent = this;
					val = e.val;
					if (val != NULL)
						val->parent = this;
					preface = e.preface;
					if (preface != NULL)
						preface->parent = this;
					expr = e.expr;

					e.var = NULL;
					e.val = NULL;
					e.preface = NULL;
				}
				// Otherwise, if both operands were constant, we can just handle that at compile time.
				else if (val != NULL && e.val != NULL)
				{
					if (last_operator == "|")
						val->value = (val->value | e.val->value);
					else if (last_operator == "&")
						val->value = (val->value & e.val->value);
					else if (last_operator == "^")
						val->value = (val->value ^ e.val->value);
					else if (last_operator == "==")
						val->value = (val->value == e.val->value);
					else if (last_operator == "~=")
						val->value = (val->value != e.val->value);
					else if (last_operator == "<")
						val->value = (val->value < e.val->value);
					else if (last_operator == ">")
						val->value = (val->value > e.val->value);
					else if (last_operator == "<=")
						val->value = (val->value <= e.val->value);
					else if (last_operator == ">=")
						val->value = (val->value >= e.val->value);
					else if (last_operator == "<<")
						val->value = (val->value << e.val->value);
					else if (last_operator == ">>")
						val->value = (val->value >> e.val->value);
					else if (last_operator == "+")
						val->value = (val->value + e.val->value);
					else if (last_operator == "-")
						val->value = (val->value - e.val->value);
					else if (last_operator == "*")
						val->value = (val->value * e.val->value);
					else if (last_operator == "/")
						val->value = (val->value / e.val->value);
					else if (last_operator == "%")
						val->value = (val->value % e.val->value);

					// combine the preface instruction trees for the two operands as is necessary
					if (e.preface != NULL && preface != NULL)
					{
						composition *new_preface = new composition();
						new_preface->terms.push_back(preface);
						new_preface->terms.push_back(e.preface);
						preface->parent = new_preface;
						e.preface->parent = new_preface;
						new_preface->operators.push_back("||");
						preface = new_preface;
					}
					else if (preface == NULL)
						preface = e.preface;

					e.preface = NULL;

					expr = to_string(val->value & 0x01);
				}
				// If we are dealing with binary Boolean operators with one bit operands 
				else if ((last_operator == "|" || last_operator == "&") && var == NULL && e.var == NULL && (val != NULL || expr != "null") && (e.val != NULL || e.expr != "null"))
				{
					// x & 0 = 0 & x = 0
					if (last_operator == "&" && ((val != NULL && (val->value & 0x01) == 0) || (e.val != NULL && (e.val->value & 0x01) == 0)))
					{
						if (val == NULL)
						{
							val = e.val;
							e.val = NULL;
						}
						val->value = 0;
						val->width = 1;
						expr = "0";
					}
					// 1 & x = x
					else if (last_operator == "&" && val != NULL && (val->value & 0x01) == 1)
					{
						delete val;
						val = e.val;
						e.val = NULL;
						expr = e.expr;
					}
					// x & 1 = x
					else if (last_operator == "&" && e.val != NULL && (e.val->value & 0x01) == 1)
					{
						// Do nothing
					}
					// x | 1 = 1 | x = 1
					else if (last_operator == "|" && ((val != NULL && (val->value & 0x01) == 1) || (e.val != NULL && (e.val->value & 0x01) == 1)))
					{
						if (val == NULL)
						{
							val = e.val;
							e.val = NULL;
						}
						val->value = 1;
						val->width = 1;
						expr = "1";
					}
					// 0 | x = x
					else if (last_operator == "|" && val != NULL && (val->value & 0x01) == 0)
					{
						delete val;
						val = e.val;
						e.val = NULL;
						expr = e.expr;
					}
					// x | 0 = x
					else if (last_operator == "|" && e.val != NULL && (e.val->value & 0x01) == 0)
					{
						// Do nothing
					}
					// x & y, x | y
					else
						expr += last_operator + e.expr;

					// combine the preface instruction trees as is necessary
					if (e.preface != NULL && preface != NULL)
					{
						composition *new_preface = new composition();
						new_preface->terms.push_back(preface);
						new_preface->terms.push_back(e.preface);
						preface->parent = new_preface;
						e.preface->parent = new_preface;
						new_preface->operators.push_back("||");
						preface = new_preface;
					}
					else if (preface == NULL)
						preface = e.preface;

					e.preface = NULL;
				}
				// Bake in boolean operations between nodes
				else if ((last_operator == "|" || last_operator == "&") && is_node(vars) && e.is_node(vars))
				{
					if (e.val != NULL)
						e.val->width = width(vars);
					if (val != NULL)
						val->width = e.width(vars);

					if (width(vars) != e.width(vars))
						error(tokens, "width mismatch '" + type_string(vars) + "' != '" + e.type_string(vars) + "'", "", __FILE__, __LINE__);

					int expr_width = width(vars);

					keyword *type = types.find("node");
					string result = vars.unique_name(type);

					// Create a variable to store the result of the operation
					instruction *prepreface = vars.insert(tokens, variable(result, type, expr_width, false, vector<string>(), start_token));
					if (prepreface != NULL)
					{
						internal(tokens, "operator argument should not generate a preface", __FILE__, __LINE__);
						delete prepreface;
						prepreface = NULL;
					}

					// set up the conditionals and transitions that implement Boolean operations
					vector<instruction*> postface;
					for (size_t j = 0; j < vars.globals.size(); j++)
					{
						size_t idx_start = vars.globals[j].name.find_last_of("[")+1;
						size_t idx_end = vars.globals[j].name.find_last_of("]");

						int index = 0;
						if (idx_start != string::npos && idx_end != string::npos)
							index = atoi(vars.globals[j].name.substr(idx_start, idx_end - idx_start).c_str());

						if (vars.globals[j].name.substr(0, result.size()) == result && ((var == NULL && (val != NULL || index == 0)) || (var != NULL && var->in_bounds(vars, index))) &&
																					   ((e.var == NULL && (e.val != NULL || index == 0)) || (e.var != NULL && e.var->in_bounds(vars, index))))
						{
							string set_name = "null";
							if (last_operator == "&" && ((val != NULL && ((val->value >> index) & 0x01) == 0) || (e.val != NULL && ((e.val->value >> index) & 0x01) == 0)))
							{
								set_name = "0";
								variable_name *left = new variable_name(tokens, types, vars, variable_index(j, false), NULL);
								expression *right = new expression(tokens, types, vars, set_name);
								postface.push_back(new assignment(tokens, types, vars, 1, left, right));
							}
							else if (last_operator == "|" && ((val != NULL && ((val->value >> index) & 0x01) == 1) || (e.val != NULL && ((e.val->value >> index) & 0x01) == 1)))
							{
								set_name = "1";
								variable_name *left = new variable_name(tokens, types, vars, variable_index(j, false), NULL);
								expression *right = new expression(tokens, types, vars, set_name);
								postface.push_back(new assignment(tokens, types, vars, 1, left, right));
							}
							else
							{
								set_name = "(" + expr_string(vars) + ")" + vars.globals[j].name.substr(result.size()) + last_operator + "(" + e.expr_string(vars) + ")" + vars.globals[j].name.substr(result.size());
								cout << set_name << endl;
								variable_name *left = new variable_name(tokens, types, vars, variable_index(j, false), NULL);
								expression *right = new expression(tokens, types, vars, set_name);
								cout << right->expr_string(vars) << " " << endl;
								postface.push_back(new control(tokens, types, vars, left, right));
							}
						}
					}

					if (postface.size() > 1)
						prepreface = new composition(tokens, types, vars, ",", postface);
					else if (postface.size() == 1)
						prepreface = postface[0];

					postface.clear();

					if (preface != NULL && e.preface != NULL && prepreface != NULL)
					{
						composition *new_preface = new composition();
						new_preface->terms.push_back(new composition());
						((composition*)new_preface->terms.front())->parent = new_preface;
						((composition*)new_preface->terms.front())->terms.push_back(preface);
						((composition*)new_preface->terms.front())->terms.push_back(e.preface);
						preface->parent = new_preface->terms.front();
						e.preface->parent = new_preface->terms.front();
						((composition*)new_preface->terms.front())->operators.push_back("||");
						new_preface->terms.push_back(prepreface);
						prepreface->parent = new_preface;
						new_preface->operators.push_back(";");
						preface = new_preface;
					}
					else if (preface != NULL && e.preface != NULL)
					{
						composition *new_preface = new composition();
						new_preface->terms.push_back(preface);
						new_preface->terms.push_back(e.preface);
						preface->parent = new_preface;
						e.preface->parent = new_preface;
						new_preface->operators.push_back("||");
						preface = new_preface;

					}
					else if (prepreface != NULL && preface != NULL)
					{
						composition *new_preface = new composition();
						new_preface->terms.push_back(preface);
						new_preface->terms.push_back(prepreface);
						preface->parent = new_preface;
						prepreface->parent = new_preface;
						new_preface->operators.push_back(";");
						preface = new_preface;
					}
					else if (prepreface != NULL && e.preface != NULL)
					{
						composition *new_preface = new composition();
						new_preface->terms.push_back(e.preface);
						new_preface->terms.push_back(prepreface);
						e.preface->parent = new_preface;
						prepreface->parent = new_preface;
						new_preface->operators.push_back(";");
						preface = new_preface;
					}
					else if (e.preface != NULL)
						preface = e.preface;
					else if (prepreface != NULL)
						preface = prepreface;

					e.preface = NULL;
					if (preface != NULL)
						preface->parent = this;

					if (var != NULL)
						delete var;
					var = NULL;
					if (val != NULL)
						delete val;
					val = NULL;
					expr = "null";

					var = new variable_name(tokens, types, vars, vars.find(result), NULL);
					if (var->var.is_valid() && vars.at(var->var).type->name == "node" && vars.at(var->var).width == 1)
					{
						expr = var->expr_string(vars);
						delete var;
						var = NULL;
					}
				}
				// handle the default case as long as we have two valid operands
				else if ((var != NULL || val != NULL || expr != "null") && (e.var != NULL || e.val != NULL || e.expr != "null"))
				{
					if (e.val != NULL)
						e.val->width = width(vars);
					if (val != NULL)
						val->width = e.width(vars);

					// figure out what operator we need to instantiate given the type names of the operands
					string op = "operator" + last_operator + "(" + type_string(vars) + "," + e.type_string(vars) + ")";

					operate *optype = ((operate*)types.find(op));
					if (optype == NULL || optype->kind() != "operator")
					{
						error(tokens, "undefined operator '" + op + "'", "did you mean " + types.closest(op), __FILE__, __LINE__);
						if (preface != NULL)
							delete preface;
						preface = NULL;
						if (var != NULL)
							delete var;
						var = NULL;
						if (val != NULL)
							delete val;
						val = NULL;
						expr = "null";
					}
					else if (optype->args.size() > 0)
					{
						vector<string> inputs;
						inputs.push_back(expr_string(vars));
						inputs.push_back(e.expr_string(vars));
						inputs.push_back(vars.unique_name(optype->args.back()->type.key));

						// instantiate the internal variable that will store the result of the operation
						instruction *prepreface = vars.insert(tokens, variable(inputs.back(), optype->args.back()->type.key, optype->args.back()->type.width.value, false, vector<string>(), start_token));
						if (prepreface != NULL)
						{
							internal(tokens, "operator argument should not generate a preface", __FILE__, __LINE__);
							delete prepreface;
							prepreface = NULL;
						}

						// instantiate the operator
						prepreface = vars.insert(tokens, variable(vars.unique_name(optype), optype, 0, false, inputs, start_token));

						// we now have to deal with three possible prefaces. The prefaces Of the operands
						// should happen first in parallel, followed by the operator instantiation.
						if (preface != NULL && e.preface != NULL && prepreface != NULL)
						{
							composition *new_preface = new composition();
							new_preface->terms.push_back(new composition());
							((composition*)new_preface->terms.front())->parent = new_preface;
							((composition*)new_preface->terms.front())->terms.push_back(preface);
							((composition*)new_preface->terms.front())->terms.push_back(e.preface);
							preface->parent = new_preface->terms.front();
							e.preface->parent = new_preface->terms.front();
							((composition*)new_preface->terms.front())->operators.push_back("||");
							new_preface->terms.push_back(prepreface);
							prepreface->parent = new_preface;
							new_preface->operators.push_back(";");
							preface = new_preface;
						}
						else if (preface != NULL && e.preface != NULL)
						{
							composition *new_preface = new composition();
							new_preface->terms.push_back(preface);
							new_preface->terms.push_back(e.preface);
							preface->parent = new_preface;
							e.preface->parent = new_preface;
							new_preface->operators.push_back("||");
							preface = new_preface;

						}
						else if (prepreface != NULL && preface != NULL)
						{
							composition *new_preface = new composition();
							new_preface->terms.push_back(preface);
							new_preface->terms.push_back(prepreface);
							preface->parent = new_preface;
							prepreface->parent = new_preface;
							new_preface->operators.push_back(";");
							preface = new_preface;
						}
						else if (prepreface != NULL && e.preface != NULL)
						{
							composition *new_preface = new composition();
							new_preface->terms.push_back(e.preface);
							new_preface->terms.push_back(prepreface);
							e.preface->parent = new_preface;
							prepreface->parent = new_preface;
							new_preface->operators.push_back(";");
							preface = new_preface;
						}
						else if (e.preface != NULL)
							preface = e.preface;
						else if (prepreface != NULL)
							preface = prepreface;

						e.preface = NULL;
						if (preface != NULL)
							preface->parent = this;

						if (var != NULL)
							delete var;
						var = NULL;
						if (val != NULL)
							delete val;
						val = NULL;
						expr = "null";

						// reinitialize this expression with the name of the internal variable that stores the result of the operation
						var = new variable_name(tokens, types, vars, vars.find(inputs.back()), NULL);
						if (var->var.is_valid() && vars.at(var->var).type->name == "node" && vars.at(var->var).width == 1)
						{
							expr = var->expr_string(vars);
							delete var;
							var = NULL;
						}
					}
				}
			}

			tokens.decrement();

			if (find(tokens.operations[i].begin(), tokens.operations[i].end(), tokens.peek_next()) != tokens.operations[i].end())
				last_operator = tokens.next();
			else if (tokens.in_bound(1) != tokens.bound.end() || tokens.peek_next() == "")
				done = true;
			else
			{
				tokens.increment();
				tokens.push_expected(tokens.operations[i]);
				string token = tokens.syntax(__FILE__, __LINE__);
				tokens.decrement();

				if (find(tokens.operations[i].begin(), tokens.operations[i].end(), token) != tokens.operations[i].end())
					last_operator = token;
				else
					done = true;
			}
		}
	}
	// handle the unary prefix operators
	else if (i == tokens.operations.size())
	{
		string token = tokens.peek_next();
		if (token == "~" || token == "+" || token == "-")
			last_operator = tokens.next();

		expression e;
		e.parent = this;
		if (last_operator != "")
			e.parse(tokens, types, vars, i);
		else
			e.parse(tokens, types, vars, i+1);

		var = e.var;
		if (var != NULL)
			var->parent = this;
		val = e.val;
		if (val != NULL)
			val->parent = this;
		preface = e.preface;
		if (preface != NULL)
			preface->parent = this;
		expr = e.expr;

		e.var = NULL;
		e.val = NULL;
		e.preface = NULL;

		if (last_operator != "")
		{
			if (val != NULL)
			{
				if (last_operator == "~")
					val->value = ~val->value;
				else if (last_operator == "-")
					val->value = -val->value;
				// plus does nothing

				expr = to_string(val->value & 0x01);
			}
			else if (var == NULL && last_operator == "~" && expr != "null")
				expr = "~" + expr;
			else if (var != NULL && last_operator == "~" && is_node(vars))
			{
				keyword *type = types.find("node");
				string result = vars.unique_name(type);
				int expr_width = width(vars);

				instruction *prepreface = vars.insert(tokens, variable(result, type, expr_width, false, vector<string>(), start_token));
				if (prepreface != NULL)
				{
					internal(tokens, "operator argument should not generate a preface", __FILE__, __LINE__);
					delete prepreface;
					prepreface = NULL;
				}

				vector<instruction*> postface;
				for (size_t j = 0; j < vars.globals.size(); j++)
				{
					size_t idx_start = vars.globals[j].name.find_last_of("[")+1;
					size_t idx_end = vars.globals[j].name.find_last_of("]");

					int index = 0;
					if (idx_start != string::npos && idx_end != string::npos)
						index = atoi(vars.globals[j].name.substr(idx_start, idx_end - idx_start).c_str());

					if (vars.globals[j].name.substr(0, result.size()) == result && ((var == NULL && (val != NULL || index == 0)) || (var != NULL && var->in_bounds(vars, index))))
					{
						string set_name = "~(" + expr_string(vars) + ")" + vars.globals[j].name.substr(result.size());
						variable_name *left = new variable_name(tokens, types, vars, variable_index(j, false), NULL);
						expression *right = new expression(tokens, types, vars, set_name);
						postface.push_back(new control(tokens, types, vars, left, right));
					}
				}

				if (postface.size() > 1)
					prepreface = new composition(tokens, types, vars, ",", postface);
				else if (postface.size() == 1)
					prepreface = postface[0];

				postface.clear();

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

				if (preface != NULL)
					preface->parent = this;

				if (var != NULL)
					delete var;
				var = NULL;
				if (val != NULL)
					delete val;
				val = NULL;
				expr = "null";

				var = new variable_name(tokens, types, vars, vars.find(result), NULL);
				if (var->var.is_valid() && vars.at(var->var).type->name == "node" && vars.at(var->var).width == 1)
				{
					expr = var->expr_string(vars);
					delete var;
					var = NULL;
				}
			}
			else
			{
				string op = "operator" + last_operator + "(" + type_string(vars) + ")";

				operate *optype = ((operate*)types.find(op));
				if (optype == NULL || optype->kind() != "operator")
				{
					error(tokens, "undefined operator '" + op + "'", "did you mean " + types.closest(op), __FILE__, __LINE__);
					if (preface != NULL)
						delete preface;
					preface = NULL;
					if (var != NULL)
						delete var;
					var = NULL;
					if (val != NULL)
						delete val;
					val = NULL;
					expr = "null";
				}
				else if (optype->args.size() > 0)
				{
					vector<string> inputs;
					inputs.push_back(expr_string(vars));
					inputs.push_back(vars.unique_name(optype->args.back()->type.key));

					instruction *prepreface = vars.insert(tokens, variable(inputs.back(), optype->args.back()->type.key, optype->args.back()->type.width.value, false, vector<string>(), start_token));
					if (prepreface != NULL)
					{
						internal(tokens, "operator argument should not generate a preface", __FILE__, __LINE__);
						delete prepreface;
						prepreface = NULL;
					}

					prepreface = vars.insert(tokens, variable(vars.unique_name(optype), optype, 0, false, inputs, start_token));

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

					if (preface != NULL)
						preface->parent = this;

					if (var != NULL)
						delete var;
					var = NULL;
					if (val != NULL)
						delete val;
					val = NULL;
					expr = "null";

					var = new variable_name(tokens, types, vars, vars.find(inputs.back()), NULL);
					if (var->var.is_valid() && vars.at(var->var).type->name == "node" && vars.at(var->var).width == 1)
					{
						expr = var->expr_string(vars);
						delete var;
						var = NULL;
					}
				}
			}
		}
	}
	else if (i == tokens.operations.size()+1)
	{
		string token = tokens.peek_next();
		if (token == "#")
			last_operator = tokens.next();

		expression e(tokens, types, vars, this, i+1);

		var = e.var;
		if (var != NULL)
			var->parent = this;
		val = e.val;
		if (val != NULL)
			val->parent = this;
		preface = e.preface;
		if (preface != NULL)
			preface->parent = this;
		expr = e.expr;

		e.var = NULL;
		e.val = NULL;
		e.preface = NULL;

		if (last_operator != "")
		{
			if (val != NULL || (var != NULL && var->var.is_valid() && vars.at(var->var).type != NULL && vars.at(var->var).type->kind() != "channel"))
				error(tokens, "expected operand of kind 'channel'", "", __FILE__, __LINE__);
			else
			{
				string op = "operator" + last_operator + "(" + type_string(vars) + ")";

				operate *optype = ((operate*)types.find(op));
				if (optype == NULL || optype->kind() != "operator")
				{
					error(tokens, "undefined operator '" + op + "'", "did you mean " + types.closest(op), __FILE__, __LINE__);
					if (preface != NULL)
						delete preface;
					preface = NULL;
					if (var != NULL)
						delete var;
					var = NULL;
					if (val != NULL)
						delete val;
					val = NULL;
					expr = "null";
				}
				else if (optype->args.size() > 0)
				{
					vector<string> inputs;
					inputs.push_back(expr_string(vars));
					inputs.push_back(vars.unique_name(optype->args.back()->type.key));

					instruction *prepreface = vars.insert(tokens, variable(inputs.back(), optype->args.back()->type.key, optype->args.back()->type.width.value, false, vector<string>(), start_token));
					if (prepreface != NULL)
					{
						internal(tokens, "operator argument should not generate a preface", __FILE__, __LINE__);
						delete prepreface;
						prepreface = NULL;
					}

					prepreface = vars.insert(tokens, variable(vars.unique_name(optype), optype, 0, false, inputs, start_token));

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

					if (preface != NULL)
						preface->parent = this;

					if (var != NULL)
						delete var;
					var = NULL;
					if (val != NULL)
						delete val;
					val = NULL;
					expr = "null";

					var = new variable_name(tokens, types, vars, vars.find(inputs.back()), NULL);
					if (var->var.is_valid() && vars.at(var->var).type->name == "node" && vars.at(var->var).width == 1)
					{
						expr = var->expr_string(vars);
						delete var;
						var = NULL;
					}
				}
			}
		}
	}
	else if (i == tokens.operations.size()+2)
	{
		tokens.increment();
		tokens.push_bound("?");
		expression e(tokens, types, vars, this, i+1);
		tokens.decrement();

		var = e.var;
		if (var != NULL)
			var->parent = this;
		val = e.val;
		if (val != NULL)
			val->parent = this;
		preface = e.preface;
		if (preface != NULL)
			preface->parent = this;
		expr = e.expr;

		e.var = NULL;
		e.val = NULL;
		e.preface = NULL;

		string token = tokens.peek_next();
		if (token == "?")
		{
			last_operator = tokens.next();
			if (val != NULL || (var != NULL && var->var.is_valid() && vars.at(var->var).type != NULL && vars.at(var->var).type->kind() != "channel"))
				error(tokens, "expected operand of kind 'channel'", "", __FILE__, __LINE__);
			else
			{
				string op = "operator" + last_operator + "(" + type_string(vars) + ")";

				operate *optype = ((operate*)types.find(op));
				if (optype == NULL || optype->kind() != "operator")
				{
					error(tokens, "undefined operator '" + op + "'", "did you mean " + types.closest(op), __FILE__, __LINE__);
					if (preface != NULL)
						delete preface;
					preface = NULL;
					if (var != NULL)
						delete var;
					var = NULL;
					if (val != NULL)
						delete val;
					val = NULL;
					expr = "null";
				}
				else if (optype->args.size() > 0)
				{
					vector<string> inputs;
					inputs.push_back(expr_string(vars));
					inputs.push_back(vars.unique_name(optype->args.back()->type.key));

					instruction *prepreface = vars.insert(tokens, variable(inputs.back(), optype->args.back()->type.key, optype->args.back()->type.width.value, false, vector<string>(), start_token));
					if (prepreface != NULL)
					{
						internal(tokens, "operator argument should not generate a preface", __FILE__, __LINE__);
						delete prepreface;
						prepreface = NULL;
					}

					prepreface = vars.insert(tokens, variable(vars.unique_name(optype), optype, 0, false, inputs, start_token));

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

					if (preface != NULL)
						preface->parent = this;

					if (var != NULL)
						delete var;
					var = NULL;
					if (val != NULL)
						delete val;
					val = NULL;
					expr = "null";

					var = new variable_name(tokens, types, vars, vars.find(inputs.back()), NULL);
					if (var->var.is_valid() && vars.at(var->var).type->name == "node" && vars.at(var->var).width == 1)
					{
						expr = var->expr_string(vars);
						delete var;
						var = NULL;
					}
				}
			}
		}
	}
	else if (i == tokens.operations.size()+3)
	{
		tokens.increment();
		tokens.push_expected("(");
		tokens.push_expected("[variable name]");
		tokens.push_expected("[constant]");
		string token = tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		if (token == "(")
		{
			tokens.increment();
			tokens.push_bound(")");
			expression e(tokens, types, vars, this);
			tokens.decrement();

			var = e.var;
			if (var != NULL)
				var->parent = this;
			val = e.val;
			if (val != NULL)
				val->parent = this;
			preface = e.preface;
			if (preface != NULL)
				preface->parent = this;
			expr = e.expr;

			e.var = NULL;
			e.val = NULL;
			e.preface = NULL;

			tokens.increment();
			tokens.push_expected(")");
			tokens.syntax(__FILE__, __LINE__);
			tokens.decrement();

			if (var == NULL && val == NULL && expr != "null")
				expr = "(" + expr + ")";

			while (slice::is_next(tokens))
			{
				if (var != NULL)
				{
					var->restrict_bits(tokens, types, vars);
					if (var->var.is_valid() && vars.at(var->var).type->name == "node" && vars.at(var->var).width == 1)
					{
						expr = var->expr_string(vars);
						delete var;
						var = NULL;
					}
				}
				else if (val != NULL)
				{
					val->restrict_bits(tokens, types, vars);
					expr = to_string(val->value & 0x01);
				}
				else
				{
					slice s(tokens, types, vars, this);
					if (s.start.value != 0 || (s.end.value != 0 && s.end.value != 1))
						error(tokens, "index out of bounds", "", __FILE__, __LINE__);
					else if (s.end.value == 0 && (var != NULL || val != NULL || expr != "null"))
					{
						expr = "0";
						val = new constant();
						val->parent = this;
						val->value = 0;
						val->width = 0;
					}
				}
			}
		}
		else if (variable_name::is_next(tokens))
		{
			var = new variable_name(tokens, types, vars, this);
			if (var->var.is_valid() && vars.at(var->var).type->name == "node" && vars.at(var->var).width == 1)
			{
				expr = var->expr_string(vars);
				delete var;
				var = NULL;
			}
		}
		else if (constant::is_next(tokens))
		{
			val = new constant(tokens, types, vars, this);
			expr = to_string(val->value & 0x01);
		}
	}

	end_token = tokens.index;
}

void expression::hide(variable_space &vars, vector<variable_index> uids)
{
	if (var != NULL && find(uids.begin(), uids.end(), var->var) != uids.end())
	{
		delete var;
		var = NULL;

		val = new constant();
		val->value = 1;
		val->width = 1;
		val->parent = this;

		expr = "1";
	}
	else if (val == NULL && expr != "null")
	{
		map<string, string> rename;
		for (size_t i = 0; i < uids.size(); i++)
			rename.insert(pair<string, string>(vars.at(uids[i]).name, "1"));
		tokenizer temp(expr);
		expr = ::hide(temp, vars, uids);
		if (expr == "")
		{
			val = new constant();
			val->value = 1;
			val->width = 1;
			val->parent = this;

			expr = "1";
		}
	}
}

void expression::print(ostream &os, string newl)
{
	if (val != NULL)
		val->print(os, newl);
	else if (var != NULL)
		var->print(os, newl);
	else
		os << expr;
}

string hide(tokenizer &tokens, variable_space &vars, vector<variable_index> uids)
{
	string result;
	string half;
	string expr;
	do
	{
		if (half != "")
			result += "|";

		expr = "";

		do
		{
			if (expr != "")
				half += "&";

			bool invert = false;
			while (tokens.peek_next() == "~" && tokens.next() == "~")
				invert = !invert;

			expr = "";
			if (tokens.peek_next() == "(")
			{
				tokens.increment();
				tokens.push_expected("(");
				tokens.syntax(__FILE__, __LINE__);
				tokens.decrement();

				expr = hide(tokens, vars, uids);
				if (expr != "")
				{
					expr = "(" + expr + ")";

					if (invert)
						expr = "~" + expr;
				}

				tokens.increment();
				tokens.push_expected(")");
				tokens.syntax(__FILE__, __LINE__);
				tokens.decrement();
			}
			else if (tokens.peek_next() != "")
			{
				type_space temp;
				variable_name v(tokens, temp, vars, NULL);
				if (find(uids.begin(), uids.end(), v.var) == uids.end())
				{
					if (invert)
						expr += "~";
					expr += v.name.value;
				}
			}


			half += expr;
		} while (tokens.peek_next() == "&" && tokens.next() == "&");

		result += half;
	} while (tokens.peek_next() == "|" && tokens.next() == "|");

	return result;
}

ostream &operator<<(ostream &os, const expression &e)
{
	if (e.val != NULL)
		os << *e.val;
	else if (e.var != NULL)
		os << *e.var;
	else
		os << e.expr;

	return os;
}
