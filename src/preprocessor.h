/*
 * preprocessor.h
 *
 *  Created on: Apr 17, 2014
 *      Author: nbingham
 */

#include "tokenizer.h"

#ifndef preprocessor_h
#define preprocessor_h

struct program;

/** This structure handles all preprocessor commands (the only one currently being #include). 
  *  Since preprocessor commands are closely tied to how the rest of the program is parsed, this 
  *  structure works closely with the tokenizer. Any new commands will probably require modification
  *  of the tokenizer structure as well.
  */
struct preprocessor
{
	preprocessor();
	preprocessor(tokenizer &tokens, vector<string> include_dirs, program &prgm);
	~preprocessor();

	static bool is_next(tokenizer &tokens, size_t i = 1);
	void parse(tokenizer &tokens, vector<string> include_dirs, program &prgm);
};

#endif
