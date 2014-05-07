/*
 * variable.h
 *
 * This structure describes a variable in the chp program. We need
 * to keep track of a variables name, its type (record, keyword, process),
 * and its bit width.
 *
 */

#include "common.h"
#include "keyword.h"

#ifndef variable_h
#define variable_h

struct variable_space;

struct variable
{
	variable();
	variable(string name, keyword *type, uint16_t width, bool arg, vector<string> inputs, int definition_location);
	~variable();

	string		name;		// the name of the instantiated variable
	keyword		*type;		// the name of the type of the instantiated variable
	uint16_t	width;		// the bit width of the instantiated variable
	bool		arg;		// Is this variable an argument variable into a process?
	vector<string> inputs;
	int			definition_location;

	string type_string() const;

	variable &operator=(variable v);
};

ostream &operator<<(ostream &os, const variable &v);

#endif
