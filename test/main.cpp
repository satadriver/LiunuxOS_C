// ReadCpuInfo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
/*
请使用Visual Studio编写一个控制台程序，功能如下:

一、使用命令CPUInfo.exe -C读取本机CPU信息，并存储到INI格式文本中：
示例如下：
[CPU]
Manufacturer = AuthenticAMD

BrandID = AMD Athlon Gold 3150U with Radeon Graphics
CPUID = 0F81
Cores = 2
HyperThread = True
MainClock = 2396MHZ
L1CacheSize = 192KB
L2CacheSize = 1024KB
L3CacheSize = 4096KB

二、使用命令CPUInfo.exe -r 读取INI文本中的CPU信息，并打印在控制台上；

wmic cpu get processorid   ----------powershell查看CPUID
dxdiag ----------------DirectX诊断工具查看电脑配置
systeminfo查看电脑配置


*/
#include <iostream>
#include<string>
#include<map>
#include<Windows.h>
#include<intrin.h>
#include<thread>
#include <vector>
#include<sstream>
#include<fstream>
#include<algorithm>
#include<iomanip>
#include <atlstr.h>

using namespace std;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

typedef BOOL(WINAPI* LPFN_GLPI)(LOGICAL_PROCESSOR_RELATIONSHIP,
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);

//缓存信息
struct CacheInfo
{
    int level;    // 第几级缓存
    int size;    // 缓存大小，单位KB

    CacheInfo()    // 构造函数
    {
        level = 0;
        size = 0;

    }

    CacheInfo(int clevel, int csize)  // 构造函数
    {
        level = clevel;
        size = csize;

    }

};

//序列号/ID号
struct Serialnumber {
    WORD nibble[6];
    Serialnumber() {
        memset(nibble, 0, sizeof(nibble));  //复制字符串
    }
};

//cpu信息
struct CpuInfo
{
    string vID;
    string bID;
    string SeriNum;
    int cores;
    string HThread;
    double Mainclock;
};

class CPUID
{
public:
    static CPUID* Instance();
    static CPUID* m_pInstance;
    map<int, CacheInfo> m_cache; // Cache information table

    virtual ~CPUID();
    string GetVID();                //获取CPU制造商信息
    string GetBrand();              //获取CPU商标信息
    string Getserialnumber();      //获取ID序列号
    string IsHyperThreading();           //是否支持HyperThread

private:
    void Executecpuid(DWORD eax); // 用来实现cpuid
    DWORD m_eax;   // 存储返回的eax
    DWORD m_ebx;   // 存储返回的ebx
    DWORD m_ecx;   // 存储返回的ecx
    DWORD m_edx;   // 存储返回的edx

};

CPUID* CPUID::m_pInstance = NULL;

CPUID* CPUID::Instance()
{
    if (NULL == m_pInstance)		// if instance is not present, create a new instance
    {
        m_pInstance = new CPUID();
    }

    return m_pInstance;
}

CPUID::~CPUID()
{
    delete m_pInstance;		// delete instance
    m_pInstance = NULL;		// set the pointer to NULL
}

void CPUID::Executecpuid(DWORD veax)
{
    // 因为嵌入式的汇编代码不能识别 类成员变量
    // 所以定义四个临时变量作为过渡
    DWORD deax;
    DWORD debx;
    DWORD decx;
    DWORD dedx;

    __asm
    {
        mov eax, veax;// 将输入参数移入eax
        cpuid;         // 执行cpuid
        mov deax, eax; //以下四行代码把寄存器中的变量存入临时变量
        mov debx, ebx;
        mov decx, ecx;
        mov dedx, edx;
    }

    m_eax = deax; // 把临时变量中的内容放入类成员变量
    m_ebx = debx;
    m_ecx = decx;
    m_edx = dedx;

}

//获取cpu核心数
int GetCores()
{
    //注释：因为这里HyperThread的缘故，所以4核读成8核心
    //方式一
    //SYSTEM_INFO sysInfo;
    //GetSystemInfo(&sysInfo);
    //cout << "Cores = " << sysInfo.dwNumberOfProcessors /2 << endl;

    //方式二
    unsigned int nCpu = max(std::thread::hardware_concurrency(), (unsigned int)1);
    //  cout << "Cores = " << nCpu / 2 << endl;
    return (nCpu);
}

