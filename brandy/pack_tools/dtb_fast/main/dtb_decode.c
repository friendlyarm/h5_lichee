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

uint dtbfast_path_offset_search(char *dtbfast_buffer, char *dtb_buffer, char *name);

static int fdt_check_header(const void *buffer)
{
	struct fdt_header *fdt = (struct fdt_header *)buffer;

	if (cpu_to_be32(fdt->magic) != FDT_MAGIC) {
		printf("fdt_check_header err: magic invalid\n");

		return -1;
	}

	if (cpu_to_be32(fdt->version) != FDT_LAST_SUPPORTED_VERSION) {
		printf("fdt_check_header err: version too new\n");

		return -1;
	}

	return 0;
}


static char *__dtb_decode_head_node_name(uint *name_sum, uint *name_sum_short, uint *name_bytes, uint *name_bytes_short, char *node_pt)
{
	char *tmp_node_pt = node_pt;
	uint sum = 0, sum_short = 0;
	uint len =0, len_short = 0;

	while((*tmp_node_pt) && (*tmp_node_pt != '@')) {
		sum_short += *tmp_node_pt++;
		len_short ++;
	}

	tmp_node_pt = node_pt;
	while(*tmp_node_pt++) {
		sum += *(tmp_node_pt-1);
		len ++;
	}

	*name_sum = sum;
	*name_sum_short = sum_short;
	*name_bytes= len;
	*name_bytes_short= len_short;

	tmp_node_pt = (char *)(((unsigned long)tmp_node_pt + 3) & ~3);

	return tmp_node_pt;
}

static int __dtbfast_store(struct head_node *head_block_buff[], uint head_depth_count[],
							struct prop_node *prop_node_block,
							struct dtbfast_header *header,
							char *buffer, int len)
{
	FILE *pfile;
	int i, offset;
	char dtbpath[MAX_PATH]="";

	GetFullPath(dtbpath, "sunxi.fex");
	pfile = fopen(dtbpath, "wb");
	if(pfile == NULL) {
		printf("__dtbfast_store fail: cant create file to store dtbfast infomation\n");

		return -1;
	}
	//首先写入原来的dtb数据
	fseek(pfile, 0, SEEK_SET);
	fwrite(buffer, len, 1, pfile);
	offset = (len + 16)  & (~15);
	fseek(pfile, offset + sizeof(struct dtbfast_header), SEEK_SET);

	for(i=0;head_depth_count[i] > 0; i++)
	{
		fwrite(head_block_buff[i], head_depth_count[i] * sizeof(struct head_node), 1, pfile);
	}
	fwrite(prop_node_block, header->prop_count * sizeof(struct prop_node), 1, pfile);

	header->totalsize = ftell(pfile);
	fseek(pfile, offset, SEEK_SET);
	fwrite(header, sizeof(struct dtbfast_header), 1, pfile);

	fclose(pfile);

	return 0;

}


static int __dtb_decode_prop_node_name(uint *name_sum, uint *name_bytes, char *node_pt)
{
	char *tmp_node_pt = node_pt;
	uint sum = 0, len = 0;

	while(*tmp_node_pt) {
		sum += *tmp_node_pt;
		len ++;
		tmp_node_pt ++;
	}

	*name_sum = sum;
	*name_bytes= len;

	return 0;
}


static int fdt_scan_head_prop(char *buffer, uint head_depth_count[], uint *prop_total)
{
	uint32 node_type;
	uint32 head_depth_index = 0;
	uint32 prop_count = 0;
	char  *node_pt = buffer;

	node_type = cpu_to_be32(*(uint *)node_pt);

	while (node_type != FDT_END) {

		//printf("node_type = 0x%x\n", node_type);
		switch (node_type)
		{
			case FDT_BEGIN_NODE:		//1
				node_pt += FDT_TAGSIZE;
				while (*node_pt++);

				node_pt = (char *)(((unsigned long)node_pt + 3) & ~3);
				head_depth_count[head_depth_index] ++;
				//printf("depth %d +1\n", head_depth_index);
				head_depth_index ++;

				break;
			case FDT_END_NODE:			//2
				node_pt += FDT_TAGSIZE;

				head_depth_index --;
				break;
			case FDT_PROP:				//3
			{
				struct fdt_property *prop = (struct fdt_property *)node_pt;

				node_pt += FDT_TAGSIZE + 2 * sizeof(uint32) + cpu_to_be32(prop->len);
				node_pt = (char *)(((unsigned long)node_pt + 3) & ~3);

				prop_count ++;

				break;
			}
			case FDT_NOP:				//4
				node_pt += FDT_TAGSIZE;

				break;
			default:
				return -1;
		}

		node_type = cpu_to_be32(*(uint *)node_pt);
	}
	*prop_total = prop_count;

//	printf("total prop = %d\n", *prop_total);

	return 0;
}

