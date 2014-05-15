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

void internal(tokenizer &tokens, string internal, string debug_file, int debug_line, int offset = 0);
void error(tokenizer &tokens, string error, string note, string debug_file, int debug_line, int offset = 0);
void warning(tokenizer &tokens, string warning, string note, string debug_file, int debug_line, int offset = 0);
void note(tokenizer &tokens, string note, string debug_file, int debug_line, int offset = 0);
void log(tokenizer &tokens, string log, string debug_file, int debug_line, int offset = 0);

void internal(string location, string internal, string debug_file, int debug_line);
void error(string location, string error, string note, string debug_file, int debug_line);
void warning(string location, string warning, string note, string debug_file, int debug_line);
void note(string location, string note, string debug_file, int debug_line);
void log(string location, string log, string debug_file, int debug_line);
void progress(string location, string log, string debug_file, int debug_line);
void done_progress();

void complete();
bool is_clean();
void set_verbose(bool value = true);
void unset_verbose();
bool get_verbose();

#endif
