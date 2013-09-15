/*
 * petri.cpp
 *
 *  Created on: May 12, 2013
 *      Author: nbingham
 */

#include "petri.h"
#include "variable_space.h"
#include "../syntax/instruction.h"
#include "../syntax/assignment.h"
#include "../syntax/sequential.h"
#include "../syntax/parallel.h"
#include "../syntax/rule_space.h"

node::node()
{
	owner = NULL;
	assumptions = 1;
}

node::node(logic index, bool active, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	this->index = index;
	this->active = active;
	this->definitely_invacuous = false;
	this->possibly_vacuous = true;
	this->definitely_vacuous = false;
	this->pbranch = pbranch;
	this->cbranch = cbranch;
	this->owner = owner;
	assumptions = 1;
}

node::~node()
{
	owner = NULL;
}

bool node::is_in_tail(int idx)
{
	return (find(tail.begin(), tail.end(), idx) != tail.end());
}

void node::add_to_tail(int idx)
{
	tail.push_back(idx);
	unique(&tail);
}

void node::add_to_tail(vector<int> idx)
{
	tail.insert(tail.end(), idx.begin(), idx.end());
	unique(&tail);
}

void node::apply_mutables()
{
	map<int, logic>::iterator ji;
	for (ji = mutables.begin(); ji != mutables.end(); ji++)
		if ((index & ji->second) != index)
			index = index.hide(ji->first);
}

petri::petri()
{
	pbranch_count = 0;
	cbranch_count = 0;
	prs = NULL;
}

petri::~petri()
{
	prs = NULL;
}

int petri::new_transition(logic root, bool active, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int ret = T.size() | 0x80000000;
	T.push_back(node(root, active, pbranch, cbranch, owner));
	Wp.addr();
	Wn.addr();
	return ret;
}

vector<int> petri::new_transitions(vector<logic> root, bool active, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	vector<int> result;
	for (int i = 0; i < (int)root.size(); i++)
		result.push_back(new_transition(root[i], active, pbranch, cbranch, owner));
	return result;
}

int petri::new_place(logic root, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int ret = S.size();
	S.push_back(node(root, false, pbranch, cbranch, owner));
	Wp.addc();
	Wn.addc();
	return ret;
}

int petri::insert_transition(int from, logic root, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int t = new_transition(root, (owner != NULL && owner->kind() == "assignment"), pbranch, cbranch, owner);
	if (is_trans(from))
		cout << "Error: Illegal arc {T[" << index(from) << "], T[" << index(t) << "]}." << endl;
	else
	{
		Wn[from][index(t)]++;
		S[from].active = S[from].active || T[index(t)].active;
	}

	return t;
}

int petri::insert_transition(vector<int> from, logic root, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int t = new_transition(root, (owner != NULL && owner->kind() == "assignment"), pbranch, cbranch, owner);
	for (size_t i = 0; i < from.size(); i++)
	{
		if (is_trans(from[i]))
			cout << "Error: Illegal arc {T[" << index(from[i]) << "], T[" << index(t) << "]}." << endl;
		else
		{
			Wn[from[i]][index(t)]++;
			S[from[i]].active = S[from[i]].active || T[index(t)].active;
		}
	}

	return t;
}

void petri::insert_sv_at(int a, logic root)
{
	map<int, int> pbranch, cbranch;
	instruction *owner;
	int t, p;

	if ((*this)[arcs[a].first].pbranch.size() > (*this)[arcs[a].second].pbranch.size() ||
		(*this)[arcs[a].first].cbranch.size() > (*this)[arcs[a].second].cbranch.size())
	{
		pbranch = (*this)[arcs[a].first].pbranch;
		cbranch = (*this)[arcs[a].first].cbranch;
		owner = (*this)[arcs[a].first].owner;
	}
	else
	{
		pbranch = (*this)[arcs[a].second].pbranch;
		cbranch = (*this)[arcs[a].second].cbranch;
		owner = (*this)[arcs[a].second].owner;
	}

	t = new_transition(root, true, pbranch, cbranch, owner);
	p = new_place(root, pbranch, cbranch, owner);

	if (is_trans(arcs[a].first) && is_place(arcs[a].second))
	{
		Wp[index(arcs[a].second)][index(arcs[a].first)] = 0;
		connect(arcs[a].first, p);
		connect(p, t);
		connect(t, arcs[a].second);
		arcs[a].first = t;
	}
	else if (is_place(arcs[a].first) && is_trans(arcs[a].second))
	{
		Wn[index(arcs[a].first)][index(arcs[a].second)] = 0;
		connect(arcs[a].first, t);
		connect(t, p);
		connect(p, arcs[a].second);
		arcs[a].first = p;
	}
}

void petri::insert_sv_before(int from, logic root)
{
	map<int, int>::iterator bi, bj;
	vector<pair<int, int> >::iterator ai;
	vector<int> input_places = input_nodes(from);
	int next;
	int i;
	instruction *ins = NULL, *prev;
	sequential *s = NULL;
	assignment *a = NULL;
	parallel *p = NULL;

	for (i = 0; i < (int)input_places.size(); i++)
		Wn[input_places[i]][index(from)] = 0;
	/*ins = T[index(from)].owner;
	prev = NULL;
	while (ins != NULL && ins->kind() != "sequential" && ins->kind() != "parallel")
	{
		prev = ins;
		ins = ins->parent;
	}

	if (ins != NULL && ins->kind() == "sequential")
	{
		s = (sequential*)ins;
		a = new assignment(s, vars->get_name(values.var(root)) + (values.high(root) ? "+" : "-"), vars, s->tab, s->verbosity);
		s->instrs.insert(find(s->instrs.begin(), s->instrs.end(), prev), a);
	}
	else if (ins != NULL && ins->kind() == "parallel")
	{
		p = (parallel*)ins;
		s = new sequential();
		a = new assignment(s, vars->get_name(values.var(root)) + (values.high(root) ? "+" : "-"), vars, s->tab, s->verbosity);
		s->instrs.push_back(a);
		s->instrs.push_back(prev);
		s->verbosity = prev->verbosity;
		s->tab = prev->tab;
		s->from = prev->from;
		s->net = prev->net;
		s->vars = prev->vars;
		s->parent = p;

		p->instrs.erase(find(p->instrs.begin(), p->instrs.end(), prev));
		p->instrs.push_back(s);
		prev->parent = s;
	}*/

	for (i = 0; i < (int)input_places.size(); i++)
		S[input_places[i]].active = true;
	next = insert_transition(input_places, root, T[index(from)].pbranch, T[index(from)].cbranch, a);
	T[index(next)].active = true;
	next = insert_place(next, T[index(from)].pbranch, T[index(from)].cbranch, s);
	connect(next, from);
}

void petri::insert_sv_parallel(int from, logic root)
{
	map<int, int>::iterator bi, bj;
	vector<int> ip = input_nodes(from);
	vector<int> op = output_nodes(from);
	vector<int> input_places;
	vector<int> output_places;
	vector<int> fvl;
	vector<int> tvl;
	int trans;
	int i;

	T[index(from)].index.vars(&fvl);
	merge_vectors(&fvl, vars->x_channel(fvl));
	root.vars(&tvl);

	// Implement the Physical Structure
	input_places = duplicate_nodes(ip);
	trans = insert_transition(input_places, root, T[index(from)].pbranch, T[index(from)].cbranch, T[index(from)].owner);
	for (i = 0; i < (int)op.size(); i++)
	{
		output_places.push_back(insert_place(trans, S[op[i]].pbranch, S[op[i]].cbranch, S[op[i]].owner));
		connect(output_places.back(), output_nodes(op[i]));
	}

	// Update Branch IDs
	for (i = 0; i < (int)input_places.size(); i++)
	{
		S[ip[i]].pbranch.insert(pair<int, int>(pbranch_count, 0));
		S[input_places[i]].pbranch.insert(pair<int, int>(pbranch_count, 1));
	}
	T[index(from)].pbranch.insert(pair<int, int>(pbranch_count, 0));
	T[index(trans)].pbranch.insert(pair<int, int>(pbranch_count, 1));
	for (i = 0; i < (int)output_places.size(); i++)
	{
		S[op[i]].pbranch.insert(pair<int, int>(pbranch_count, 0));
		S[output_places[i]].pbranch.insert(pair<int, int>(pbranch_count, 1));
	}

	pbranch_count++;
}

void petri::insert_sv_after(int from, logic root)
{
	map<int, int>::iterator bi, bj;
	vector<int> output_places = output_nodes(from);
	vector<pair<int, int> >::iterator ai;
	vector<int> vl;
	int next;
	int i;

	for (i = 0; i < (int)output_places.size(); i++)
		Wp[output_places[i]][index(from)] = 0;

	next = insert_place(from, T[index(from)].pbranch, T[index(from)].cbranch, T[index(from)].owner);
	next = insert_transition(next, root, T[index(from)].pbranch, T[index(from)].cbranch, T[index(from)].owner);

	connect(next, output_places);
}

vector<int> petri::insert_transitions(int from, vector<logic> root, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int t, i;
	vector<int> result;
	for (i = 0; i < (int)root.size(); i++)
	{
		t = new_transition(root[i], (owner->kind() == "assignment"), pbranch, cbranch, owner);
		if (is_trans(from))
			cout << "Error: Illegal arc {T[" << index(from) << "], T[" << index(t) << "]}." << endl;
		else
		{
			Wn[from][index(t)]++;
			S[from].active = S[from].active || T[index(t)].active;
		}

		result.push_back(t);
	}
	return result;
}

vector<int> petri::insert_transitions(vector<int> from, vector<logic> root, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int t, i, j;
	vector<int> result;
	for (i = 0; i < (int)root.size(); i++)
	{
		t = new_transition(root[i], (owner->kind() == "assignment"), pbranch, cbranch, owner);
		for (j = 0; j < (int)from.size(); j++)
		{
			if (is_trans(from[j]))
				cout << "Error: Illegal arc {T[" << index(from[j]) << "], T[" << index(t) << "]}." << endl;
			else
			{
				Wn[from[j]][index(t)]++;
				S[from[j]].active = S[from[j]].active || T[index(t)].active;
			}
		}

		result.push_back(t);
	}
	return result;
}

