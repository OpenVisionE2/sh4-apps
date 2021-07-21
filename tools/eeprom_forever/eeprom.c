/*
 * eeprom.c - Fortis version for 4th generation to read MAC address
 *
 * (c) 2011-2020 konfetti, Audioniek
 * largely copied from uboot source!
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/*
 * Description:
 *
 * Little utility to dump the EEPROM contents or show the
 * MAC address on Fortis receivers.
 * In effect this a stripped down version of ipbox_eeprom with major
 * modifications and additions. Kudos to the original authors.
 *
 * Principal NAND Emulation code taken from uboot.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

//#include <stdint.h>
//#include <linux/mtd/mtd.h>
//#include <linux/mtd/nand.h>
#define CFG_EEPROM_ADDR  0x57
#define CFG_EEPROM_SIZE  256

#define EEPROM_WRITE
#if defined EEPROM_WRITE
//#define CFG_EEPROM_PAGE_WRITE_DELAY_MS	11	/* 10ms. but give more */
//#define CFG_EEPROM_PAGE_WRITE_BITS 4
#define	EEPROM_PAGE_SIZE  (1 << CFG_EEPROM_PAGE_WRITE_BITS)
#endif

#define EEPROM_MTD             0x00380000

#define EEPROM_ETHADDR_OFFSET  0x00
#define EEPROM_USB_BOOT_OFFSET EEPROM_ETHADDR_OFFSET + 6

#define NAND_MINREAD_SIZE      0x800
#define EEPROM_EMUL_NR_BLOCK   64

typedef enum
{
	FT_OK,  // free block
	FT_NO,  // none free; need format
	FT_NO_INIT,  // no available, only 64 partition is available
} FT_NAND_STATUS;

typedef struct Nand_Emul_Data
{
	int Nand_Emul_BlockAddr0;
	int Nand_Emul_BlockAddr1;
	int Addr[EEPROM_EMUL_NR_BLOCK * 2];
	unsigned char Status[EEPROM_EMUL_NR_BLOCK * 2];
} MAND_EMUL_DATA;

MAND_EMUL_DATA *EmulData;
//nand_info_t *FtNand;
struct mtd_info *FtNand;

#if defined EEPROM_WRITE
int Nand_Emul_GetIdx(void)
{
	int i;

	for (i = 0; i < EEPROM_EMUL_NR_BLOCK * 2; i++)
	{
		if (EmulData->Status[i] == FT_OK)
		{
			return i;
		}
	}
	return -1;
}
#endif

int Nand_Emul_CheckPartition(uint addr)
{
	size_t len = NAND_MINREAD_SIZE;
	int j;
	unsigned char *EEPROM_Data;

	EEPROM_Data = (unsigned char *)malloc(NAND_MINREAD_SIZE);

	nand_read_skip_bad(FtNand, addr, &len, EEPROM_Data);

	for (j = 0; j < NAND_MINREAD_SIZE; j++)  // only check MAC addr
	{
		if (EEPROM_Data[j] != 0xff)  // found not erased sector
		{
			return FT_NO;
		}
	}
	free(EEPROM_Data);
	return FT_OK;
}

int Nand_Emul_GetNrFree(void)
{
	int i, count = 0;
	for (i = 0; i < EEPROM_EMUL_NR_BLOCK * 2; i++)
	{
		if ((EmulData->Status[i] == FT_OK))
		{
			count++;
		}
	}
	return count;
}

