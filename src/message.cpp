/*
 * message.cpp
 *
 *  Created on: May 6, 2014
 *      Author: nbingham
 */

#include "message.h"
#include "tokenizer.h"

int num_errors = 0;
int num_warnings = 0;
int num_internal = 0;
int num_notes = 0;
int num_log = 0;
bool verbose = false;

void internal(tokenizer &tokens, string internal, string debug_file, int debug_line, int offset)
{
	size_t i = tokens.index < tokens.tokens.size() ? tokens.index : 0;

	size_t col_start = tokens.tokens[i].col+1+offset;
	size_t col_end = 1;
	string str = tokens.line(i);
	for (size_t j = 0; j < str.size() && j < tokens.tokens[i].col+offset; j++)
	{
		if (str[j] != '\t')
		{
			str[j] = ' ';
			col_end++;
		}
		else
			col_end+=8;
	}
	str.resize(tokens.tokens[i].col+offset, ' ');

	cout << debug_file << ":" << debug_line << ":";
	cout << tokens.file(i) << ":" << tokens.tokens[i].line+1 << ":" << col_start << "-" << col_end << ": internal failure: " << internal << endl;
	cout << tokens.line(i) << endl;
	cout << str << "^" << endl;

	num_internal++;
}

void error(tokenizer &tokens, string error, string note, string debug_file, int debug_line, int offset)
{
	size_t i = tokens.index < tokens.tokens.size() ? tokens.index : 0;

	size_t col_start = tokens.tokens[i].col+1+offset;
	size_t col_end = 1;
	string str = tokens.line(i);
	for (size_t j = 0; j < str.size() && j < tokens.tokens[i].col+offset; j++)
	{
		if (str[j] != '\t')
		{
			str[j] = ' ';
			col_end++;
		}
		else
			col_end+=8;
	}
	str.resize(tokens.tokens[i].col+offset, ' ');

#ifdef DEBUG
	cout << debug_file << ":" << debug_line << ":";
#endif
	cout << tokens.file(i) << ":" << tokens.tokens[i].line+1 << ":" << col_start << "-" << col_end << ": error: " << error << endl;
	if (note != "")
	{
#ifdef DEBUG
		cout << debug_file << ":" << debug_line << ":";
#endif
		cout << tokens.file(i) << ":" << tokens.tokens[i].line+1 << ":" << col_start << "-" << col_end << ": note: " << note << endl;
	}
	cout << tokens.line(i) << endl;
	cout << str << "^" << endl;

	num_errors++;
}

void warning(tokenizer &tokens, string warning, string note, string debug_file, int debug_line, int offset)
{
	size_t i = tokens.index < tokens.tokens.size() ? tokens.index : 0;

	size_t col_start = tokens.tokens[i].col+1+offset;
	size_t col_end = 1;
	string str = tokens.line(i);
	for (size_t j = 0; j < str.size() && j < tokens.tokens[i].col+offset; j++)
	{
		if (str[j] != '\t')
		{
			str[j] = ' ';
			col_end++;
		}
		else
			col_end+=8;
	}
	str.resize(tokens.tokens[i].col+offset, ' ');

#ifdef DEBUG
	cout << debug_file << ":" << debug_line << ":";
#endif
	cout << tokens.file(i) << ":" << tokens.tokens[i].line+1 << ":" << col_start << "-" << col_end << ": warning: " << warning << endl;
	if (note != "")
	{
#ifdef DEBUG
		cout << debug_file << ":" << debug_line << ":";
#endif
		cout << tokens.file(i) << ":" << tokens.tokens[i].line+1 << ":" << col_start << "-" << col_end << ": note: " << note << endl;
	}
	cout << tokens.line(i) << endl;
	cout << str << "^" << endl;

	num_warnings++;
}