int dtb_decode(char *buffer, int len)
{
	struct fdt_header *fdt;
	uint32 struct_off, string_off;
	uint   head_depth_count[DTBFAST_HEAD_MAX_DEPTH] = {0};
	uint   head_depth_index[DTBFAST_HEAD_MAX_DEPTH] = {0};

	int    depth_index = -1;	//初始为一个无效的值
	uint   prop_count, prop_index = 0;

	struct head_node *head_block_buff[DTBFAST_HEAD_MAX_DEPTH];
	struct head_node *head = NULL;

	struct prop_node *prop_node_block = NULL;
	struct prop_node *prop = NULL;

	struct dtbfast_header fast_header;
	uint32  node_type;
	uint32  head_total_count=0, head_block_offset;
	char    *node_pt, *struct_base;

	int    ret=-1, i, j;

	if (fdt_check_header(buffer))
		return -1;

	fdt = (struct fdt_header *)buffer;
	struct_off = fdt->off_dt_struct;
	string_off = fdt->off_dt_strings;
	node_pt = buffer + cpu_to_be32(fdt->off_dt_struct);

	ret = fdt_scan_head_prop(node_pt, head_depth_count, &prop_count);
	if (ret) {
		printf("dtb_decode err: dtb file is invalid\n");

		return -1;
	}

	for(i=0;head_depth_count[i] > 0;i++) {
		head_block_buff[i] = (struct head_node *)malloc(head_depth_count[i] * sizeof(struct head_node));
		if(head_block_buff[i] == NULL) {
			printf("dtb_decode err: cant malloc memory for head node\n");

			goto __dtb_decode_err2;
		}
		head_total_count += head_depth_count[i];
	}
	head_block_offset = head_total_count * sizeof(struct head_node);

	prop_node_block = (struct prop_node *)malloc(prop_count * sizeof(struct prop_node));
	if(prop_node_block == NULL) {
		printf("dtb_decode err: cant malloc memory for prop node\n");

		goto __dtb_decode_err1;
	}

	node_pt = buffer + cpu_to_be32(fdt->off_dt_struct);
	struct_base = node_pt;
	node_type = cpu_to_be32(*(uint *)node_pt);

	while (node_type != FDT_END) {

		//printf("node_type = 0x%x\n", node_type);
		switch (node_type)
		{
			case FDT_BEGIN_NODE:
			{
				struct head_node *pre_head_node;

				node_pt += FDT_TAGSIZE;
				//找到一个新的BEGIN_NODE，就指向下一级level
				depth_index ++;
				//printf("depth_index=%d\n", depth_index);
				//从block中取出当前level的head地址,当前level数据块，当前level的已用个数
				head = head_block_buff[depth_index] + head_depth_index[depth_index];
				//填充名称偏移量，从DTB的struct算起
				head->name_offset = (uint32)((unsigned long)node_pt - (unsigned long)buffer) - cpu_to_be32(fdt->off_dt_struct);
				//printf("name offset=0x%x\n", head->name_offset);

				//填充名称累加和，名称长度
				node_pt = __dtb_decode_head_node_name(&head->name_sum, &head->name_sum_short, &head->name_bytes, &head->name_bytes_short, node_pt);
				//如果这是一个深度超过1的head，检查其父节点是否已经被链接
				if (depth_index > 0) {
					uint32 this_offset = 0;

					pre_head_node = head_block_buff[depth_index-1] + head_depth_index[depth_index-1] - 1;
					if (!pre_head_node->head_offset) {
						//如果没有被链接，则把父节点指向当前head
						for(j=0;j<depth_index;j++)
							this_offset += head_depth_count[j];

						this_offset *= sizeof(struct head_node);
						this_offset += head_depth_index[depth_index] * sizeof(struct head_node);
						pre_head_node->head_offset = this_offset;
						//printf("depth %d offset=0x%x\n", depth_index, pre_head_node->head_offset);
					}
					//父节点拥有的子节点个数+1
					pre_head_node->head_count ++;
				}
				//当前深度的head个数+1
				head_depth_index[depth_index] ++;

				break;
			}
			case FDT_END_NODE:
				node_pt += FDT_TAGSIZE;
				//遇到end结束符，返回上一个level
				depth_index --;

			//	printf("depth_index=%d\n", depth_index);

				break;
			case FDT_PROP:
			{
				struct fdt_property *fdt_prop = (struct fdt_property *)node_pt;
				struct head_node *pre_head_node;
				char *prop_name_base;

				prop = prop_node_block + prop_index;
				prop->name_offset = cpu_to_be32(fdt_prop->nameoff);
				prop_name_base = buffer + prop->name_offset + cpu_to_be32(fdt->off_dt_strings);
				//printf("prop name=%s\n", prop_name_base);
				__dtb_decode_prop_node_name(&(prop->name_sum), &(prop->name_bytes), prop_name_base);
				prop->offset = node_pt - struct_base;
				//printf("prop->name_offset=%d\n", prop->name_offset);
				//printf("depth_index=%d\n", depth_index);
				//printf("head_depth_index[%d]=%d\n", depth_index, head_depth_index[depth_index]);
				if (depth_index > 0) {
					pre_head_node = head_block_buff[depth_index] + head_depth_index[depth_index] - 1;

					//printf("prop addr=0x%x\n", (uint32)pre_head_node);
					if (!pre_head_node->data_offset) {
						//如果没有被链接，则把父节点指向当前head
						//pre_head_node->data_offset = (uint32)((unsigned long)prop - (unsigned long)prop_node_block);
						pre_head_node->data_offset = head_block_offset + prop_index * sizeof(struct prop_node);
						//printf("pre_head_node->data_offset=0x%x\n", pre_head_node->data_offset);
					}
					//父节点拥有的子节点个数+1
					pre_head_node->data_count ++;
					//printf("pre_head_node->data_count=%d\n", pre_head_node->data_count);
				}

				node_pt += FDT_TAGSIZE + 2 * sizeof(uint32) + cpu_to_be32(fdt_prop->len);
				node_pt = (char *)(((unsigned long)node_pt + 3) & ~3);

				prop_index ++;

				break;
			}
			case FDT_NOP:
				node_pt += FDT_TAGSIZE;

				break;
			default:
				goto __dtb_decode_err1;
		}

		node_type = cpu_to_be32(*(uint *)node_pt);
	}
#if 0
	//扫描head表，统计出重复名称的head
	for(i=1;head_depth_count[i] > 0;i++) {
		printf("depth %d count = %d\n", i, head_depth_count[i]);
		for (j=0;j<head_depth_count[i]-1;j++) {
			head = head_block_buff[i] + j;
			if (head->repeate_count == 0) {
				for (k=j+1;k<head_depth_count[i];k++) {
					next_head = head_block_buff[i] + k;
					if (next_head->repeate_count == 0) {
						if( !memcmp(buffer + head->name_offset + cpu_to_be32(fdt->off_dt_struct),
									buffer + next_head->name_offset + cpu_to_be32(fdt->off_dt_struct),
									head->name_bytes_short)) {
							head->repeate_count = 1;
							next_head->repeate_count = 1;

							continue;
						}
					}
				}
			}
		}
	}
#endif
//	total_head_count = 0;
//	for(i=0;head_depth_count[i] > 0;i++)
//		total_head_count += head_depth_count[i];
//
//	for(i=0;i<prop_count;i++) {
//		prop = prop_node_block + i;
//		prop->
//	}


	memset(&fast_header, 0, sizeof(fast_header));
	fast_header.magic = FDT_MAGIC;
	fast_header.off_head = sizeof(struct dtbfast_header);
	for(i=0;head_depth_count[i] > 0;i++)
		fast_header.off_head += head_depth_count[i];

	fast_header.level0_count = head_depth_count[0];
	fast_header.off_prop = fast_header.off_head + fast_header.off_head * sizeof(struct head_node);
	fast_header.prop_count = prop_count;

	ret = __dtbfast_store(head_block_buff, head_depth_count, prop_node_block, &fast_header, buffer, len);

__dtb_decode_err1:
	if (prop_node_block)
		free(prop_node_block);

__dtb_decode_err2:
	for(i=0;head_depth_count[i] > 0;i++)
		if (head_block_buff[i])
			free(head_block_buff[i]);

	return ret;
}


