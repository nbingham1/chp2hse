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
	string working_directory = argv[0];
	working_directory = working_directory.substr(0, working_directory.find_last_of("/\\")+1);

	vector<string> files;
	string output = "";
	bool flat = false;
	bool stream = false;
	vector<string> process_names;
	vector<string> include_dirs(1, "");
	for (int i = 1; i < argc; i++)
	{
		string arg = argv[i];
		if (arg == "--help" || arg == "-h")			// Help
		{
			cout << "Usage: chp2hse [options] file" << endl;
			cout << "Options:" << endl;
			cout << " -h,--help\t\t\t\tDisplay this information" << endl;
			cout << "    --version\t\t\t\tDisplay compiler version information" << endl;
			cout << " -o\t\t\t\t\tPut all resulting files in this directory. If the directory doesn't exist, create it." << endl;
			cout << " -I\t\t\t\t\tAdd a directory in which to search for included files" << endl;
			cout << endl;
			cout << " -s,--stream\t\t\t\tPrint HSE to standard out" << endl;
			cout << " -f,--flat\t\t\t\tPrint the human readable HSE" << endl;
			cout << " -p,--process <process name>,...\tOnly print the HSE of the specified processes. If this option is not used, the HSE for all processes are printed" << endl;
			return 0;
		}
		else if (arg == "--version")	// Version Information
		{
			cout << "chp2hse 1.0.0" << endl;
			cout << "Copyright (C) 2013 Sol Union." << endl;
			cout << "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << endl;
			cout << endl;
			return 0;
		}
		else if (arg == "-o")
		{
			if (i+1 < argc)
			{
				i++;
				output = argv[i];
				struct stat status;
				int error_code = stat(output.c_str(), &status);
				if (error_code == -1)
				{
					if (mkdir(output.c_str()) != 0)
						error("", "could not create directory \'" + output + "\'", "", __FILE__, __LINE__);
				}
				else if (!(status.st_mode & S_IFDIR))
					error("", "path is not a writable directory \'" + output + "\'", "", __FILE__, __LINE__);

				if (output.size() > 0 && output[output.size()-1] != '/' && output[output.size()-1] != '\\')
					output.push_back('/');
			}
			else
				error("", "expected directory path after -o", "", __FILE__, __LINE__);
		}
		else if (arg == "-I")
		{
			if (i+1 < argc)
			{
				i++;
				include_dirs.push_back(argv[i]);

				if (include_dirs.back().size() > 0 && include_dirs.back()[include_dirs.back().size()-1] != '/' && include_dirs.back()[include_dirs.back().size()-1] != '\\')
					include_dirs.back().push_back('/');

				if ((include_dirs.back().size() < 3 || include_dirs.back().substr(1, 2) != ":\\") && (include_dirs.back().size() < 1 || include_dirs.back()[0] != '/'))
					include_dirs.back() = working_directory + include_dirs.back();
			}
			else
				error("", "expected directory path after -I", "", __FILE__, __LINE__);
		}
		else if (arg == "--flat" || arg == "-f")
			flat = true;
		else if (arg == "--stream" || arg == "-s")
			stream = true;
		else if (arg == "--process" || arg == "-p")
		{
			if (i+1 < argc)
			{
				i++;
				size_t j, k;
				for (j = 0, k = arg.find_first_of(","); j < arg.size() && k != string::npos; j=k+1, k = arg.find_first_of(",", j))
					process_names.push_back(arg.substr(j, k-j));
				process_names.push_back(arg.substr(j));
			}
			else
				error("", "expected process names after --process or -p", "", __FILE__, __LINE__);
		}
		else
			files.push_back(arg);
	}

	program prgm;

	if (files.size() > 0)
	{
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
				prgm.parse(recurse, include_dirs);
				prgm.loading.pop_back();
			}
		}
	}
	else
	{
		string pipe = "";
		string line = "";
		while (getline(cin, line))
			pipe += line + "\n";

		prgm.loading.push_back("stdin");
		tokenizer recurse("stdin", pipe);
		prgm.parse(recurse, include_dirs);
		prgm.loading.pop_back();
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
						string filename = output + name + (flat ? ".hse" : ".dot");
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

	return 0;
}