int petri::insert_dummy(int from, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int t = new_transition(logic(1), false, pbranch, cbranch, owner);
	if (is_trans(from))
		cout << "Error: Illegal arc {T[" << index(from) << "], T[" << index(t) << "]}." << endl;
	else
		Wn[from][index(t)]++;
	return t;
}

int petri::insert_dummy(vector<int> from, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int t = new_transition(logic(1), false, pbranch, cbranch, owner);
	for (size_t i = 0; i < from.size(); i++)
	{
		if (is_trans(from[i]))
			cout << "Error: Illegal arc {T[" << index(from[i]) << "], T[" << index(t) << "]}." << endl;
		else
			Wn[from[i]][index(t)]++;
	}
	return t;
}

int petri::insert_place(int from, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int p = new_place(logic(0), pbranch, cbranch, owner);

	if (is_place(from))
		cout << "Error: Illegal arc {S[" << from << "], S[" << p << "]}." << endl;
	else
		Wp[p][index(from)]++;
	return p;
}

int petri::insert_place(vector<int> from, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	int p = new_place(logic(0), pbranch, cbranch, owner);
	for (size_t i = 0; i < from.size(); i++)
	{
		if (is_place(from[i]))
			cout << "Error: Illegal arc {S[" << from[i] << "], S[" << p << "]}." << endl;
		else
			Wp[p][index(from[i])]++;
	}

	return p;
}

vector<int> petri::insert_places(vector<int> from, map<int, int> pbranch, map<int, int> cbranch, instruction *owner)
{
	vector<int> res;
	for (size_t i = 0; i < from.size(); i++)
		res.push_back(insert_place(from[i], pbranch, cbranch, owner));
	return res;
}

void petri::remove_place(int from)
{
	int i;
	vector<int> ot = output_nodes(from);
	vector<int> op;
	for (i = 0; i < (int)ot.size(); i++)
		merge_vectors(&op, output_nodes(ot[i]));
	unique(&op);
	for (i = 0; i < (int)op.size(); i++)
		if (op[i] > from)
			op[i]--;

	bool init = false;

	if (is_place(from))
	{
		for (i = 0; i < (int)M0.size(); i++)
		{
			if (M0[i] > from)
				M0[i]--;
			else if (M0[i] == from)
			{
				M0.erase(M0.begin() + i);
				init = true;
				i--;
			}
		}

		if (init)
			M0.insert(M0.end(), op.begin(), op.end());
		S.erase(S.begin() + from);
		Wp.remc(from);
		Wn.remc(from);
	}
	else
		cout << "Error: " << index(from) << " must be a place." << endl;
}

void petri::remove_place(vector<int> from)
{
	sort(from.rbegin(), from.rend());
	for (int i = 0; i < (int)from.size(); i++)
		remove_place(from[i]);
}

void petri::propogate_marking_forward(int from, vector<bool> *covered)
{
	vector<int> ot = output_nodes(from);
	vector<int> op, ip;
	bool success;
	int i, j;
	for (i = 0; i < (int)ot.size(); i++)
	{
		if (T[index(ot[i])].index == 1 && !(*covered)[index(ot[i])])
		{
			(*covered)[index(ot[i])] = true;
			ip = input_nodes(ot[i]);
			success = true;
			for (j = 0; j < (int)ip.size() && success; j++)
				if (find(M0.begin(), M0.end(), ip[j]) == M0.end())
					success = false;

			if (success)
			{
				op = output_nodes(ot[i]);
				M0.insert(M0.end(), op.begin(), op.end());
				for (j = 0; j < (int)op.size(); j++)
					propogate_marking_forward(op[j], covered);
			}
		}
	}
}

void petri::propogate_marking_backward(int from, vector<bool> *covered)
{
	vector<int> it = input_nodes(from);
	vector<int> ip, op;
	bool success;
	int i, j;
	for (i = 0; i < (int)it.size(); i++)
	{
		if (T[index(it[i])].index == 1 && !(*covered)[index(it[i])])
		{
			(*covered)[index(it[i])] = true;
			op = output_nodes(it[i]);
			success = true;
			for (j = 0; j < (int)op.size() && success; j++)
				if (find(M0.begin(), M0.end(), op[j]) == M0.end())
					success = false;

			if (success)
			{
				ip = input_nodes(it[i]);
				M0.insert(M0.end(), ip.begin(), ip.end());
				for (j = 0; j < (int)ip.size(); j++)
					propogate_marking_backward(ip[j], covered);
			}
		}
	}
}

bool petri::updateplace(int p, int i)
{
	map<int, logic>::iterator ji;
	vector<int> ia = input_nodes(p);
	vector<int> oa = output_nodes(p);
	vector<int> ip;
	vector<int> vl;
	vector<logic> options;
	int l, k, j;
	logic t(0);
	logic o;
	bool fireup, firedown;

	o = S[p].index;
	S[p].index = 0;

	(*flags->log_file) << string(i, '\t') << p << "\t";

	for (k = 0; k < (int)ia.size(); k++)
	{
		(*flags->log_file) << "{";
		ip = input_nodes(ia[k]);

		for (j = 0, t = 1; j < (int)ip.size(); j++)
		{
			t &= S[ip[j]].index;
			(*flags->log_file) << S[ip[j]].index.print(vars) << " ";
		}

		(*flags->log_file) << "}>>" << T[index(ia[k])].index.print(vars) << " = (" << (t >> T[index(ia[k])].index).print(vars) << ")\t";
		options.push_back(t >> T[index(ia[k])].index);
		T[index(ia[k])].definitely_invacuous = is_mutex(&t, &options.back());
		T[index(ia[k])].possibly_vacuous = !T[index(ia[k])].definitely_invacuous;
		T[index(ia[k])].definitely_vacuous = (t == options.back());
	}

	for (k = 0; k < (int)options.size(); k++)
	{
		t = 1;
		for (l = 0, t = 1; l < (int)options.size(); l++)
			t &= ((l == k) ? options[l] : ~options[l]);
		S[p].index |= t;
	}

	(*flags->log_file) << S[p].index.print(vars) << " ";

	S[p].apply_mutables();

	(*flags->log_file) << S[p].index.print(vars) << " ";

	if (S[p].index != 0)
	{
		S[p].index &= vars->enforcements;

		if (S[p].index == 0)
			cout << "Error: Enforcements conflict with known state state at state " << p << "." << endl;
		else
		{
			S[p].index &= S[p].assumptions;

			if (S[p].index == 0)
				cout << "Error: Assumption " << S[p].assumptions.print(vars) << " conflict with known state state at state " << p << "." << endl;
		}
	}

	(*flags->log_file) << S[p].index.print(vars) << " ? " << o.print(vars) << endl;

	return (S[p].index != o);
}

int petri::update(int p, vector<bool> *covered, int i, bool immune)
{
	(*flags->log_file) << string(i, '\t') << "Update ";
	if (is_trans(p))
		(*flags->log_file) << "T";
	else
		(*flags->log_file) << "S";
	(*flags->log_file) << index(p) << endl;

	vector<int> next;
	vector<int> curr;
	vector<int> it;
	map<int, pair<int, int> >::iterator cpi;
	int j, k;
	bool updated;

	if (covered != NULL && covered->size() < S.size())
		covered->resize(S.size(), false);

	next.push_back(p);

	while (1)
	{
		if (!immune && i != 0 && (it = input_nodes(next[0])).size() > 1)
		{
			if (is_trans(next[0]))
			{
				(*flags->log_file) << string(i, '\t') << "pmerge" << endl;
				return next[0];
			}
			else
			{
				for (j = 0; j < (int)it.size(); j++)
					for (k = j+1; k < (int)it.size(); k++)
						if (csiblings(it[j], it[k]) != -1)
						{
							(*flags->log_file) << string(i, '\t') << "cmerge" << endl;
							return next[0];
						}
			}
		}

		if (is_place(next[0]))
		{
			updated = updateplace(next[0], i);

			if (!updated && (*covered)[next[0]])
			{
				(*flags->log_file) << string(i, '\t') << "end" << endl;
				return (int)S.size();
			}

			(*covered)[next[0]] = true;
		}

		(*flags->log_file) << string(i, '\t') << "{";
		for (j = 0; j < (int)next.size(); j++)
			(*flags->log_file) << (is_trans(next[j]) ? "T" : "S") << index(next[j]) << " ";
		(*flags->log_file) << "} -> ";
		curr = next;
		next.clear();
		for (j = 0; j < (int)arcs.size(); j++)
			for (k = 0; k < (int)curr.size(); k++)
				if (curr[k] == arcs[j].first)
					next.push_back(arcs[j].second);
		unique(&next);
		(*flags->log_file) << "{";
		for (j = 0; j < (int)next.size(); j++)
			(*flags->log_file) << (is_trans(next[j]) ? "T" : "S") << index(next[j]) << " ";
		(*flags->log_file) << "}" << endl;

		immune = false;
		if (next.size() > 1)
		{
			curr = next;
			next.clear();
			for (j = 0; j < (int)curr.size(); j++)
			{
				next.push_back(update(curr[j], covered, i+1));
				if (next.back() == (int)S.size())
					next.pop_back();
				else
					immune = true;
			}

			unique(&next);
		}

		if (next.size() < 1)
			return (int)S.size();
	}
}