int Nand_Emul_GetFreeIdx(void)
{
	int i;

	// ALL FULL
	if (Nand_Emul_GetNrFree() == 0)
	{
		if ((EmulData->Status[EEPROM_EMUL_NR_BLOCK] == FT_NO_INIT))
		{
			return EEPROM_EMUL_NR_BLOCK - 1;  // return 63
		}
		else
		{
			return EEPROM_EMUL_NR_BLOCK * 2 - 1;  // return 127
		}
	}
#if 0
	// Round : 2 block
	if ((EmulData->Status[EEPROM_EMUL_NR_BLOCK * 2 - 1] == FT_NO) && (EmulData->Status[0] == FT_OK))
	{
		return EEPROM_EMUL_NR_BLOCK * 2 - 1;  // last idx : return 127
	}
#endif
	// Round : 1 block
	if ((EmulData->Status[EEPROM_EMUL_NR_BLOCK] == FT_NO_INIT))
	{
		for (i = 0; i < EEPROM_EMUL_NR_BLOCK; i++)
		{
			if ((EmulData->Status[i] == FT_OK))
			{
				if (i == 0)
				{
					return EEPROM_EMUL_NR_BLOCK - 1;  // return 63
				}
				else
				{
					return (i - 1);  // last idx
				}
			}
		}
	}
	// Round : 2 block
	for (i = 0; i < EEPROM_EMUL_NR_BLOCK * 2; i++)
	{
		if ((EmulData->Status[i] == FT_OK))
		{
				if (i == 0)
				{
					return EEPROM_EMUL_NR_BLOCK * 2 - 1;  // return 127
				}
				else
				{
					return (i - 1);  // last idx
				}
		}
	}
	return -1;  // no data
}

#if defined EEPROM_WRITE
void Nand_Emul_eeprom_write(uint addr, unsigned char *data, int length)
{
	nand_erase_options_t e_opts;

	int format_force = 0;
	int i, idx;

	int ret;
	int write_idx;
	unsigned char *EEPROM_Data;

	EEPROM_Data = (unsigned char *)malloc(NAND_MINREAD_SIZE);

	size_t len = NAND_MINREAD_SIZE;

	write_idx = ret = Nand_Emul_GetIdx();

	if ((ret == -1) \
	||  (ret == 63) \
	||  (ret == 127))  // no free block
	{
		format_force = 1;
	}
	idx = Nand_Emul_GetFreeIdx();

	nand_read_skip_bad(FtNand, EmulData->Addr[idx], &len, EEPROM_Data);
	memcpy(EEPROM_Data + addr, data, length);

	if (format_force == 1)
	{
		memset(&e_opts, 0, sizeof(e_opts));

		e_opts.length=FtNand->erasesize;

		if ((ret == -1)
		||  (ret == (EEPROM_EMUL_NR_BLOCK * 2 - 1)))  // all partitions are full or Need block0 format
		{
			e_opts.offset = EmulData->Addr[0];
			for (i = 0; i < EEPROM_EMUL_NR_BLOCK; i++)
			{
				EmulData->Status[i] = FT_OK;
			}
			if (ret == -1)
			{
				write_idx = 0;
			}
		}
		else if (ret == EEPROM_EMUL_NR_BLOCK - 1)  // need block1 format
		{
			if (EmulData->Status[EEPROM_EMUL_NR_BLOCK] == FT_NO_INIT) // only 1 block is available
			{
				e_opts.offset =  EmulData->Addr[0];
				for (i = 0; i < EEPROM_EMUL_NR_BLOCK; i++)
				{
					EmulData->Status[i] = FT_OK;
				}
				write_idx = 0;
			}
			else
			{
				e_opts.offset =  EmulData->Addr[EEPROM_EMUL_NR_BLOCK];
				for (i = EEPROM_EMUL_NR_BLOCK; i < EEPROM_EMUL_NR_BLOCK * 2; i++)
				{
					EmulData->Status[i] = FT_OK;
				}
				write_idx = EEPROM_EMUL_NR_BLOCK-1;
			}
		}
		e_opts.jffs2  = 0;  // jdc1004 '11.11.29 must be zero for kernel loading
		e_opts.quiet  = 0;
		e_opts.opt    = 0;
		printf("%s at [0x%x] [%d]\n", __func__, (unsigned int)e_opts.offset, ret);

		nand_erase_opts(FtNand, &e_opts);
	}
	nand_write_skip_bad(FtNand, EmulData->Addr[write_idx], &len, EEPROM_Data);
	EmulData->Status[write_idx] = FT_NO;
	printf("%s at [0x%x] [%d]\n", __func__, EmulData->Addr[write_idx], write_idx);
	free(EEPROM_Data);
}
#endif

