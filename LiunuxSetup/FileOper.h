#pragma once

#include <windows.h>
#include <iostream>

using namespace std;

class FileOper {
public:
	static int FileOper::fileWriter(string filename, const char * lpdate, int datesize, int cover);
	static	int FileOper::isFileExist(string filename);
	static	int FileOper::getFileSize(string filename);
	static	string FileOper::getDateTime();
	static	int FileOper::fileReader(string filename, char ** lpbuf, int *bufsize);

};