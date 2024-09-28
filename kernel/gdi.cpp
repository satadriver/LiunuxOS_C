
#include "gdi.h"
#include "Utils.h"
#include "math.h"
#include "file.h"
#include "malloc.h"
#include "cmosPeriodTimer.h"
#include "video.h"
#include "cmosExactTimer.h"
#include "hardware.h"

float f(float x, float y, float z) {
	return x*x + y*y + z*z - 1;
}

float h(float x, float z) {
	//获取表面(解方程的过程)
	//这个函数说明 Y轴 垂直纸面向外 右手坐标系
	for (float y = 1.0f; y >= 0.0f; y -= 0.001f) {
		if (f(x, y, z) <= 0.0f)
			return y;
	}
	return 0.0f;
}


void sphere(int x,int y,int raduis,DWORD color,unsigned char * backup) {

// 	__asm {
// 		clts
// 		fwait
// 		fninit
// 	}
	unsigned int pos = __getpos(x - raduis, y - raduis);
	unsigned char * showpos = pos + (unsigned char *)gGraphBase;
	unsigned char * keepy = showpos;


	DWORD r = 0;

	int width = raduis*2, height = raduis*2;
	for (int sy = 0; sy < height; sy++) {
		float z = 1.5f - sy * 3.0f / height;//按比例从屏幕像素转到模型大小的尺度
		for (int sx = 0; sx < width; sx++) {
			float x = sx *3.0f / width - 1.5f;//按比例从屏幕像素转到模型大小的尺度

			if (f(x, 0.0f, z) <= 0.0f) {//属于球体
										//球面法向量公式d=[x,y,z]
				float y = h(x, z);
				float cosA = (-x + y + z) / sqrt(x*x + y*y + z*z) / sqrt(3);//(光线向量：[1,-1,-1])
				float d = cosA*0.5f + 0.5f;//wrapped diffuse处理
											//float d = cosA+0.2f>1.0f?1.0f:cosA+0.2f;//加入全局光处理

				r = (DWORD)(d*255.0f);

			}
			else {
				r = color;
			}

			char * data = (char*)&r;
			for (int i = 0; i < gBytesPerPixel; i++)
			{
				*backup = *showpos;
				backup++;

				*showpos = data[i];
				showpos++;
			}
		}
		keepy += gBytesPerLine;
		showpos = keepy;
	}
}


void sphere7(int x, int y, int raduis,DWORD color,unsigned char *backup) {
 	__asm {
 		//clts
 		//fwait
 		//fninit
 	}
	unsigned int pos = __getpos(x-raduis, y-raduis);
	unsigned char * showpos = pos + (unsigned char *)gGraphBase;
	unsigned char * keepy = showpos;
	int width = raduis*2, height = raduis*2;

	DWORD c = 0;

	for (int sy = 0; sy < height; sy++) {
		float z = 1.5f - sy * 3.0f / height;//按比例从屏幕像素转到模型大小的尺度
		for (int sx = 0; sx < width; sx++) {
			float x = sx *3.0f / width - 1.5f;//按比例从屏幕像素转到模型大小的尺度

			if (f(x, 0.0f, z) <= 0.0f) {//属于球体
										//球面法向量公式d=[x,y,z]
				float y = h(x, z);
				float cosA = (-x + y + z) / sqrt(x*x + y*y + z*z) / sqrt(3);//(光线向量：[-1,1,1])

																			  //漫反射，wrapped diffuse处理
				float diffuse = cosA*0.5f + 0.5f;
				//float d = cosA+0.2f>1.0f?1.0f:cosA+0.2f;//加入全局光处理

				//高光分
				float specular = pow(cosA, 64.0);

				int r = (int)(diffuse * 255.0f + specular * 200.0f);
				int g = (int)(diffuse * 255.0f + specular * 200.0f);
				int b = (int)(diffuse * 255.0f + specular * 200.0f);
// 				int g = (int)(specular * 200.0f);
// 				int b = (int)(specular * 200.0f);
				r = r > 255 ? 255 : r;
				g = g > 255 ? 255 : g;
				b = b > 255 ? 255 : b;

				c = (r << 16) + (g << 8) + b;
			}
			else {
				c = color;
			}
			char * data = (char*)&c;
			for (int i = 0; i < gBytesPerPixel; i++)
			{
				*backup = *showpos;
				backup++;

				*showpos = data[i];
				showpos++;
			}
		}
		keepy += gBytesPerLine;
		showpos = keepy;
	}

}



DWORD showIcon(int x,int y, LPBITMAPINFOHEADER lpbmpinfo) {

	int width = lpbmpinfo->biWidth;

	int height = lpbmpinfo->biHeight;

	int bitspix = lpbmpinfo->biBitCount>>3;

	unsigned char * data = (unsigned char*)lpbmpinfo + sizeof(BITMAPINFOHEADER);

	unsigned char * ptr = (unsigned char*)__getpos(x, y) + gGraphBase;

	unsigned char * keep = ptr;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned char * p = ptr;
			unsigned char * d = data;
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*p = *d;
				p++;
				d++;
			}

			ptr += gBytesPerPixel;
			data += bitspix;
		}

		keep += gBytesPerLine;
		ptr = keep;
	}

	return (DWORD)ptr- gGraphBase;
}


