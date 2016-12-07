/*
**********************************************************************************************************************
*											        eGon
*						           the Embedded GO-ON Bootloader System
*									       eGON arm boot sub-system
*
*						  Copyright(C), 2006-2014, Allwinner Technology Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      : Jerry
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/


#include "common.h"
#include "include.h"




static void usage(void)
{
	printf(" dtbfile   create a help file to search dtb file much more faster\n");

	return ;
}


char *__probe_file_data(char *file_name, int *file_len)
{
	FILE *pfile;
	int   len;
	char *buffer;

	pfile = fopen(file_name, "rb");
	if (pfile == NULL) {
		printf("Fast DTB : file cant be opened\n");

		return NULL;
	}
	fseek(pfile, 0, SEEK_END);
	len = ftell(pfile);

	buffer = malloc(len);
	if (buffer == NULL) {
		printf("Fast DTB : buffer cant be malloc\n");
		fclose(pfile);

		return NULL;
	}

	memset(buffer, 0, len);

	fseek(pfile, 0, SEEK_SET);
	fread(buffer, len, 1, pfile);
	fclose(pfile);

	*file_len = len;

	return buffer;
}

int main(int argc, char *argv[])
{
	char dtbpath[MAX_PATH]="";
	char *data;
	int  len;

	if(argc < 2)
	{
		printf("Fast DTB : not enough parameters\n");
		usage();

		return -1;
	}

	GetFullPath(dtbpath, argv[1]);
	printf("dtbpath=%s\n", dtbpath);

	data = __probe_file_data(dtbpath, &len);
	if (data == NULL) {

		return -1;
	}
	dtb_decode(data, len);
	//if (dtb_decode(data, len) == 0)
	//	dtbfast_path_test(data);


	if (data)
		free(data);

	return 0;
}
