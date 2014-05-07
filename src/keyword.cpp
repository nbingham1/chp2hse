/*
 * keyword.cpp
 *
 *  Created on: Jan 31, 2013
 *      Author: nbingham
 */

#include "keyword.h"
#include "syntax/process.h"
#include "syntax/operator.h"
#include "syntax/record.h"
#include "syntax/channel.h"
#include "message.h"

keyword::keyword()
{
	name = "";
	_kind = "keyword";
	start_token = 0;
	end_token = 0;
}
keyword::keyword(string name)
{
	this->name = name;
	this->_kind = "keyword";
	start_token = 0;
	end_token = 0;
}
keyword::~keyword()
{
	name = "";
	_kind = "keyword";
}

string keyword::kind()
{
	return _kind;
}

keyword &keyword::operator=(keyword k)
{
	name = k.name;
	return *this;
}

type_space::type_space()
{

}

type_space::~type_space()
{
	for (list<keyword*>::iterator t = types.begin(); t != types.end(); t++)
		delete *t;
	types.clear();
}

void type_space::insert(tokenizer &tokens, keyword *type)
{
	if (type != NULL)
	{
		keyword *check = find(type->name);
		if (check != NULL)
		{
			int temp = tokens.index;
			tokens.index = type->start_token;
			error(tokens, "conflicting definition of '" + type->name + "'", "", __FILE__, __LINE__);
			tokens.index = check->start_token;
			error(tokens, "previously defined here", "", __FILE__, __LINE__);
			tokens.index = temp;
		}
		else
		{
			types.push_back(type);

			if (type->kind() == "channel")
			{
				for (size_t i = 0; i < ((channel*)type)->send.size(); i++)
					if (((channel*)type)->send[i] != NULL)
						types.push_back(((channel*)type)->send[i]);
				for (size_t i = 0; i < ((channel*)type)->recv.size(); i++)
					if (((channel*)type)->recv[i] != NULL)
						types.push_back(((channel*)type)->recv[i]);
				if (((channel*)type)->probe != NULL)
					types.push_back(((channel*)type)->probe);
			}
		}
	}
}

keyword *type_space::find(string name)
{
	for (list<keyword*>::iterator t = types.begin(); t != types.end(); t++)
		if ((*t)->name == name)
			return *t;

	return NULL;
}

bool type_space::contains(string name)
{
	return (find(name) != NULL);
}

string type_space::closest(string name)
{
	int dist = 9999999;
	string result = "";
	size_t count = 0;

	for (list<keyword*>::iterator t = types.begin(); t != types.end(); t++)
	{
		int temp = 0;
		size_t j = 0;
		for (j = 0; j < name.size() && j < (*t)->name.size(); j++)
			if (name[j] != (*t)->name[j])
				temp += name.size() - j;

		for (; j < (*t)->name.size(); j++)
			temp += j - name.size();

		for (; j < name.size(); j++)
			temp += name.size() - j;

		if (temp < dist)
		{
			result = "'" + (*t)->name + "'";
			dist = temp;
			count = 1;
		}
		else if (temp == dist)
		{
			if (count == 1)
				result = "or " + result;
			result = "'" + (*t)->name + "' ";
			count++;
		}
	}

	return result;
}

ostream &operator<<(ostream &os, keyword *key)
{
	if (key == NULL)
		os << "null";
	else if (key->kind() == "process")
		os << *((process*)key);
	else if (key->kind() == "operator")
		os << *((operate*)key);
	else if (key->kind() == "record")
		os << *((record*)key);
	else if (key->kind() == "channel")
		os << *((channel*)key);
	else
		os << key->name;

	return os;
}
