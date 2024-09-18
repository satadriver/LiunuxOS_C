#pragma once
#include "../def.h"


int getNextPath(char * curpath, char * leastpath);
int getNameFormFullName(char * fullname, char * dst);
int getMainNameFromName(char * fullname, char * dst);
int getPathFromFullName(char * fullname, char * dst);
int removeDriver(char *  path, char * dst);

int wideChar2MultiBytes(wchar_t * unicode, char * str);
int multiBytes2WideChar(char * str, wchar_t * unicode);

