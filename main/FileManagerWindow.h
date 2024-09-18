#pragma once

#pragma pack(1)


typedef struct 
{
// 	POINT pos;
// 	int width;
// 	int height;
// 
// 	int frameSize;
// 	int frameColor;
// 
// 	int backcolor;
// 	char caption[64];
// 	char winname[64];
// 	
// 	int capHeight;
// 	int capColor;
// 
// 	int zoomin;
// 	int showY;
// 	int showX;
// 	int id;
// 	int pid;
// 	int top;
// 	int left;
// 	int right;
// 	int bottom;
// 
// 	int shutdownx;
// 	int shutdowny;
// 
// 	int fontcolor;
// 	int windowid;
// 	DWORD backGround;
// 	DWORD backsize;

	WINDOWCLASS window;

	int cpl;
	int fsheight;

}FMWINDOW,*LPFMWINDOW;


int restoreFileManager(LPFMWINDOW w);

int drawFileManager(LPFMWINDOW w);

#pragma pack()