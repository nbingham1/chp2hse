/*
 * chp.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: Ned Bingham and Nicholas Kramer
 *
 *  DO NOT DISTRIBUTE
 */

#include "common.h"
#include "program.h"
#include "syntax/process.h"
#include "message.h"

int main(int argc, char **argv)
{
	vector<string> files;
	bool flat = false;
	bool stream = false;
	vector<string> process_names;
	for (int i = 1; i < argc; i++)
	{
		if (strncmp(argv[i], "--help", 6) == 0 || strncmp(argv[i], "-h", 2) == 0)			// Help
		{
			cout << "Usage: chp2hse [options] file" << endl;
			cout << "Options:" << endl;
			cout << " -h,--help\t\t\t\tDisplay this information" << endl;
			cout << " -v,--version\t\t\t\tDisplay compiler version information" << endl;
			cout << " -s,--stream\t\t\t\tPrint HSE to standard out" << endl;
			cout << " -f,--flat\t\t\t\tPrint the human readable HSE" << endl;
			cout << " -p,--process <process name>,...\tOnly print the HSE of the specified processes. If this option is not used, the HSE for all processes are printed" << endl;
			return 0;
		}
		else if (strncmp(argv[i], "--version", 9) == 0 || strncmp(argv[i], "-v", 2) == 0)	// Version Information
		{
			cout << "chp2hse 1.0.0" << endl;
			cout << "Copyright (C) 2013 Sol Union." << endl;
			cout << "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << endl;
			cout << endl;
			return 0;
		}
		else if (strncmp(argv[i], "--flat", 6) == 0 || strncmp(argv[i], "-f", 2) == 0)
			flat = true;
		else if (strncmp(argv[i], "--stream", 8) == 0 || strncmp(argv[i], "-s", 2) == 0)
			stream = true;
		else if (strncmp(argv[i], "--process", 9) == 0 || strncmp(argv[i], "-p", 2) == 0)
		{
			if (i+1 < argc)
			{
				i++;
				string temp = argv[i];
				size_t j, k;
				for (j = 0, k = temp.find_first_of(","); j < temp.size() && k != string::npos; j=k+1, k = temp.find_first_of(",", j))
					process_names.push_back(temp.substr(j, k-j));
				process_names.push_back(temp.substr(j));
			}
			else
				error("", "expected process names after --process or -p", "", __FILE__, __LINE__);
		}
		else
			files.push_back(argv[i]);
	}

	if (files.size() > 0)
	{
		program prgm;
		for (size_t i = 0; i < files.size(); i++)
		{
			ifstream fin;
			fin.open(files[i].c_str(), ios::binary | ios::in);
			if (!fin.is_open())
			{
				error("", "file not found '" + files[i] + "'", "", __FILE__, __LINE__);
				return false;
			}
			else
			{
				prgm.loading.push_back(files[i]);
				fin.seekg(0, fin.end);
				size_t size = fin.tellg();
				string buffer(size, ' ');
				fin.seekg(0, fin.beg);
				fin.read(&buffer[0], size);
				fin.close();
				tokenizer recurse(files[i], buffer);
				prgm.parse(recurse);
				prgm.loading.pop_back();
			}
		}

		if (is_clean())
		{
			for (list<keyword*>::iterator key = prgm.types.types.begin(); key != prgm.types.types.end(); key++)
			{
				if ((*key)->kind() == "process")
				{
					string name = (*key)->name.substr(0, (*key)->name.find_first_of("()"));
					if (process_names.size() == 0 || find(process_names.begin(), process_names.end(), name) != process_names.end())
					{
						ostream *fout = &cout;
						ofstream file;
						if (!stream)
						{
							string filename = name + (flat ? ".hse" : ".dot");
							file.open(filename.c_str());
							fout = &file;
						}

						if (flat)
							((process*)(*key))->print(*fout);
						else
						{
							((process*)(*key))->build_hse();
							((process*)(*key))->hse.print(*fout);
						}

						if (file.is_open())
							file.close();
					}
				}
			}
		}

		complete();
	}
	else
	{
		cout << "Usage: chp2hse [options] file" << endl;
		cout << "Options:" << endl;
		cout << " -h,--help\t\t\t\tDisplay this information" << endl;
		cout << " -v,--version\t\t\t\tDisplay compiler version information" << endl;
		cout << " -s,--stream\t\t\t\tPrint HSE to standard out" << endl;
		cout << " -f,--flat\t\t\t\tPrint the human readable HSE" << endl;
		cout << " -p,--process <process name>,...\tOnly print the HSE of the specified processes. If this option is not used, the HSE for all processes are printed" << endl;
	}

	return 0;
}
