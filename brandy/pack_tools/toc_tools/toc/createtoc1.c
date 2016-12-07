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

int createtoc1(toc_descriptor_t *toc1, char *toc1_name, int main_v, int sub_v)
{
	char toc1_full_name[MAX_PATH];
	char toc1_content_name[MAX_PATH];
	FILE *p_file, *src_file;
	uint offset, offset_align;
	uint file_len;
	int  ret = -1;
	int  item_count = 0, content_count = 0, i;
	sbrom_toc1_head_info_t  *toc1_head;
	sbrom_toc1_item_info_t  *item_head, *p_item_head;
	char *toc1_content;
	toc_descriptor_t *p_toc1 = toc1;

	for(i=0;i<TOC1_CONFIG_MAX;i++)
	{
		if(!toc1[i].item[0])
		{
			break;
		}
		if(toc1[i].type == NORMAL_TYPE)
		{
			content_count ++;
		}
		content_count ++;
		item_count ++;
	}

	printf("item_count=%d\n", item_count);
	printf("content_count=%d\n", content_count);
	//申请空间用于保存文件内容
	toc1_content = (char *)malloc(10 * 1024 * 1024);
	if(toc1_content == NULL)
	{
		printf("createtoc1 err: cant malloc memory to store file content\n");

		goto __createtoc1_err;
	}
	memset(toc1_content, 0, 10 * 1024 * 1024);
	//toc1_head指针
	toc1_head = (sbrom_toc1_head_info_t *)toc1_content;
	//item_head指针
	item_head = (sbrom_toc1_item_info_t *)(toc1_content + sizeof(sbrom_toc1_head_info_t));
	//填充main info
	strcpy(toc1_head->name, "sunxi-secure");
	toc1_head->magic    = TOC_MAIN_INFO_MAGIC;

	printf("toc1_head->main_version=%d\n", toc1_head->main_version);
	printf("toc1_head->sub_version=%d\n", toc1_head->sub_version);

	toc1_head->main_version = main_v;
	toc1_head->sub_version  = sub_v;
	toc1_head->end      = TOC_MAIN_INFO_END;
	toc1_head->items_nr = content_count;
	//填充itme info
	offset = (sizeof(sbrom_toc1_head_info_t) + content_count * sizeof(sbrom_toc1_item_info_t) + 1023) & (~1023);
	//遍历
	for(p_item_head=item_head;item_count > 0;p_item_head++, p_toc1++, item_count--)
	{
		memset(toc1_content_name, 0, MAX_PATH);
		sprintf(toc1_content_name, "%s.der", p_toc1->cert);

		//printf("toc1_content_name=%s\n", toc1_content_name);

		src_file = fopen(toc1_content_name, "rb");
		if(src_file == NULL)
		{
			printf("file %s cant be open\n", toc1_content_name);

			goto __createtoc1_err;
		}
		fseek(src_file, 0, SEEK_END);
		file_len = ftell(src_file);
		fseek(src_file, 0, SEEK_SET);
		fread(toc1_content + offset, file_len, 1, src_file);
		fclose(src_file);
		src_file = NULL;

		p_item_head->data_offset = offset;
		p_item_head->data_len    = file_len;
		if(p_item_head==item_head)
		{
			p_item_head->type        = ITEM_TYPE_ROOTKEY;
		}
		else
		{
			p_item_head->type        = ITEM_TYPE_BINKEY;
		}
		p_item_head->end         = TOC_ITEM_INFO_END;

		strcpy(p_item_head->name, p_toc1->item);

		offset = (offset + file_len + 1023) & (~1023);

		if(p_toc1->type == NORMAL_TYPE)
		{
			memset(toc1_content_name, 0, MAX_PATH);
			strcpy(toc1_content_name, p_toc1->bin);
			//printf("toc1_content_name=%s\n", toc1_content_name);

			src_file = fopen(toc1_content_name, "rb");
			if(src_file == NULL)
			{
				printf("file %s cant be open\n", toc1_content_name);

				goto __createtoc1_err;
			}
			fseek(src_file, 0, SEEK_END);
			file_len = ftell(src_file);
			fseek(src_file, 0, SEEK_SET);
			fread(toc1_content + offset, file_len, 1, src_file);
			fclose(src_file);
			src_file = NULL;

			p_item_head++;

			struct spare_boot_ctrl_head  *file_head = (struct spare_boot_ctrl_head *)(toc1_content + offset);

			p_item_head->data_offset = offset;
			p_item_head->data_len    = file_len;
			p_item_head->end         = TOC_ITEM_INFO_END;
			p_item_head->type        = ITEM_TYPE_BINFILE;
			p_item_head->run_addr    = file_head->run_addr;

			strcpy(p_item_head->name, p_toc1->item);

			offset = (offset + file_len + 1023) & (~1023);
		}
	}

	offset_align = (offset + 16 * 1024 - 1) & (~(16*1024-1));
	toc1_head->valid_len = offset_align;
	toc1_head->add_sum = STAMP_VALUE;
	toc1_head->add_sum = gen_general_checksum(toc1_content, offset_align);

	//printf("offset_align=%d\n", offset_align);
	//创建toc1
	memset(toc1_full_name, 0, MAX_PATH);
	GetFullPath(toc1_full_name, toc1_name);
	//printf("toc1_full_name=%s\n", toc1_full_name);
	p_file = fopen(toc1_full_name, "wb");
	if(p_file == NULL)
	{
		printf("createtoc1 err: cant create toc1\n");

		goto __createtoc1_err;
	}
	fwrite(toc1_content, offset_align, 1, p_file);
	fclose(p_file);
	p_file = NULL;

	ret = 0;

__createtoc1_err:
	if(p_file)
		fclose(p_file);
	if(src_file)
		fclose(src_file);
	if(toc1_content)
		free(toc1_content);

	return ret;
}

