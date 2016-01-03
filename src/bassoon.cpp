#include "../include/bassoon.h"
#include <dchain/dchain.h>
#include <nihdb/nihdb.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <limits>

#define DBFILE ".passwords"

void toggleEcho(bool state)
{
	if (state)
		system("/usr/bin/stty echo");
	else
		system("/usr/bin/stty -echo");
}

void printHelp()
{
	std::cout << "Usage: command [options] (arguments)\n";
}

int initUser()
{
	std::string temp = std::getenv("HOME");
	temp += "/";
	temp += DBFILE;
	char ans;
	std::ofstream createDB(temp.c_str());
	nihdb::dataBase tempDB(temp);
	temp = "bassoon passsword database for ";
	temp += std::getenv("USER");
	tempDB.AddComment(temp);
	tempDB.AddComment("All sensitive data is encrypted with dchain encryption library");
	tempDB.CreateSection("meta");
	tempDB.CreateVar("meta", "name", std::getenv("USER"));
	tempDB.CreateVar("meta", "twostep", "false");
	tempDB.CreateVar("meta", "pass1");
	tempDB.CreateVar("meta", "pass2");
	tempDB.CreateVar("meta", "items");
	std::cout << "Do you want to use two-step authentication? (two passwords instead of one)? [y/n]: ";
	std::cin >> ans;
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	if ( (ans == 'y') || (ans == 'Y')) {
		tempDB.ChangeVarValue("meta", "twostep", "true");
	}

	bool btemp = false;
	std::string pass1, pass2;

	std::cout << "Password will not be visible while you type them\n";
	while (!btemp)
	{
		std::cout << "Enter password: ";
		toggleEcho(false);
		std::getline(std::cin, pass1);
		toggleEcho(true);
		std::cout << '\n';
		std::cout << "Confirm password: ";
		toggleEcho(false);
		std::getline(std::cin, pass2);
		toggleEcho(true);
		std::cout << '\n';

		btemp = (pass1 == pass2);
		if (!btemp) {
			std::cout << "Passwords do not match, please try again\n";
		}
	}
	tempDB.ChangeVarValue("meta", "pass1", dchain::strEncrypt(pass1, pass1));

	if (tempDB.ReturnVar("meta", "twostep") != "true") {
		if (!tempDB.ApplyChanges()) {
			std::cout << "Error saving database, please make sure that ~/" << DBFILE << "is read and writeable.";
			return 1;
		}
		std::cout << "Password database successfully created - please restart bassoon\n";
		return 0;
	}

	btemp = false;
	while (!btemp)
	{
		std::cout << "Enter second password: ";
		toggleEcho(false);
		std::getline(std::cin, pass1);
		toggleEcho(true);
		std::cout << '\n';
		std::cout << "Confirm password: ";
		toggleEcho(false);
		std::getline(std::cin, pass2);
		toggleEcho(true);
		std::cout << '\n';

		btemp = (pass1 == pass2);
		if (!btemp) {
			std::cout << "Passwords do not match, please try again\n";
		}
	}

	tempDB.ChangeVarValue("meta", "pass2", dchain::strEncrypt(pass1, pass1));

	if (!tempDB.ApplyChanges()) {
		std::cout << "Error saving database, please make sure that ~/" << DBFILE << " is read and writeable.";
		return 1;
	}
	std::cout << "Password database successfully created - please restart bassoon\n";
	return 0;
}

int startCLI()
{
	std::string temp = std::getenv("HOME");
	temp += "/";
	temp += DBFILE;
	nihdb::dataBase datb(temp);

	if (!datb.IsParsed()){
		std::cerr << "Database corrupt... Aborting";
		return 1;
	}

	int count = 0;
	bool btemp = false;
	std::string pass1, pass2;
	bool twostep = (datb.ReturnVar("meta", "twostep") == "true");
	while (!btemp)
	{
		std::cout << "Enter password: ";
		toggleEcho(false);
		std::getline(std::cin, pass1);
		toggleEcho(true);
		std::cout << '\n';
		if (twostep) {
			std::cout << "Enter second password: ";
			toggleEcho(false);
			std::getline(std::cin, pass2);
			toggleEcho(true);
			std::cout << '\n';
		}

		btemp = (pass1 == dchain::strDecrypt(datb.ReturnVar("meta", "pass1"), pass1));
		if (twostep)
			btemp = btemp && (pass2 == dchain::strDecrypt(datb.ReturnVar("meta", "pass2"), pass2));

		if (!btemp) {
			count++;
			std::cout << "Incorrect password(s) (" << count << " out of 3 tries)";
			if (count == 3) {
				std::cout << "Too many incorrect passwords. Aborting...";
				return 1;
			}
		}
	}

	std::vector<std::string> items;
	temp = datb.ReturnVar("meta", "items");
	if (temp == "empty") {
		temp.clear();
	}
	std::string foo;
	for (int i = 0; i < temp.length(); i++)
	{
		if (temp[i] == ' ') {
			items.push_back(foo);
			foo.clear();
		} else
			foo += temp[i];
	}

	return 0;
}

int main(int argc, char* arcgs[])
{
	std::string temp = std::getenv("HOME");
	temp += "/";
	temp += DBFILE;
	std::ifstream test(temp.c_str());

	if (!test.is_open()) {
		std::cout << "Error, password database not found. Would you like to create one? [y/n]: ";
		char ans;
		std::cin >> ans;
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		if ( (ans != 'Y') &&  (ans != 'y')){
			std::cerr << ".password file not created. Aborting...\n";
			return 1;
		}

		test.close();
		return initUser();
	}
	test.close();
	return startCLI();
}
