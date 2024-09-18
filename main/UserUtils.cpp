#include "Utils.h"
#include "UserUtils.h"

int isTxtFile(char * filename, int fnlen) {
	if (__memcmp(filename + fnlen - 4, ".txt", 4) == 0 || __memcmp(filename + fnlen - 4, ".asm", 4) == 0 ||
		__memcmp(filename + fnlen - 4, ".json", 4) == 0 || __memcmp(filename + fnlen - 4, ".cpp", 4) == 0 ||
		__memcmp(filename + fnlen - 2, ".c", 2) == 0 || 
		__memcmp(filename + fnlen - 4, ".htm", 4) == 0 || __memcmp(filename + fnlen - 5, ".html", 5) == 0 ||
		__memcmp(filename + fnlen - 4, ".php", 4) == 0 || __memcmp(filename + fnlen - 5, ".java", 5) == 0 ||
		__memcmp(filename + fnlen - 3, ".js", 3) == 0 || __memcmp(filename + fnlen - 3, ".py", 3) == 0 ||
		__memcmp(filename + fnlen - 4, ".log", 4) == 0 || __memcmp(filename + fnlen - 4, ".ini", 4) == 0 || 
		__memcmp(filename + fnlen - 4, ".bat", 4) == 0)
	{
		return TRUE;
	}
	return FALSE;
}


int getPrevPath(char * path) {
	int len = __strlen(path);
	int flag = 0;
	for (int i = len - 1; i >= 0; i --)
	{
		if (path[i] == '/' || path[i] == '\\')
		{
			if (flag)
			{
				*(path + i + 1) = 0;
				return i;
			}
			else {
				flag = TRUE;
			}
		}
	}

	*path = 0;
	return FALSE;
}


int parseCmdParams(char * cmd, char params[COMMAND_LINE_STRING_LIMIT][COMMAND_LINE_STRING_LIMIT]) {
	char * str = cmd;
	int counter = 0;

	int flag = FALSE;

	for (int i = 0; i <= __strlen(cmd); i++)
	{
		if (cmd[i] == '\t' || cmd[i] == ' ' || cmd[i] == 0)
		{
			if (flag )
			{
				int len = &cmd[i] - str;
				if (len > 0 && len < COMMAND_LINE_STRING_LIMIT)
				{
					__memcpy(params[counter], str, len);
					*(params[counter] + len) = 0;
					counter++;
				}
				else {
					//show error
				}

				//str = cmd + i + 1;

				flag = FALSE;
			}
		}
		else {
			if (flag == FALSE)
			{
				flag = TRUE;
				str = &cmd[i];
			}
		}
	}

	return counter;
}


int beEndWith(char * str,char * flag) {
	int len = __strlen(str);
	int flaglen = __strlen(flag);
	if (__memcmp(str + len - flaglen,flag,flaglen) == 0)
	{
		return TRUE;
	}
	return FALSE;
}