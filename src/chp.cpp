/*
 * 	Variable Definition
 * 	x1,x2,...,xn:=E1,E2,...,En
 *
 *  x+
 *  x-
 *
 *	Expression Operators
 *  | & ~ < > == ~=
 *
 *  Composition Operators
 *  ; || *
 *
 *	Order of Operations
 *	()
 *
 *	Loops
 * 	*[g1->s1[]g2->s2[]...[]gn->sn]
 *  *[g1->s1|g2->s2|...|gn->sn]
 *  *[s1]
 *
 *	Conditionals
 *   [g1->s1[]g2->s2[]...[]gn->sn]
 *   [g1->s1|g2->s2|...|gn->sn]
 *   [g1]
 *
 *  Replication
 *   <op i : n..m : s(i)>
 *
 *	Communication
 * 	 x!y
 * 	 x?y
 * 	 @x
 *
 *	Miscellaneous
 * 	 skip
 *
 *	Assertion
 * 	 {...}
 *
 *	Process Definition and Function Block Definitions
 * 	proc ...(...){...}
 *
 * 	Record Definition
 * 	struct ...{...}
 *
 *	Data Types
 * 	int<...> ...
 *
 *	Preprocessor
 * 	 #
 *
 */

#include "common.h"
#include "keyword.h"
#include "variable.h"
#include "instruction.h"
#include "space.h"
#include "record.h"
#include "block.h"
#include "process.h"
#include "channel.h"


/* This structure describes a whole program. It contains a record of all
 * of the types described in this program and all of the global variables
 * defined in this program. It also contains a list of all of the errors
 * produced during the compilation and a list of all of the production rules
 * that result from this compilation.
 */
struct program
{
	program()
	{
		type_space.insert(pair<string, keyword*>("int", new keyword("int")));
	}
	program(string chp)
	{
		parse(chp);
	}
	~program()
	{
		map<string, keyword*>::iterator i;
		for (i = type_space.begin(); i != type_space.end(); i++)
			delete i->second;

		type_space.clear();
	}

	map<string, keyword*>	type_space;
	variable				main;
	list<string>			prs;
	list<string>			errors;

	program &operator=(program p)
	{
		type_space = p.type_space;
		main = p.main;
		prs = p.prs;
		errors = p.errors;
		return *this;
	}

	void parse(string chp)
	{
		string::iterator i, j;
		string cleaned_chp = "";
		string word;
		string error;
		int error_start, error_len;

		process *p;
		record *r;

		// Define the basic types. In this case, 'int'
		type_space.insert(pair<string, keyword*>("int", new keyword("int")));

		// remove extraneous whitespace
		for (i = chp.begin(); i != chp.end(); i++)
		{
			if (!sc(*i))
				cleaned_chp += *i;
			else if (nc(*(i-1)) && (i == chp.end()-1 || nc(*(i+1))))
				cleaned_chp += ' ';
		}

		// split the program into records and processes
		int depth[3] = {0};
		for (i = cleaned_chp.begin(), j = cleaned_chp.begin(); i != cleaned_chp.end(); i++)
		{
			if (*i == '(')
				depth[0]++;
			else if (*i == '[')
				depth[1]++;
			else if (*i == '{')
				depth[2]++;
			else if (*i == ')')
				depth[0]--;
			else if (*i == ']')
				depth[1]--;
			else if (*i == '}')
				depth[2]--;

			// Are we at the end of a record or process definition?
			if (depth[0] == 0 && depth[1] == 0 && depth[2] == 0 && *i == '}')
			{
				// Make sure this isn't vacuous
				if (i-j+1 > 0)
				{
					// Is this a process?
					if (cleaned_chp.compare(j-cleaned_chp.begin(), 5, "proc ") == 0)
					{
						p = new process(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), type_space);
						type_space.insert(pair<string, process*>(p->name, p));
					}
					// This isn't a process, is it a record?
					else if (cleaned_chp.compare(j-cleaned_chp.begin(), 7, "record ") == 0)
					{
						r = new record(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), type_space, "");
						type_space.insert(pair<string, record*>(r->name, r));
					}
					// Is it a channel definition?
					else if (cleaned_chp.compare(j-cleaned_chp.begin(), 8, "chan ") == 0)
					{
						r = new channel(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), type_space, "");
						type_space.insert(pair<string, record*>(r->name, r));
					}
					// This isn't either a process or a record, this is an error.
					else
					{
						error = "Error: CHP block outside of process.\nIgnoring block:\t";
						error_start = j-cleaned_chp.begin();
						error_len = min(min(cleaned_chp.find("proc ", error_start), cleaned_chp.find("record ", error_start)), cleaned_chp.find("chan ", error_start)) - error_start;
						error += cleaned_chp.substr(error_start, error_len);
						cout << error << endl;
						j += error_len;

						// Make sure we don't miss the next record or process though.
						if (cleaned_chp.compare(j-cleaned_chp.begin(), 5, "proc ") == 0)
						{
							p = new process(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), type_space);
							type_space.insert(pair<string, process*>(p->name, p));
						}
						else if (cleaned_chp.compare(j-cleaned_chp.begin(), 7, "record ") == 0)
						{
							r = new record(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), type_space, "");
							type_space.insert(pair<string, record*>(r->name, r));
						}
						else if (cleaned_chp.compare(j-cleaned_chp.begin(), 8, "chan ") == 0)
						{
							r = new channel(cleaned_chp.substr(j-cleaned_chp.begin(), i-j+1), type_space, "");
							type_space.insert(pair<string, record*>(r->name, r));
						}
					}
				}
				j = i+1;
			}
		}

		main.parse("main m()", "");
	}
};


int main(int argc, char **argv)
{
	ifstream t("test.chp");
	string prgm((istreambuf_iterator<char>(t)),
	             istreambuf_iterator<char>());
	program p(prgm);
}
