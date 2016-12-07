#ifndef __DTBFAST_H_
#define __DTBFAST_H_

#include <include.h>


#define  DTBFAST_HEAD_MAX_DEPTH       (16)


struct dtbfast_header {
	uint32 magic;			 /* magic word FDT_MAGIC */
	uint32 totalsize;		 /* total file size */
	uint32 level0_count;	 /* the count of level0 head */
	uint32 off_head;    	 /* offset to head */
	uint32 head_count;		 /* total head */
	uint32 off_prop;		 /* offset to prop */
	uint32 prop_count;		 /* total prop */
	uint32 reserved[9];

};


struct head_node {
	uint32  name_sum;		//完整名称的每个字符的累加和
	uint32  name_sum_short; //不计算@之后字符的累加和
	uint32  name_offset;	//具体名称的偏移量，从dtb中寻找
	uint32  name_bytes;		//完整名称的长度
	uint32  name_bytes_short;//@之前名称的长度
	uint32  repeate_count;
	uint32  head_offset;	//指向第一个head的offset
	uint32  head_count;		//head总的个数
	uint32  data_offset;	//指向第一个prop的offset
	uint32  data_count;		//prop总的个数
	uint32  reserved[2];
};

struct prop_node {
	uint32  name_sum;		//名称的每个字符的累加和
	uint32  name_offset;	//具体名称的偏移量，dtb中寻找
	uint32  name_bytes;		//具体名称的长度
	uint32  offset;			//具体prop的偏移量，dtb中寻找
};


#endif /* __DTBFAST_H_ */
