/*
 * preprocessor.cpp
 *
 *  Created on: Apr 17, 2014
 *      Author: nbingham
 */

#include "preprocessor.h"
#include "program.h"
#include "message.h"

preprocessor::preprocessor()
{

}

preprocessor::preprocessor(tokenizer &tokens, program &prgm)
{
	parse(tokens, prgm);
}

preprocessor::~preprocessor()
{

}

/** Check to see if the next i'th token is a preprocessor command.
  */
bool preprocessor::is_next(tokenizer &tokens, size_t i)
{
	return (tokens.peek(i) == "#");
}

/** Parse a preprocessor command assuming that there is a preprocessor command next in the token stream.
  */
void preprocessor::parse(tokenizer &tokens, program &prgm)
{
	tokens.increment();
	tokens.push_expected("#");
	tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	tokens.increment();
	tokens.push_expected("include");
	// Add the hooks for new commands here
	string token = tokens.syntax(__FILE__, __LINE__);
	tokens.decrement();

	if (token == "include")
	{
		string filename = tokens.next();
		if (filename.size() == 0 || filename[0] != '\"' || filename[filename.size()-1] != '\"')
			error(tokens, "file names must be contained within quotes", "", __FILE__, __LINE__);
		else
			filename = filename.substr(1, filename.size()-2);

		if (tokens.file(tokens.index).find_last_of("\\/") != string::npos)
			filename = tokens.file(tokens.index).substr(0, tokens.file(tokens.index).find_last_of("\\/")+1) + filename;

		if (find(prgm.loading.begin(), prgm.loading.end(), filename) != prgm.loading.end())
			error(tokens, "cycle found in include graph", "", __FILE__, __LINE__);
		else if (find(prgm.loaded.begin(), prgm.loaded.end(), filename) == prgm.loaded.end())
		{
			ifstream fin;
			fin.open(filename.c_str(), ios::binary | ios::in);
			if (!fin.is_open())
				error(tokens, "file not found '" + filename + "'", "", __FILE__, __LINE__);
			else
			{
				prgm.loading.push_back(filename);
				fin.seekg(0, ios::end);
				size_t size = fin.tellg();
				string buffer(size, ' ');
				fin.seekg(0, ios::beg);
				fin.read(&buffer[0], size);
				fin.clear();
				tokenizer recurse(filename, buffer);
				if (!prgm.parse(recurse))
					log(tokens, tokens.line(tokens.index), __FILE__, __LINE__);
				prgm.loading.pop_back();
			}
		}
	}
}
