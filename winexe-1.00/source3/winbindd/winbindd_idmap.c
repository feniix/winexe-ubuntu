/*
   Unix SMB/CIFS implementation.

   Async helpers for blocking functions

   Copyright (C) Volker Lendecke 2005
   Copyright (C) Gerald Carter 2006
   Copyright (C) Simo Sorce 2007

   The helpers always consist of three functions:

   * A request setup function that takes the necessary parameters together
     with a continuation function that is to be called upon completion

   * A private continuation function that is internal only. This is to be
     called by the lower-level functions in do_async(). Its only task is to
     properly call the continuation function named above.

   * A worker function that is called inside the appropriate child process.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "includes.h"
#include "winbindd.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

static struct winbindd_child static_idmap_child;

struct winbindd_child *idmap_child(void)
{
	return &static_idmap_child;
}

static void winbindd_sid2uid_recv(TALLOC_CTX *mem_ctx, bool success,
			       struct winbindd_response *response,
			       void *c, void *private_data)
{
	void (*cont)(void *priv, bool succ, uid_t uid) =
		(void (*)(void *, bool, uid_t))c;

	if (!success) {
		DEBUG(5, ("Could not trigger sid2uid\n"));
		cont(private_data, False, 0);
		return;
	}

	if (response->result != WINBINDD_OK) {
		DEBUG(5, ("sid2uid returned an error\n"));
		cont(private_data, False, 0);
		return;
	}

	cont(private_data, True, response->data.uid);
}

void winbindd_sid2uid_async(TALLOC_CTX *mem_ctx, const DOM_SID *sid,
			 void (*cont)(void *private_data, bool success, uid_t uid),
			 void *private_data)
{
	struct winbindd_request request;
	struct winbindd_domain *domain;

	ZERO_STRUCT(request);
	request.cmd = WINBINDD_DUAL_SID2UID;

	domain = find_domain_from_sid(sid);

	if (domain != NULL) {
		DEBUG(10, ("winbindd_sid2uid_async found domain %s, "
			   "have_idmap_config = %d\n", domain->name,
			   (int)domain->have_idmap_config));

	}
	else {
		DEBUG(10, ("winbindd_sid2uid_async did not find a domain for "
			   "%s\n", sid_string_dbg(sid)));
	}

	if ((domain != NULL) && (domain->have_idmap_config)) {
		fstrcpy(request.domain_name, domain->name);
	}

	sid_to_fstring(request.data.dual_sid2id.sid, sid);
	do_async(mem_ctx, idmap_child(), &request, winbindd_sid2uid_recv,
		 (void *)cont, private_data);
}

enum winbindd_result winbindd_dual_sid2uid(struct winbindd_domain *domain,
					   struct winbindd_cli_state *state)
{
	DOM_SID sid;
	NTSTATUS result;

	DEBUG(3, ("[%5lu]: sid to uid %s\n", (unsigned long)state->pid,
		  state->request->data.dual_sid2id.sid));

	if (!string_to_sid(&sid, state->request->data.dual_sid2id.sid)) {
		DEBUG(1, ("Could not get convert sid %s from string\n",
			  state->request->data.dual_sid2id.sid));
		return WINBINDD_ERROR;
	}

	result = idmap_sid_to_uid(state->request->domain_name, &sid,
				  &state->response->data.uid);

	DEBUG(10, ("winbindd_dual_sid2uid: 0x%08x - %s - %u\n",
		   NT_STATUS_V(result), sid_string_dbg(&sid),
		   (unsigned int)state->response->data.uid));

	return NT_STATUS_IS_OK(result) ? WINBINDD_OK : WINBINDD_ERROR;
}

static void winbindd_sid2gid_recv(TALLOC_CTX *mem_ctx, bool success,
			       struct winbindd_response *response,
			       void *c, void *private_data)
{
	void (*cont)(void *priv, bool succ, gid_t gid) =
		(void (*)(void *, bool, gid_t))c;

	if (!success) {
		DEBUG(5, ("Could not trigger sid2gid\n"));
		cont(private_data, False, 0);
		return;
	}

	if (response->result != WINBINDD_OK) {
		DEBUG(5, ("sid2gid returned an error\n"));
		cont(private_data, False, 0);
		return;
	}

	cont(private_data, True, response->data.gid);
}

void winbindd_sid2gid_async(TALLOC_CTX *mem_ctx, const DOM_SID *sid,
			 void (*cont)(void *private_data, bool success, gid_t gid),
			 void *private_data)
{
	struct winbindd_request request;
	struct winbindd_domain *domain;

	ZERO_STRUCT(request);
	request.cmd = WINBINDD_DUAL_SID2GID;

	domain = find_domain_from_sid(sid);
	if ((domain != NULL) && (domain->have_idmap_config)) {
		fstrcpy(request.domain_name, domain->name);
	}

	sid_to_fstring(request.data.dual_sid2id.sid, sid);

	DEBUG(7,("winbindd_sid2gid_async: Resolving %s to a gid\n",
		request.data.dual_sid2id.sid));

	do_async(mem_ctx, idmap_child(), &request, winbindd_sid2gid_recv,
		 (void *)cont, private_data);
}

enum winbindd_result winbindd_dual_sid2gid(struct winbindd_domain *domain,
					   struct winbindd_cli_state *state)
{
	DOM_SID sid;
	NTSTATUS result;

	DEBUG(3, ("[%5lu]: sid to gid %s\n", (unsigned long)state->pid,
		  state->request->data.dual_sid2id.sid));

	if (!string_to_sid(&sid, state->request->data.dual_sid2id.sid)) {
		DEBUG(1, ("Could not get convert sid %s from string\n",
			  state->request->data.dual_sid2id.sid));
		return WINBINDD_ERROR;
	}

	/* Find gid for this sid and return it, possibly ask the slow remote idmap */

	result = idmap_sid_to_gid(state->request->domain_name, &sid,
				  &state->response->data.gid);

	DEBUG(10, ("winbindd_dual_sid2gid: 0x%08x - %s - %u\n",
		   NT_STATUS_V(result), sid_string_dbg(&sid),
		   (unsigned int)state->response->data.gid));

	return NT_STATUS_IS_OK(result) ? WINBINDD_OK : WINBINDD_ERROR;
}

