/*
 * tokenizer.h
 *
 *  Created on: Apr 7, 2014
 *      Author: nbingham
 */

#include "common.h"

#ifndef tokenizer_h
#define tokenizer_h

/** A token is the smallest subsection of an input stream to have
 *  meaning in a given language.
 */ 
struct token
{
	token();
	token(size_t file, size_t line, size_t col, size_t size);
	~token();

	size_t file;
	size_t line;
	size_t col;
	size_t size;
};

/** This structure splits up multiple input folks onto a stream of
 *  tokens and then facilitates search and iteration though those
 *  tokens and handling errors.
 */
struct tokenizer
{
	tokenizer();
	tokenizer(string filename, string contents);
	tokenizer(string contents);
	~tokenizer();

	// These are both initialized in the constructor
	vector<vector<string> > compositions;		// The set of symbols that represent composition operators in CHP grouped and ordered by precedence
	vector<vector<string> > operations;			// The set of symbols that represent operators in an equation grouped and ordered by precedence
	
	// The tokenized files stored with a file name and a list of lines
	vector<pair<string, vector<string> > > files;

	// The list of tokens which are really just indices into the files variable
	vector<token> tokens;
	string curr;	// The current token string value
	size_t index;	// The current token (index into the tokens list)

	/* These two are stacks of groups of bound and expected token types respectively. 
	 * They pair with the instructions push_expected push_bound increment decrement and syntax
	 * to provide an easy interface for searching for a set of expected tokens within a set 
	 * of bounding tokens. Here is an example of their usage:
	 * 
	 * tokens.increment();
	 * tokens.push_bound("]");
	 *
	 * tokens.increment();
	 * tokens.push_expected(";");
	 * tokens.syntax(__FILE__, __LINE__);
	 * tokens.decrement();
	 * 
	 * tokens.decrement();
	 *
	 * In this example, the tokenizer will check to see if the next token is a semicolon.
	 * If it isn't, then it will throw an error and iterate forward until it finds a semicolon,
	 * or the closing square bracket. If it finds the closing square bracket, then it goes
	 * back to where the semicolon should have been. If it finds the semicolon, then it stays
	 * there.
	 */
	vector<vector<string> > bound;
	vector<vector<string> > expected;

	void increment();
	void decrement();
	void push_bound(string s);
	void push_bound(vector<string> s);
	void push_expected(string s);
	void push_expected(vector<string> s);
	
	// Check if the ith next token is in the bound or expected stack
	vector<vector<string> >::iterator in_bound(size_t i);
	vector<vector<string> >::iterator in_expected(size_t i);
	
	// Check if this string is in the bound or expected stack
	vector<vector<string> >::iterator in_bound(string s);
	vector<vector<string> >::iterator in_expected(string s);

	// Condense the bound or expected stack into one readable string
	string bound_string();
	string expect_string();
	int count_expected();

	string syntax(string debug_file, int debug_line);

	// Tokenize an input stream. if the file name is not specified, the stream is considered internal and all errors as a result of that stream will be considered internal failures.
	void insert(string filename, string contents);

	string at(size_t i);
	string line(size_t i);
	string file(size_t i);
	string prev();
	string next();
	string peek(size_t i);
	string peek_next();
	string peek_prev();

	tokenizer &operator=(string contents);
};

#endif
