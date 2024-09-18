
#include "../Utils.h"
#include "FAT32.h"

int removeDriver(char *  path, char * dst) {

	int len = __strlen(path);
	if (len <= 0)
	{
		return 0;
	}
	if (*(path + len - 1) == '\\' || *(path + len - 1) == '/')
	{
		*(path + len - 1) = 0;
	}

	if ((__strlen(path) >= 3) && (path[1] == ':') && (path[2] == '\\' || path[2] == '/'))
	{
		__substr(path, 3, dst);
	}
	else {
		__strcpy(dst, path);
	}

	return 0;
}

int getNextPath(char * curpath,char * path) {

	if (path[0] == '/' || path[0] == '\\')
	{
		__strcpy(path, path + 1);
	}

	int pathlen = __strlen(path);

	for (int i = 0;i <pathlen; i ++ )
	{
		if (path[i] == '/' || path[i] == '\\' )
		{
			__memcpy(curpath, path, i);
			curpath[i] = 0;
			__strcpy(path, path + i + 1);
			return i;
		}
	}

	__strcpy(curpath, path);
	path[0] = 0;
	return 0;

// 	char * pos = __strstr(leastpath,"\\");
// 	if (pos == 0)
// 	{
// 		pos = __strstr(leastpath,"/");
// 		if (pos == 0)
// 		{
// 			__strcpy(curpath, leastpath);
// 			leastpath[0] = 0;
// 			return 0;
// 		}
// 	}
// 
// 	__substrLen(leastpath, 0, pos - leastpath, curpath);
// 
// 	__strcpy(leastpath,  pos + 1);
	
	return 0;
}


int getMainNameFromName(char * fullname, char * dst) {

	int len = __strlen(fullname);
	for (int i = len - 1; i >= 0; i--)
	{
		if (fullname[i] == '.')
		{
			__memcpy(dst, fullname, i);
			*(dst + i) = 0;
			return i;
		}
	}

	return 0;
}


int getNameFormFullName(char * fullname, char * dst) {
	int len = __strlen(fullname);

	for (int i = len - 1; i >= 0; i--)
	{
		if (fullname[i] == '\\' || fullname[i] == '/')
		{
			__substr(fullname, i + 1, dst);
			return __strlen(dst);
		}
	}

	return 0;
}


int getPathFromFullName(char * fullname,char * dst) {

	int len = __strlen(fullname);

	for (int i = len - 1; i >= 0; i--)
	{
		if (fullname[i] == '\\' || fullname[i] == '/')
		{
			__memcpy(dst, fullname, i);
			*(dst + i) = 0;
			return i;
		}
	}

	return 0;
}

int multiBytes2WideChar( char * str, wchar_t * unicode) {
	wchar_t * pu = unicode;
	char * p = str;

	while (1)
	{
		unsigned char c = *p;
		if (c == 0)
		{
			break;
		}
		*pu = c;

		p++;
		pu++;
	}

	*pu = 0;
	return pu - unicode;
}


int wideChar2MultiBytes(wchar_t * unicode,char * str) {
	wchar_t * pu = unicode;
	char * p = str;

	while (1)
	{
		wchar_t wc = *pu;
		if (wc == 0)
		{
			break;
		}
		*p = wc & 0xff;

		p++;
		pu++;
	}

	*p = 0;
	return p - str;
}






