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
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>

__asm__(".symver memcpy ,memcpy@GLIBC_2.2.5");

#define   SYSCONFIG_ALIGN_SIZE      (16 * 1024)
#define   UBOOT_MAX_SIZE            (2 * 1024 * 1024)

static int align_uboot(char *uboot_mem, char *multi_config_head, int index, char *config_full_name, char *config_file_name);

static void usage(void)
{
	printf("Usage: update_uboot_v2  arg1 arg2\n");
	printf("arg1: the name of uboot\n");
	printf("arg2: the name of the config directory\n");

	return;
}

/*
static int dir_list( char *path, char *uboot_buff)
{
    struct dirent* ent = NULL;
    DIR     *pDir;
    char    dir[1024];
    struct  stat   statbuf;

    if((pDir=opendir(path))==NULL)
    {
        printf("Cannot open directory: %s\n", path);

        return -1;
    }

    while(  (ent=readdir(pDir))!=NULL  )
    {
        //得到读取文件的绝对路径名
        memset(dir, 0, 1024);
        sprintf( dir, "%s/%s", path, ent->d_name );
        //得到文件信息
        lstat(dir, &statbuf);
        //判断是目录还是文件
        printf("full name: %s\n", dir);
        if( S_ISDIR(statbuf.st_mode) )
        {
            //排除当前目录和上级目录
            continue;
        }
        else
        {
            int len = strlen(dir) - 1;
            char *ext = dir + len;

            while ((*ext != '.') && (*ext != '/') && (len > 0))
            {
            	ext --;
            	len --;
            }
            if(!strncmp(ext, ".fex", 4))
            {
            	char cmdline[1024];
            	unsigned offset;

            	memset(cmdline, 0, 1024);
				sprintf(cmdline, "busybox unix2dos %s", dir);
				system(cmdline);

				memset(cmdline, 0, 1024);
				sprintf(cmdline, "script %s", dir);
				printf("cmdline=%s\n", cmdline);

				if(system(cmdline) == -1)
				{
					printf("script %s failed\n", dir);
					closedir(pDir);

					return -1;
				}

				offset = ext - dir + 1;
				dir[offset + 0] = 'b';
				dir[offset + 1] = 'i';
				dir[offset + 2] = 'n';
				if(align_uboot(uboot_buff, dir, ent->d_name))
				{
					closedir(pDir);

					return -1;
				}
            	printf("full name: %s\n", dir);
            }
        }
    }//while
    closedir(pDir);

	return 0;
}
*/
int prepare_uboot(char *uboot_mem, unsigned uboot_len)
{
	unsigned   total_length, uboot_source_len;
	struct spare_boot_ctrl_head   *head;

	//读取原始uboot
	head = (struct spare_boot_ctrl_head *)uboot_mem;
	uboot_source_len = head->uboot_length;

	printf("uboot source = %d\n", uboot_source_len);
	printf("uboot input = %d\n", uboot_len);
	if(!uboot_source_len)
	{
		uboot_source_len = uboot_len;
	}

	printf("uboot source = %d\n", uboot_source_len);
	printf("uboot align = %d\n", head->align_size);
	if(uboot_source_len & (head->align_size - 1))
	{
		printf("uboot align = 0x%x\n", ~(head->align_size - 1));
		total_length = (uboot_source_len + head->align_size) & (~(head->align_size - 1));
	}
	else
	{
		total_length = uboot_source_len;
	}
	printf("uboot total = %d\n", total_length);

	head->uboot_length = total_length;
	head->length = total_length;

	head->check_sum = sunxi_sprite_generate_checksum(uboot_mem, head->length, head->check_sum);

	return 0;
}

int prepare_script_head(char *uboot_mem, unsigned uboot_len, char * script_buffer)
{
	struct spare_boot_ctrl_head   *head;
	unsigned  script_len, script_head_len;

	multi_script_head *script_head =  (multi_script_head *)script_buffer;

	//读取原始uboot
	head = (struct spare_boot_ctrl_head *)uboot_mem;

	if(uboot_len != head->length)
	{
		printf("input length=%d is not equre source=%d\n", uboot_len, head->length);
		return -1;
	}
	printf("uboot total = %d\n", uboot_len);
	printf("uboot source = %d\n", head->uboot_length);

	script_len = sizeof(multi_script_head);

	if(script_len & (SYSCONFIG_ALIGN_SIZE - 1))
	{
		script_head_len = (script_len + SYSCONFIG_ALIGN_SIZE) & (~(SYSCONFIG_ALIGN_SIZE - 1));
	}
	else
	{
		script_head_len = script_len;
	}
	script_head->config_length = script_head_len;
	script_head->length = script_head_len;

	memcpy(uboot_mem + uboot_len,  (char *)script_head, script_len);

	head->length = uboot_len + script_head_len;
	printf("uboot total = %d\n", head->length);

	head->check_sum = sunxi_sprite_generate_checksum(uboot_mem, head->length, head->check_sum);

	return 0;
}