void petri::update()
{
	int i, j, k;
	int s = (int)M0.size();
	vector<bool> covered;
	vector<int> M = M0;
	vector<int> r1, r2;

	/*for (i = 0; i < s; i++)
	{
		covered.clear();
		covered.resize(T.size(), false);
		propogate_marking_forward(i, &covered);
		covered.clear();
		covered.resize(T.size(), false);
		propogate_marking_backward(i, &covered);
	}
	unique(&M0);*/

	gen_arcs();

	(*flags->log_file) << "Reset: {";
	for (i = 0; i < (int)M0.size(); i++)
		cout << M0[i] << " ";
	cout << "} " << vars->reset.print(vars) << endl;
	for (i = 0; i < (int)S.size(); i++)
	{
		if (find(M.begin(), M.end(), i) != M.end())
			S[i].index = vars->reset & vars->enforcements;
		else
			S[i].index = 0;
	}
	vector<int> ia;

	covered.clear();
	covered.resize(S.size(), false);

	k = 0;
	if (M.size() > 1)
		k++;
	for (i = 0; i < (int)M.size(); i++)
	{
		ia = output_nodes(M[i]);
		if (ia.size() > 1)
			k++;
		r1.clear();
		for (j = 0; j < (int)ia.size(); j++)
		{
			r1.push_back(update(ia[j], &covered, k));
			if (r1.back() == (int)S.size())
				r1.pop_back();
		}
		if (ia.size() > 1)
			k--;

		unique(&r1);

		for (j = 0; j < (int)r1.size(); j++)
		{
			r2.push_back(update(r1[j], &covered, k, true));
			if (r2.back() == (int)S.size())
				r2.pop_back();
		}
	}

	if (M.size() > 1)
		k--;

	unique(&r2);
	for (j = 0; j < (int)r2.size(); j++)
		update(r2[j], &covered, k, true);
}

void petri::check_assertions()
{
	for (int i = 0; i < (int)S.size(); i++)
	{
		for (int j = 0; j < (int)S[i].assertions.size(); j++)
			if ((S[i].index & S[i].assertions[j]) != 0)
				cout << "Error: Assertion " << (~S[i].assertions[j]).print(vars) << " fails at state " << i << " with a state encoding of " << S[i].index.print(vars) << "." << endl;

		for (int j = 0; j < (int)vars->requirements.size(); j++)
			if ((S[i].index & vars->requirements[j]) != 0)
				cout << "Error: Requirement " << (~vars->requirements[j]).print(vars) << " fails at state " << i << " with a state encoding of " << S[i].index.print(vars) << "." << endl;
	}
}

void petri::connect(vector<int> from, vector<int> to)
{
	for (size_t i = 0; i < from.size(); i++)
		for (size_t j = 0; j < to.size(); j++)
			connect(from[i], to[j]);
}

void petri::connect(vector<int> from, int to)
{
	for (size_t i = 0; i < from.size(); i++)
		connect(from[i], to);
}

void petri::connect(int from, vector<int> to)
{
	for (size_t j = 0; j < to.size(); j++)
		connect(from, to[j]);
}

void petri::connect(int from, int to)
{
	if (is_place(from) && is_trans(to))
	{
		Wn[from][index(to)]++;
		S[from].active = S[from].active || T[index(to)].active;
	}
	else if (is_trans(from) && is_place(to))
		Wp[to][index(from)]++;
	else if (is_place(from) && is_place(to))
		cout << "Error: Illegal arc {S[" << from << "], S[" << to << "]}." << endl;
	else if (is_trans(from) && is_trans(to))
		cout << "Error: Illegal arc {T[" << index(from) << "], T[" << index(to) << "]}." << endl;
}

pair<int, int> petri::closest_input(vector<int> from, vector<int> to, path p, int i)
{
	int count = 0;
	vector<int> next;
	vector<int> it, ip;
	vector<pair<int, int> > results;
	int j;
	int a;

	if (from.size() == 0)
		return pair<int, int>(arcs.size(), -1);

	next = to;

	while (next.size() == 1)
	{
		count++;
		a = next[0];

		if (p[a] > 0)
			return pair<int, int>(arcs.size(), -1);

		p[a]++;

		if (find(from.begin(), from.end(), a) != from.end())
			return pair<int, int>(count, a);

		next.clear();
		for (j = 0; j < (int)arcs.size(); j++)
			if (arcs[j].second == arcs[a].first)
				next.push_back(j);
	}

	for (j = 0; j < (int)next.size(); j++)
		if (p[next[j]] == 0)
		{
			count++;
			results.push_back(closest_input(from, vector<int>(1, next[j]), p, i+1));
		}
	unique(&results);

	if (results.size() > 0)
		return pair<int, int>(results.front().first + count, results.front().second);
	else
		return pair<int, int>(arcs.size(), -1);
}

pair<int, int> petri::closest_output(vector<int> from, vector<int> to, path p, int i)
{
	int count = 0;
	vector<int> next;
	vector<int> it, ip;
	vector<pair<int, int> > results;
	int j;
	int a;

	if (to.size() == 0)
		return pair<int, int>(arcs.size(), -1);

	next = from;

	while (next.size() == 1)
	{
		count++;
		a = next[0];

		if (p[a] > 0)
			return pair<int, int>(arcs.size(), -1);

		p[a]++;

		if (find(to.begin(), to.end(), a) != to.end())
			return pair<int, int>(count, a);

		next.clear();
		for (j = 0; j < (int)arcs.size(); j++)
			if (arcs[j].first == arcs[a].second)
				next.push_back(j);
	}

	for (j = 0; j < (int)next.size(); j++)
		if (p[next[j]] == 0)
		{
			count++;
			results.push_back(closest_output(vector<int>(1, next[j]), to, p, i+1));
		}
	unique(&results);

	if (results.size() > 0)
		return pair<int, int>(results.front().first + count, results.front().second);
	else
		return pair<int, int>(arcs.size(), -1);
}

bool petri::dead(int from)
{
	if (is_place(from))
	{
		for (size_t i = 0; i < Wp[from].size(); i++)
			if (Wp[from][i] > 0)
				return false;

		for (size_t i = 0; i < Wn[from].size(); i++)
			if (Wn[from][i] > 0)
				return false;
	}
	else
	{
		from = index(from);
		for (size_t i = 0; i < Wn.size(); i++)
			if (Wn[i][from] > 0)
				return false;

		for (size_t i = 0; i < Wp.size(); i++)
			if (Wp[i][from] > 0)
				return false;
	}

	return true;
}

bool petri::is_place(int from)
{
	return (from >= 0);
}

bool petri::is_trans(int from)
{
	return (from < 0);
}

int petri::index(int from)
{
	return (from&0x7FFFFFFF);
}

int petri::place_id(int idx)
{
	return (idx&0x7FFFFFFF);
}

int petri::trans_id(int idx)
{
	return (idx|0x80000000);
}

logic petri::base(vector<int> idx)
{
	logic res(1);
	int i;
	for (i = 0; i < (int)idx.size(); i++)
		res = res & S[idx[i]].index;

	return res;
}

bool petri::connected(int from, int to)
{
	vector<int> i1 = Wn[from];
	vector<int> o1 = Wp[to];
	vector<int> i2 = Wn[to];
	vector<int> o2 = Wp[from];

	for (int k = 0; k < (int)i1.size(); k++)
		if ((i1[k] > 0 && o1[k] > 0) || (i2[k] > 0 && o2[k] > 0) || (i1[k] > 0 && i2[k] > 0) || (o1[k] > 0 && o2[k] > 0))
			return true;

	return false;
}

int petri::psiblings(int p0, int p1)
{
	node *n0 = &((*this)[p0]);
	node *n1 = &((*this)[p1]);
	map<int, int>::iterator bi, bj;
	for (bi = n0->pbranch.begin(); bi != n0->pbranch.end(); bi++)
		for (bj = n1->pbranch.begin(); bj != n1->pbranch.end(); bj++)
			if (bi->first == bj->first && bi->second != bj->second)
				return bi->first;
	return -1;
}

int petri::csiblings(int p0, int p1)
{
	node *n0 = &((*this)[p0]);
	node *n1 = &((*this)[p1]);
	map<int, int>::iterator bi, bj;
	for (bi = n0->cbranch.begin(); bi != n0->cbranch.end(); bi++)
		for (bj = n1->cbranch.begin(); bj != n1->cbranch.end(); bj++)
			if (bi->first == bj->first && bi->second != bj->second)
				return bi->first;
	return -1;
}

bool petri::same_inputs(int p0, int p1)
{
	vector<int> it0, it1;
	bool diff_in = true;
	int k, l;

	it0 = input_nodes(p0);
	it1 = input_nodes(p1);

	if (it0.size() == it1.size())
	{
		diff_in = false;
		for (k = 0; k < (int)it0.size() && !diff_in; k++)
		{
			diff_in = true;
			for (l = 0; l < (int)it1.size() && diff_in; l++)
				if (T[it0[k]].index == T[it1[l]].index)
					diff_in = false;
		}
	}

	return !diff_in;
}

bool petri::same_outputs(int p0, int p1)
{
	vector<int> ot0, ot1;
	bool diff_out = true;
	int k, l;

	ot0 = output_nodes(p0);
	ot1 = output_nodes(p1);

	if (ot0.size() == ot1.size())
	{
		diff_out = false;
		for (k = 0; k < (int)ot0.size() && !diff_out; k++)
		{
			diff_out = true;
			for (l = 0; l < (int)ot1.size() && diff_out; l++)
				if (T[ot0[k]].index == T[ot1[l]].index)
					diff_out = false;
		}
	}

	return !diff_out;
}

vector<int> petri::duplicate_nodes(vector<int> from)
{
	vector<int> ret;
	for (size_t i = 0; i < from.size(); i++)
	{
		if (is_place(from[i]))
			ret.push_back(insert_place(input_nodes(from[i]), S[from[i]].pbranch, S[from[i]].cbranch, S[from[i]].owner));
		else
			ret.push_back(insert_transition(input_nodes(from[i]), T[index(from[i])].index, T[index(from[i])].pbranch, T[index(from[i])].cbranch, T[index(from[i])].owner));

		if (find(M0.begin(), M0.end(), from[i]) != M0.end())
			M0.push_back(ret.back());
	}
	sort(ret.rbegin(), ret.rend());
	return ret;
}

int petri::duplicate_node(int from)
{
	int ret;
	if (is_place(from))
		ret = insert_place(input_nodes(from), S[from].pbranch, S[from].cbranch, S[from].owner);
	else
		ret = insert_transition(input_nodes(from), T[index(from)].index, T[index(from)].pbranch, T[index(from)].cbranch, T[index(from)].owner);

	if (find(M0.begin(), M0.end(), from) != M0.end())
		M0.push_back(ret);

	return ret;
}

