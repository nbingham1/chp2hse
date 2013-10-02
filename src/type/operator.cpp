/*
 * process.cpp
 *
 *  Created on: Oct 29, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "../common.h"
#include "../utility.h"
#include "../syntax.h"
#include "../data.h"

#include "record.h"
#include "channel.h"
#include "process.h"
#include "operator.h"

operate::operate()
{
	name = "";
	_kind = "operate";
	is_inline = true;
	def.parent = NULL;
}

operate::operate(sstring raw, type_space *types, flag_space *flags)
{
	_kind = "operate";
	vars.types = types;
	is_inline = true;
	def.parent = NULL;
	this->flags = flags;

	parse(raw);

	types->insert(pair<sstring, operate*>(name, this));
}

operate::~operate()
{
	name = "";
	_kind = "operate";
	is_inline = true;
	def.parent = NULL;

	vars.clear();
}

operate &operate::operator=(operate p)
{
	def = p.def;
	prs = p.prs;
	vars = p.vars;
	flags = p.flags;
	return *this;
}

void operate::parse(sstring raw)
{
	chp = raw;

	int name_start = 0;
	int name_end = chp.find_first_of("(");
	int input_start = chp.find_first_of("(")+1;
	int input_end = chp.find_first_of(")");
	int sequential_start = chp.find_first_of("{")+1;
	int sequential_end = chp.length()-1;
	sstring io_sequential;
	sstring::iterator i, j;

	smap<sstring, variable> temp;
	smap<sstring, variable>::iterator vi, vj;
	type_space::iterator ti;
	list<sstring>::iterator ii, ij;

	name = chp.substr(name_start, name_end - name_start);
	io_sequential = chp.substr(input_start, input_end - input_start);

	if (flags->log_base_hse())
	{
		(*flags->log_file) << "Operator:\t" << chp << endl;
		(*flags->log_file) << "\tName:\t" << name << endl;
		(*flags->log_file) << "\tInputs:\t" << io_sequential << endl;
	}

	vars.insert(variable("reset", "node", 1, true, flags));

	for (i = io_sequential.begin(), j = io_sequential.begin(); i != io_sequential.end(); i++)
	{
		if (*(i+1) == ',' || i+1 == io_sequential.end())
		{
			expand_instantiation(NULL, io_sequential.substr(j-io_sequential.begin(), i+1 - j), &vars, &args, flags, false);
			j = i+2;
		}
	}

	if (args.size() > 3)
		cerr << "Error: Operators can have at most two inputs and one output." << endl;

	def.init(chp.substr(sequential_start, sequential_end - sequential_start), &vars, flags);

	variable *tv;

	name += "(";
	ij = args.begin();
	ij++;
	for (ii = ij; ii != args.end(); ii++)
	{
		if (ii != ij)
			name += ",";
		tv = vars.find(*ii);

		if (tv != NULL)
		{
			if (tv->driven)
				cerr << "Error: Input " << *ii << " driven in " << chp << endl;

			name += tv->type;
			if (tv->type == "node" && tv->fixed)
				name += "<" + sstring(tv->width) + ">";
		}
	}
	name += ")";

	if (flags->log_base_hse())
	{
		(*flags->log_file) << "\tVariables:" << endl;
		vars.print("\t\t", flags->log_file);
		(*flags->log_file) << "\tHSE:" << endl;
		def.print_hse("\t\t", flags->log_file);
		(*flags->log_file) << endl << endl;
	}
}

void operate::print_prs(ostream *fout, sstring prefix, svector<sstring> driven)
{
	smap<sstring, variable>::iterator vi;
	type_space::iterator ki;
	for (int i = 0; i < prs.rules.size(); i++)
		if (find(driven.begin(), driven.end(), prefix + vars.get_name(i)) == driven.end() && prs[i].vars != NULL)
			prs[i].print(*fout, prefix);
}

