#pragma once

#include "def.h"
#include "video.h"


#define INPUT_TEXTMODE_COLOR		0X0f
#define OUTPUT_TEXTMODE_COLOR		0X0a

#define LINE_CHAR_COUNT				80
#define ROW_CHAR_COUNT				25
#define LINE_SIZE					(LINE_CHAR_COUNT<<1)

#define TEXTMODE_BASE				0XB8000

#define TEXTMODE_BUF_SIZE			0X8000


int runcmd(char* cmd);

int outputStr(char* str, char color);

int outputChar(char c, char color);

int setCursor(int pos);

int clearTextScreen();

int setScreenPos(int pos);

extern "C" __declspec(dllexport) int __kTextModeEntry(LPVESAINFORMATION vesa, DWORD fontbase, DWORD v86ProcessBase, int v86ProcessLen,
	DWORD v86IntBase, DWORD kerneldata, DWORD kernel16, DWORD kernel32);