/* The following uid2sid/gid2sid functions has been contributed by
 * Keith Reynolds <Keith.Reynolds@centrify.com> */

static void winbindd_uid2sid_recv(TALLOC_CTX *mem_ctx, bool success,
				  struct winbindd_response *response,
				  void *c, void *private_data)
{
	void (*cont)(void *priv, bool succ, const char *sid) =
		(void (*)(void *, bool, const char *))c;

	if (!success) {
		DEBUG(5, ("Could not trigger uid2sid\n"));
		cont(private_data, False, NULL);
		return;
	}

	if (response->result != WINBINDD_OK) {
		DEBUG(5, ("uid2sid returned an error\n"));
		cont(private_data, False, NULL);
		return;
	}

	cont(private_data, True, response->data.sid.sid);
}

void winbindd_uid2sid_async(TALLOC_CTX *mem_ctx, uid_t uid,
			    void (*cont)(void *private_data, bool success, const char *sid),
			    void *private_data)
{
	struct winbindd_domain *domain;
	struct winbindd_request request;

	ZERO_STRUCT(request);
	request.cmd = WINBINDD_DUAL_UID2SID;
	request.data.uid = uid;

	for (domain = domain_list(); domain != NULL; domain = domain->next) {
		if (domain->have_idmap_config
		    && (uid >= domain->id_range_low)
		    && (uid <= domain->id_range_high)) {
			fstrcpy(request.domain_name, domain->name);
		}
	}

	do_async(mem_ctx, idmap_child(), &request, winbindd_uid2sid_recv,
		 (void *)cont, private_data);
}

enum winbindd_result winbindd_dual_uid2sid(struct winbindd_domain *domain,
					   struct winbindd_cli_state *state)
{
	DOM_SID sid;
	NTSTATUS result;

	DEBUG(3,("[%5lu]: uid to sid %lu\n",
		 (unsigned long)state->pid,
		 (unsigned long) state->request->data.uid));