int petri::merge_places(vector<int> from)
{
	map<int, int> pbranch;
	map<int, int> cbranch;
	vector<pair<int, int> >::iterator ai;
	int j, k;
	logic idx(0);
	pbranch = S[from[0]].pbranch;
	cbranch = S[from[0]].cbranch;
	for (j = 0, idx = 0; j < (int)from.size(); j++)
	{
		intersect(pbranch, S[from[j]].pbranch, &pbranch);
		intersect(cbranch, S[from[j]].cbranch, &cbranch);
		idx = idx | S[from[j]].index;
	}

	int p = new_place(idx, pbranch, cbranch, NULL);
	for (j = 0; j < (int)from.size(); j++)
	{
		for (k = 0; k < (int)T.size(); k++)
		{
			Wn[p][k] = (Wn[p][k] || Wn[from[j]][k]);
			Wp[p][k] = (Wp[p][k] || Wp[from[j]][k]);
		}
	}

	return p;
}

int petri::merge_places(int a, int b)
{
	map<int, int> pbranch;
	map<int, int> cbranch;
	vector<pair<int, int> >::iterator ai;
	int k, p;

	intersect(S[a].pbranch, S[b].pbranch, &pbranch);
	intersect(S[a].cbranch, S[b].cbranch, &cbranch);
	p = new_place((S[a].index | S[b].index), pbranch, cbranch, NULL);
	for (k = 0; k < (int)T.size(); k++)
	{
		Wn[p][k] = (Wn[a][k] || Wn[b][k]);
		Wp[p][k] = (Wp[a][k] || Wp[b][k]);
	}

	return p;
}

vector<int> petri::input_nodes(int from)
{
	vector<int> ret;
	if (is_place(from))
	{
		for (int i = 0; i < (int)Wp[from].size(); i++)
			if (Wp[from][i] > 0)
				ret.push_back(trans_id(i));
	}
	else
	{
		from = index(from);
		for (int i = 0; i < (int)Wn.size(); i++)
			if (Wn[i][from] > 0)
				ret.push_back(place_id(i));
	}
	sort(ret.rbegin(), ret.rend());
	return ret;
}

vector<int> petri::output_nodes(int from)
{
	vector<int> ret;
	if (is_place(from))
	{
		for (int i = 0; i < (int)Wn[from].size(); i++)
			if (Wn[from][i] > 0)
				ret.push_back(trans_id(i));
	}
	else
	{
		from = index(from);
		for (int i = 0; i < (int)Wp.size(); i++)
			if (Wp[i][from] > 0)
				ret.push_back(place_id(i));
	}
	return ret;
}

vector<int> petri::input_arcs(int from)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].second == from)
			result.push_back(i);
	return result;
}

vector<int> petri::output_arcs(int from)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].first == from)
			result.push_back(i);
	return result;
}

int petri::get_split_place(int merge_place, vector<bool> *covered)
{
	int i = merge_place, j, k, l;
	vector<int> ot, op, it, ip;
	vector<bool> c;
	bool loop;

	if ((*covered)[i])
		return -1;

	loop = true;
	it = input_nodes(i);
	if ((int)it.size() <= 0)
		return i;
	else if ((int)it.size() == 1)
	{
		ot = output_nodes(i);
		for (j = 0; j < (int)ot.size() && loop; j++)
		{
			op = output_nodes(ot[j]);
			for (k = 0; k < (int)op.size() && loop; k++)
				if (!(*covered)[op[k]])
					loop = false;
		}
	}

	(*covered)[i] = true;

	while (loop)
	{
		it = input_nodes(i);
		if ((int)it.size() <= 0)
			return i;
		else if ((int)it.size() == 1)
		{
			ip = input_nodes(it[0]);
			if (ip.size() == 0)
				return i;
			i = ip[0];

			if ((*covered)[i])
				return -1;
		}
		else
		{
			for (l = 0, k = -1; l < (int)it.size() && k == -1; l++)
			{
				ip = input_nodes(it[l]);
				for (j = 0; j < (int)ip.size() && k == -1; j++)
				{
					c = *covered;
					k = get_split_place(ip[j], &c);
				}
			}

			if (k == -1)
				return i;
			else
				i = ip[--j];
		}

		(*covered)[i] = true;

		loop = true;
		ot = output_nodes(i);
		for (j = 0; j < (int)ot.size() && loop; j++)
		{
			op = output_nodes(ot[j]);
			for (k = 0; k < (int)op.size() && loop; k++)
				if (!(*covered)[op[k]])
					loop = false;
		}
	}

	return i;
}

void petri::add_conflict_pair(map<int, list<vector<int> > > *c, int i, int j)
{
	map<int, list<vector<int> > >::iterator ri;
	list<vector<int> >::iterator li;
	vector<list<vector<int> >::iterator > gi;
	vector<int> group;
	int k;

	ri = c->find(i);
	if (ri != c->end())
	{
		gi.clear();
		for (li = ri->second.begin(); li != ri->second.end(); li++)
			for (k = 0; k < (int)li->size(); k++)
				if (connected(j, (*li)[k]))// || psiblings(j, (*li)[k]) != -1)
				{
					gi.push_back(li);
					k = (int)li->size();
				}

		group = vector<int>(1, j);
		for (k = 0; k < (int)gi.size(); k++)
		{
			group.insert(group.end(), gi[k]->begin(), gi[k]->end());
			ri->second.erase(gi[k]);
		}
		unique(&group);
		ri->second.push_back(group);
	}
	else
		c->insert(pair<int, list<vector<int> > >(i, list<vector<int> >(1, vector<int>(1, j))));
}

void petri::gen_mutables()
{
	int i, j;
	vector<int> ia;

	map<int, int>::iterator bi, bj;

	for (i = 0; i < (int)S.size(); i++)
	{
		S[i].mutables.clear();

		for (j = 0; j < (int)T.size(); j++)
			if (T[j].active && psiblings(i, trans_id(j)) >= 0)
			{
				T[j].index.extract(&S[i].mutables);
				vars->x_channel(T[j].index.vars(), &S[i].mutables);
			}

		ia = input_nodes(i);
		for (j = 0; j < (int)ia.size(); j++)
			if (T[index(ia[j])].active)
				vars->x_channel(T[index(ia[j])].index.vars(), &S[i].mutables);
	}
}

void petri::gen_conditional_places()
{
	map<int, pair<int, int> >::iterator ci;
	vector<int> oa, ia, sibs;
	int i, j, k;

	conditional_places.clear();
	for (i = 0; i < (int)S.size(); i++)
	{
		oa = output_nodes(i);
		ia = input_nodes(i);

		sibs.clear();
		for (j = 0; j < (int)oa.size(); j++)
			for (k = j+1; k < (int)oa.size(); k++)
				sibs.push_back(csiblings(oa[j], oa[k]));
		unique(&sibs);

		for (k = 0; k < (int)sibs.size(); k++)
			if (sibs[k] != -1)
			{
				ci = conditional_places.find(sibs[k]);
				if (ci == conditional_places.end())
					conditional_places.insert(pair<int, pair<int, int> >(sibs[k], pair<int, int>(-1, i)));
				else
					ci->second.second = i;
			}

		sibs.clear();
		for (j = 0; j < (int)ia.size(); j++)
			for (k = j+1; k < (int)ia.size(); k++)
				sibs.push_back(csiblings(ia[j], ia[k]));
		unique(&sibs);

		for (k = 0; k < (int)sibs.size(); k++)
			if (sibs[k] != -1)
			{
				ci = conditional_places.find(sibs[k]);
				if (ci == conditional_places.end())
					conditional_places.insert(pair<int, pair<int, int> >(sibs[k], pair<int, int>(i, -1)));
				else
					ci->second.first = i;
			}
	}
}

void petri::gen_conflicts()
{
	map<int, list<vector<int> > >::iterator ri;
	list<vector<int> >::iterator li;
	vector<list<vector<int> >::iterator > gi;
	map<int, int>::iterator bi, bj;
	vector<int> group;
	vector<int> oa;
	vector<int> vl;
	vector<int> temp;
	int i, j;
	logic t(0);
	logic nt(0);

	logic s1(0);

	conflicts.clear();
	indistinguishable.clear();

	for (i = 0; i < (int)S.size(); i++)
	{
		oa = output_nodes(i);

		// INDEX
		vl.clear();
		t = 1;
		for (j = 0; j < (int)oa.size(); j++)
			if (T[index(oa[j])].active)
			{
				T[index(oa[j])].index.vars(&vl);
				t = t & T[index(oa[j])].index;
			}
		unique(&vl);
		s1 = S[i].index.hide(vl);

		for (j = 0; j < (int)S.size(); j++)
		{
			/* States are conflicting if:
			 *  - they are not the same state
			 *  - one is not in the tail of another (the might as well be here case)
			 *  - the transition which causes the conflict is not a vacuous firing in the other state
			 *  - they are indistinguishable
			 *  - the two states do not exist in parallel
			 */

			// INDEX
			if (i != j && psiblings(i, j) < 0 && !S[i].is_in_tail(j) && !is_mutex(&s1, &S[j].index))
			{
				oa = output_nodes(j);
				nt = ~t;
				// is it a conflicting state? (e.g. not vacuous)
				if (S[i].active && (!is_mutex(&nt, &S[i].index, &S[j].index) || (oa.size() > 0 && T[index(oa[0])].active && is_mutex(&t, &T[index(oa[0])].index))))
					add_conflict_pair(&conflicts, i, j);

				// it is at least an indistinguishable state at this point
				add_conflict_pair(&indistinguishable, i, j);
			}
		}
	}
}

