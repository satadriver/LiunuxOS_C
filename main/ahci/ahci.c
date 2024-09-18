//#include <stdio.h>

#include "ahci.h"
#include "fis.h"
#include "../Utils.h"



void debug_HBA_PORT(HBA_PORT *ptr) {
	__printf("---debug_HBA_PORT---\n");
	__printf("command list base address %x %x\n", ptr->clbu, ptr->clb);
	__printf("FIS base address %x %x\n", ptr->fbu, ptr->fb);
	__printf("interrupt status %x\n", ptr->is);
	__printf("interrupt enable %x\n", ptr->ie);
	__printf("commnad ans status %x\n", ptr->cmd);
	__printf("commnad issue\n", ptr->ci);
	__printf("------\n");
}

void debug_HBA_MEM(HBA_MEM *ptr) {
	__printf("---debug_HBA_MEM---\n");
	__printf("hport[0] addr %p\n", &(ptr->ports[0]));
	__printf("hport[1] addr %p\n", &(ptr->ports[1]));
	__printf("------\n");
}

void debug_HBA_CMD_HEADER(HBA_CMD_HEADER *ptr) {
	__printf("---debug_HBA_CMD_HEADER---\n");
	__printf("Command FIS length %x\n", ptr->cfl);
	__printf("w %x\n", ptr->w);
	__printf("prdt length\n", ptr->prdtl);
	__printf("Command table base addr %x %x\n", ptr->ctbau, ptr->ctba);
	__printf("------\n");
}

void debug_HBA_FIS(HBA_FIS *ptr) {
	__printf("---debug_HBA_FIS---\n");
	debug_FIS_DMA_SETUP(&(ptr->dsfis));
	debug_FIS_PIO_SETUP(&(ptr->psfis));
	debug_FIS_REG_D2H(&(ptr->rfis));
	// debug_FIS_DEV_BITS(&(ptr->sdafis));
	__printf("------\n");
}

void debug_HBA_PRDT_ENTRY(HBA_PRDT_ENTRY *ptr) {
	__printf("---debug_HBA_PRDT_ENTRY---\n");
	__printf("Data base address %x %x\n", ptr->dbau, ptr->dba);
	__printf("Byte Count %x\n", ptr->dbc);
	__printf("i %x\n", ptr->i);
	__printf("------\n");	
}

void debug_HBA_CMD_TBL(HBA_CMD_TBL *ptr) {
	__printf("---debug_HBA_CMD_TBL---\n");
	// TODO: cfis[64]
	debug_HBA_CMD_TBL(ptr);
	__printf("------\n");
}