//返回对应的name的head node的偏移量
//非0表示找到，0值表示找不到
uint __dtbfast_search_head(char *head_base, char *dtb_base, char *head_buff, char *name)
{
	struct head_node *head = NULL, *next_head;
	char *tmp_name = name, *next_end_char;
	uint  full_string_sum = 0, full_string_len = 0;
	uint  at_flag = 0;
	int i;
	char *test_buffer;

	//取出首节点
	head = (struct head_node *)head_buff;
	test_buffer = (char *)head;

	//跳过name的/
	while(*tmp_name == '/')
		tmp_name ++;
	//找到下一个/，或者空字符
	next_end_char = tmp_name;
	while(1) {
		if (*next_end_char == '/')
			break;
		if (*next_end_char == '\0')
			break;
		if (*next_end_char == '@')
			at_flag = 1;

		full_string_sum += *next_end_char;
		full_string_len ++;

		next_end_char   ++;
	}

	//printf("string_sum=%d\n", full_string_sum);
	for(i=0;i<head->head_count;i++) {

		next_head = (struct head_node *)(head->head_offset + head_base) + i;

		//printf("next_head->name_offset=0x%x\n", next_head->name_offset);
		//比较内存内容
		//printf("offset=0x%x\n", (uint)((unsigned long)next_head - (unsigned long)head_base));
		//printf("i=%d\n", i);
		if (!memcmp(tmp_name, next_head->name_offset + dtb_base, full_string_len)) {
			//完整字符串相同，检查节点是不是重复出现
			if ((next_head->repeate_count) && (at_flag))
				continue;		//节点名字重复出现，丢弃
			//节点没有重复出现，继续处理
			if (*next_end_char != '\0')
				return __dtbfast_search_head(head_base, dtb_base, (char *)next_head, next_end_char);
			else
				return (uint)((unsigned long)next_head - (unsigned long)head_base);
		}
	}

	return 0;

}


