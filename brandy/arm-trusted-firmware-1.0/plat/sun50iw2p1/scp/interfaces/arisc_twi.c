/*
 *  drivers/arisc/interfaces/arisc_twi.c
 *
 * Copyright (c) 2013 Allwinner.
 * 2013-07-01 Written by superm (superm@allwinnertech.com).
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../arisc_i.h"

/**
 * twi read block data.
 * @cfg:    point of arisc_twi_block_cfg struct;
 *
 * return: result, 0 - read register successed,
 *                !0 - read register failed or the len more then max len;
 */
int arisc_twi_read_block_data(uint32_t *paras)
{
	int                   result;
	struct arisc_message *pmessage;

	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);

	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_TWI_READ_BLOCK_DATA;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	memcpy((void *)pmessage->paras, (const void *)paras, sizeof(pmessage->paras));

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	memcpy((void *)paras, (const void *)pmessage->paras, sizeof(pmessage->paras));

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}


/**
 * twi write block data.
 * @cfg:    point of arisc_twi_block_cfg struct;
 *
 * return: result, 0 - write register successed,
 *                !0 - write register failedor the len more then max len;
 */
int arisc_twi_write_block_data(uint32_t *paras)
{
	int                   result;
	struct arisc_message *pmessage;

	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);

	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	/* initialize message */
	pmessage->type       = ARISC_TWI_WRITE_BLOCK_DATA;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	memcpy((void *)pmessage->paras, (const void *)paras, sizeof(pmessage->paras));

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}


/**
 * twi read pmu reg.
 * @addr:  pmu reg addr;
 *
 * return: if read pmu reg successed, return data of pmu reg;
 *         if read pmu reg failed, return -1.
 */
uint8_t arisc_twi_read_pmu_reg(uint32_t addr)
{
	int result;
	uint32_t paras[22];
	uint32_t data = 0;

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]|para[1]|para[2]|para[3]|para[4]|
	 * |len    |addr0~3|addr4~7|data0~3|data4~7|
	 */
	memset((void *)paras, 0, sizeof(uint32_t) * 6);
	paras[0] = 1;
	paras[1] = addr&0xff;

	result = arisc_twi_read_block_data(paras);
	if (!result) {
		data = paras[3];
	} else {
		ARISC_ERR("arisc twi read pmu reg 0x%x err\n", addr);
		return -1;
	}

	ARISC_INF("read pmu reg 0x%x:0x%x\n", addr, data);

	return data;
}


/**
 * twi write pmu reg.
 * @addr: pmu reg addr;
 * @data: pmu reg data;
 *
 * return: result, 0 - write register successed,
 *                !0 - write register failedor the len more then max len;
 */
int arisc_twi_write_pmu_reg(uint32_t addr, uint32_t data)
{
	int result;
	uint32_t paras[22];

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]|para[1]|para[2]|para[3]|para[4]|
	 * |len    |addr0~3|addr4~7|data0~3|data4~7|
	 */
	memset((void *)paras, 0, sizeof(uint32_t) * 6);
	paras[0] = 1;
	paras[1] = addr&0xff;
	paras[3] = data&0xff;

	result = arisc_twi_write_block_data(paras);
	if (result) {
		ARISC_ERR("arisc twi write pmu reg 0x%x:0x%x err\n", addr, data);
	}
	ARISC_INF("write pmu reg 0x%x:0x%x\n", addr, data);

	return result;
}


/**
 * twi bits operation sync.
 * @cfg:    point of arisc_twi_bits_cfg struct;
 *
 * return: result, 0 - bits operation successed,
 *                !0 - bits operation failed, or the len more then max len;
 *
 * twi clear bits internal:
 * data = twi_read(regaddr);
 * data = data & (~mask);
 * twi_write(regaddr, data);
 *
 * twi set bits internal:
 * data = twi_read(addr);
 * data = data | mask;
 * twi_write(addr, data);
 *
 */
int twi_bits_ops_sync(uint32_t *paras)
{
	int                   result;
	struct arisc_message *pmessage;

	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);

	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	/* initialize message */
	pmessage->type       = ARISC_TWI_BITS_OPS_SYNC;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	memcpy((void *)pmessage->paras, (const void *)paras, sizeof(pmessage->paras));

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