static int align_uboot(char *uboot_mem, char *multi_config_head, int index, char *config_full_name, char *config_file_name)
{
	FILE      *config_file;
	unsigned  config_len, i;
	char      *config_mem;
	unsigned   config_total_len;
	struct spare_boot_ctrl_head   *head;
	script_head_t  *config_head;
	multi_script_head *multi_head;
	int    ret = -1;

	printf("config_full_name=%s\n", config_full_name);
	printf("filename=%s\n", config_file_name);

	config_file = fopen(config_full_name, "rb");
	if(config_file == NULL)
	{
		printf("update_uboot_v2 failed: cant open the config file %s\n", config_full_name);

		return -1;
	}
	fseek(config_file, 0, SEEK_END);
	config_len = ftell(config_file);
	rewind(config_file);

	config_mem = (char *)malloc(config_len);
	if(config_mem == NULL)
	{
		printf("update_uboot_v2 failed: cant malloc memory for config\n");

		fclose(config_file);
	}
	fread(config_mem, 1, config_len, config_file);
	fclose(config_file);

 	multi_head = (multi_script_head *)multi_config_head;

	//读取原始uboot
	head = (struct spare_boot_ctrl_head *)uboot_mem;
	//读取配置
	config_head = (script_head_t *)config_mem;

	printf("config1=%d\n", config_len);
	printf("mainkey=%d\n", config_head->main_key_count);
	printf("length=%d\n", config_head->length);
	if(config_len & (SYSCONFIG_ALIGN_SIZE - 1))
	{
		config_total_len = (config_len + SYSCONFIG_ALIGN_SIZE) & (~(SYSCONFIG_ALIGN_SIZE - 1));
	}
	else
	{
		config_total_len = config_len;
	}
	config_head->length = config_total_len;
	multi_head->length += config_total_len;

	multi_head->array[index].length = config_total_len;
	if(index == 0)
	{
		multi_head->array[index].addr = 0;
	}
	else 
	{
		multi_head->array[index].addr = multi_head->array[index - 1].addr + multi_head->array[index - 1].length;
	}

	memset(config_head->name, 0, 8);
	for(i=0;i<8;i++)
	{
		if(config_file_name[i]!='.')
		{
			config_head->name[i] = config_file_name[i];
		}
		else
		{
			break;
		}
	}

	printf("input total length=%d\n", head->length);
	memcpy(uboot_mem + head->length, config_mem, config_len);
	head->length +=config_total_len;

	printf("uboot source=%d\n", head->uboot_length);
	printf("config=%d\n", config_total_len);
	printf("total length=%d\n", head->length);

	head->check_sum = sunxi_sprite_generate_checksum(uboot_mem, head->length, head->check_sum);

	free(config_mem);

	ret = 0;

	return ret;
}

void *script_file_decode(char *script_file_name)
{
	FILE  *script_file;
	void  *script_buf = NULL;
	int script_length;
	//读取原始脚本
	script_file = fopen(script_file_name, "rb");
	if(!script_file)
	{
        printf("update error:unable to open script file\n");
		return NULL;
	}
    //获取原始脚本长度
    fseek(script_file, 0, SEEK_END);
	script_length = ftell(script_file);
	if(!script_length)
	{
		fclose(script_file);
		printf("the length of script is zero\n");

		return NULL;
	}
	//读取原始脚本
	script_buf = (char *)malloc(script_length);
	if(!script_buf)
	{
		fclose(script_file);
		printf("unable malloc memory for script\n");

		return NULL;;
	}
    fseek(script_file, 0, SEEK_SET);
	fread(script_buf, script_length, 1, script_file);
	fclose(script_file);

	return script_buf;
}

int prepare_config(char *buffer)
{
	multi_script_head	*multi_head = NULL;
	char value[128]={0};
	char table_name[16]={0};
	int table_count = 0;
	int i, j, ret;

	multi_head = (multi_script_head *)buffer;

	ret = script_parser_fetch("hardware_code_table", "table_count", &table_count);
	printf("table_count=%d\n", table_count);
	if(ret)
	{
		printf("cant get hardware_code_table, please check\n");
		return -1;
	}

	if(table_count == 0)
	{
		printf("no to use multi config\n");
		return 1;
	}

	multi_head->multi_count = table_count;
	multi_head->version = 0x01;
	strcpy(multi_head->magic, MULTI_CONFIG_MAGIC);

	for(i = 0; i < table_count; i++)
	{
		memset(table_name, 0, 16);
		memset(value, 0, 128);

		sprintf(table_name, "%s%d", "table_", i + 1);
		j = 0;
		ret = script_parser_fetch("hardware_code_table", table_name, (int *)value);
		if(ret)
		{
			printf("please check [hardware_code_table], table_name=%s is no exist\n", table_name);
			return -1;
		}
		while((value[j] != ':') && (value[j] != '\0'))
		{
			if(j > 127)
			{
				printf("please check [hardware_code_table], table_name%d=%s is to large\n", i, value);
				return -1;
			}
			j++;
		}

		multi_head->array[i].coding = atoi(value);
		strcpy(multi_head->array[i].name, &(value[j + 1]));
		multi_head->array[i].index = i;

		printf("coding=%d\n", multi_head->array[i].coding);
		printf("name=%s\n", multi_head->array[i].name);
	}
	return 0;
}

