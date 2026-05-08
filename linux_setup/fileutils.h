#pragma once


#include <iostream>
#include "public.h"

using namespace std;

class FileOper {
public:
	static int fileWriter(string filename, const char * lpdate, int datesize, int cover);
	static	int isFileExist(string filename);
	static	int getFileSize(string filename);
	static	string getDateTime();
	static	int fileReader(string filename, char ** lpbuf, int *bufsize);

};
