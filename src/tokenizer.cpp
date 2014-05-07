/*
 * tokenizer.cpp
 *
 *  Created on: Apr 7, 2014
 *      Author: nbingham
 */

#include "tokenizer.h"
#include "syntax/instance.h"
#include "syntax/constant.h"
#include "syntax/declaration.h"
#include "syntax/control.h"
#include "syntax/slice.h"
#include "syntax/skip.h"
#include "syntax/debug.h"
#include "syntax/expression.h"
#include "syntax/variable_name.h"
#include "syntax/type_name.h"
#include "syntax/assignment.h"
#include "syntax/process.h"
#include "syntax/operator.h"
#include "syntax/record.h"
#include "syntax/channel.h"
#include "preprocessor.h"
#include "message.h"

token::token()
{
	file = 0;
	line = 0;
	col = 0;
	size = 0;
}

token::token(size_t file, size_t line, size_t col, size_t size)
{
	this->file = file;
	this->line = line;
	this->col = col;
	this->size = size;
}

token::~token()
{

}

tokenizer::tokenizer()
{
	index = ((size_t)-1);
	compositions.push_back(vector<string>());
	compositions.back().push_back("||");
	compositions.push_back(vector<string>());
	compositions.back().push_back(";");
	compositions.push_back(vector<string>());
	compositions.back().push_back(",");

	operations.push_back(vector<string>());
	operations.back().push_back("|");
	operations.push_back(vector<string>());
	operations.back().push_back("&");
	operations.push_back(vector<string>());
	operations.back().push_back("^");
	operations.push_back(vector<string>());
	operations.back().push_back("==");
	operations.back().push_back("~=");
	operations.push_back(vector<string>());
	operations.back().push_back("<");
	operations.back().push_back(">");
	operations.back().push_back("<=");
	operations.back().push_back(">=");
	operations.push_back(vector<string>());
	operations.back().push_back("<<");
	operations.back().push_back(">>");
	operations.push_back(vector<string>());
	operations.back().push_back("+");
	operations.back().push_back("-");
	operations.push_back(vector<string>());
	operations.back().push_back("*");
	operations.back().push_back("/");
	operations.back().push_back("%");
}

tokenizer::tokenizer(string filename, string contents)
{
	index = ((size_t)-1);
	compositions.push_back(vector<string>());
	compositions.back().push_back("||");
	compositions.push_back(vector<string>());
	compositions.back().push_back(";");
	compositions.push_back(vector<string>());
	compositions.back().push_back(",");

	operations.push_back(vector<string>());
	operations.back().push_back("|");
	operations.push_back(vector<string>());
	operations.back().push_back("&");
	operations.push_back(vector<string>());
	operations.back().push_back("^");
	operations.push_back(vector<string>());
	operations.back().push_back("==");
	operations.back().push_back("~=");
	operations.push_back(vector<string>());
	operations.back().push_back("<");
	operations.back().push_back(">");
	operations.back().push_back("<=");
	operations.back().push_back(">=");
	operations.push_back(vector<string>());
	operations.back().push_back("<<");
	operations.back().push_back(">>");
	operations.push_back(vector<string>());
	operations.back().push_back("+");
	operations.back().push_back("-");
	operations.push_back(vector<string>());
	operations.back().push_back("*");
	operations.back().push_back("/");
	operations.back().push_back("%");

	insert(filename, contents);
}

