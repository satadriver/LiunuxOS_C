
#include "acpi.h"
#include "hardware.h"
#include "video.h"



static const ACPIFadt *acpiFadt = 0;

static const ACPIHeader *acpiSsdt = 0;

static const ACPIHeader *acpiDsdt = 0;






static int parseApic(ACPIHeaderApic *apic)
{
	char szout[1024];
	char temp[20];
	temp[0] = '0'; 
	temp[1] = 'x';
	//atoi(apic->localApicAddress, temp + 2, 0x10, 8, '0', 1);
	//__printf("\nLocal Apic Address: %s\n", temp);
	u8*localApicAddress = (u8 *)apic->localApicAddress;

	u8 *start = (u8*) apic + sizeof(ACPIHeaderApic);
	u8 *end = ((u8 *)apic) + apic->header.length;
	while (start < end)
	{
		ApicHeader *apicHeader = (ApicHeader *)start;
		u8 type = apicHeader->type;
		u8 length = apicHeader->length;
		switch (type)
		{
			case APIC_TYPE_LOCAL_APIC:
			{
				LocalApic *localApic = (LocalApic *)apicHeader;
				//__printf("Found CPU: processor ID => %d, apic ID => %d \n",(int)localApic->apicProcessorID, (int)localApic->apicID);
				//cpus[cpuCount++] = localApic->apicID;
				break;
			}
			case APIC_TYPE_IO_APIC:
			{
				IOApic *ioApic = (IOApic *)apicHeader;
				//itoa(ioApic->ioApicAddress, temp + 2, 0x10, 8, '0', 1);
				//__printf("Found I/O Apic : I/O Apic ID => %d, I/O Apic Address => %s\n",(int)ioApic->ioApicID, temp);
				u8 *ioApicAddress = (u8 *)ioApic->ioApicAddress;
				break;
			}
			case APIC_TYPE_INTERRUPT_OVERRIDE: {
				break;
			}
			default: {
				__printf(szout,"Unknow Apic information type:%d,length:%d.\n", (int)type, (int)length);
				break;
			}
		}
		start += length;
	}

	return TRUE;
}

int acpiEnable(const void *fadt, const void *ssdt)
{
	/*因为fadt和ssdt是两次分别传进来的,所以这边要多处理一下*/
	if (!acpiSsdt && ssdt)
		acpiSsdt = (const ACPIHeader *)ssdt; /*如果传进来了ssdt,但是acpiSsdt还是空的就设置它*/
	if (acpiFadt) /*如果fadt已经设置过了就不用enable了,直接返回*/
		return 0;
	if (!fadt) /*如果acpiFadt没有设置,fadt也是0,那么就是失败了,返回*/
		return -1;
	acpiFadt = (const ACPIFadt *)fadt; /*设置fadt和dsdt*/
	//acpiDsdt = (const ACPIHeader *)pa2va(acpiFadt->dsdt);
	acpiDsdt = (const ACPIHeader *)(acpiFadt->dsdt);
	if (acpiFadt->smiCommandPort)
		if (acpiFadt->acpiEnable || acpiFadt->acpiDisable) /*判断需不需要我们自己enable,有可能bios已经enable好了,那样的话这里就会跳出*/
			outportb(acpiFadt->smiCommandPort, acpiFadt->acpiEnable);
	while (!(inportw(acpiFadt->controlRegister1a) & SCI_ENABLED)) /*等待enable完毕*/
	{
		__asm {
			pause
		}
	}
	if (acpiFadt->controlRegister1b) /*如果这个存在,也等待这个完毕*/
	{
		while (!(inportw(acpiFadt->controlRegister1b) & SCI_ENABLED)) {
			__asm {
				pause
			}
		}
	}
	return TRUE;
}

static int parseDT(ACPIHeader *dt)
{
	u32 signature = dt->signature;
	char signatureString[5];
	__memcpy((char *)signatureString, (char *)&signature, 4);
	signatureString[4] = '\0';

	if (signature == *(u32 *)"APIC") {

	}
	else if (signature == *(u32 *)"HPET") {

	}
	else if (signature == *(u32 *)"FACP") {

	}
	else if (signature == *(u32 *)"SSDT") {

	}
	return TRUE;
}



static int parseRSDT(ACPIHeader *rsdt)
{
	//u32 *start = rsdt->data;
	u32 *start = (u32 *) &rsdt[1];
	u32 *end = (u32 *)((u8 *)rsdt + rsdt->length);
	while (start < end)		//（length-36）/4便是RSDT所指向的table个数
	{
		u32 dt = *(start++);
		parseDT((ACPIHeader *)dt);
	}
	return TRUE;
}