//获取cpu的主频信息
double GetMainClock()
{

    static int time[2];              //定义一个整型数组time
    int  a = 0;                   //定义整形变量a＝0(在后面的运算中用来存商)
    int  b = 0;                   //定义整形变量b＝0(在后面的运算中用来存余数)
    double sum = 0;
    __asm {
        rdtsc;                    //RDTSC指令，意思是读取时间标记计数器(Read Time-Stamp Counter)
        mov ecx, offset time;          //将time的偏移地址存入ecx
        mov[ecx + 0], edx;            //把TSC的值的高32位存入[ecx+0]中
        mov[ecx + 4], eax;            //把TSC的值的低32位存入[ecx+4]中
    }
    Sleep(1000);                  //延时1秒
    __asm {
        rdtsc;
        mov ebx, offset time;            //将time的偏移地址存入ebx
        sub eax, [ebx + 4];             //把延时1秒后的TSC值的高32位减去1秒前的TSC值的高32位
        sbb edx, [ebx + 0];             //把延时1秒后的TSC值的低32位减去1秒前的TSC值的低32位
        mov ecx, 1000000000;
        div ecx;               //将2次TSC差值除以1,000,000,000
        mov a, eax;
        mov b, edx;
    }
    b = b / 100000000;
    b = b * 10;
    // cout << "MainClock = " << a << "." << b << "GHz" << endl;
    sum = (a * 100 + b);
    sum = sum / 100;
    //cout << sum << endl;
    return sum;
}

//初始化除cpu三级缓存之外的其它信息
CpuInfo* init()
{
    CpuInfo* CInfo = new CpuInfo[2];
    CPUID* info = CPUID::Instance();

    CInfo[0].vID = info->GetVID();
    CInfo[0].bID = info->GetBrand();
    CInfo[0].SeriNum = info->Getserialnumber();
    CInfo[0].cores = GetCores();
    CInfo[0].HThread = info->IsHyperThreading();
    CInfo[0].Mainclock = GetMainClock();

    return CInfo;
}

//获得CPU的制造商信息(Vender ID String)
//把eax = 0作为输入参数，可以得到CPU的制造商信息。
string CPUID::GetVID()
{
    char cVID[13];   // 字符串，用来存储制造商信息
    memset(cVID, 0, 13);  // 把数组清0
    Executecpuid(0);  // 执行cpuid指令，使用输入参数 eax = 0
    memcpy(cVID, &m_ebx, 4); // 复制前四个字符到数组
    memcpy(cVID + 4, &m_edx, 4); // 复制中间四个字符到数组
    memcpy(cVID + 8, &m_ecx, 4); // 复制最后四个字符到数组
    return string(cVID);  // 以string的形式返回
}

//获得CPU商标信息（Brand String）
string CPUID::GetBrand()
{
    const DWORD BRANDID = 0x80000002;  // 从0x80000002开始，到0x80000004结束
    char cBrand[49];    // 用来存储商标字符串，48个字符
    memset(cBrand, 0, 49);    // 初始化为0

    for (DWORD i = 0; i < 3; i++)   // 依次执行3个指令
    {
        Executecpuid(BRANDID + i);
        memcpy(cBrand + i * 16, &m_eax, 16); // 每次执行结束后，保存四个寄存器里的asc码到数组
    }      // 由于在内存中，m_eax, m_ebx, m_ecx, m_edx是连续排列
          // 所以可以直接以内存copy的方式进行保存
    return string(cBrand);  // 以string的形式返回
}

//获取ID序列号
string CPUID::Getserialnumber() {

    string serialnumber;
    char num[32];
    memset(num, 0, 32);
    Executecpuid(1);
    sprintf_s(num, 32, "%08X", m_edx);
    serialnumber = serialnumber + num;
    //memcpy(&serial.nibble[4], &m_eax, 4);
    sprintf_s(num, 32, "%08X", m_eax);
    serialnumber = serialnumber + num;
    //memcpy(&serial.nibble[0], &m_ecx, 8);

    return serialnumber;
}

