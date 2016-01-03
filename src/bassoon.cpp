#include "../include/bassoon.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

void printHelp()
{
	std::cout << "Usage: command [options] (arguments)\n";
}

int main(int argc, char* arcgs[])
{
	std::string temp = std::getenv("HOME");
	temp += ".passwords";
	std::ifstream config(temp.c_str());

	if (!config.is_open()) {
		std::cout << "Error, password database not found. Would you like to create one? [y/n]: ";
		char ans;
		std::cin >> ans;

		if ( (ans != 'Y') &&  (ans != 'y')){
			std::cerr << ".password file not created. Aborting...\n";
			return 1;
		}
	}
}
