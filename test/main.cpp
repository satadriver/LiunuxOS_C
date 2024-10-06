// ReadCpuInfo.cpp : ���ļ����� "main" ����������ִ�н��ڴ˴���ʼ��������
//
/*
��ʹ��Visual Studio��дһ������̨���򣬹�������:

һ��ʹ������CPUInfo.exe -C��ȡ����CPU��Ϣ�����洢��INI��ʽ�ı��У�
ʾ�����£�
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

����ʹ������CPUInfo.exe -r ��ȡINI�ı��е�CPU��Ϣ������ӡ�ڿ���̨�ϣ�

wmic cpu get processorid   ----------powershell�鿴CPUID
dxdiag ----------------DirectX��Ϲ��߲鿴��������
systeminfo�鿴��������


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

//������Ϣ
struct CacheInfo
{
    int level;    // �ڼ�������
    int size;    // �����С����λKB

    CacheInfo()    // ���캯��
    {
        level = 0;
        size = 0;

    }

    CacheInfo(int clevel, int csize)  // ���캯��
    {
        level = clevel;
        size = csize;

    }

};

//���к�/ID��
struct Serialnumber {
    WORD nibble[6];
    Serialnumber() {
        memset(nibble, 0, sizeof(nibble));  //�����ַ���
    }
};

//cpu��Ϣ
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
    string GetVID();                //��ȡCPU��������Ϣ
    string GetBrand();              //��ȡCPU�̱���Ϣ
    string Getserialnumber();      //��ȡID���к�
    string IsHyperThreading();           //�Ƿ�֧��HyperThread

private:
    void Executecpuid(DWORD eax); // ����ʵ��cpuid
    DWORD m_eax;   // �洢���ص�eax
    DWORD m_ebx;   // �洢���ص�ebx
    DWORD m_ecx;   // �洢���ص�ecx
    DWORD m_edx;   // �洢���ص�edx

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
    // ��ΪǶ��ʽ�Ļ����벻��ʶ�� ���Ա����
    // ���Զ����ĸ���ʱ������Ϊ����
    DWORD deax;
    DWORD debx;
    DWORD decx;
    DWORD dedx;

    __asm
    {
        mov eax, veax;// �������������eax
        cpuid;         // ִ��cpuid
        mov deax, eax; //�������д���ѼĴ����еı���������ʱ����
        mov debx, ebx;
        mov decx, ecx;
        mov dedx, edx;
    }

    m_eax = deax; // ����ʱ�����е����ݷ������Ա����
    m_ebx = debx;
    m_ecx = decx;
    m_edx = dedx;

}

//��ȡcpu������
int GetCores()
{
    //ע�ͣ���Ϊ����HyperThread��Ե�ʣ�����4�˶���8����
    //��ʽһ
    //SYSTEM_INFO sysInfo;
    //GetSystemInfo(&sysInfo);
    //cout << "Cores = " << sysInfo.dwNumberOfProcessors /2 << endl;

    //��ʽ��
    unsigned int nCpu = max(std::thread::hardware_concurrency(), (unsigned int)1);
    //  cout << "Cores = " << nCpu / 2 << endl;
    return (nCpu);
}

//��ȡcpu����Ƶ��Ϣ
double GetMainClock()
{

    static int time[2];              //����һ����������time
    int  a = 0;                   //�������α���a��0(�ں������������������)
    int  b = 0;                   //�������α���b��0(�ں��������������������)
    double sum = 0;
    __asm {
        rdtsc;                    //RDTSCָ���˼�Ƕ�ȡʱ���Ǽ�����(Read Time-Stamp Counter)
        mov ecx, offset time;          //��time��ƫ�Ƶ�ַ����ecx
        mov[ecx + 0], edx;            //��TSC��ֵ�ĸ�32λ����[ecx+0]��
        mov[ecx + 4], eax;            //��TSC��ֵ�ĵ�32λ����[ecx+4]��
    }
    Sleep(1000);                  //��ʱ1��
    __asm {
        rdtsc;
        mov ebx, offset time;            //��time��ƫ�Ƶ�ַ����ebx
        sub eax, [ebx + 4];             //����ʱ1����TSCֵ�ĸ�32λ��ȥ1��ǰ��TSCֵ�ĸ�32λ
        sbb edx, [ebx + 0];             //����ʱ1����TSCֵ�ĵ�32λ��ȥ1��ǰ��TSCֵ�ĵ�32λ
        mov ecx, 1000000000;
        div ecx;               //��2��TSC��ֵ����1,000,000,000
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

//��ʼ����cpu��������֮���������Ϣ
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

//���CPU����������Ϣ(Vender ID String)
//��eax = 0��Ϊ������������Եõ�CPU����������Ϣ��
string CPUID::GetVID()
{
    char cVID[13];   // �ַ����������洢��������Ϣ
    memset(cVID, 0, 13);  // ��������0
    Executecpuid(0);  // ִ��cpuidָ�ʹ��������� eax = 0
    memcpy(cVID, &m_ebx, 4); // ����ǰ�ĸ��ַ�������
    memcpy(cVID + 4, &m_edx, 4); // �����м��ĸ��ַ�������
    memcpy(cVID + 8, &m_ecx, 4); // ��������ĸ��ַ�������
    return string(cVID);  // ��string����ʽ����
}

//���CPU�̱���Ϣ��Brand String��
string CPUID::GetBrand()
{
    const DWORD BRANDID = 0x80000002;  // ��0x80000002��ʼ����0x80000004����
    char cBrand[49];    // �����洢�̱��ַ�����48���ַ�
    memset(cBrand, 0, 49);    // ��ʼ��Ϊ0

    for (DWORD i = 0; i < 3; i++)   // ����ִ��3��ָ��
    {
        Executecpuid(BRANDID + i);
        memcpy(cBrand + i * 16, &m_eax, 16); // ÿ��ִ�н����󣬱����ĸ��Ĵ������asc�뵽����
    }      // �������ڴ��У�m_eax, m_ebx, m_ecx, m_edx����������
          // ���Կ���ֱ�����ڴ�copy�ķ�ʽ���б���
    return string(cBrand);  // ��string����ʽ����
}

//��ȡID���к�
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

// �ж��Ƿ�֧��hyper-threading
string CPUID::IsHyperThreading()
{
    bool HyperThread;
    string str;
    Executecpuid(1);  // ִ��cpuidָ�ʹ��������� eax = 1
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
    // return m_edx & (1 << 28);  // ����edx��bit 28
}

//��ȡcpu����������Ϣ
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

//����ȡ��cpu��Ϣд��INI�ĵ�
void WriteInfo(CpuInfo* CInfo, CacheInfo* cpuCache)
{
    int i = 0;
    //�Զ���ȡ�ı���ַ
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
    ofs << "MainClock = " << fixed << setprecision(2) << CInfo[i].Mainclock << "GHz" << endl;  //������λС����
    ofs << "L" << cpuCache[0].level << "CacheSize = " << cpuCache[0].size << "KB" << endl;
    ofs << "L" << cpuCache[1].level << "CacheSize = " << cpuCache[1].size << "KB" << endl;
    ofs << "L" << cpuCache[2].level << "CacheSize = " << cpuCache[2].size << "KB" << endl;

    ofs << "\n";
    cout << "\n ini file write success!" << endl;
    ofs.close();
    return;
}

//��INI��ʽ�ı��ж�ȡ����
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
    while (!ifs.eof())           //�����ļ�����
    {
        getline(ifs, str);
        cout << str << endl;
    }
    cout << "\n" << endl;
    cout << "read file success" << endl;
    ifs.close();
    return;
}

//��ӡ��Ҫ��ȡ��CPU��Ϣ
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
    cout << "MainClock = " << fixed << setprecision(2) << CInfo[i].Mainclock << "GHz" << endl;  //������λС����
    cout << "L" << cpuCache[0].level << "CacheSize = " << cpuCache[0].size << "KB" << endl;
    cout << "L" << cpuCache[1].level << "CacheSize = " << cpuCache[1].size << "KB" << endl;
    cout << "L" << cpuCache[2].level << "CacheSize = " << cpuCache[2].size << "KB" << endl;

    return;
}

//���INI�ı�����
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
    cout << "INI�ı������ѳɹ��������" << endl;
    return;
}

//�û���ʾ����
void showhelp()
{
    cout << "--------------------------------------------" << endl;
    cout << "��ӭʹ�ñ�ϵͳ" << endl;
    cout << "��Ҫ����������ʾ����ſ���ʵ�ֶ�Ӧ����\n" << endl;
    cout << "C: ��ȡ����CPU��Ϣ�����洢��INI��ʽ�ı�" << endl;
    cout << "R����ȡINI�ı��е�CPU��Ϣ������ʾ" << endl;
    cout << "L: ��մ洢CPU��Ϣ��INI�ı�����" << endl;
    cout << "exit: �˳�ϵͳ" << endl;
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
            cout << "������������showhelp��ʾ���� " << endl;
            return 0;
            //   showhelp();
            //   system("pause");
        }
    }
    else
    {
        cout << "������������showhelp��ʾ���� " << endl;
        // showhelp();
        system("pause");
    }
}

// ���г���: Ctrl + F5 ����� >����ʼִ��(������)���˵�
// ���Գ���: F5 ����� >����ʼ���ԡ��˵�

