
#include "gdi.h"
#include "Utils.h"
#include "math.h"
#include "file.h"
#include "malloc.h"
#include "cmosPeriodTimer.h"
#include "video.h"
#include "cmosExactTimer.h"
#include "hardware.h"
#include "window.h"

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
	unsigned char* videoBase = (unsigned char*)GetVideoBase();
	unsigned char * showpos = pos + (unsigned char *)videoBase;
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
				float cosA = (-x + y + z) / __sqrt(x*x + y*y + z*z) / __sqrt(3);//(光线向量：[1,-1,-1])
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
	char* vb = GetVideoBase();
	unsigned char * showpos = pos + (unsigned char *)vb;
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
				float cosA = (-x + y + z) / __sqrt(x*x + y*y + z*z) / __sqrt(3);//(光线向量：[-1,1,1])

																			  //漫反射，wrapped diffuse处理
				float diffuse = cosA*0.5f + 0.5f;
				//float d = cosA+0.2f>1.0f?1.0f:cosA+0.2f;//加入全局光处理

				//高光分
				float specular = __pow(cosA, 64.0);

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
	char* vb = GetVideoBase();
	unsigned char * ptr = (unsigned char*)__getpos(x, y) +(unsigned long) vb;

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

	return (DWORD)ptr- (DWORD)vb;
}


