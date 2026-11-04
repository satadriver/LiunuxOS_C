
#include "acpi.h"
#include "hardware.h"
#include "video.h"



static const ACPIFadt *acpiFadt = 0;

static const ACPIHeader *acpiSsdt = 0;

static const ACPIHeader *acpiDsdt = 0;





/* ---------- 辅助宏 ---------- */
#define MADT_TYPE_LOCAL_APIC  0
#define MADT_FLAG_ENABLED     1

/* ---------- 遍历 RSDT/XSDT 查找指定签名的表 ---------- */
static void *acpi_find_table(void *root_sdt, const char *sig, int is_xsdt)
{
	acpi_sdt_header_t *sdt = (acpi_sdt_header_t *)root_sdt;
	uint8_t *entry_start = (uint8_t *)root_sdt + sizeof(acpi_sdt_header_t);
	size_t entry_size = is_xsdt ? 8 : 4;
	size_t entry_count = (sdt->length - sizeof(acpi_sdt_header_t)) / entry_size;

	for (size_t i = 0; i < entry_count; i++) {
		uint64_t table_addr;
		if (is_xsdt)
			table_addr = *(uint64_t *)(entry_start + i * 8);
		else
			table_addr = *(uint32_t *)(entry_start + i * 4);

		acpi_sdt_header_t *table = (acpi_sdt_header_t *)table_addr;
		if (__memcmp(table->signature, sig, 4) == 0)
			return table;
	}
	return NULL;
}

/*
* 从 RSDP 开始，找到 MADT 并遍历所有 CPU。
* 返回启用的 CPU 数量；lapic_ids 用于填充每个 CPU 的 APIC ID。
* 注意：需确保物理地址已映射到当前虚拟地址空间。
*/
int acpi_enumerate_cpus(acpi_rsdp_t *rsdp, uint8_t *lapic_ids, int max_cpus)
{
	acpi_sdt_header_t *madt;
	int cpu_count = 0;

	/* 1. 根据 RSDP 修订版本选择 XSDT 或 RSDT */
	if (rsdp->revision >= 2 && rsdp->xsdt_address != 0) {
		madt = (acpi_sdt_header_t *)acpi_find_table((void *)rsdp->xsdt_address, "APIC", 1);
	}
	else if (rsdp->rsdt_address != 0) {
		madt = (acpi_sdt_header_t *) acpi_find_table((void *)rsdp->rsdt_address, "APIC", 0);
	}
	else {
		return -1;  /* 无有效根表 */
	}

	if (!madt)
		return -1;  /* 未找到 MADT */

					/* 2. 遍历 MADT 子表 */
	uint8_t *ptr = (uint8_t *)madt + sizeof(acpi_sdt_header_t);
	uint8_t *end = (uint8_t *)madt + madt->length;

	while (ptr + sizeof(madt_subtable_header_t) <= end) {
		madt_subtable_header_t *sub = (madt_subtable_header_t *)ptr;

		if (sub->length == 0)
			break;  /* 防止死循环 */

		if (sub->type == MADT_TYPE_LOCAL_APIC) {
			madt_local_apic_t *lapic = (madt_local_apic_t *)sub;
			/* 仅统计 Enabled 的处理器 [citation:7] */
			if (lapic->flags & MADT_FLAG_ENABLED) {
				if (cpu_count < max_cpus)
					lapic_ids[cpu_count] = lapic->apic_id;
				cpu_count++;
			}
		}
		ptr += sub->length;
	}

	return cpu_count;
}


static int parseApic(ACPIHeaderApic *apic)
{
	char szout[256];
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
				__printf(szout,"Found CPU: processor ID => %d, apic ID => %d \n",(int)localApic->apicProcessorID, (int)localApic->apicID);
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

int parseDT(ACPIHeader *dt)
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



int parseRSDT(ACPIHeader *rsdt)
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


int parseXSDT(ACPIHeader* rsdt)
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



int parseRSDP(XSDP_HEADER* xsdp)
{
	int res = 0;

	char szout[256];
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
	char szout[256];
	u8 *start = (u8 *)0xe0000;
	u8 * const end = (u8 *)0xfffff;
	while (start < end) {
		u64 signature = *(u64 *)start;
		if (signature == *(const u64 *)"RSD PTR ")
		{
			unsigned char cpus[256];
			int cpucnt = acpi_enumerate_cpus((acpi_rsdp_t *)start,cpus,256);
			if (cpucnt) {
				__printf(szout, "find apic cpu:%d\r\n",cpucnt);
			}

			if (parseRSDP((XSDP_HEADER * )start) == 0)
			{
				return TRUE;
			}
		}
		start += 0x10;
	}
	u8 *ebda = (u8 *)( * (u16 *) (0x40E << 4));
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