static int parseXSDT(ACPIHeader* rsdt)
{
	//u32 *start = rsdt->data;
	u32* start = (u32*)&rsdt[1];
	u32* end = (u32*)((u8*)rsdt + rsdt->length);
	while (start < end)		//（length-36）/8便是RSDT所指向的table个数
	{
		u32 dt = *start;
		start += 2;
		parseDT((ACPIHeader*)dt);
	}
	return TRUE;
}



static int parseRSDP(XSDP_HEADER* xsdp)
{
	int res = 0;

	char szout[1024];
	u8 sum = 0;
	char * ptr = (char*)xsdp;
	for (int i = 0; i < 20; ++i)
	{
		sum += ptr[i];
	}
	if (sum)
	{
		__printf(szout,( char*)"parseRSDP checksum error\r\n");
		return FALSE;
	}

	char oem[7];
	__memset(oem, 0, sizeof(oem));
	__memcpy(oem, (char *)xsdp->OEMID , sizeof(oem) / sizeof(char) - 1);

	u8 version = xsdp->Revision;

	__printf(szout,"ACPI rsdp:%x,OEM:%s,version:%u\r\n", xsdp, oem,version);
	
	if (version == 0)
	{
		u32 rsdt = xsdp->RsdtAddress;
		res = parseRSDT((ACPIHeader *)rsdt);
	}
	else if (version == 2)
	{
		u64 xsdt = xsdp->XsdtAddress;
		u32 rsdt = xsdp->RsdtAddress;
		if (xsdt) {
			parseXSDT((ACPIHeader *)xsdt);
		}
		else {
			res = parseRSDT((ACPIHeader *)rsdt);
		}
	}
	else {
		return 0;
	}

	return res;
}


int initACPI(void)
{
	u8 *start = (u8 *)0xe0000;
	u8 * const end = (u8 *)0xfffff;
	while (start < end) {
		u64 signature = *(u64 *)start;
		if (signature == *(const u64 *)"RSD PTR ")
		{
			if (parseRSDP((XSDP_HEADER * )start) == 0)
			{
				return TRUE;
			}
		}
		start += 0x10;
	}
	u8 *ebda = (u8 *)( * (u16 *)0x40E << 4);
	if (ebda != (u8 *)0x9FC00) {
		return -1;
	}

	u8 * const ebdaEnd = ebda + 0x3FF;
	while (ebda < ebdaEnd) {
		u64 signature = *(u64 *)ebda;
		if (signature == *(const u64 *)"RSD PTR ")
		{
			if (parseRSDP((XSDP_HEADER*)ebda) == 0)
			{
				return TRUE;
			}
		}

		ebda += 0x10;
	}

	return FALSE;
}



extern "C"  __declspec(dllexport) int doPowerOff(void)
{
	__asm {
		//cli
	}
	const ACPIHeader *dt = acpiDsdt; /*先找dsdt*/
retry:;
	u8 *data = (u8 *)dt + sizeof(ACPIHeader); /*获取data和长度*/
	u32 length = dt->length - sizeof(*dt);
	for (; length-- > 0; ++data)
	{
		if (data[0] != '_' || data[1] != 'S' || data[2] != '5' || data[3] != '_')
			continue;   /*检查_S5_*/
		if (data[-1] != 0x8)
			if (data[-1] != '\\' || data[-2] != 0x8)
				continue;   /*检查是否符合格式*/
		if (data[4] != 0x12)
			continue;
		length = 1; /*找到了!*/
		break;
	}
	if (length != 1) /*如果dsdt中没有*/
	{
		if (!acpiSsdt)  /*如果没有ssdt,失败*/
			for (;;);
		if (dt == acpiSsdt) /*如果已经找完了ssdt和dsdt,失败*/
			for (;;);
		dt = acpiSsdt; /*找ssdt*/
		goto retry;
	}

	data += 5; /*跳过一些东西*/
	data += ((data[0] & 0xc0) >> 6) + 2;
	if (data[0] == 0xa)
		++data;
	u16 s5a = (*data++) << 10; /*获取第一个数据*/
	if (data[0] == 0xa)
		++data;
	u16 s5b = (*data++) << 10; /*获取第二个数据*/

	for (;;)
	{
		outportw(acpiFadt->controlRegister1a, s5a | (1 << 13)); /*发送命令*/
		if (acpiFadt->controlRegister1b)
			outportw(acpiFadt->controlRegister1b, s5b | (1 << 13));
	}
	return -1;
}


extern "C"  __declspec(dllexport) int doReboot(void)
{
	__asm {
		//cli
	}
	if (acpiFadt->header.length < 129)
		goto next;  /*检查是否支持reset寄存器*/
	for (;;) /*发送Reset命令*/
		outportb(acpiFadt->resetRegister.address, acpiFadt->resetValue);
next: /*不支持就采用键盘控制器*/
	for (;;)
	{
		while (inportb(0x64) & 0x3)
			inportb(0x60);  /*把buf读空*/
		outportb(0x64, 0xfe); /*发送 Reset CPU 指令*/
	}
	return -1;
}

