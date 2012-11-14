/*
 * rule.h
 *
 *  Created on: Nov 11, 2012
 *      Author: Ned Bingham
 */

#include "common.h"
#include "space.h"

#ifndef rule_h
#define rule_h

struct rule
{
	rule();
	~rule();

	string var;
	space plus;
	space minus;

	int check();
	void clear(int n);
};

#endif