void note(tokenizer &tokens, string note, string debug_file, int debug_line, int offset)
{
	size_t i = tokens.index < tokens.tokens.size() ? tokens.index : 0;

	size_t col_start = tokens.tokens[i].col+1+offset;
	size_t col_end = 1;
	string str = tokens.line(i);
	for (size_t j = 0; j < str.size() && j < tokens.tokens[i].col+offset; j++)
	{
		if (str[j] != '\t')
		{
			str[j] = ' ';
			col_end++;
		}
		else
			col_end+=8;
	}
	str.resize(tokens.tokens[i].col+offset, ' ');

#ifdef DEBUG
	cout << debug_file << ":" << debug_line << ":";
#endif
	cout << tokens.file(i) << ":" << tokens.tokens[i].line+1 << ":" << col_start << "-" << col_end << ": note: " << note << endl;
	cout << tokens.line(i) << endl;
	cout << str << "^" << endl;

	num_notes++;
}

void log(tokenizer &tokens, string log, string debug_file, int debug_line, int offset)
{
	if (verbose)
	{
		size_t i = tokens.index+1 < tokens.tokens.size() ? tokens.index+1 : 0;
	#ifdef DEBUG
		cout << debug_file << ":" << debug_line << ":";
	#endif
		cout << tokens.file(i) << ":" << tokens.tokens[i].line+1 << ":" << tokens.tokens[i].col+1+offset << ":\t" << log << endl;

		num_log++;
	}
}

void internal(string location, string internal, string debug_file, int debug_line)
{
	cout << debug_file << ":" << debug_line << ":";
	if (location != "")
		cout << location << ": ";
	cout << "internal failure: " << internal << endl;
	num_internal++;
}

void error(string location, string error, string note, string debug_file, int debug_line)
{
#ifdef DEBUG
	cout << debug_file << ":" << debug_line << ":";
#endif
	if (location != "")
		cout << location << ": ";
	cout << "error: " << error << endl;
	if (note != "")
	{
#ifdef DEBUG
		cout << debug_file << ":" << debug_line << ":";
#endif
		if (location != "")
			cout << location << ": ";
		cout << "note: " << note << endl;
	}

	num_errors++;
}

void warning(string location, string warning, string note, string debug_file, int debug_line)
{
#ifdef DEBUG
	cout << debug_file << ":" << debug_line << ":";
#endif
	if (location != "")
		cout << location << ": ";
	cout << "warning: " << warning << endl;
	if (note != "")
	{
#ifdef DEBUG
		cout << debug_file << ":" << debug_line << ":";
#endif
		if (location != "")
			cout << location << ": ";
		cout << "note: " << note << endl;
	}

	num_warnings++;
}

void note(string location, string note, string debug_file, int debug_line)
{
#ifdef DEBUG
	cout << debug_file << ":" << debug_line << ":";
#endif
	if (location != "")
		cout << location << ": ";
	cout << "note: " << note << endl;

	num_notes++;
}

void log(string location, string log, string debug_file, int debug_line)
{
	if (verbose)
	{
	#ifdef DEBUG
		cout << debug_file << ":" << debug_line << ":";
		if (location == "")
			cout << "\t";
	#endif
		if (location != "")
			cout << location << ":\t";
		cout << log << endl;

		num_log++;
	}
}

void progress(string location, string log, string debug_file, int debug_line)
{
	cout << "\r";
#ifdef DEBUG
	cout << debug_file << ":" << debug_line << ":";
	if (location == "")
		cout << "\t";
#endif
	if (location != "")
		cout << location << ":\t";
	cout << log << "                    ";
	cout.flush();
}

void done_progress()
{
	cout << "\r";
	cout.flush();
}

void complete()
{
	cout << "completed with ";
	if (num_internal > 0)
		cout << num_internal << " internal failures, ";
	cout << num_errors << " errors and " << num_warnings << " warnings" << endl;
}

bool is_clean()
{
	return (num_internal == 0 && num_errors == 0);
}

void set_verbose(bool value)
{
	verbose = value;
}

void unset_verbose()
{
	verbose = false;
}

bool get_verbose()
{
	return verbose;
}