tokenizer::tokenizer(string contents)
{
	index = ((size_t)-1);
	compositions.push_back(vector<string>());
	compositions.back().push_back("||");
	compositions.push_back(vector<string>());
	compositions.back().push_back(";");
	compositions.push_back(vector<string>());
	compositions.back().push_back(",");

	operations.push_back(vector<string>());
	operations.back().push_back("|");
	operations.push_back(vector<string>());
	operations.back().push_back("&");
	operations.push_back(vector<string>());
	operations.back().push_back("^");
	operations.push_back(vector<string>());
	operations.back().push_back("==");
	operations.back().push_back("~=");
	operations.push_back(vector<string>());
	operations.back().push_back("<");
	operations.back().push_back(">");
	operations.back().push_back("<=");
	operations.back().push_back(">=");
	operations.push_back(vector<string>());
	operations.back().push_back("<<");
	operations.back().push_back(">>");
	operations.push_back(vector<string>());
	operations.back().push_back("+");
	operations.back().push_back("-");
	operations.push_back(vector<string>());
	operations.back().push_back("*");
	operations.back().push_back("/");
	operations.back().push_back("%");

	insert("", contents);
}

tokenizer::~tokenizer()
{

}

void tokenizer::increment()
{
	bound.push_back(vector<string>());
	expected.push_back(vector<string>());
}


void tokenizer::decrement()
{
	bound.pop_back();
}

void tokenizer::push_bound(string s)
{
	bound.back().push_back(s);
}

void tokenizer::push_bound(vector<string> s)
{
	bound.back().insert(bound.back().end(), s.begin(), s.end());
}

void tokenizer::push_expected(string s)
{
	expected.back().push_back(s);
}

void tokenizer::push_expected(vector<string> s)
{
	expected.back().insert(expected.back().end(), s.begin(), s.end());
}

vector<vector<string> >::iterator tokenizer::in_bound(size_t i)
{
	string token = peek(i);
	for (vector<vector<string> >::iterator b = bound.begin(); b != bound.end(); b++)
		if ((find(b->begin(), b->end(), token) != b->end()) ||
			(find(b->begin(), b->end(), "[assignment]") != b->end() && assignment::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[instance]") != b->end() && instance::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[type name]") != b->end() && type_name::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[variable name]") != b->end() && variable_name::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[constant]") != b->end() && constant::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[declaration]") != b->end() && declaration::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[conditional]") != b->end() && control::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[slice]") != b->end() && slice::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[skip]") != b->end() && skip::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[debug]") != b->end() && debug::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[expression]") != b->end() && expression::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[record]") != b->end() && record::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[channel]") != b->end() && channel::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[process]") != b->end() && process::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[operator]") != b->end() && operate::is_next(*this, i)) ||
			(find(b->begin(), b->end(), "[preprocessor]") != b->end() && preprocessor::is_next(*this, i)))
			return b;
	return bound.end();
}

vector<vector<string> >::iterator tokenizer::in_expected(size_t i)
{
	string token = peek(i);
	for (vector<vector<string> >::iterator e = expected.begin(); e != expected.end(); e++)
		if ((find(e->begin(), e->end(), token) != e->end()) ||
			(find(e->begin(), e->end(), "[assignment]") != e->end() && assignment::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[instance]") != e->end() && instance::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[type name]") != e->end() && type_name::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[variable name]") != e->end() && variable_name::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[constant]") != e->end() && constant::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[declaration]") != e->end() && declaration::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[conditional]") != e->end() && control::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[slice]") != e->end() && slice::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[skip]") != e->end() && skip::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[debug]") != e->end() && debug::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[expression]") != e->end() && expression::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[record]") != e->end() && record::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[channel]") != e->end() && channel::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[process]") != e->end() && process::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[operator]") != e->end() && operate::is_next(*this, i)) ||
			(find(e->begin(), e->end(), "[preprocessor]") != e->end() && preprocessor::is_next(*this, i)))
			return e;
	return expected.end();
}

vector<vector<string> >::iterator tokenizer::in_bound(string s)
{
	for (vector<vector<string> >::iterator b = bound.begin(); b != bound.end(); b++)
		if (find(b->begin(), b->end(), s) != b->end())
			return b;
	return bound.end();
}

vector<vector<string> >::iterator tokenizer::in_expected(string s)
{
	for (vector<vector<string> >::iterator e = expected.begin(); e != expected.end(); e++)
		if (find(e->begin(), e->end(), s) != e->end())
			return e;
	return expected.end();
}