int showBmpBits(int x, int y, BITMAPINFOHEADER* info, unsigned char * data) {

	char szout[256];
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

	char* vb = GetVideoBase();
	unsigned char * screenpos = (unsigned char*)vb + __getpos(x, y);
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






unsigned char* poem_jjj = (unsigned char*)
"《将进酒》\r\n"
"唐·李白\r\n"
"君不见，黄河之水天上来，奔流到海不复回。\r\n"
"君不见，高堂明镜悲白发，朝如青丝暮成雪！\r\n"
"人生得意须尽欢，莫使金樽空对月。\r\n"
"天生我材必有用，千金散尽还复来。\r\n"
"烹羊宰牛且为乐，会须一饮三百杯。\r\n"
"岑夫子，丹丘生，将进酒，杯莫停。\r\n"
"与君歌一曲，请君为我倾耳听。\r\n"
"钟鼓馔玉不足贵，但愿长醉不复醒。\r\n"
"古来圣贤皆寂寞，惟有饮者留其名。\r\n"
"陈王昔时宴平乐，斗酒十千恣欢谑。\r\n"
"主人何为言少钱，径须沽取对君酌。\r\n"
"五花马、千金裘，呼儿将出换美酒，与尔同销万古愁！\r\n\0";


unsigned char* poem_xln = (unsigned char*)
"《行路难》\r\n"
"唐·李白\r\n"
"金樽清酒斗十千，玉盘珍羞直万钱。\r\n"
"停杯投箸不能食，拔剑四顾心茫然。\r\n"
"欲渡黄河冰塞川，将登太行雪满山。\r\n"
"闲来垂钓碧溪上，忽复乘舟梦日边。\r\n"
"行路难！行路难！多歧路，今安在？\r\n"
"长风破浪会有时，直挂云帆济沧海。\r\n\0";


unsigned char* poem_nnjcbhg = (unsigned char*)
"《念奴娇·赤壁怀古》 \r\n"
"北宋·苏轼\r\n"
"大江东去，浪淘尽，千古风流人物。\r\n"
"故垒西边，人道是，三国周郎赤壁。\r\n"
"乱石穿空，惊涛拍岸，卷起千堆雪。\r\n"
"江山如画，一时多少豪杰。\r\n"
"遥想公瑾当年，小乔初嫁了，雄姿英发。\r\n"
"羽扇纶巾，谈笑间，樯橹灰飞烟灭。\r\n"
"故国神游，多情应笑我，早生华发。\r\n"
"人生如梦，一尊还酹江月。\r\n\0";

unsigned char* poem_gjtyqy = (unsigned char*)
"《归园田居·其一》\r\n"
"东晋·陶渊明\r\n"
"少无适俗韵，性本爱丘山。\r\n"
"误落尘网中，一去三十年。\r\n"
"羁鸟恋旧林，池鱼思故渊。\r\n"
"开荒南野际，守拙归园田。\r\n"
"方宅十余亩，草屋八九间。\r\n"
"榆柳荫后檐，桃李罗堂前。\r\n"
"暧暧远人村，依依墟里烟。\r\n"
"狗吠深巷中，鸡鸣桑树颠。\r\n"
"户庭无尘杂，虚室有余闲。\r\n"
"久在樊笼里，复得返自然。\r\n\0";


unsigned char* poem_gldy = (unsigned char*)
"《过零丁洋》\r\n"
"南宋·文天祥\r\n"
"辛苦遭逢起一经，干戈寥落四周星。\r\n"
"山河破碎风飘絮，身世浮沉雨打萍。\r\n"
"惶恐滩头说惶恐，零丁洋里叹零丁。\r\n"
"人生自古谁无死？留取丹心照汗青。\r\n\0";


unsigned char* poem_mjhxh = (unsigned char*)
"《满江红·写怀》\r\n"
" 南宋·岳飞\r\n"
"怒发冲冠，凭栏处、潇潇雨歇。\r\n"
"抬望眼、仰天长啸，壮怀激烈。三十功名尘与土，八千里路云和月。\r\n"
"莫等闲、白了少年头，空悲切。\r\n"
"靖康耻，犹未雪。臣子恨，何时灭。\r\n"
"驾长车，踏破贺兰山缺。壮志饥餐胡虏肉，笑谈渴饮匈奴血。\r\n"
"待从头、收拾旧山河，朝天阙。\r\n\0";

unsigned char* poem_dgx = (unsigned char*)
"《短歌行》\r\n"
"曹操·〔两汉〕\r\n"
"对酒当歌，人生几何！\r\n"
"譬如朝露，去日苦多。\r\n"
"慨当以慷，忧思难忘。\r\n"
"何以解忧？唯有杜康。\r\n"
"青青子衿，悠悠我心。\r\n"
"但为君故，沉吟至今。\r\n"
"呦呦鹿鸣，食野之苹。\r\n"
"我有嘉宾，鼓瑟吹笙。\r\n"
"明明如月，何时可掇？\r\n"
"忧从中来，不可断绝。\r\n"
"越陌度阡，枉用相存。\r\n"
"契阔谈讌，心念旧恩。\r\n"
"月明星稀，乌鹊南飞。\r\n"
"绕树三匝，何枝可依？\r\n"
"山不厌高，海不厌深。\r\n"
"周公吐哺，天下归心。\r\n\0";


unsigned char* poem_gch = (unsigned char*)
"《观沧海》\r\n"
"曹操·〔两汉〕\r\n"
"东临碣石，以观沧海。\r\n"
"水何澹澹，山岛竦峙。\r\n"
"树木丛生，百草丰茂。\r\n"
"秋风萧瑟，洪波涌起。\r\n"
"日月之行，若出其中；\r\n"
"星汉灿烂，若出其里。\r\n"
"幸甚至哉，歌以咏志。\r\n\0";

unsigned char* poem_qyccs = (unsigned char*)
"《沁园春·长沙》\r\n"
"毛泽东\r\n"
"独立寒秋，湘江北去，橘子洲头。\r\n"
"看万山红遍，层林尽染；漫江碧透，百舸争流。\r\n"
"鹰击长空，鱼翔浅底，万类霜天竞自由。\r\n"
"怅寥廓，问苍茫大地，谁主沉浮？\r\n"
"携来百侣曾游。忆往昔峥嵘岁月稠。\r\n"
"恰同学少年，风华正茂；书生意气，挥斥方遒。\r\n"
"指点江山，激扬文字，粪土当年万户侯。\r\n"
"曾记否，到中流击水，浪遏飞舟？\r\n\0";

unsigned char* poem_tjsqs = (unsigned char*)
"《天净沙·秋思》\r\n"
"元 ·马致远\r\n"
"枯藤老树昏鸦，小桥流水人家，古道西风瘦马。夕阳西下，断肠人在天涯。\r\n\0";

unsigned char* poem_ljx = (unsigned char*)
"《临江仙》\r\n"
"明·杨慎\r\n"
"滚滚长江东逝水，浪花淘尽英雄。是非成败转头空。青山依旧在，几度夕阳红。\r\n"
"白发渔樵江渚上，惯看秋月春风。一壶浊酒喜相逢。古今多少事，都付笑谈中。\r\n\0";

unsigned char* poem_ltsl = (unsigned char*)
"《浪淘沙令》\r\n"
"五代  李煜\r\n"
"帘外雨潺潺，春意阑珊。罗衾不耐五更寒。梦里不知身是客，一晌贪欢。\r\n"
"独自莫凭栏，无限江山，别时容易见时难。流水落花春去也，天上人间。\0";


unsigned char* poem_cjhyy = (unsigned char*)
"《春江花月夜》\r\n"
"唐 · 张若虚\r\n"
"春江潮水连海平，海上明月共潮生。\r\n"
"滟滟随波千万里，何处春江无月明！\r\n"
"江流宛转绕芳甸，月照花林皆似霰。\r\n"
"空里流霜不觉飞，汀上白沙看不见。\r\n"
"江天一色无纤尘，皎皎空中孤月轮。\r\n"
"江畔何人初见月？江月何年初照人？\r\n"
"人生代代无穷已，江月年年只相似。\r\n"
"不知江月待何人，但见长江送流水。\r\n"
"白云一片去悠悠，青枫浦上不胜愁。\r\n"
"谁家今夜扁舟子？何处相思明月楼？\r\n"
"可怜楼上月徘徊，应照离人妆镜台。\r\n"
"玉户帘中卷不去，捣衣砧上拂还来。\r\n"
"此时相望不相闻，愿逐月华流照君。\r\n"
"鸿雁长飞光不度，鱼龙潜跃水成文。\r\n"
"昨夜闲潭梦落花，可怜春半不还家。\r\n"
"江水流春去欲尽，江潭落月复西斜。\r\n"
"斜月沉沉藏海雾，碣石潇湘无限路。\r\n"
"不知乘月几人归，落月摇情满江树。\0";

unsigned char* poem_qycx = (unsigned char*)
"《沁园春·雪》\r\n"
"毛泽东· 近现代\r\n"
"北国风光，千里冰封，万里雪飘。\r\n"
"望长城内外，惟余莽莽；大河上下，顿失滔滔。\r\n"
"山舞银蛇，原驰蜡象，欲与天公试比高。\r\n"
"须晴日，看红装素裹，分外妖娆。\r\n"
"江山如此多娇，引无数英雄竞折腰。\r\n"
"惜秦皇汉武，略输文采；唐宗宋祖，稍逊风骚。\r\n"
"一代天骄，成吉思汗，只识弯弓射大雕。\r\n"
"俱往矣，数风流人物，还看今朝。\r\n\0";

unsigned char* poem_cyj = (unsigned char*)
"《插秧偈》\r\n"
"布袋和尚·〔唐代〕\r\n"
"手把青秧插满田，\r\n"
"低头便见水中天。\r\n"
"心地清净方为道，\r\n"
"退步原来是向前。\r\n\0";

unsigned char* poem_qthcx = (unsigned char*)
"《钱塘湖春行》\r\n"
"唐朝 白居易\r\n"
"孤山寺北贾亭西，水面初平云脚低。\r\n"
"几处早莺争暖树，谁家新燕啄春泥。\r\n"
"乱花渐欲迷人眼，浅草才能没马蹄。\r\n"
"最爱湖东行不足，绿杨阴里白沙堤。\r\n\0";


unsigned char* poem_dkh = (unsigned char*)
"《登科后》\r\n"
"孟郊\r\n"
"昔日龌龊不足夸，今朝放荡思无涯。\r\n"
"春风得意马蹄疾，一日看尽长安花。\r\n\0";

unsigned char* poem_yqelsg = (unsigned char*)
"《忆秦娥·娄山关》\r\n"
"现代·毛泽东\r\n"
"西风烈，长空雁叫霜晨月。\r\n"
"霜晨月，马蹄声碎，喇叭声咽。\r\n"
"雄关漫道真如铁，而今迈步从头越。\r\n"
"从头越，苍山如海，残阳如血。\r\n\0";

unsigned char* poem_ltsbdh = (unsigned char*)
"《浪淘沙·北戴河》\r\n"
"现代·毛泽东\r\n"
"大雨落幽燕，白浪滔天，秦皇岛外打鱼船。\r\n"
"一片汪洋都不见，知向谁边？\r\n"
"往事越千年，魏武挥鞭，东临碣石有遗篇。\r\n"
"萧瑟秋风今又是，换了人间。\r\n\0";




#define CHINESE_POEM_DELAY 60


unsigned char* g_cc_peoms[] = { poem_jjj,poem_xln, poem_nnjcbhg ,poem_gjtyqy ,poem_gldy ,poem_mjhxh,poem_dgx ,poem_gch ,
poem_qyccs,poem_tjsqs,poem_ljx ,poem_ltsl,poem_cjhyy,poem_qycx ,poem_cyj ,poem_dkh ,poem_qthcx ,poem_yqelsg ,poem_ltsbdh };

DWORD g_cc_color = 0;
DWORD g_cc_idx = 0;
DWORD g_cc_timer = 0;
DWORD g_poem_num = 0;
DWORD g_poem_pos_x = 0;
DWORD g_poem_pos_y = 0;

int initCCFont() {
	int result = 0;
	if (gCCFontBase == 0)
	{
		result = readFile(LIUNUX_BASE_PATH "HZK16", (char**)&gCCFontBase);
	}

	// 	char szout[256];
	// 	int len = __printf(szout, "initCCFont address:%x,size:%x\r\n",gCCFontBase,result);
	// 	__drawGraphChars((unsigned char*)szout, 0);
	return result;
}


//类似*lptest++;中，++的优先级高于*，会导致错误发生
void drawCCFontChar(DWORD param1, DWORD param2, DWORD param3, DWORD param4) {

	//__drawGraphChars((unsigned char*)"drawCCFontChar\r\n", 0);

	DWORD* lpcolor = (DWORD*)param2;
	DWORD* idx = (DWORD*)param3;
	DWORD* timer = (DWORD*)param4;

	unsigned short* lpcc = *(unsigned short**)param1;

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

	result = initCCFont();
	if (result)
	{
		g_poem_num = 0;
		g_cc_idx = 0;
		g_poem_pos_x = 0;
		g_poem_pos_y = gWindowHeight - TASKBAR_HEIGHT;

		__drawCCS((unsigned char*)"欢迎来到汉字的世界！\r\n", 0xff0000);

		g_cc_timer = __kAddExactTimer((DWORD)drawCCFontChar, CHINESE_POEM_DELAY, 
			(DWORD)&poem_ljx, (DWORD)&g_cc_color, (DWORD)&g_cc_idx, (DWORD)&g_cc_timer);
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