// 判断是否支持hyper-threading
string CPUID::IsHyperThreading()
{
    bool HyperThread;
    string str;
    Executecpuid(1);  // 执行cpuid指令，使用输入参数 eax = 1
    HyperThread = (m_edx & (1 << 28));
    if (!HyperThread)
    {
        //   cout << "HyperThread = False" << endl;
        str = "False";
        return str;
    }
    else
    {
        //  cout << "HyperThread = Ture" << endl;
        str = "Ture";
        return str;
    }
    // return m_edx & (1 << 28);  // 返回edx的bit 28
}

//获取cpu三级缓存信息
CacheInfo* GetCacheInfo()
{
    CacheInfo* cpuCache = new CacheInfo[3];
    int L1 = 0;
    int L2 = 0;
    int L3 = 0;

    LPFN_GLPI glpi = (LPFN_GLPI)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformationEx");

    if (!glpi)
        return NULL;

    DWORD bytes = 0;
    glpi(RelationAll, 0, &bytes);
    vector<char> buffer(bytes);
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* info;

    if (!glpi(RelationAll, (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)&buffer[0], &bytes))
        return NULL;

    for (size_t i = 0; i < bytes; i += info->Size)
    {
        info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)&buffer[i];

        if (info->Relationship == RelationCache &&
            (info->Cache.Type == CacheData ||
                info->Cache.Type == CacheUnified))
        {
            if ((int)info->Cache.Level == 1)
            {

                L1 += (int)info->Cache.CacheSize;

            }
            if ((int)info->Cache.Level == 2)
            {
                L2 += (int)info->Cache.CacheSize;
            }
            if ((int)info->Cache.Level == 3)
            {
                L3 += (int)info->Cache.CacheSize;
            }
        }
    }
    //cout << "L1CacheSize = " << L1 * 2 / 1024 << "KB" << endl;
    //cout << "L2CacheSize = " << L2 / 1024 << "KB" << endl;
    //cout << "L3CacheSize = " << L3 / 1024 << "KB" << endl;

    cpuCache[0].level = 1;
    cpuCache[0].size = L1 * 2 / 1024;
    cpuCache[1].level = 2;
    cpuCache[1].size = L2 / 1024;
    cpuCache[2].level = 3;
    cpuCache[2].size = L3 / 1024;

    return cpuCache;
}