int getfile_size(FILE *pFile)
{
	if( pFile == NULL)  
		return -1 ;  

	int cur_pos = ftell(pFile); 
	if(cur_pos == -1)
		return -1 ; 

	if(fseek(pFile, 0L, SEEK_END) == -1)
		return -1 ; 
	int size= ftell(pFile); 
	if(fseek(pFile, cur_pos, SEEK_SET)== -1) 
		return -1 ;
	return size ;
} 

/*split uboot image from toc1*/
int splittoc1(char *toc1 )
{
	char full_toc1[MAX_PATH] , full_uboot[MAX_PATH] ;
	FILE *ftoc1 = NULL, *fuboot = NULL;
	int ret = -1, totallen, actlen ,cnt;
	char *body = NULL ;
	char * uboot = "u-boot.fex";

	unsigned int offset;

	printf("split %s\n",toc1);
	sbrom_toc1_head_info_t  *toc1_head;
	sbrom_toc1_item_info_t  *item_head ;

	GetFullPath(full_toc1, toc1);
	GetFullPath(full_uboot, uboot);

	if( (ftoc1=fopen(full_toc1,"r+")) == NULL) {
		printf("Open toc1 file %s fail ---%s\n",full_toc1,
				strerror(errno));
		goto splittoc1_out;
	}

	totallen = getfile_size(ftoc1);
	if( (body= malloc(totallen)) ==NULL ){
		printf("malloc toc1 body fail\n");
		goto splittoc1_out;
	}

	if( fread(body, totallen,1,ftoc1) != 1 ){
		printf("Read toc1 %s file head fail --- %s\n", full_toc1,
				strerror(errno));
			goto splittoc1_out;
	}
	toc1_head = (sbrom_toc1_head_info_t *)body ;
	item_head = body + sizeof(sbrom_toc1_head_info_t);
	cnt = toc1_head->items_nr;
	offset = (sizeof(sbrom_toc1_head_info_t) + cnt* sizeof(sbrom_toc1_item_info_t) + 1023) & (~1023);

	for(; cnt >0 ;item_head ++, cnt -- ) 
		if( (!strcmp(item_head->name, "u-boot") )&& 
				(item_head->type == 3))
			break ;
		
	offset = item_head->data_offset ;
	actlen = item_head->data_len ;
	printf("uboot offset 0x%x len 0x%x\n",offset, actlen);

	if( (fuboot=fopen(full_uboot,"w+")) == NULL) {
		printf("Open uboot file %s fail ---%s\n",full_uboot,
				strerror(errno));
		goto splittoc1_out;
	}

	if( fwrite(body+offset, actlen, 1, fuboot ) != 1){
		printf("write uboot file fail --- %s\n", strerror(errno));
		goto splittoc1_out;
	}
	ret = 0;	

splittoc1_out:
	if(body)
		free(body);
	if(ftoc1)
		fclose(ftoc1);
	if(fuboot)
		fclose(fuboot);
	return ret ;
}