	/* Find sid for this uid and return it, possibly ask the slow remote idmap */
	result = idmap_uid_to_sid(state->request->domain_name, &sid,
				  state->request->data.uid);

	if (NT_STATUS_IS_OK(result)) {
		sid_to_fstring(state->response->data.sid.sid, &sid);
		state->response->data.sid.type = SID_NAME_USER;
		return WINBINDD_OK;
	}

	return WINBINDD_ERROR;
}

static void winbindd_gid2sid_recv(TALLOC_CTX *mem_ctx, bool success,
				  struct winbindd_response *response,
				  void *c, void *private_data)
{
	void (*cont)(void *priv, bool succ, const char *sid) =
		(void (*)(void *, bool, const char *))c;

	if (!success) {
		DEBUG(5, ("Could not trigger gid2sid\n"));
		cont(private_data, False, NULL);
		return;
	}

	if (response->result != WINBINDD_OK) {
		DEBUG(5, ("gid2sid returned an error\n"));
		cont(private_data, False, NULL);
		return;
	}

	cont(private_data, True, response->data.sid.sid);
}

void winbindd_gid2sid_async(TALLOC_CTX *mem_ctx, gid_t gid,
			    void (*cont)(void *private_data, bool success, const char *sid),
			    void *private_data)
{
	struct winbindd_domain *domain;
	struct winbindd_request request;

	ZERO_STRUCT(request);
	request.cmd = WINBINDD_DUAL_GID2SID;
	request.data.gid = gid;

	for (domain = domain_list(); domain != NULL; domain = domain->next) {
		if (domain->have_idmap_config
		    && (gid >= domain->id_range_low)
		    && (gid <= domain->id_range_high)) {
			fstrcpy(request.domain_name, domain->name);
		}
	}

	do_async(mem_ctx, idmap_child(), &request, winbindd_gid2sid_recv,
		 (void *)cont, private_data);
}

enum winbindd_result winbindd_dual_gid2sid(struct winbindd_domain *domain,
					   struct winbindd_cli_state *state)
{
	DOM_SID sid;
	NTSTATUS result;

	DEBUG(3,("[%5lu]: gid %lu to sid\n",
		(unsigned long)state->pid,
		(unsigned long) state->request->data.gid));

	/* Find sid for this gid and return it, possibly ask the slow remote idmap */
	result = idmap_gid_to_sid(state->request->domain_name, &sid,
				  state->request->data.gid);

	if (NT_STATUS_IS_OK(result)) {
		sid_to_fstring(state->response->data.sid.sid, &sid);
		DEBUG(10, ("[%5lu]: retrieved sid: %s\n",
			   (unsigned long)state->pid,
			   state->response->data.sid.sid));
		state->response->data.sid.type = SID_NAME_DOM_GRP;
		return WINBINDD_OK;
	}

	return WINBINDD_ERROR;
}

static const struct winbindd_child_dispatch_table idmap_dispatch_table[] = {
	{
		.name		= "PING",
		.struct_cmd	= WINBINDD_PING,
		.struct_fn	= winbindd_dual_ping,
	},{
		.name		= "DUAL_SID2UID",
		.struct_cmd	= WINBINDD_DUAL_SID2UID,
		.struct_fn	= winbindd_dual_sid2uid,
	},{
		.name		= "DUAL_SID2GID",
		.struct_cmd	= WINBINDD_DUAL_SID2GID,
		.struct_fn	= winbindd_dual_sid2gid,
	},{
		.name		= "DUAL_UID2SID",
		.struct_cmd	= WINBINDD_DUAL_UID2SID,
		.struct_fn	= winbindd_dual_uid2sid,
	},{
		.name		= "DUAL_GID2SID",
		.struct_cmd	= WINBINDD_DUAL_GID2SID,
		.struct_fn	= winbindd_dual_gid2sid,
	},{
		.name		= "NDRCMD",
		.struct_cmd	= WINBINDD_DUAL_NDRCMD,
		.struct_fn	= winbindd_dual_ndrcmd,
	},{
		.name		= NULL,
	}
};

void init_idmap_child(void)
{
	setup_child(NULL, &static_idmap_child,
		    idmap_dispatch_table,
		    "log.winbindd", "idmap");
}
