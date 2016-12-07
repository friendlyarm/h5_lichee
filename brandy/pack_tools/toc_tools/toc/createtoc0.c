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
#include <errno.h>

int createtoc0(toc_descriptor_t *toc0, char *toc0_name)
{
	char toc0_full_name[MAX_PATH];
	char toc0_content_name[MAX_PATH];
	FILE *p_file, *src_file;
	uint offset, offset_align;
	uint file_len;
	int  ret = -1;
	sbrom_toc0_head_info_t  *toc0_head;
	sbrom_toc0_item_info_t  *item_head, *p_item_head;
	char *toc0_content;
	toc_descriptor_t *p_toc0 = toc0;

	//申请空间用于保存文件内容
	toc0_content = (char *)malloc(10 * 1024 * 1024);
	if(toc0_content == NULL)
	{
		printf("createtoc0 err: cant malloc memory to store file content\n");

		goto __createtoc0_err;
	}
	memset(toc0_content, 0, 10 * 1024 * 1024);
	//toc0_head指针
	toc0_head = (sbrom_toc0_head_info_t *)toc0_content;
	//item_head指针
	item_head = (sbrom_toc0_item_info_t *)(toc0_content + sizeof(sbrom_toc0_head_info_t));
	//填充main info
	strcpy(toc0_head->name, TOC0_MAGIC);
	toc0_head->magic    = TOC_MAIN_INFO_MAGIC;
	toc0_head->end      = TOC_MAIN_INFO_END;
	toc0_head->items_nr = 2;
	//填充itme info
	offset = ((sizeof(sbrom_toc0_head_info_t) + 2 * sizeof(sbrom_toc0_item_info_t) + 31) & (~31)) + 1024;
	printf("cert offset=%d\n", offset);
	//遍历
	memset(toc0_content_name, 0, MAX_PATH);
	sprintf(toc0_content_name, "%s.crtpt", p_toc0->cert);

	//读取第一个文件，为证书文件
	printf("toc0_content_name=%s\n", toc0_content_name);
	src_file = fopen(toc0_content_name, "rb");
	if(src_file == NULL)
	{
		printf("file %s cant be open\n", toc0_content_name);

		goto __createtoc0_err;
	}
	fseek(src_file, 0, SEEK_END);
	file_len = ftell(src_file);
	fseek(src_file, 0, SEEK_SET);
	fread(toc0_content + offset, file_len, 1, src_file);
	fclose(src_file);
	src_file = NULL;

	p_item_head = item_head;
	p_item_head->name        = ITEM_NAME_SBROMSW_CERTIF;
	p_item_head->data_offset = offset;
	p_item_head->data_len    = file_len;
	p_item_head->type        = 1;
	p_item_head->end         = TOC_ITEM_INFO_END;

	offset = (offset + file_len + 15) & (~15);
	printf("offset=%d\n", offset);

	memset(toc0_content_name, 0, MAX_PATH);
	strcpy(toc0_content_name, p_toc0->bin);

	p_item_head++;
	//读取第二个文件，为sboot文件
	printf("toc0_content_name=%s\n", toc0_content_name);
	src_file = fopen(toc0_content_name, "rb");
	if(src_file == NULL)
	{
		printf("file %s cant be open\n", toc0_content_name);

		goto __createtoc0_err;
	}
	fseek(src_file, 0, SEEK_END);
	file_len = ftell(src_file);
	fseek(src_file, 0, SEEK_SET);
	fread(toc0_content + offset, file_len, 1, src_file);
	fclose(src_file);
	src_file = NULL;

	struct standard_Boot_file_head  *file_head = (struct standard_Boot_file_head *)(toc0_content + offset);

	p_item_head->name        = ITEM_NAME_SBROMSW_FW;
	p_item_head->data_offset = offset;
	p_item_head->data_len    = file_len;
	p_item_head->end         = TOC_ITEM_INFO_END;
	p_item_head->type        = 2;
	p_item_head->run_addr    = file_head->run_addr;
	//文件读取完毕
	offset = (offset + file_len + 15) & (~15);
	printf("offset=%d\n", offset);
	offset_align = (offset + 16 * 1024 - 1) & (~(16*1024-1));
	toc0_head->valid_len = offset_align;
	toc0_head->add_sum = STAMP_VALUE;
	toc0_head->add_sum = gen_general_checksum(toc0_content, offset_align);

	printf("offset_align=%d\n", offset_align);
	//创建toc1
	memset(toc0_full_name, 0, MAX_PATH);
	GetFullPath(toc0_full_name, toc0_name);
	printf("toc0_full_name=%s\n", toc0_full_name);
	p_file = fopen(toc0_full_name, "wb");
	if(p_file == NULL)
	{
		printf("createtoc0 err: cant create toc0\n");

		goto __createtoc0_err;
	}
	fwrite(toc0_content, offset_align, 1, p_file);
	fclose(p_file);
	p_file = NULL;

	ret = 0;

__createtoc0_err:
	if(p_file)
		fclose(p_file);
	if(src_file)
		fclose(src_file);
	if(toc0_content)
		free(toc0_content);

	return ret;
}

