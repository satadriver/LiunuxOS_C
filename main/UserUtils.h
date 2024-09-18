#pragma once


#include "def.h"

#define COMMAND_LINE_STRING_LIMIT	64

int isTxtFile(char * filename, int fnlen);

int getPrevPath(char * path);

int parseCmdParams(char * cmd, char params[COMMAND_LINE_STRING_LIMIT][COMMAND_LINE_STRING_LIMIT]);

int beEndWith(char * str, char * flag);