int multi_script_align(char *path, char *uboot_mem)
{
	int i;
	multi_script_head *multi_head;
	script_head_t *script_head;
	char * temp_head;
	struct spare_boot_ctrl_head *uboot_head;

	char filename[64];
	char dir[1024];
	char cmdline[1024];

	uboot_head = (struct spare_boot_ctrl_head *)uboot_mem;
	temp_head = (char *)uboot_mem + uboot_head->uboot_length;

	script_head = (script_head_t *)temp_head;

	temp_head = (char *)uboot_mem + uboot_head->uboot_length + script_head->length;
	multi_head = (multi_script_head *)temp_head;

	if(strcmp((multi_head->magic), MULTI_CONFIG_MAGIC))
	{
		printf("multi head magic %s err\n", multi_head->magic);
		return -1;
	}

	for(i = 0; i < multi_head->multi_count; i++)
	{
		memset(filename, 0, 64);
		sprintf(filename, "%s.fex", multi_head->array[i].name);
		
		memset(dir, 0, 1024);
		sprintf(dir, "%s/%s.fex", path, multi_head->array[i].name);
		printf("full name: %s\n", dir);
		memset(cmdline, 0, 1024);

		sprintf(cmdline, "busybox unix2dos %s", dir);
		system(cmdline);
		if(system(cmdline) == -1)
		{
			printf("busybox unix2dos %s failed\n", dir);
			return -1;
		}

		memset(cmdline, 0, 1024);
		sprintf(cmdline, "script %s", dir);
		printf("cmdline=%s\n", cmdline);
		if(system(cmdline) == -1)
		{
			printf("script %s failed\n", dir);
			return -1;
		}

		memset(dir, 0, 1024);
		sprintf(dir, "%s/%s.bin", path, multi_head->array[i].name);
		printf("dir=%s\n", dir);
		if(align_uboot(uboot_mem, (char *)multi_head, i, dir, filename))
		{
			printf("align uboot error\n");
			return -1;
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	char  config_fullpath[512];
	char  uboot_fullpath[512];
	char  script_file_name[MAX_PATH];
	char  multi_buffer[1024];

	FILE   *uboot_file = NULL;
	char  *uboot_mem = NULL;
	unsigned uboot_len;
	int ret, value[8];
	struct spare_boot_ctrl_head   *head = NULL;
	char   *script_buf = NULL;

	if(argc != 4)
	{
		usage();

		return -1;
	}

	memset(uboot_fullpath, 0, 512);
	memset(config_fullpath, 0, 512);
	memset(config_fullpath, 0, MAX_PATH);
	memset(multi_buffer, 0, 1024);

	//默认配置处理
	GetFullPath(script_file_name, argv[2]);
	printf("script file Path=%s\n", script_file_name);
	script_buf = (char *)script_file_decode(script_file_name);
	if(!script_buf)
	{
		printf("update uboot error: unable to get default script data\n");
		return -1;
	}
	script_parser_init(script_buf);
	ret = script_parser_fetch("hardware_version", "hid_used", value);
	if(ret || (value[0] != 1))
	{
		printf("not used muti config\n");
		if(script_buf)
		{
			free(script_buf);
		}
		return 1;
	}

	ret = prepare_config(multi_buffer); 
	if(ret)
	{
		return ret;
	}

	//uboot.fex 处理
	GetFullPath(uboot_fullpath, argv[1]);
	uboot_file = fopen(uboot_fullpath, "rb+");
	if(uboot_file == NULL)
	{
		printf("the uboot file cant be opend, please make sure %s exist\n", uboot_fullpath);

		return -1;
	}
	fseek(uboot_file, 0, SEEK_END);
	uboot_len = ftell(uboot_file);
	rewind(uboot_file);

	uboot_mem = (char *)malloc(UBOOT_MAX_SIZE);
	if(uboot_mem == NULL)
	{
		printf("update_uboot_v2 failed: cant malloc memory for uboot\n");
		fclose(uboot_file);
		return -1;
	}
	memset(uboot_mem, 0xff, UBOOT_MAX_SIZE);
	fread(uboot_mem, 1, uboot_len, uboot_file);
	//prepare_uboot(uboot_mem, uboot_len);
	if(prepare_script_head(uboot_mem, uboot_len, multi_buffer))
	{
		printf("prepare script head failed\n");

		return -1;
	}

	//多份配置处理
	GetFullPath(config_fullpath, argv[3]);
	if(!multi_script_align(config_fullpath, uboot_mem))
	{
		head = (struct spare_boot_ctrl_head *)uboot_mem;

		printf("dir_list ok\n");
		rewind(uboot_file);
		fwrite(uboot_mem, 1, head->length, uboot_file);
	}
	else
	{
		printf("add multi config failed\n");
		ret = -1;
	}

	free(uboot_mem);
	fclose(uboot_file);

	return ret;
}