int splittoc0(char *toc0)
{
	char full_toc0[MAX_PATH] , full_sboot[MAX_PATH];
	FILE *ftoc0 = NULL, *fsboot= NULL;

	int ret = -1 , totallen ,actlen ;
	char *sboot = "sboot.bin";
	char *body ;

	unsigned int offset;

	printf("split %s\n", toc0);
	
	sbrom_toc0_head_info_t  *toc0_head;
	sbrom_toc0_item_info_t  *item_head ;

	GetFullPath(full_toc0, toc0);
	GetFullPath(full_sboot, sboot);
	printf("getfull name : toc0=%s,sboot=%s\n",
			toc0,sboot);
	if( (ftoc0=fopen(full_toc0,"r+")) == NULL) {
		printf("Open toc0 file %s fail ---%s\n",full_toc0,
				strerror(errno));
		goto splittoc0_out;
	}

	totallen = getfile_size(ftoc0);
	if( (body= malloc(totallen)) ==NULL ){
		printf("malloc toc0 body fail\n");
		goto splittoc0_out;
	}

	if( fread(body, totallen,1,ftoc0) != 1 ){
		printf("Read toc0 %s file head fail --- %s\n", full_toc0,
				strerror(errno));
			goto splittoc0_out;
	}

	toc0_head = (sbrom_toc0_head_info_t *)body;
	item_head = (sbrom_toc0_item_info_t *)(sizeof(sbrom_toc0_head_info_t) + body) ;

	item_head ++ ; /*point to sboot*/
	offset = item_head->data_offset ;
	actlen = item_head->data_len ;

	if( (fsboot=fopen(full_sboot,"w+")) == NULL) {
		printf("Open sboot file %s fail ---%s\n",full_sboot,
				strerror(errno));
		goto splittoc0_out;
	}

	if( fwrite(body+offset, actlen, 1, fsboot ) != 1){
		printf("write sboot file fail --- %s\n", strerror(errno));
		goto splittoc0_out;
	}
	ret = 0;	
	printf("split sboot from toc0 done\n");
splittoc0_out:
	if(body)
		free(body);
	if(fsboot)
		fclose(fsboot);
	if(ftoc0)
		fclose(ftoc0);
	return ret ;
}

int update_toc0_cert(toc_descriptor_t* toc0, char *toc0_name)
{
	char toc0_full_name[MAX_PATH];
	char toc0_content_name[MAX_PATH];
	FILE *p_file, *src_file;
	uint offset;
	uint file_len;
	int  ret = -1;
	sbrom_toc0_item_info_t  *item_head ;
	sbrom_toc0_head_info_t  *toc0_head;

	char *toc0_content;
	toc_descriptor_t *p_toc0 = toc0;
	int actlen ;

	memset(toc0_full_name, 0, MAX_PATH);
	strcpy(toc0_full_name, toc0_name);
	printf("toc0_full_name=%s\n", toc0_full_name);
	p_file = fopen(toc0_full_name, "r+");
	if(p_file == NULL)
	{
		printf("createtoc0 err: cant create toc0\n");
		goto out;
	}

	actlen = getfile_size(p_file);
	toc0_content = (char *)malloc(actlen);
	if(toc0_content == NULL)
	{
		printf("createtoc0 err: cant malloc memory to store file content\n");
		goto out;
	}
	memset(toc0_content, 0, actlen);
	printf("file actlen %x\n",actlen);
	if( fread(toc0_content, actlen,1,p_file) <0 ){
		printf("Read toc0 %s file head fail --- %s\n", toc0_full_name,
				strerror(errno));
		goto out;
	}
	toc0_head = (sbrom_toc0_head_info_t *)(toc0_content);
	item_head = (sbrom_toc0_item_info_t *)(toc0_content + sizeof(sbrom_toc0_head_info_t));
	offset = item_head->data_offset;

	memset(toc0_content_name, 0, MAX_PATH);
	sprintf(toc0_content_name, "%s.crtpt", p_toc0->cert);
	//读取第一个文件，为证书文件
	printf("toc0_content_name=%s\n", toc0_content_name);
	src_file = fopen(toc0_content_name, "rb");
	if(src_file == NULL)
	{
		printf("file %s cant be open\n", toc0_content_name);
		goto out;
	}
	fseek(src_file, 0, SEEK_END);
	file_len = ftell(src_file);
	fseek(src_file, 0, SEEK_SET);
	fread(toc0_content + offset, file_len, 1, src_file);
	fclose(src_file);
	src_file = NULL;

	item_head->name        = ITEM_NAME_SBROMSW_CERTIF;
	item_head->data_offset = offset;
	item_head->data_len    = file_len;
	item_head->type        = 1;
	item_head->end         = TOC_ITEM_INFO_END;

	/*update cert to toc0*/

	toc0_head->add_sum = STAMP_VALUE;
	toc0_head->add_sum = gen_general_checksum(toc0_content, actlen);

	fseek(p_file, 0, SEEK_SET);
	fwrite(toc0_content, actlen, 1, p_file);
	fclose(p_file);
	p_file = NULL;
	ret = 0 ;
out:
	if(toc0_content)
		free(toc0_content);
	return ret;
}


