#include "../include/bassoon.h"
#include <dchain/dchain.h>
#include <nihdb/nihdb.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <limits>

void printHelp()
{
	std::cout << "Usage: command [options] (arguments)\n";
}

int initUser()
{
	std::string temp = std::getenv("HOME");
	temp += "/.passwords";
	char ans;
	std::ofstream createDB(temp.c_str());
	nihdb::dataBase tempDB(temp);
	temp = "bassoon passsword database for ";
	temp += std::getenv("USER");
	tempDB.AddComment(temp);
	tempDB.AddComment("All sensitive data is encrypted with dchain encryption library");
	tempDB.CreateSection("user");
	tempDB.CreateVar("user", "name", std::getenv("USER"));
	tempDB.CreateVar("user", "twostep", "false");
	tempDB.CreateVar("user", "pass1");
	tempDB.CreateVar("user", "pass2");
	std::cout << "Do you want to use two-step authentication? (two passwords instead of one)? [y/n]: ";
	std::cin >> ans;
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	if ( (ans == 'y') || (ans == 'Y')) {
		tempDB.ChangeVarValue("user", "twostep", "true");
	}

	bool btemp = false;
	std::string pass1, pass2;

	std::cout << "Password will not be visible while you type them\n";
	while (!btemp)
	{
		std::cout << "Enter password: ";
		system("/usr/bin/stty -echo");
		std::getline(std::cin, pass1);
		std::cout << '\n';
		system("/usr/bin/stty echo");
		std::cout << "Confirm password: ";
		system("/usr/bin/stty -echo");
		std::getline(std::cin, pass2);
		system("/usr/bin/stty echo");
		std::cout << '\n';

		btemp = (pass1 == pass2);
		if (!btemp) {
			std::cout << "Passwords do not match, please try again\n";
		}
	}
	tempDB.ChangeVarValue("user", "pass1", dchain::strEncrypt(pass1, pass1));

	if (tempDB.ReturnVar("user", "twostep") != "true") {
		if (!tempDB.ApplyChanges()) {
			std::cout << "Error saving database, please make sure that ~/.passwords is read and writeable.";
			return 1;
		}
		std::cout << "Password database successfully created - please restart bassoon\n";
		return 0;
	}

	btemp = false;
	while (!btemp)
	{
		std::cout << "Enter second password: ";
		system("/usr/bin/stty -echo");
		std::getline(std::cin, pass1);
		std::cout << '\n';
		system("/usr/bin/stty echo");
		std::cout << "Confirm password: ";
		system("/usr/bin/stty -echo");
		std::getline(std::cin, pass2);
		system("/usr/bin/stty echo");
		std::cout << '\n';

		btemp = (pass1 == pass2);
		if (!btemp) {
			std::cout << "Passwords do not match, please try again\n";
		}
	}

	tempDB.ChangeVarValue("user", "pass2", dchain::strEncrypt(pass1, pass1));

	if (!tempDB.ApplyChanges()) {
		std::cout << "Error saving database, please make sure that ~/.passwords is read and writeable.";
		return 1;
	}
	std::cout << "Password database successfully created - please restart bassoon\n";
	return 0;
}

int main(int argc, char* arcgs[])
{
	std::string temp = std::getenv("HOME");
	temp += "/.passwords";
	std::ifstream config(temp.c_str());

	if (!config.is_open()) {
		std::cout << "Error, password database not found. Would you like to create one? [y/n]: ";
		char ans;
		std::cin >> ans;
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		if ( (ans != 'Y') &&  (ans != 'y')){
			std::cerr << ".password file not created. Aborting...\n";
			return 1;
		}

		config.close();
		return initUser();
	}
}