int showBmpBits(int x, int y, BITMAPINFOHEADER* info, unsigned char * data) {

	char szout[1024];
	if (info->biBitCount < 8)
	{
		__printf(szout, ( char*)"bmp file bit error\n");
		return FALSE;
	}

	int bytesperline = info->biWidth * (info->biBitCount >> 3);
	int mod = (info->biWidth * (info->biBitCount >> 3)) % 4;
	if (mod)
	{
		bytesperline = bytesperline + 4 - mod;
	}

	unsigned char * colordata = 0;
	unsigned char * color = 0;
	unsigned int deltaline = 0;

	int width = info->biWidth;
	int height = info->biHeight;
	if (width < 0 && height < 0)
	{
		colordata = data;
		color = colordata;
		deltaline = bytesperline;
	}
	else {
		colordata = data + bytesperline*height - bytesperline;
		color = colordata;
		deltaline = -bytesperline;
	}

	RGBQUAD * rgb = (RGBQUAD*)(data);;
	if (info->biBitCount == 8) {
		data += sizeof(RGBQUAD) * 256;
	}
	unsigned char * screenpos = (unsigned char*)gGraphBase + __getpos(x, y);
	unsigned char * keepy = screenpos;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int ic = 0;
			unsigned char * cc = (unsigned char*)&ic;
			if (info->biBitCount == 8)
			{
				cc[0] = rgb[*color].rgbBlue;
				cc[1] = rgb[*color].rgbGreen;
				cc[2] = rgb[*color].rgbRed;
				color++;
			}
			else if (info->biBitCount == 16)
			{
				//xxxx x xxx xxx x xxxx
				int tmpc = *(unsigned short*)color;
				cc[2] = (tmpc & 0xf800) >> 8;
				cc[1] = (tmpc & 0x7e0) >> 3;
				cc[0] = tmpc & 0x1f << 3;
				color += 2;
			}
			else if (info->biBitCount == 15)
			{
				//xxxx xx xx xxx x xxxx
				int tmpc = *(unsigned short*)color;
				cc[2] = (tmpc & 0x7c00) >> 8;
				cc[1] = (tmpc & 0x3e0) >> 3;
				cc[0] = tmpc & 0x1f << 3;
				color += 2;
			}
			else if (info->biBitCount == 24)
			{
				cc[0] = color[0];
				cc[1] = color[1];
				cc[2] = color[2];
				color += 3;
			}
			else if (info->biBitCount == 32)
			{
				cc[0] = color[0];
				cc[1] = color[1];
				cc[2] = color[2];
				cc[3] = color[3];
				color += 4;
			}
			else {
				return FALSE;
			}

			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*screenpos = *cc;
				cc++;
				screenpos++;
			}
		}

		keepy += gBytesPerLine;
		screenpos = keepy;

		colordata = colordata + deltaline;
		color = colordata;
	}

	return info->biSizeImage;
}



int initCCFont() {
	int result = 0;
	if (gCCFontBase == 0)
	{
		result =  readFile("c:\\liunux\\HZK16", (char**)&gCCFontBase);
	}

// 	char szout[1024];
// 	int len = __printf(szout, "initCCFont address:%x,size:%x\r\n",gCCFontBase,result);
// 	__drawGraphChars((unsigned char*)szout, 0);
	return result;
}



unsigned char * g_cc_poem = (unsigned char *)
	"          沁园春·雪\r\n"
	"【作者】毛泽东【朝代】近现代\r\n"
	"北国风光，千里冰封，万里雪飘。\r\n"

	"望长城内外，惟余莽莽；大河上下，顿失滔滔。\r\n"

	"山舞银蛇，原驰蜡象，欲与天公试比高。\r\n"

	"须晴日，看红装素裹，分外妖娆。\r\n"

	"江山如此多娇，引无数英雄竞折腰。\r\n"

	"惜秦皇汉武，略输文采；\r\n"

	"唐宗宋祖，稍逊风骚。\r\n"

	"一代天骄，成吉思汗，只识弯弓射大雕。\r\n"

	"俱往矣，数风流人物，还看今朝。\r\n";

// "插秧偈\r\n"
// "布袋和尚〔唐代〕\r\n"
// "手把青秧插满田，\r\n"
// "低头便见水中天。\r\n"
// "心地清净方为道，\r\n"
// "退步原来是向前。\r\n\0";


DWORD g_cc_color = 0;
DWORD g_cc_idx = 0;
DWORD g_cc_timer = 0;


//类似*lptest++;中，++的优先级高于*，会导致错误发生
void drawCCFontChar(DWORD param1, DWORD param2, DWORD param3, DWORD param4) {

	//__drawGraphChars((unsigned char*)"drawCCFontChar\r\n", 0);

	DWORD * lpcolor = (DWORD*)param2;
	DWORD *idx = (DWORD*)param3;
	DWORD * timer = (DWORD*)param4;

	unsigned short * lpcc = * (unsigned short **)param1;

	if (lpcc[*idx])
	{
		unsigned short unicode[2];
		unicode[0] = lpcc[*idx];
		unicode[1] = 0;
		__drawCCS((unsigned char*)unicode, *lpcolor);
		(*idx)++;
	}
	else {
		__kRemoveExactTimer(*timer);
	}
}


int displayCCPoem() {

	int result = 0;
	//global variable do not need to initialize
	//initDPC();

	result = initCCFont();
	if (result)
	{
		__drawCCS((unsigned char*)"欢迎来到汉字的世界！\r\n", 0xff0000);
		g_cc_timer = __kAddExactTimer((DWORD)drawCCFontChar, 100, (DWORD)&g_cc_poem, (DWORD)&g_cc_color, (DWORD)&g_cc_idx, (DWORD)&g_cc_timer);
	}

	return g_cc_timer;
}




void SetPalette(char* palette) {

	for (int i = 0; i < 256; i++)
	{
		outportb(0x3c8, i);
		outportb(0x3c9, palette[i*4]);
		outportb(0x3c9, palette[i*4 + 1]);
		outportb(0x3c9, palette[i*4 + 2]);
	}
}