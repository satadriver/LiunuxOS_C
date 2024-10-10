#pragma once
#pragma once
#include "video.h"
#include "Utils.h"
#include "def.h"

#define COMMAND_LINE_STRING_LIMIT	32

int parseCmdParams(char* cmd, char params[COMMAND_LINE_STRING_LIMIT][COMMAND_LINE_STRING_LIMIT]);

extern "C" int __cmd(char* cmd, WINDOWCLASS * window, char* filename, int pid);