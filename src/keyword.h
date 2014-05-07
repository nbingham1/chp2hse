/*
 * keyword.h
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham
 */

#include "common.h"
#include "tokenizer.h"

#ifndef keyword_h
#define keyword_h

/** This structure represents the basic data types and acts as
 *  the base class for the more complex ones. The only basic
 *  data type currently defined is 'node<n>' which represents
 *  an array of n nodes (wires, nets, whatever you want to call
 *  it). The only thing that we need to keep track of for basic
 *  data types is their names.
 */
struct keyword
{
protected:
	/** Used for type checking during runtime. All classes that inherit
	 *  from keyword must define their _kind field as their name in
	 *  all of their constructors.
	 */
	string _kind;

public:
	keyword();
	keyword(string name);
	~keyword();

	string name;

	/** The range of tokens from which this keyword was parsed.
	 */
	int start_token;
	int end_token;

	string kind();

	keyword &operator=(keyword k);
};

struct type_space
{
	type_space();
	~type_space();

	list<keyword*> types;

	void insert(tokenizer &tokens, keyword *type);
	keyword *find(string name);
	bool contains(string name);
	string closest(string name);
};

ostream &operator<<(ostream &os, keyword *key);

#endif
