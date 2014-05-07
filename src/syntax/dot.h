/*
 * dot.h
 *
 *  Created on: Feb 3, 2014
 *      Author: nbingham
 */

#include "../common.h"

#ifndef dot_h
#define dot_h

struct dot_a_list
{
	dot_a_list();
	~dot_a_list();

	vector<pair<string, string> > as;

	void print(ostream &fout = cout, string pre = "");
};

struct dot_attr_list
{
	dot_attr_list();
	~dot_attr_list();

	vector<dot_a_list> attrs;

	void print(ostream &fout = cout, string pre = "");
};

struct dot_node_id
{
	dot_node_id();
	dot_node_id(string str);
	~dot_node_id();

	string id;
	string port;
	string compass_pt;

	void print(ostream &fout = cout, string pre = "");

	dot_node_id &operator=(dot_node_id node_id);
};

struct dot_stmt;

struct dot_stmt_list
{
	dot_stmt_list();
	~dot_stmt_list();

	vector<dot_stmt> stmts;

	void print(ostream &fout = cout, string pre = "");
};

struct dot_stmt
{
	dot_stmt();
	~dot_stmt();

	string stmt_type;
	string attr_type;
	vector<dot_node_id> node_ids;
	dot_attr_list attr_list;
	string id;
	dot_stmt_list stmt_list;

	void print(ostream &fout = cout, string pre = "");

	dot_stmt &operator=(dot_stmt stmt);
};

struct dot_graph
{
	dot_graph();
	~dot_graph();

	bool strict;
	string type;
	string id;
	dot_stmt_list stmt_list;

	void print(ostream &fout = cout, string pre = "");

	void clear();
};

struct dot_graph_cluster
{
	dot_graph_cluster();
	~dot_graph_cluster();

	vector<dot_graph> graphs;

	void print(ostream &fout = cout, string pre = "");
};

#endif
