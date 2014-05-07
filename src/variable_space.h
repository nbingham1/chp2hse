/*
 * variable_space.h
 *
 * A variable space collects information about all the variables in a given program.
 * It includes a list variables, as well as types and labels for those variables.
 * The concept behind variable_space is to consolidate information about variables into one
 * easy to access place. variable_space is primarily used in program.
 */

#include "common.h"
#include "variable.h"
#include "tokenizer.h"

#ifndef variable_space_h
#define variable_space_h

struct expression;
struct instruction;

struct variable_index
{
	variable_index();
	variable_index(size_t uid, bool label);
	~variable_index();

	size_t uid;
	bool label;

	bool is_valid();

	variable_index &operator=(variable_index i);
};

bool operator==(variable_index v1, variable_index v2);

struct variable_space
{
	variable_space();
	~variable_space();

	vector<variable> globals;
	vector<variable> labels;

	string reset;
	string assumptions;
	string assertions;

	instruction *insert(tokenizer &tokens, variable v);

	size_t find_global(string name);
	size_t find_label(string name);
	variable_index find(string name);

	bool contains(string name);
	string closest_name(string name);

	string unique_name(keyword *type);

	variable_space &operator=(variable_space s);
	variable &at(variable_index i);

	string variable_list();
};

ostream &operator<<(ostream &os, const variable_space &s);

#endif
