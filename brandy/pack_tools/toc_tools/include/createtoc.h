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

#ifndef __CREATE_TOC__H__
#define __CREATE_TOC__H__

#include "toc_config.h"

int splittoc0(char *toc0);
int splittoc1(char *toc1 );
int update_toc0_cert(toc_descriptor_t* toc0, char *toc0_name);

int createtoc0(toc_descriptor_t *toc1, char *toc1_name);
int createtoc1(toc_descriptor_t *toc1, char *toc1_name, int main_v, int sub_v);
int create_package(toc_descriptor_t *package, char *package_name);

#endif  //__CREATE_TOC__H__
