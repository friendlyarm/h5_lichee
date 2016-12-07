/*
 * Copyright (c) 2013, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <context_mgmt.h>
#include <debug.h>
#include <platform.h>
#include <tsp.h>
#include "tspd_private.h"
#include <head_data.h>

extern  struct spare_monitor_head  monitor_head;
extern  unsigned int     cpu_on_flag[8];
/*******************************************************************************
 * Export the platform specific power ops.
 ******************************************************************************/
void sunxi_tspd_cpu_on_init(void)
{
	int32_t rc = 0;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);
	tsp_context_t *tsp_ctx = &tspd_sp_context[linear_id];
	entry_point_info_t *tsp_on_entrypoint;

	if(!linear_id)
		return ;

	if(!monitor_head.secureos_base)
		return ;

	if(cpu_on_flag[linear_id])
		return ;

	cpu_on_flag[linear_id] = 1;

	assert(tsp_vectors);
	assert(get_tsp_pstate(tsp_ctx->state) == TSP_PSTATE_OFF);

	tsp_on_entrypoint = bl31_plat_get_next_image_ep_info(SECURE);

	/*
	 * If there's no valid entry point for SP, we return a non-zero value
	 * signalling failure initializing the service. We bail out without
	 * registering any handlers
	 */
	if (!tsp_on_entrypoint->pc)
		panic();

	tspd_init_tsp_ep_state(tsp_on_entrypoint,
				TSP_AARCH32,
				(uint64_t)&tsp_vectors->cpu_on_entry,
				tsp_ctx);

	/* Initialise this cpu's secure context */
	cm_init_context(mpidr, tsp_on_entrypoint);

	/* Enter the TSP */
	rc = tspd_synchronous_sp_entry(tsp_ctx);

	/*
	 * Read the response from the TSP. A non-zero return means that
	 * something went wrong while communicating with the SP.
	 */
	if (rc != 0)
		panic();

	/* Update its context to reflect the state the SP is in */
	set_tsp_pstate(tsp_ctx->state, TSP_PSTATE_ON);

	return ;
}

