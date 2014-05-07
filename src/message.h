/*
 * message.h
 *
 *  Created on: May 5, 2014
 *      Author: nbingham
 */

#include "common.h"

#ifndef message_h
#define message_h

struct tokenizer;

void internal(tokenizer &tokens, string internal, string debug_file, int debug_line);
void error(tokenizer &tokens, string error, string note, string debug_file, int debug_line);
void warning(tokenizer &tokens, string warning, string note, string debug_file, int debug_line);
void note(tokenizer &tokens, string note, string debug_file, int debug_line);
void log(tokenizer &tokens, string log, string debug_file, int debug_line);

void internal(string location, string internal, string debug_file, int debug_line);
void error(string location, string error, string note, string debug_file, int debug_line);
void warning(string location, string warning, string note, string debug_file, int debug_line);
void note(string location, string note, string debug_file, int debug_line);
void log(string location, string log, string debug_file, int debug_line);

void complete();
bool is_clean();

#endif