struct fdt_property *__dtbfast_search_prop(char *dtbfast_buffer, char *fdt_buffer, uint32 offset, char *name)
{
	struct fdt_header *fdt = (struct fdt_header *)fdt_buffer;
	struct prop_node *node;
	struct head_node *head;

	char *tmp_name = name;
	uint32  string_sum, i;
	char *fdt_string_base, *fdt_string_off;

	head = (struct head_node *)(dtbfast_buffer + offset + sizeof(struct dtbfast_header));
	while(*tmp_name)
		string_sum += *tmp_name++;

	fdt_string_base = fdt_buffer + cpu_to_be32(fdt->off_dt_strings);
	for(i=0;i<head->data_count;i++) {
		node = (struct prop_node *)(dtbfast_buffer + head->data_offset + sizeof(struct dtbfast_header)) + i;
		fdt_string_off = fdt_string_base + node->name_offset;
		if (string_sum == node->name_sum) {
			if (!strcmp(name, fdt_string_off)) {
				return (struct fdt_property *)(fdt_buffer + cpu_to_be32(fdt->off_dt_struct) + node->offset);
			}
		}
	}

	return NULL;

}

uint dtbfast_path_offset_search(char *dtbfast_buffer, char *dtb_buffer, char *name)
{
	uint32 base_header_size = sizeof(struct dtbfast_header);
	struct fdt_header *fdt;
	uint32 offset;

	fdt = (struct fdt_header *)dtb_buffer;
	if (name == NULL)
		return 0;

	//如果不是以"/"开始，表示这是一个aliase，需要从aliase节点中找
	if (name[0] != '/') {
		struct fdt_property *prop_node;
		struct fdt_header *fdt = (struct fdt_header *)dtb_buffer;

		offset = __dtbfast_search_head(dtbfast_buffer + base_header_size,
										dtb_buffer + cpu_to_be32(fdt->off_dt_struct),
										dtbfast_buffer + base_header_size, "/aliases");
		if (offset == 0) {
			printf("dtbfast path offset err: cant find aliases\n");

			return 0;
		}

		prop_node = __dtbfast_search_prop(dtbfast_buffer, dtb_buffer, offset, name);

		return __dtbfast_search_head(dtbfast_buffer + base_header_size,
										dtb_buffer + cpu_to_be32(fdt->off_dt_struct),
										dtbfast_buffer + base_header_size,
										prop_node->data);
	}
	else
		return __dtbfast_search_head(dtbfast_buffer + base_header_size, dtb_buffer +
										cpu_to_be32(fdt->off_dt_struct),
										dtbfast_buffer + base_header_size, name);
}

int dtbfast_path_test(char *fdt_buffer)
{
	char dtbfastpath[MAX_PATH]="";
	char *dtbfast_buffer;
	int  len;
	uint  offset;
	struct fdt_property *prop;

	GetFullPath(dtbfastpath, "test.bin");
	printf("dtbpath=%s\n", dtbfastpath);

	dtbfast_buffer = __probe_file_data(dtbfastpath, &len);
	if (dtbfast_buffer == NULL) {

		return -1;
	}

	offset = dtbfast_path_offset_search(dtbfast_buffer, fdt_buffer, "/soc/twi_para");

	printf("offset=0x%x\n", offset);
	prop = __dtbfast_search_prop(dtbfast_buffer, fdt_buffer, offset, "pinctrl-0");
	if (prop != NULL) {
		uint32 *addr = prop->data;

		printf("prop value = 0x%x\n", cpu_to_be32(*addr));
	} else {
		printf("failed\n");
	}

	offset = dtbfast_path_offset_search(dtbfast_buffer, fdt_buffer, "charger0");

	printf("offset=0x%x\n", offset);
	prop = __dtbfast_search_prop(dtbfast_buffer, fdt_buffer, offset, "pmu_bat_para21");
	if (prop != NULL) {
		uint32 *addr = prop->data;

		printf("prop value = 0x%x\n", cpu_to_be32(*addr));
	} else {
		printf("failed\n");
	}


	if (dtbfast_buffer)
		free(dtbfast_buffer);

	return 0;
}
