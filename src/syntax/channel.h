/*
 * channel.h
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "record.h"
#include "operator.h"

#ifndef channel_h
#define channel_h

/* This structure represents a structure or channel. A channel
 * contains a bunch of member variables that help you index
 * segments of bits within the multibit signal.
 */
/* Requirements:
 * All variables are boolean valued
 * There are no communication actions
 * All assignment statements have only constant expressions, only true or false
 */

struct channel : record
{
	channel();
	channel(tokenizer &tokens, type_space &types);
	~channel();

	vector<operate*> send;
	vector<operate*> recv;
	operate *probe;

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, type_space &types);
	void print(ostream &os = cout, string newl = "\n");
};

ostream &operator<<(ostream &os, const channel &c);

#endif