string tokenizer::bound_string()
{
	string result;
	for (size_t i = 0; i < bound.size(); i++)
	{
		result += "{";
		for (size_t j = 0; j < bound[i].size(); j++)
			result += "'" + bound[i][j] + "' ";
		result += "} ";
	}
	return result;
}

string tokenizer::expect_string()
{
	string result;
	for (size_t i = 0; i < expected.size(); i++)
	{
		result += "{";
		for (size_t j = 0; j < expected[i].size(); j++)
			result += "'" + expected[i][j] + "' ";
		result += "} ";
	}
	return result;
}

int tokenizer::count_expected()
{
	int total = 0;
	for (size_t i = 0; i < expected.size(); i++)
		total += expected[i].size();
	return total;
}

string tokenizer::syntax(string debug_file, int debug_line)
{
	size_t i = 1;
	vector<vector<string> >::iterator e = in_expected(i), b = in_bound(i);
	while (peek(i) != "" && b == bound.end() && e == expected.end())
	{
		i++;
		b = in_bound(i);
		e = in_expected(i);
	}

	bool is_struct = (e != expected.end() && find(e->begin(), e->end(), peek(i)) == e->end());

	if (i != 1 || e == expected.end())
	{
		string error_string = "expected ";
		if (e == expected.end())
		{
			for (size_t j = 0; j < expected.size(); j++)
			{
				for (size_t k = 0; k < expected[j].size(); k++)
				{
					if (j == expected.size()-1 && k == expected[j].size()-1 && count_expected() > 1)
						error_string += "or ";

					if (expected[j][k].size() > 2 && expected[j][k][0] == '[' && expected[j][k][expected[j][k].size()-1] == ']')
						error_string += expected[j][k].substr(1, expected[j][k].size()-2) + " ";
					else
						error_string += "'" + expected[j][k] + "' ";
				}
			}
			next();
			error(*this, error_string, "", debug_file, debug_line);
			prev();
			//index += i-1;
			//curr = at(index);
			expected.clear();
			return "";
		}
		else
		{
			for (size_t k = 0; k < e->size(); k++)
			{
				if (k == e->size()-1 && e->size() > 1)
					error_string += "or ";

				if (e->at(k).size() > 2 && e->at(k)[0] == '[' && e->at(k)[e->at(k).size()-1] == ']')
					error_string += e->at(k).substr(1, e->at(k).size()-2) + " ";
				else
					error_string += "'" + e->at(k) + "' ";
			}
			next();
			error(*this, error_string, "", debug_file, debug_line);
			prev();

			if (is_struct)
				index += i-1;
			else
				index += i;

			curr = at(index);
			expected.clear();
			return curr;
		}
	}
	else
	{
		expected.clear();
		if (is_struct)
			return "";
		else
			return next();
	}
}

