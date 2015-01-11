#include <iostream>
#include "systemc.h"
#include "parser.cpp"
#include <regex>

using namespace std;
/*
SC_MODULE(test){
	sc_in<sc_logic> input;
	sc_out<sc_logic> output;

	void p1(){
		while(true)
		{
			wait(input.negedge_event());

		}
	}
}*/

void main(int &argc,char &argv)
{
	char c;
	//const string text="entity shiftreg is\n\tport (clk    : in  std_logic;\n         reset  : in  std_logic;\n         din    : in  std_logic;\n\         dout   : out std_logic);\nend shiftreg;";
	VHDLParser prs("VHDL.txt",0);
	string tmp;
	prs.MakeModel();


	scanf("%c",&c);


	return;
}