int Nand_Emul_eeprom_read(uint addr, unsigned char *buffer, int length)
{
	size_t len = NAND_MINREAD_SIZE;
	int idx;
	unsigned char *EEPROM_Data;

	EEPROM_Data = (unsigned char *)malloc(NAND_MINREAD_SIZE);

	idx = Nand_Emul_GetFreeIdx();

	printf("%s at 0x%x [%d]\n", __func__, EmulData->Addr[idx], idx);

	nand_read_skip_bad(FtNand, EmulData->Addr[idx], &len, EEPROM_Data);

	memcpy(buffer, EEPROM_Data + addr, length);
	free(EEPROM_Data);
	return 0;
}

int Nand_Emul_Init(void)
{
	int offset;
	int i = 0;

	FtNand = &nand_info[0];

	EmulData = (MAND_EMUL_DATA *)malloc(sizeof(MAND_EMUL_DATA));

	EmulData->Nand_Emul_BlockAddr0 = -1;
	EmulData->Nand_Emul_BlockAddr1 = -1;

	for (i = 0; i < EEPROM_EMUL_NR_BLOCK * 2; i++)
	{
		EmulData->Status[i] = FT_NO_INIT;
	}
	for (offset = EEPROM_MTD; offset < EEPROM_MTD + 4 * (FtNand->erasesize); offset += FtNand->erasesize)
	{
		if (!nand_block_isbad(FtNand, offset))
		{
			if (EmulData->Nand_Emul_BlockAddr0 == -1)
			{
				EmulData->Nand_Emul_BlockAddr0 = offset;
				for (i = 0; i < EEPROM_EMUL_NR_BLOCK; i++)
				{
					EmulData->Addr[i] = offset + i * NAND_MINREAD_SIZE;
					EmulData->Status[i] = Nand_Emul_CheckPartition(offset + i * NAND_MINREAD_SIZE);
				}
			}
			else
			{
				EmulData->Nand_Emul_BlockAddr1 = offset;
				for (i = 0; i < EEPROM_EMUL_NR_BLOCK; i++)
				{
					EmulData->Addr[i + EEPROM_EMUL_NR_BLOCK] = (offset + i * NAND_MINREAD_SIZE);
					EmulData->Status[i + EEPROM_EMUL_NR_BLOCK] = Nand_Emul_CheckPartition(offset + i * NAND_MINREAD_SIZE);
				}
				break;
			}
		}
	}
#if 0
	for (i = 0; i < EEPROM_EMUL_NR_BLOCK; i++)
	{
		printf("partition[%d] %x[%d] %x\[%d]\n", i, EmulData->Addr[i], EmulData->Status[i], EmulData->Addr[i + EEPROM_EMUL_NR_BLOCK], EmulData->Status[i + EEPROM_EMUL_NR_BLOCK]);
	}
#endif
	printf("%s [0x%x 0x%x]\n", __func__, EmulData->Nand_Emul_BlockAddr0, EmulData->Nand_Emul_BlockAddr1);

	if (EmulData->Nand_Emul_BlockAddr0 == -1)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

#if defined EEPROM_WRITE
int Restore_Eeprom(unsigned char *buffer)
{
	int i;
	int restore_eeprom = 0;
	int idx;

	size_t len= NAND_MINREAD_SIZE;
	unsigned char *EEPROM_Data;

	EEPROM_Data = (unsigned char *)malloc(NAND_MINREAD_SIZE);

#if 0
	i2c_read(FORTIS_I2C_EEPROM_ADDR, EEPROM_ETHADDR_OFFSET, FORTIS_I2C_EEPROM_ADDR_LEN, (unsigned char *)buffer, 6);
	printf("%x %x %x %x %x %x\n",buffer[0], buffer[1], buffer[2], buffer[3] , buffer[4], buffer[5]);
#endif

	idx = Nand_Emul_GetFreeIdx();

	//printf("%s at 0x%x [%d]\n", __func__, EmulData->Addr[idx], idx);

	nand_read_skip_bad(FtNand, EmulData->Addr[idx], &len, EEPROM_Data);

	for (i = 0; i < 6; i++)  // do MAC only
	{
		if (buffer[i] != EEPROM_Data[i])
		{
			restore_eeprom = 1;
			printf("%s [%x:%x:%x:%x:%x:%x]\n", __func__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
			break;
		}
	}

	if (restore_eeprom == 1)
	{
		Nand_Emul_eeprom_write(EEPROM_ETHADDR_OFFSET, buffer, 6);
	}
	free(EEPROM_Data);
	return 0;
}

int do_erase_NAND_EMUL( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	nand_erase_options_t e_opts;

	memset(&e_opts, 0, sizeof(e_opts));

	e_opts.length = FtNand->erasesize * 4; // 4 block
	e_opts.jffs2  = 0;
	e_opts.quiet  = 0;
	e_opts.opt    = 0;
	e_opts.offset = EEPROM_MTD;

	nand_erase_opts(FtNand, &e_opts);
	printf("do_erase_NAND_EMUL at [0x%x] [0x%x]\n", (unsigned int)e_opts.offset, (unsigned int)e_opts.length);
	return 0;
}
#endif

int Read_Ethaddr(unsigned char *str)
{
	unsigned char mac[6];

	if (!Nand_Emul_eeprom_read(EEPROM_ETHADDR_OFFSET, mac, 6))
	{
//		sprintf((char *)str, (const char *)env_getcommand(e_mac_str), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
	else
	{
		strcpy((char *)str, "00:11:22:33:00:26");  // jdc1004 '13.03.08 make default
		return -1;
	}
	return 0;
}

#if defined EEPROM_WRITE
int Save_Ethaddr(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char buffer[20];
	char *end;
	unsigned char env_ethaddr[6];
	int i;
	char *s = argv[1];

	for (i = 0; i < 6; ++i)
	{
		env_ethaddr[i] = (argv[1] ? simple_strtoul(s, &end, 16) : 0); //&& 0xff
		if (s)
		{
			s = (*end) ? end + 1 : end;
		}
	}

	Nand_Emul_eeprom_write(EEPROM_ETHADDR_OFFSET, env_ethaddr, 6);

	udelay(20000); // jdc1004 '08.12.22 for 10ms sleep // change to 20ms

	Read_Ethaddr(buffer);
	printf("set ethaddr %s\n",buffer);
	setenv((char *)env_getcommand(e_ethaddr), buffer);
	return 0;
}
#endif


int main(int argc, char *argv[])
{
	int vLoop;

	printf("%s >\n", argv[0]);

	if (argc == 2 ) // 1 argument given
	{
		int i, rcode = 0;
		if ((strcmp(argv[1], "-d") == 0) || (strcmp(argv[1], "--dump") == 0))
		{
			unsigned char buf[CFG_EEPROM_SIZE];

//			rcode |= eeprom_read(fd_i2c, CFG_EEPROM_ADDR, 0, buf, sizeof(buf));
			rcode = Nand_Emul_eeprom_read(0, buf, sizeof(buf));
			if (rcode < 0)
			{
				rcode = 1;
				goto failed;
			}
			printf("EEPROM dump\n");
			printf("Addr  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
			printf("-----------------------------------------------------\n");
			for (vLoop = 0; vLoop < CFG_EEPROM_SIZE; vLoop += 0x10)
			{
				printf(" %02x ", vLoop);
				for (i = 0; i < 0x10; i++)
				{
					printf(" %02x", buf[vLoop + i]);
				}
				printf("\n");
			}
		}
		else if ((strcmp(argv[1], "-m") == 0) || (strcmp(argv[1], "--mac") == 0))
		{
			unsigned char buf[7];

//			rcode |= eeprom_read(fd_i2c, CFG_EEPROM_ADDR, 0, buf, sizeof(buf));
			rcode = Read_Ethaddr(buf);
			if (rcode < 0)
			{
				rcode = 1;
				goto failed;
			}
			for (vLoop = 0; vLoop < 6; vLoop++)
			{
				printf("%02x", buf[vLoop]);
				if (vLoop != 5)
				{
					printf(":");
				}
			}
			printf("\n");
		}
	}
	else // no arguments; show usage
	{
		printf("%s version 1.0\n\n", argv[0]);
		printf(" Usage:\n");
		printf("%s [ -d | --dump | -m | --mac ]\n\n", argv[0]);
		printf(" no args    : show this usage\n");
		printf(" -d | --dump: hex dump eeprom contents\n");
		printf(" -m | --mac : show MAC address\n");
	}
	return 0;

failed:
	printf("[eeprom] failed <\n");
	return -1;
}
// vim:ts=4