void petri::gen_bubbleless_conflicts()
{
	map<int, list<vector<int> > >::iterator ri;
	list<vector<int> >::iterator li;
	vector<list<vector<int> >::iterator > gi;
	map<int, int>::iterator bi, bj;
	vector<int> group;
	vector<int> oa;
	vector<int> vl;
	vector<int> temp;
	int i, j;
	logic tp(0), sp1(0);
	logic tn(0), sn1(0);
	bool parallel;
	bool strict;

	positive_conflicts.clear();
	positive_indistinguishable.clear();
	negative_conflicts.clear();
	negative_indistinguishable.clear();

	gen_conflicts();

	for (i = 0; i < (int)S.size(); i++)
	{
		ri = conflicts.find(i);
		oa = output_nodes(i);

		// POSITIVE
		vl.clear();
		for (j = 0, tp = 1; j < (int)oa.size(); j++)
			if (T[index(oa[j])].active)
			{
				T[index(oa[j])].negative.vars(&vl);
				tp &= T[index(oa[j])].negative;
			}
		unique(&vl);
		sp1 = S[i].positive.hide(vl);

		// NEGATIVE
		vl.clear();
		for (j = 0, tn = 1; j < (int)oa.size(); j++)
			if (T[index(oa[j])].active)
			{
				T[index(oa[j])].positive.vars(&vl);
				tn &= T[index(oa[j])].positive;
			}
		unique(&vl);
		sn1 = S[i].negative.hide(vl);

		for (j = 0; j < (int)S.size(); j++)
		{
			strict = false;
			if (ri != conflicts.end())
				for (li = ri->second.begin(); li != ri->second.end() && !strict; li++)
					if (find(li->begin(), li->end(), j) != li->end())
						strict = true;

			if (!strict)
			{
				/* A node has to have at least one output transition due to the trim function.
				 * A node can only have one output transition if that transition is active,
				 * otherwise it can have multiple.
				 */
				oa = output_nodes(j);

				/* States are conflicting if:
				 *  - they are not the same state
				 *  - one is not in the tail of another (the might as well be here case)
				 *  - the transition which causes the conflict is not a vacuous firing in the other state
				 *  - they are indistinguishable
				 *  - the two states do not exist in parallel
				 */

				parallel = (psiblings(i, j) >= 0);
				// POSITIVE
				if (i != j && !parallel && !S[i].is_in_tail(j) && (sp1 & S[j].index) != 0)
				{
					// is it a conflicting state? (e.g. not vacuous)
					if (S[i].active && ((~tp & sp1 & S[j].index) != 0 || (oa.size() > 0 && T[index(oa[0])].active && (tp & T[index(oa[0])].index) == 0)))
						add_conflict_pair(&positive_conflicts, i, j);

					// it is at least an indistinguishable state at this point
					add_conflict_pair(&positive_indistinguishable, i, j);
				}

				// NEGATIVE
				if (i != j && !parallel && !S[i].is_in_tail(j) && (sn1 & S[j].index) != 0)
				{
					// is it a conflicting state? (e.g. not vacuous)
					if (S[i].active && ((~tn & sn1 & S[j].index) != 0 || (oa.size() > 0 && T[index(oa[0])].active && (tn & T[index(oa[0])].index) == 0)))
						add_conflict_pair(&negative_conflicts, i, j);

					// it is at least an indistinguishable state at this point
					add_conflict_pair(&negative_indistinguishable, i, j);
				}
			}
		}
	}
}

void petri::gen_senses()
{
	int i;
	for (i = 0; i < (int)S.size(); i++)
	{
		S[i].positive = S[i].index.pabs();
		S[i].negative = S[i].index.nabs();
	}

	for (i = 0; i < (int)T.size(); i++)
	{
		T[i].positive = T[i].index.pabs();
		T[i].negative = T[i].index.nabs();
	}
}

void petri::trim_branch_ids()
{
	map<int, vector<int> > pbranch_counts;
	map<int, vector<int> > cbranch_counts;
	map<int, vector<int> >::iterator bci;
	map<int, int>::iterator bi;
	vector<map<int, int>::iterator > remove;
	int i, j;

	for (i = 0; i < pbranch_count; i++)
		pbranch_counts.insert(pair<int, vector<int> >(i, vector<int>()));

	for (i = 0; i < cbranch_count; i++)
		cbranch_counts.insert(pair<int, vector<int> >(i, vector<int>()));

	for (i = 0; i < (int)T.size(); i++)
	{
		for (bi = T[i].pbranch.begin(); bi != T[i].pbranch.end(); bi++)
			pbranch_counts[bi->first].push_back(bi->second);
		for (bi = T[i].cbranch.begin(); bi != T[i].cbranch.end(); bi++)
			cbranch_counts[bi->first].push_back(bi->second);
	}

	for (bci = pbranch_counts.begin(); bci != pbranch_counts.end(); bci++)
		unique(&bci->second);

	for (bci = cbranch_counts.begin(); bci != cbranch_counts.end(); bci++)
		unique(&bci->second);

	for (i = 0; i < (int)T.size(); i++)
	{
		remove.clear();
		for (bi = T[i].pbranch.begin(); bi != T[i].pbranch.end(); bi++)
			if (pbranch_counts[bi->first].size() <= 1)
				remove.push_back(bi);

		for (j = 0; j < (int)remove.size(); j++)
			T[i].pbranch.erase(remove[j]);

		remove.clear();
		for (bi = T[i].cbranch.begin(); bi != T[i].cbranch.end(); bi++)
			if (cbranch_counts[bi->first].size() <= 1)
				remove.push_back(bi);

		for (j = 0; j < (int)remove.size(); j++)
			T[i].cbranch.erase(remove[j]);
	}

	for (i = 0; i < (int)S.size(); i++)
	{
		remove.clear();
		for (bi = S[i].pbranch.begin(); bi != S[i].pbranch.end(); bi++)
			if (pbranch_counts[bi->first].size() <= 1)
				remove.push_back(bi);

		for (j = 0; j < (int)remove.size(); j++)
			S[i].pbranch.erase(remove[j]);

		remove.clear();
		for (bi = S[i].cbranch.begin(); bi != S[i].cbranch.end(); bi++)
			if (cbranch_counts[bi->first].size() <= 1)
				remove.push_back(bi);

		for (j = 0; j < (int)remove.size(); j++)
			S[i].cbranch.erase(remove[j]);
	}
}

void petri::gen_tails()
{
	for (int i = 0; i < (int)S.size(); i++)
		S[i].tail.clear();
	for (int i = 0; i < (int)T.size(); i++)
		T[i].tail.clear();

	vector<int> ia, oa;
	vector<int> old;
	int i, j;
	bool done = false;
	while (!done)
	{
		done = true;
		for (i = 0; i < (int)arcs.size(); i++)
		{
			// State A->T is in the tail of T1 if A -> T -> Tail of T1 and T is either not active or not invacuous
			if (is_trans(arcs[i].first) && (!T[index(arcs[i].first)].active || T[index(arcs[i].first)].possibly_vacuous))
			{
				old = S[arcs[i].second].tail;
				S[arcs[i].second].add_to_tail(T[index(arcs[i].first)].tail);
				done = done && (old == S[arcs[i].second].tail);
			}
			else if (is_place(arcs[i].first))
			{
				old = T[index(arcs[i].second)].tail;
				T[index(arcs[i].second)].add_to_tail(arcs[i].first);
				T[index(arcs[i].second)].add_to_tail(S[arcs[i].first].tail);
				done = done && (old == T[index(arcs[i].second)].tail);
			}
		}
	}
}

void petri::gen_arcs()
{
	arcs.clear();
	for (int i = 0; i < (int)S.size(); i++)
		for (int j = 0; j < (int)T.size(); j++)
		{
			if (Wn[i][j] > 0)
				arcs.push_back(pair<int, int>(i, trans_id(j)));
			if (Wp[i][j] > 0)
				arcs.push_back(pair<int, int>(trans_id(j), i));
		}
}

