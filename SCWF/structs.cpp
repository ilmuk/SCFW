#pragma once
#include <iostream>
#include <vector>

#define P_INPUT 0
#define P_OUTPUT 1
#define P_INOUT 2
#define P_SIGNAL 3

using namespace std; 

struct dtype{
	string name;
	int count;
	int offset;
	dtype* arr_type;
};

struct port{
	string name; 
	unsigned char ptype;
	dtype dtp;
	int count;
};

struct module{
	string name;
	vector<port> ports;
	vector<module> components;
	string logic;
};

struct strct{
	string name;
	vector<port> components;
	dtype dtp;
};

struct proc{
	string name;
	vector<string> sensitive;
	vector<string> logic_tokens;
};