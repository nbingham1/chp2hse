/*
 * loop.h
 *
 *  Created on: Oct 28, 2012
 *      Author: Ned Bingham
 */

#include "conditional.h"
#include "common.h"

#ifndef loop_h
#define loop_h

struct loop : conditional
{
	loop();
	loop(string raw, map<string, keyword*> types, map<string, variable*> vars, map<string, state> init, string tab);
	~loop();

	void parse(string raw, map<string, keyword*> types, map<string, variable*> vars, map<string, state> init, string tab);
};

#endif
