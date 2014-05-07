/*
 * dot.cpp
 *
 *  Created on: Feb 3, 2014
 *      Author: nbingham
 */

#include "dot.h"

dot_a_list::dot_a_list()
{
}

dot_a_list::~dot_a_list()
{
}

void dot_a_list::print(ostream &fout, string pre)
{
	for (size_t i = 0; i < as.size(); i++)
	{
		if (i != 0)
			fout << " ";
		fout << "\"" << as[i].first << "\"=\"" << as[i].second << "\"";
	}
}

dot_attr_list::dot_attr_list()
{
}

dot_attr_list::~dot_attr_list()
{
}

void dot_attr_list::print(ostream &fout, string pre)
{
	for (size_t i = 0; i < attrs.size(); i++)
	{
		fout << "[";
		attrs[i].print(fout, pre);
		fout << "]";
	}
}

dot_node_id::dot_node_id()
{
}

dot_node_id::dot_node_id(string str)
{
	id = str;
}

dot_node_id::~dot_node_id()
{
}

void dot_node_id::print(ostream &fout, string pre)
{
	fout << id;
	if (port != "")
		fout << ":" << port;
	if (compass_pt != "")
		fout << ":" << compass_pt;
}

dot_node_id &dot_node_id::operator=(dot_node_id node_id)
{
	id = node_id.id;
	port = node_id.port;
	compass_pt = node_id.compass_pt;
	return *this;
}

dot_stmt::dot_stmt()
{
}

dot_stmt::~dot_stmt()
{

}

void dot_stmt::print(ostream &fout, string pre)
{
	if (stmt_type == "subgraph")
	{
		fout << "subgraph " << id << endl;
		fout << pre << "{" << endl;
		stmt_list.print(fout, pre + "\t");
		fout << pre << "}";
	}
	else
	{
		for (size_t i = 0; i < node_ids.size(); i++)
		{
			if (i != 0)
				fout << "->";
			node_ids[i].print(fout, pre);
		}

		fout << attr_type;
		attr_list.print(fout, pre);
		fout << ";";
	}
}

dot_stmt &dot_stmt::operator=(dot_stmt stmt)
{
	stmt_type = stmt.stmt_type;
	attr_type = stmt.attr_type;
	node_ids = stmt.node_ids;
	attr_list = stmt.attr_list;
	id = stmt.id;
	stmt_list = stmt.stmt_list;
	return *this;
}

dot_stmt_list::dot_stmt_list()
{

}

dot_stmt_list::~dot_stmt_list()
{

}

void dot_stmt_list::print(ostream &fout, string pre)
{
	for (size_t i = 0; i < stmts.size(); i++)
	{
		fout << pre;
		stmts[i].print(fout, pre);
		fout << endl;
	}
}

dot_graph::dot_graph()
{
	strict = false;
	type = "";
}

dot_graph::~dot_graph()
{
	stmt_list.stmts.clear();
}

void dot_graph::print(ostream &fout, string pre)
{
	if (strict)
		fout << pre << "strict ";
	else
		fout << pre;

	fout << type << " " << id << endl << pre << "{" << endl;
	stmt_list.print(fout, pre + "\t");
	fout << pre << "}" << endl;
}

void dot_graph::clear()
{
	strict = false;
	type.clear();
	id.clear();
	stmt_list.stmts.clear();
}

dot_graph_cluster::dot_graph_cluster()
{

}

dot_graph_cluster::~dot_graph_cluster()
{

}

void dot_graph_cluster::print(ostream &fout, string pre)
{
	for (size_t i = 0; i < graphs.size(); i++)
	{
		graphs[i].print(fout, pre);
		fout << "\n";
	}
}