bool petri::trim()
{
	bool result = false;
	int i, j, k, l;
	vector<int> ia, oa;
	map<int, int>::iterator bi;
	vector<pair<int, int> >::iterator ai;
	bool vacuous;
	logic v;

	// Remove unreachable places
	/**
	 * A place is "unreachable" when there is no possible state encoding
	 * that would enable an input transition.
	 */
	for (i = 0; i < (int)S.size(); i++)
		while (i < (int)S.size() && S[i].index == 0)
		{
			remove_place(i);
			result = true;
		}

	i = 0;
	while (i < (int)T.size())
	{
		ia = input_nodes(trans_id(i));
		oa = output_nodes(trans_id(i));

		for (j = 0, v = 1; j < (int)ia.size(); j++)
			v &= S[ia[j]].index;

		// Impossible and dangling transitions
		/**
		 * A transition is "impossible" when it can never fire, regardless of the state
		 * encodings at it's input places. Logically, you would think that this
		 * never should happen in the first place, but this can happen during the
		 * compilation of infinite loops. In an infinite loop, you will always
		 * continue back to the start of a loop, except what if the circuit designer
		 * placed an instruction after an infinite loop? It is easier for us to just
		 * place an impossible transition at the exit of the infinite loop, and then
		 * concatenate other instructions on there.
		 *
		 * A transition is "dangling" when it either has no input arcs or has
		 * no output arcs. This can happen after removing an unreachable state.
		 * Doing so makes it's neighbors dangle.
		 */
		if (T[i].index == 0 || ia.size() == 0 || oa.size() == 0)
		{
			Wp.remr(i);
			Wn.remr(i);
			T.erase(T.begin() + i);
			result = true;
		}

		// Vacuous Transitions
		/**
		 * A transition is "vacuous" when it does nothing to change the state of a
		 * system. We can remove all but two cases: First, if it has more than one
		 * input arc and more than one output arc, it actually makes the system more
		 * complex overall to remove that transition because you would have to merge
		 * every input place with every output place. If there were n input places
		 * and m output places, the number of places in the system would increase
		 * by (n*m - n - m). There is also a case we can't handle where there is
		 * only one output arc, but the place at the end of that arc is a conditional
		 * merge. The reason we can't handle this is because it would break the structure
		 * of the petri-net. You essentially have a parallel merge right before a
		 * conditional merge, and removing the transition that acts as the parallel
		 * merge would break the parallel block.
		 *
		 * The problem with this "vacuous" check is that while it may be locally vacuous,
		 * it can be globally invacuous. For example, If you have *[S1; x-; S2] where
		 * S1 and S2 don't modify the value of x, then x will always be 0 and the x-
		 * transition will be locally vacuous. However, if you then remove the x- transition
		 * and have *[S1;S2], you no longer know the value of x. Therefore, the x- transition
		 * is globally invacuous.
		 */
		else if (T[index(i)].definitely_vacuous && ia.size() == 1 && oa.size() == 1 && output_nodes(ia[0]).size() == 1 && (input_nodes(ia[0]).size() == 1 || output_nodes(trans_id(i)).size() == 1) && input_nodes(oa[0]).size() == 1 && (output_nodes(oa[0]).size() == 1 || input_nodes(trans_id(i)).size() == 1))
		{
			for (k = 0; k < (int)oa.size(); k++)
			{
				for (l = 0; l < (int)Wn[oa[k]].size(); l++)
				{
					Wn[oa[k]][l] = (Wn[oa[k]][l] || Wn[ia[0]][l]);
					Wp[oa[k]][l] = (Wp[oa[k]][l] || Wp[ia[0]][l]);
				}
				S[oa[k]].assumptions = S[ia[0]].assumptions >> S[oa[k]].assumptions;
				S[oa[k]].assertions.insert(S[oa[k]].assertions.end(), S[ia[0]].assertions.begin(), S[ia[0]].assertions.end());
			}

			remove_place(ia[0]);

			Wp.remr(i);
			Wn.remr(i);
			T.erase(T.begin() + i);
			result = true;
		}
		/*else if (T[index(i)].definitely_vacuous && oa.size() == 1 && input_nodes(oa[0]).size() == 1 && (output_nodes(oa[0]).size() == 1 || input_nodes(trans_id(i)).size() == 1))
		{
			for (j = 0; j < (int)ia.size(); j++)
			{
				for (l = 0; l < (int)Wn[oa[0]].size(); l++)
				{
					Wn[ia[j]][l] = (Wn[ia[j]][l] || Wn[oa[0]][l]);
					Wp[ia[j]][l] = (Wp[ia[j]][l] || Wp[oa[0]][l]);
				}

				S[ia[j]].assumptions = S[ia[j]].assumptions >> S[oa[0]].assumptions;
				S[ia[j]].assertions.insert(S[ia[j]].assertions.end(), S[oa[0]].assertions.begin(), S[oa[0]].assertions.end());
			}

			remove_place(oa[0]);

			Wp.remr(i);
			Wn.remr(i);
			T.erase(T.begin() + i);
			result = true;
		}*/
		// All other transitions are fine
		else
			i++;
	}

	// Vacuous pbranches

	/**
	 * Whenever a branch in a parallel block has no transitions, it is a "vacuous branch".
	 * It is equivalent to saying that "nothing" is happening in parallel with "something".
	 * This can happen after the removal of vacuous transitions in a parallel block.
	 */
	i = 0;
	while (i < (int)S.size())
	{
		vacuous = true;
		for (j = 0; j < (int)T.size() && vacuous; j++)
		{
			vacuous = false;
			for (bi = S[i].pbranch.begin(); bi != S[i].pbranch.end() && !vacuous; bi++)
				if (find(T[j].pbranch.begin(), T[j].pbranch.end(), *bi) == T[j].pbranch.end())
					vacuous = true;
		}

		if (vacuous)
		{
			remove_place(i);
			result = true;
		}
		else
			i++;
	}

	/* Despite our best efforts to fix up the pbranch id's on the fly, we still
	 * need to clean up.
	 */
	trim_branch_ids();

	return result;
}

void petri::merge_conflicts()
{
	vector<int>				remove;
	vector<int>				ia, oa;
	vector<int>				vli, vlji, vljo;
	vector<vector<int> >	groups(S.size());
	vector<pair<logic, logic> > merge_value(S.size());	// the aggregated state encoding of input places and aggregated state encoding of output places for each group
	vector<pair<logic, logic> >::iterator ai;
	map<int, int>			pbranch, cbranch;
	int						i, j, k, p;
	bool					conflict;

	/**
	 * Identify the groups of indistinguishable states and keep track of their aggregate
	 * state encoding. Aggregated state encodings the of input and output places within a
	 * group must be tracked separately because the aggregate state encoding of input
	 * places is the binary AND of all of the individual state encodings and the aggregate
	 * state encoding of the output places is the binary OR of all of the individual state
	 * encodings. Finally, the two aggregate values are combined with a binary AND. The
	 * reason for this is that if you have two transitions that have the same input
	 * place, the state encoding for that input place must allow for either transition
	 * to fire at any time. That means that there cannot exist a state encoding within
	 * the input place that prevents one of the transitions from firing, hence binary AND
	 * (intersection). Also, if there are two transitions with the same output places,
	 * then either transition can fire and place a token in that place, hence binary OR
	 * (union).
	 */
	for (i = 0; i < (int)S.size(); i++)
	{
		groups[i].push_back(i);
		if (input_nodes(i).size() == 0)
		{
			merge_value[i].first = S[i].index;
			merge_value[i].second = 0;
		}
		else if (output_nodes(i).size() == 0)
		{
			merge_value[i].first = 1;
			merge_value[i].second = S[i].index;
		}
	}

	for (i = 0; i < (int)S.size(); i++)
	{
		p = (int)groups.size();
		for (j = 0; j < p; j++)
			if (find(groups[j].begin(), groups[j].end(), i) == groups[j].end())
			{
				ia = input_nodes(i);
				oa = output_nodes(i);

				conflict = true;
				for (k = 0; k < (int)groups[j].size() && conflict; k++)
					if ((S[i].index & S[groups[j][k]].index) == 0)
						conflict = false;

				/**
				 * To add a place into a conflict group, it's state encoding must be indistinguishable
				 * with the state encoding of every other place in that group.
				 */
				if (conflict)
				{
					vli.clear();
					vlji.clear();
					vljo.clear();

					S[i].index.vars(&vli);
					merge_value[j].first.vars(&vlji);
					merge_value[j].second.vars(&vljo);

					unique(&vli);
					unique(&vlji);
					unique(&vljo);

					/**
					 * Furthermore, if it is an input place, the amount of information that it's state
					 * encoding contains must be less than or equal to the amount of information contained
					 * in the state encoding of every output place currently in the group and exactly equal
					 * to the amount of state encoding in every input place currently in the group. If it
					 * is an output place, the amount of information that it's state encode contains must
					 * be greater than or equal to the amount of information contained in every input place
					 * currently in the group.
					 *
					 * For example if we start out knowing that the variable X is equal to 0, and then do a
					 * transition and that changes the value of X to 1, we can't suddenly know that the
					 * value of Y is 1 in order to execute the next transition. Also, if we know that
					 * X must be 1 to fire transition A, and X and Y must both be 1 to fire transition B,
					 * we can't suddenly know that Y is 1 every time we want to fire transition A.
					 */

					/* This check for information can be streamlined by checking which variables are
					 * included in the encoding, and not checking their value. The check to make sure that
					 * the state encodings are indistinguishable does the value check for us. For example, if we
					 * knew that X must be 1 to fire transition A, we can't suddenly know that X must also
					 * be 0 to fire transition B. Those two state encodings are not conflicting.
					 */
					/*if ((ia.size() == 0 && (vli == vlji && includes(vljo.begin(), vljo.end(), vli.begin(), vli.end()))) ||
						(oa.size() == 0 && (includes(vli.begin(), vli.end(), vlji.begin(), vlji.end()))))
					{*/
						groups.push_back(groups[j]);
						merge_value.push_back(merge_value[j]);
						groups[j].push_back(i);

						if (ia.size() == 0)
							merge_value[j].first = merge_value[j].first & S[i].index;

						if (oa.size() == 0)
							merge_value[j].second = merge_value[j].second | S[i].index;
					//}
				}
			}
	}

	/* The code segment above results in many duplicate groups and groups that
	 * are subsets of other groups. We need to remove these to make sure that we
	 * only get one place for each unique set of indistinguishable places.
	 */
	for (i = 0; i < (int)groups.size(); i++)
		unique(&(groups[i]));

	for (i = 0; i < (int)groups.size(); i++)
		for (j = 0; j < (int)groups.size(); j++)
			if (i != j && (includes(groups[i].begin(), groups[i].end(), groups[j].begin(), groups[j].end()) || (int)groups[j].size() <= 1))
			{
				groups.erase(groups.begin() + j);
				merge_value.erase(merge_value.begin() + j);
				j--;
				if (i > j)
					i--;
			}

	cout << "MERGES" << endl;
	for (i = 0; i < (int)groups.size(); i++)
	{
		for (j = 0; j < (int)groups[i].size(); j++)
			cout << groups[i][j] << " ";
		cout << endl;
	}
	cout << endl << endl;

	// Merge all of the places in each group identified above.
	for (i = 0; i < (int)groups.size(); i++)
	{
		pbranch.clear();
		cbranch.clear();
		for (j = 0; j < (int)groups[i].size(); j++)
		{
			pbranch.insert(S[groups[i][j]].pbranch.begin(), S[groups[i][j]].pbranch.end());
			cbranch.insert(S[groups[i][j]].cbranch.begin(), S[groups[i][j]].cbranch.end());
		}
		if (merge_value[i].second == 0)
			merge_value[i].second = 1;

		p = new_place((merge_value[i].first & merge_value[i].second), pbranch, cbranch, NULL);
		for (j = 0; j < (int)groups[i].size(); j++)
		{
			for (k = 0; k < (int)T.size(); k++)
			{
				Wn[p][k] = (Wn[p][k] || Wn[groups[i][j]][k]);
				Wp[p][k] = (Wp[p][k] || Wp[groups[i][j]][k]);
			}
			remove.push_back(groups[i][j]);
		}
	}

	unique(&remove);
	for (i = (int)remove.size()-1; i >= 0; i--)
		remove_place(remove[i]);
}