//将获取的cpu信息写进INI文档
void WriteInfo(CpuInfo* CInfo, CacheInfo* cpuCache)
{
    int i = 0;
    //自动获取文本地址
    CString path;
    GetModuleFileName(NULL, path.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
    path.ReleaseBuffer();
    int pos = path.ReverseFind('\\');
    path = path.Left(pos);
    path = path + L"\\cpuINfoTest.ini";
    ofstream ofs;
    ofs.open(path, ios::out | ios::app);
    if (!ofs)
    {
        cout << "Open file error" << endl;
        return;
    }
    ofs << "[CPU]" << endl;
    ofs << "Manufacturer = " << CInfo[i].vID << endl;
    ofs << "BrandID = " << CInfo[i].bID << endl;
    ofs << "CPUID = " << CInfo[i].SeriNum << endl;
    ofs << "Cores = " << CInfo[i].cores << endl;
    ofs << "HyperThread =" << CInfo[i].HThread << endl;
    ofs.setf(ios::fixed);
    ofs << "MainClock = " << fixed << setprecision(2) << CInfo[i].Mainclock << "GHz" << endl;  //保留两位小数点
    ofs << "L" << cpuCache[0].level << "CacheSize = " << cpuCache[0].size << "KB" << endl;
    ofs << "L" << cpuCache[1].level << "CacheSize = " << cpuCache[1].size << "KB" << endl;
    ofs << "L" << cpuCache[2].level << "CacheSize = " << cpuCache[2].size << "KB" << endl;

    ofs << "\n";
    cout << "\n ini file write success!" << endl;
    ofs.close();
    return;
}

//从INI格式文本中读取内容
void ReadInfo(CpuInfo* CInfo, CacheInfo* cpuCache)
{
    int n = 0;
    ifstream ifs;

    CString path;
    GetModuleFileName(NULL, path.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
    path.ReleaseBuffer();
    int pos = path.ReverseFind('\\');
    path = path.Left(pos);
    path = path + L"\\cpuINfoTest.ini";

    ifs.open(path, ios::in);
    if (!ifs)
    {
        cout << "Open file error" << endl;
        return;
    }
    string str;
    while (!ifs.eof())           //读到文件结束
    {
        getline(ifs, str);
        cout << str << endl;
    }
    cout << "\n" << endl;
    cout << "read file success" << endl;
    ifs.close();
    return;
}

//打印需要获取的CPU信息
void display(CpuInfo* CInfo, CacheInfo* cpuCache)
{
    int i = 0;
    cout << "[CPU]" << endl;
    cout << "Manufacturer = " << CInfo[i].vID << endl;
    cout << "BrandID = " << CInfo[i].bID << endl;
    cout << "CPUID = " << CInfo[i].SeriNum << endl;
    cout << "Cores = " << CInfo[i].cores << endl;
    cout << "HyperThread =" << CInfo[i].HThread << endl;
    cout.setf(ios::fixed);
    cout << "MainClock = " << fixed << setprecision(2) << CInfo[i].Mainclock << "GHz" << endl;  //保留两位小数点
    cout << "L" << cpuCache[0].level << "CacheSize = " << cpuCache[0].size << "KB" << endl;
    cout << "L" << cpuCache[1].level << "CacheSize = " << cpuCache[1].size << "KB" << endl;
    cout << "L" << cpuCache[2].level << "CacheSize = " << cpuCache[2].size << "KB" << endl;

    return;
}

//清空INI文本内容
void fileEmpty()
{
    CString path;
    GetModuleFileName(NULL, path.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
    path.ReleaseBuffer();
    int pos = path.ReverseFind('\\');
    path = path.Left(pos);
    path = path + L"\\cpuINfoTest.ini";

    fstream file;
    fstream open(path, ios::out | ios::binary);
    cout << "INI文本内容已成功清除内容" << endl;
    return;
}

//用户提示功能
void showhelp()
{
    cout << "--------------------------------------------" << endl;
    cout << "欢迎使用本系统" << endl;
    cout << "需要按照以下提示输入才可以实现对应功能\n" << endl;
    cout << "C: 读取本机CPU信息，并存储到INI格式文本" << endl;
    cout << "R：读取INI文本中的CPU信息，并显示" << endl;
    cout << "L: 清空存储CPU信息的INI文本内容" << endl;
    cout << "exit: 退出系统" << endl;
    cout << "--------------------------------------------" << endl;
    cout << "\n";
}

int main(int argc, char* argv[])
{

    CpuInfo* cpuInfo = init();
    CacheInfo* cache = GetCacheInfo();
    showhelp();
    if (argc == 2)
    {

        if (strcmp(argv[1], "C") == 0 || strcmp(argv[1], "c") == 0)
        {
            display(cpuInfo, cache);
            WriteInfo(cpuInfo, cache);
        }
        else if (strcmp(argv[1], "R") == 0 || strcmp(argv[1], "r") == 0)
        {
            ReadInfo(cpuInfo, cache);
        }
        else if (strcmp(argv[1], "L") == 0 || strcmp(argv[1], "l") == 0)
        {
            fileEmpty();
        }
        else if (strcmp(argv[1], "exit") == 0)
        {
            //return -1;
            exit(0);
        }
        else
        {
            cout << "输入错误，请根据showhelp提示输入 " << endl;
            return 0;
            //   showhelp();
            //   system("pause");
        }
    }
    else
    {
        cout << "输入错误，请根据showhelp提示输入 " << endl;
        // showhelp();
        system("pause");
    }
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