void tokenizer::insert(string filename, string contents)
{
	files.push_back(pair<string, vector<string> >(filename, vector<string>()));

	size_t start = 0;
	size_t line = 0;
	size_t col = 0;
	size_t size = 0;

	files.back().second.push_back("");
	while (start + size < contents.size())
	{
		col += size;
		start += size;
		while (start < contents.size() && sc(contents[start]))
		{
			if (contents[start] == '\n')
			{
				files.back().second.push_back("");
				line++;
				col = 0;
				if (at(tokens.size()-1) == "//")
					tokens.pop_back();
			}
			else if (contents[start] != '\r')
			{
				files.back().second.back().push_back(contents[start]);
				col++;
			}

			start++;
		}

		size = 0;
		if (start < contents.size())
		{
			if (contents[start] >= '0' && contents[start] <= '9')
				while (start + size < contents.size() && ((contents[start + size] >= '0' && contents[start + size] <= '9') || (size == 1 && (contents[start+size] == 'x' || contents[start + size] == 'b'))))
				{
					files.back().second.back().push_back(contents[start + size]);
					size++;
				}
			else if ((contents[start] >= 'A' && contents[start] <= 'Z') || contents[start] == '_' || (contents[start] >= 'a' && contents[start] <= 'z'))
				while (start + size < contents.size() && nc(contents[start+size]) && (start + size + 1 >= contents.size() || contents[start+size] != '.' || contents[start+size+1] != '.'))
				{
					files.back().second.back().push_back(contents[start + size]);
					size++;
				}
			else if (contents[start] == '\"')
				while (start + size < contents.size() && contents[start+size] != '\n' && (size <= 1 || contents[start+size-1] != '\"' || contents[start+size-2] == '\\'))
				{
					files.back().second.back().push_back(contents[start + size]);
					size++;
				}
			else if (contents[start] == '\'')
				while (start + size < contents.size() && contents[start+size] != '\n' && (size <= 1 || contents[start+size-1] != '\'' || contents[start+size-2] == '\\'))
				{
					files.back().second.back().push_back(contents[start + size]);
					size++;
				}
			else
			{
				if ((contents[start] == ':' && contents[start+1] == '=') ||
					(contents[start] == '-' && contents[start+1] == '>') ||
					(contents[start] == '=' && contents[start+1] == '=') ||
					(contents[start] == '~' && contents[start+1] == '=') ||
					(contents[start] == '<' && contents[start+1] == '=') ||
					(contents[start] == '>' && contents[start+1] == '=') ||
					(contents[start] == '&' && contents[start+1] == '&') ||
					(contents[start] == '|' && contents[start+1] == '|') ||
					(contents[start] == '^' && contents[start+1] == '^') ||
					(contents[start] == '>' && contents[start+1] == '>') ||
					(contents[start] == '<' && contents[start+1] == '<') ||
					(contents[start] == '/' && contents[start+1] == '*') ||
					(contents[start] == '/' && contents[start+1] == '/') ||
					(contents[start] == '*' && contents[start+1] == '/') ||
					(contents[start] == '[' && contents[start+1] == ']') ||
					(contents[start] == '.' && contents[start+1] == '.') ||
					(contents[start] == '*' && contents[start+1] == '['))
				{
					files.back().second.back().push_back(contents[start + size]);
					size++;
				}

				files.back().second.back().push_back(contents[start + size]);
				size++;
			}

			if (tokens.size() == 0 || at(tokens.size()-1) != "//")
				tokens.push_back(token(files.size()-1, line, col, size));

			if (tokens.size() >= 2 && at(tokens.size()-2) == "/*")
			{
				if (at(tokens.size()-1) == "*/")
					tokens.pop_back();

				tokens.pop_back();
			}
		}
	}
}

string tokenizer::at(size_t i)
{
	return files[tokens[i].file].second[tokens[i].line].substr(tokens[i].col, tokens[i].size);
}

string tokenizer::line(size_t i)
{
	return files[tokens[i].file].second[tokens[i].line];
}

string tokenizer::file(size_t i)
{
	return files[tokens[i].file].first;
}

string tokenizer::next()
{
	if (index == ((size_t)-1) || index < tokens.size()-1)
	{
		index++;
		return (curr = at(index));
	}
	else
		return "";
}

string tokenizer::prev()
{
	if (index > 0)
	{
		index--;
		return (curr = at(index));
	}
	else
		return "";
}

string tokenizer::peek(size_t i)
{
	if (index+i < tokens.size())
		return at(index+i);
	else
		return "";
}

string tokenizer::peek_next()
{
	if (index+1 < tokens.size())
		return at(index+1);
	else
		return "";
}

string tokenizer::peek_prev()
{
	if (index-1 < tokens.size())
		return at(index-1);
	else
		return "";
}

tokenizer &tokenizer::operator=(string contents)
{
	insert("", contents);
	return *this;
}