void petri::zip()
{
	vector<int> it0, ot0, it1, ot1, remove;
	vector<vector<int> > groups;
	map<int, int> pbranch;
	map<int, int>::iterator bi, bj;
	vector<pair<int, int> >::iterator ai;
	int i, j, k, p;

	/**
	 * First, check the places for possible merges, and then move on to the
	 * transitions because the act of merging places gives more information
	 * about the transitions to which they are connected.
	 *
	 * If two places have exactly the same neighborhood (the same sets of
	 * input and output arcs), then these two places are really just two
	 * possible state encodings for the same place. This generally happens
	 * directly after a conditional merge (where there is one state encoding
	 * for each possible path through the conditional). Instead of merging
	 * just two places at a time, we go ahead and merge all places that have
	 * the same neighborhoods.
	 */
	for (i = 0; i < (int)S.size(); i++)
		groups.push_back(vector<int>(1, i));
	for (i = 0; i < (int)S.size(); i++)
	{
		p = (int)groups.size();
		for (j = 0; j < p; j++)
			if (find(groups[j].begin(), groups[j].end(), i) == groups[j].end() && same_inputs(i, groups[j][0]) && same_outputs(i, groups[j][0]))
			{
				groups.push_back(groups[j]);
				groups[j].push_back(i);
			}
	}

	/* The code segment above results in many duplicate groups and groups that
	 * are subsets of other groups. We need to remove these to make sure that we
	 * only get one place for each unique neighborhood once we have merge all of
	 * the places in each individual group.
	 */
	for (i = 0; i < (int)groups.size(); i++)
		unique(&(groups[i]));
	unique(&groups);
	for (i = 0; i < (int)groups.size(); i++)
		for (j = 0; j < (int)groups.size(); j++)
			if (i != j && (includes(groups[i].begin(), groups[i].end(), groups[j].begin(), groups[j].end()) || (int)groups[j].size() <= 1))
			{
				groups.erase(groups.begin() + j);
				j--;
				if (i > j)
					i--;
			}

	for (i = 0; i < (int)groups.size(); i++)
		if (groups[i].size() > 1)
		{
			merge_places(groups[i]);
			remove.insert(remove.end(), groups[i].begin(), groups[i].end());
		}

	unique(&remove);
	for (i = (int)remove.size()-1; i >= 0; i--)
		remove_place(remove[i]);

	remove.clear();
	groups.clear();

	/**
	 * After checking the places for possible merge points, we also need to
	 * check the transitions. Logically, If there are multiple transitions that
	 * start at the same place and do the same thing, then they should also
	 * have the same result. So we can just merge their output arcs and delete
	 * all but one of the transitions.
	 */
	for (i = 0; i < (int)T.size(); i++)
		for (j = i+1; j < (int)T.size(); j++)
			if (T[i].index == T[j].index && input_nodes(trans_id(i)) == input_nodes(trans_id(j)))
			{
				for (k = 0; k < (int)S.size(); k++)
					Wp[k][i] = (Wp[k][i] || Wp[k][j]);

				intersect(T[i].pbranch, T[j].pbranch, &T[i].pbranch);
				intersect(T[i].cbranch, T[j].cbranch, &T[i].cbranch);

				remove.push_back(j);
			}

	unique(&remove);
	for (i = (int)remove.size()-1; i >= 0; i--)
	{
		T.erase(T.begin() + remove[i]);
		for (j = 0; j < (int)S.size(); j++)
		{
			Wn[j].erase(Wn[j].begin() + remove[i]);
			Wp[j].erase(Wp[j].begin() + remove[i]);
		}
	}

	/* Despite our best efforts to fix up the branch id's on the fly, we still
	 * need to clean up.
	 */
	trim_branch_ids();
}

void petri::get_paths(vector<int> from, vector<int> to, path_space *p)
{
	path_space t(arcs.size());
	vector<int> from_arcs, to_arcs, ex;
	int i;

	for (i = 0; i < (int)from.size(); i++)
		merge_vectors(&from_arcs, output_arcs(from[i]));
	for (i = 0; i < (int)to.size(); i++)
		merge_vectors(&to_arcs, input_arcs(to[i]));

	unique(&from_arcs);
	unique(&to_arcs);

	for (i = 0; i < (int)from_arcs.size(); i++)
	{
		t.clear();
		t.push_back(path(arcs.size()));
		ex = from_arcs;
		ex.erase(ex.begin() + i);
		arc_paths(from_arcs[i], to_arcs, ex, &t, p);
	}

	unique(&p->total.from);
	unique(&p->total.to);
}

int petri::arc_paths(int from, vector<int> to, vector<int> ex, path_space *t, path_space *p, int i)
{
	path_space tmp1(arcs.size()), tmp2(arcs.size());
	vector<int> next;
	vector<int> curr;
	map<int, pair<int, int> >::iterator cpi;
	int j, k;
	list<path>::iterator pi;
	bool immune = false;

	/*cout << string(i, '\t') << "Arc Paths " << from << " -> {";
	for (j = 0; j < (int)to.size(); j++)
	{
		if (j != 0)
			cout << ", ";
		cout << to[j];
	}
	cout << "}" << endl;
	*/
	if (i == 0)
	{
		t->paths.front().from.push_back(from);
		t->total.from.push_back(from);
	}

	next.push_back(from);

	while (1)
	{
		//cout << string(i, '\t') << next[0] << " ";

		if (find(ex.begin(), ex.end(), next[0]) != ex.end())
		{
			//cout << "EX " << t->total << endl;
			return -1;
		}

		for (pi = t->paths.begin(); pi != t->paths.end();)
		{
			if ((*pi)[next[0]] > 0)
				pi = t->erase(pi);
			else
				pi++;
		}

		if (t->paths.size() == 0)
		{
			//cout << "ALREADY COVERED " << t->total << endl;
			return -1;
		}

		if (!immune && i != 0 && is_trans(arcs[next[0]].first) && input_arcs(arcs[next[0]].first).size() > 1)
		{
			//cout << "PARALLEL MERGE " << t->total << endl;
			return next[0];
		}

		for (cpi = conditional_places.begin(); !immune && i != 0 && cpi != conditional_places.end(); cpi++)
			if (arcs[next[0]].first == cpi->second.first)
			{
				//cout << "CONDITIONAL MERGE " << t->total << endl;
				return next[0];
			}

		t->inc(next[0]);

		for (j = 0; j < (int)to.size(); j++)
			if (next[0] == to[j])
			{
				for (pi = t->paths.begin(); pi != t->paths.end(); pi++)
				{
					pi->to.push_back(to[j]);
					p->push_back(pi->mask());
				}
				//cout << "END " << t->total << endl;
				return -1;
			}

		curr = next;
		next.clear();
		for (j = 0; j < (int)arcs.size(); j++)
			for (k = 0; k < (int)curr.size(); k++)
				if (arcs[curr[k]].second == arcs[j].first)
					next.push_back(j);
		unique(&next);

		//cout << "\t" << t->total << endl;

		immune = false;
		if (next.size() > 1)
		{
			curr = next;
			tmp2 = *t;

			t->clear();
			next.clear();
			for (j = 0; j < (int)curr.size(); j++)
			{
				tmp1 = tmp2;
				next.push_back(arc_paths(curr[j], to, ex, &tmp1, p, i+1));
				if (next.back() == -1)
					next.pop_back();
				else
				{
					immune = true;
					t->merge(tmp1);
				}
			}

			unique(&next);
		}

		if (next.size() < 1)
		{
			//cout << "KILL" << endl;
			return -1;
		}
	}
}

void petri::filter_path_space(path_space *p)
{
	map<int, pair<int, int> >::iterator ci;
	vector<int> it, ip;
	list<path>::iterator pi;
	int i;
	int count, total;

	for (ci = conditional_places.begin(); ci != conditional_places.end(); ci++)
	{
		for (i = 0; i < (int)arcs.size(); i++)
			if (arcs[i].second == ci->second.first)
				it.push_back(i);

		for (i = 0, count = 0, total = 0; i < (int)it.size(); i++)
		{
			count += (p->total[it[i]] > 0);
			total++;
		}

		for (pi = p->paths.begin(); pi != p-> paths.end() && count > 0 && count < total; pi++)
			for (i = 0; i < (int)it.size(); i++)
				if ((*pi)[it[i]] > 0)
					filter_path(ci->second.second, it[i], &(*pi));

		for (int j = 0; j < (int)p->total.size(); j++)
			p->total[j] = 0;

		for (pi = p->paths.begin(); pi != p->paths.end(); pi++)
		{
			count = 0;
			for (int j = 0; j < (int)pi->size(); j++)
			{
				p->total[j] += (*pi)[j];
				count += (*pi)[j];
			}
			if (count == 0)
				pi = p->paths.erase(pi);
		}
	}
}

void petri::filter_path(int from, int to, path *p)
{
	vector<int> next;
	vector<int> it, ip;
	int i;

	next.push_back(to);

	while (next.size() == 1)
	{
		to = next[0];
		if (p->nodes[to] == 0)
			return;

		p->nodes[to] = 0;

		if (arcs[to].first == from || find(p->from.begin(), p->from.end(), arcs[to].first) != p->from.end() ||
							   	   	  find(p->to.begin(), p->to.end(), arcs[to].first) != p->to.end())
			return;

		next.clear();
		for (i = 0; i < (int)arcs.size(); i++)
			if (arcs[to].first == arcs[i].second)
				next.push_back(i);
	}

	for (i = 0; i < (int)next.size(); i++)
		filter_path(from, next[i], p);
}

void petri::zero_paths(path_space *paths, int from)
{
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].first == from || arcs[i].second == from)
			paths->zero(i);
}

void petri::zero_paths(path_space *paths, vector<int> from)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)arcs.size(); j++)
			if (arcs[j].first == from[i] || arcs[j].second == from[i])
				paths->zero(j);
}

void petri::zero_ins(path_space *paths, int from)
{
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].second == from)
			paths->zero(i);
}

void petri::zero_ins(path_space *paths, vector<int> from)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)arcs.size(); j++)
			if (arcs[j].second == from[i])
				paths->zero(j);
}

void petri::zero_outs(path_space *paths, int from)
{
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].first == from)
			paths->zero(i);
}

void petri::zero_outs(path_space *paths, vector<int> from)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)arcs.size(); j++)
			if (arcs[j].first == from[i])
				paths->zero(j);
}

