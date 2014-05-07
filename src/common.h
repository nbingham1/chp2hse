/*

 * common.h
 *
 * Common is a collection of functions not specific to the compiler that
 * we found useful to define. Note that our #defines for user flags are
 * also stored in common.h as it is accessed by everyone.
 */

#ifndef common_h
#define common_h

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <sstream>
#include <math.h>
#include <sys/time.h>

#include <vector>
#include <list>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <iterator>

using namespace std;

template <class type>
string to_string(type value)
{
	ostringstream os;
	os << value;
	return os.str();
}

bool ac(char c);
bool nc(char c);
bool oc(char c);
bool sc(char c);

int hex_to_int(string str);
int dec_to_int(string str);
int bin_to_int(string str);
string hex_to_bin(string str);
string dec_to_bin(string str);

unsigned int count_1bits(unsigned int x);
unsigned int count_0bits(unsigned int x);

int powi(int base, int exp);
int log2i(unsigned long long value);

uint32_t bitwise_or(uint32_t a, uint32_t b);
uint32_t bitwise_and(uint32_t a, uint32_t b);
uint32_t bitwise_not(uint32_t a);

string readfile(string filename);

//#define DEBUG

#endif