vector<int> petri::start_path(int from, vector<int> ex)
{
	vector<int> result, oa;
	map<int, pair<int, int> >::iterator ci;
	map<int, int>::iterator bi, bj, bk;
	int j, k;
	bool same;

	result.push_back(from);
	for (bi = S[from].cbranch.begin(); bi != S[from].cbranch.end(); bi++)
	{
		ci = conditional_places.find(bi->first);
		if (ci != conditional_places.end())
		{
			oa = output_nodes(ci->second.second);
			for (j = 0; j < (int)oa.size(); j++)
			{
				bk = T[index(oa[j])].cbranch.find(bi->first);
				same = (bk->second == bi->second);

				for (k = 0; k < (int)ex.size() && !same; k++)
				{
					bj = (*this)[ex[k]].cbranch.find(bi->first);
					if (bj != (*this)[ex[k]].cbranch.end() && bj->second == bk->second)
						same = true;
				}

				if (!same)
					result.push_back(oa[j]);
			}
		}
		else
		{
			cout << "Error: Internal failure at line " << __LINE__ << " in file " << __FILE__ << "." << endl;
			cout << "Failed to find the conditional branch " << bi->first << "." << endl;
			print_branch_ids(&cout);
		}
	}
	unique(&result);

	/*cout << "Start {" << (from < 0 ? "T" : "S") << index(from) << "} -> {";
	for (j = 0; j < (int)result.size(); j++)
		cout << (result[j] < 0 ? "T" : "S") << index(result[j]) << " ";
	cout << "}" << endl;*/

	return result;
}

vector<int> petri::start_path(vector<int> from, vector<int> ex)
{
	vector<int> result, oa;
	map<int, pair<int, int> >::iterator ci;
	map<int, int>::iterator bi, bj, bk;
	int i, j, k;
	bool same = false;

	for (i = 0; i < (int)from.size(); i++)
	{
		result.push_back(from[i]);
		for (bi = S[from[i]].cbranch.begin(); bi != S[from[i]].cbranch.end(); bi++)
		{
			ci = conditional_places.find(bi->first);
			if (ci != conditional_places.end())
			{
				oa = output_nodes(ci->second.second);
				for (j = 0; j < (int)oa.size(); j++)
				{
					bk = T[index(oa[j])].cbranch.find(bi->first);

					same = false;
					for (k = 0; k < (int)from.size() && !same; k++)
					{
						bj = (*this)[from[k]].cbranch.find(bi->first);
						if (bj != (*this)[from[k]].cbranch.end() && bj->second == bk->second)
							same = true;
					}

					for (k = 0; k < (int)ex.size() && !same; k++)
					{
						bj = (*this)[ex[k]].cbranch.find(bi->first);
						if (bj != (*this)[ex[k]].cbranch.end() && bj->second == bk->second)
							same = true;
					}

					if (!same)
						result.push_back(oa[j]);
				}
			}
			else
			{
				cout << "Error: Internal failure at line " << __LINE__ << " in file " << __FILE__ << "." << endl;
				cout << "Failed to find the conditional branch " << bi->first << "." << endl;
				print_branch_ids(&cout);
			}
		}
	}
	unique(&result);

	/*cout << "Start {";
	for (i = 0; i < (int)from.size(); i++)
		cout << (from[i] < 0 ? "T" : "S") << index(from[i]) << " ";
	cout << "} -> {";

	for (i = 0; i < (int)result.size(); i++)
		cout << (result[i] < 0 ? "T" : "S") << index(result[i]) << " ";

	cout << "}" << endl;*/

	return result;
}

vector<int> petri::end_path(int to, vector<int> ex)
{
	vector<int> result, oa;
	map<int, pair<int, int> >::iterator ci;
	map<int, int>::iterator bi, bj, bk;
	int j, k;
	bool same;

	result.push_back(to);
	for (bi = S[to].cbranch.begin(); bi != S[to].cbranch.end(); bi++)
	{
		ci = conditional_places.find(bi->first);
		if (ci != conditional_places.end())
		{
			oa = input_nodes(ci->second.first);
			for (j = 0; j < (int)oa.size(); j++)
			{
				bk = T[index(oa[j])].cbranch.find(bi->first);
				same = (bk->second == bi->second);

				for (k = 0; k < (int)ex.size() && !same; k++)
				{
					bj = (*this)[ex[k]].cbranch.find(bi->first);
					if (bj != (*this)[ex[k]].cbranch.end() && bj->second == bk->second)
						same = true;
				}

				if (!same)
					result.push_back(oa[j]);
			}
		}
		else
		{
			cout << "Error: Internal failure at line " << __LINE__ << " in file " << __FILE__ << "." << endl;
			cout << "Failed to find the conditional branch " << bi->first << "." << endl;
			print_branch_ids(&cout);
		}
	}
	unique(&result);

	/*cout << "End {" << (to < 0 ? "T" : "S") << index(to) << "} -> {";
	for (j = 0; j < (int)result.size(); j++)
		cout << (result[j] < 0 ? "T" : "S") << index(result[j]) << " ";
	cout << "}" << endl;*/

	return result;
}

vector<int> petri::end_path(vector<int> to, vector<int> ex)
{
	vector<int> result, oa;
	map<int, pair<int, int> >::iterator ci;
	map<int, int>::iterator bi, bj, bk;
	int i, j, k;
	bool same = false;

	for (i = 0; i < (int)to.size(); i++)
	{
		result.push_back(to[i]);
		for (bi = S[to[i]].cbranch.begin(); bi != S[to[i]].cbranch.end(); bi++)
		{
			ci = conditional_places.find(bi->first);
			if (ci != conditional_places.end())
			{
				oa = input_nodes(ci->second.first);
				for (j = 0; j < (int)oa.size(); j++)
				{
					bk = T[index(oa[j])].cbranch.find(bi->first);

					same = false;
					for (k = 0; k < (int)to.size() && !same; k++)
					{
						bj = (*this)[to[k]].cbranch.find(bi->first);
						if (bj != (*this)[to[k]].cbranch.end() && bj->second == bk->second)
							same = true;
					}

					for (k = 0; k < (int)ex.size() && !same; k++)
					{
						bj = (*this)[ex[k]].cbranch.find(bi->first);
						if (bj != (*this)[ex[k]].cbranch.end() && bj->second == bk->second)
							same = true;
					}

					if (!same)
						result.push_back(oa[j]);
				}
			}
			else
			{
				cout << "Error: Internal failure at line " << __LINE__ << " in file " << __FILE__ << "." << endl;
				cout << "Failed to find the conditional branch " << bi->first << "." << endl;
				print_branch_ids(&cout);
			}
		}
	}
	unique(&result);

	/*cout << "End {";
	for (i = 0; i < (int)to.size(); i++)
		cout << (to[i] < 0 ? "T" : "S") << index(to[i]) << " ";
	cout << "} -> {";

	for (i = 0; i < (int)result.size(); i++)
		cout << (result[i] < 0 ? "T" : "S") << index(result[i]) << " ";

	cout << "}" << endl;*/

	return result;
}


node &petri::operator[](int i)
{
	if (is_place(i))
		return S[i];
	else
		return T[index(i)];
}

void petri::print_dot(ostream *fout, string name)
{
	int i, j, k;
	string label;
	gen_arcs();
	(*fout) << "digraph " << name << endl;
	(*fout) << "{" << endl;

	for (i = 0; i < (int)S.size(); i++)
		if (!dead(i))
		{
			(*fout) << "\tS" << i << " [label=\"" << to_string(i) << " ";
			label = S[i].index.print(vars);

			for (j = 0, k = 0; j < (int)label.size(); j++)
				if (label[j] == '|')
				{
					(*fout) << label.substr(k, j+1 - k) << "\\n";
					k = j+1;
				}

			(*fout) << label.substr(k) << "\"];" << endl;
		}

	for (i = 0; i < (int)T.size(); i++)
	{
		label = T[i].index.print(vars);
		label = to_string(i) + " " + label;
		if (label != "")
			(*fout) << "\tT" << i << " [shape=box] [label=\"" << label << "\"];" << endl;
		else
			(*fout) << "\tT" << i << " [shape=box];" << endl;
	}

	for (i = 0; i < (int)arcs.size(); i++)
		(*fout) << "\t" << (is_trans(arcs[i].first) ? "T" : "S") << index(arcs[i].first) << " -> " << (is_trans(arcs[i].second) ? "T" : "S") << index(arcs[i].second) << "[label=\" " << i << " \"];" <<  endl;

	(*fout) << "}" << endl;
}

void petri::print_mutables()
{
	for (int i = 0; i < (int)S.size(); i++)
	{
		(*flags->log_file) << "S" << i << ": ";
		for (map<int, logic>::iterator j = S[i].mutables.begin(); j != S[i].mutables.end(); j++)
			(*flags->log_file) << "{" << vars->get_name(j->first) << " " << j->second.print(vars) << "} ";
		(*flags->log_file) << endl;
	}
}

void petri::print_branch_ids(ostream *fout)
{
	for (int i = 0; i < (int)S.size(); i++)
	{
		(*fout) << "S" << i << ": ";
		for (map<int, int>::iterator j = S[i].pbranch.begin(); j != S[i].pbranch.end(); j++)
			(*fout) << "p{" << j->first << " " << j->second << "} ";
		for (map<int, int>::iterator j = S[i].cbranch.begin(); j != S[i].cbranch.end(); j++)
			(*fout) << "c{" << j->first << " " << j->second << "} ";
		(*fout) << endl;
	}
	for (int i = 0; i < (int)T.size(); i++)
	{
		(*fout) << "T" << i << ": ";
		for (map<int, int>::iterator j = T[i].pbranch.begin(); j != T[i].pbranch.end(); j++)
			(*fout) << "p{" << j->first << " " << j->second << "} ";
		for (map<int, int>::iterator j = T[i].cbranch.begin(); j != T[i].cbranch.end(); j++)
			(*fout) << "c{" << j->first << " " << j->second << "} ";
		(*fout) << endl;
	}
	(*fout) << endl;
}
