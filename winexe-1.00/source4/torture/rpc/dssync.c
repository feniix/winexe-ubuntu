/* 
   Unix SMB/CIFS implementation.

   DsGetNCChanges replication test

   Copyright (C) Stefan (metze) Metzmacher 2005
   Copyright (C) Brad Henry 2005
   
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
#include "lib/cmdline/popt_common.h"
#include "librpc/gen_ndr/ndr_drsuapi_c.h"
#include "librpc/gen_ndr/ndr_drsblobs.h"
#include "libcli/cldap/cldap.h"
#include "torture/torture.h"
#include "../libcli/drsuapi/drsuapi.h"
#include "auth/gensec/gensec.h"
#include "param/param.h"
#include "dsdb/samdb/samdb.h"
#include "lib/ldb_wrap.h"
#include "torture/rpc/rpc.h"
#include "torture/drs/proto.h"


struct DsSyncBindInfo {
	struct dcerpc_pipe *drs_pipe;
	struct drsuapi_DsBind req;
	struct GUID bind_guid;
	struct drsuapi_DsBindInfoCtr our_bind_info_ctr;
	struct drsuapi_DsBindInfo28 our_bind_info28;
	struct drsuapi_DsBindInfo28 peer_bind_info28;
	struct policy_handle bind_handle;
};

struct DsSyncLDAPInfo {
	struct ldb_context *ldb;
};

struct DsSyncTest {
	struct dcerpc_binding *drsuapi_binding;
	
	const char *ldap_url;
	const char *site_name;
	
	const char *domain_dn;

	/* what we need to do as 'Administrator' */
	struct {
		struct cli_credentials *credentials;
		struct DsSyncBindInfo drsuapi;
		struct DsSyncLDAPInfo ldap;
	} admin;

	/* what we need to do as the new dc machine account */
	struct {
		struct cli_credentials *credentials;
		struct DsSyncBindInfo drsuapi;
		struct drsuapi_DsGetDCInfo2 dc_info2;
		struct GUID invocation_id;
		struct GUID object_guid;
	} new_dc;

	/* info about the old dc */
	struct {
		struct drsuapi_DsGetDomainControllerInfo dc_info;
	} old_dc;
};

static struct DsSyncTest *test_create_context(struct torture_context *tctx)
{
	NTSTATUS status;
	struct DsSyncTest *ctx;
	struct drsuapi_DsBindInfo28 *our_bind_info28;
	struct drsuapi_DsBindInfoCtr *our_bind_info_ctr;
	const char *binding = torture_setting_string(tctx, "binding", NULL);
	ctx = talloc_zero(tctx, struct DsSyncTest);
	if (!ctx) return NULL;

	status = dcerpc_parse_binding(ctx, binding, &ctx->drsuapi_binding);
	if (!NT_STATUS_IS_OK(status)) {
		printf("Bad binding string %s\n", binding);
		return NULL;
	}
	ctx->drsuapi_binding->flags |= DCERPC_SIGN | DCERPC_SEAL;

	ctx->ldap_url = talloc_asprintf(ctx, "ldap://%s", ctx->drsuapi_binding->host);

	/* ctx->admin ...*/
	ctx->admin.credentials				= cmdline_credentials;

	our_bind_info28				= &ctx->admin.drsuapi.our_bind_info28;
	our_bind_info28->supported_extensions	= 0xFFFFFFFF;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_ADDENTRYREPLY_V3;
	our_bind_info28->site_guid		= GUID_zero();
	our_bind_info28->pid			= 0;
	our_bind_info28->repl_epoch		= 1;

	our_bind_info_ctr			= &ctx->admin.drsuapi.our_bind_info_ctr;
	our_bind_info_ctr->length		= 28;
	our_bind_info_ctr->info.info28		= *our_bind_info28;

	GUID_from_string(DRSUAPI_DS_BIND_GUID, &ctx->admin.drsuapi.bind_guid);

	ctx->admin.drsuapi.req.in.bind_guid		= &ctx->admin.drsuapi.bind_guid;
	ctx->admin.drsuapi.req.in.bind_info		= our_bind_info_ctr;
	ctx->admin.drsuapi.req.out.bind_handle		= &ctx->admin.drsuapi.bind_handle;

	/* ctx->new_dc ...*/
	ctx->new_dc.credentials			= cmdline_credentials;

	our_bind_info28				= &ctx->new_dc.drsuapi.our_bind_info28;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_BASE;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_ASYNC_REPLICATION;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_REMOVEAPI;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_MOVEREQ_V2;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_GETCHG_COMPRESS;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_DCINFO_V1;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_RESTORE_USN_OPTIMIZATION;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_KCC_EXECUTE;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_ADDENTRY_V2;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_LINKED_VALUE_REPLICATION;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_DCINFO_V2;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_INSTANCE_TYPE_NOT_REQ_ON_MOD;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_CRYPTO_BIND;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_GET_REPL_INFO;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_STRONG_ENCRYPTION;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_DCINFO_V01;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_TRANSITIVE_MEMBERSHIP;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_ADD_SID_HISTORY;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_POST_BETA3;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_GET_MEMBERSHIPS2;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_GETCHGREQ_V6;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_NONDOMAIN_NCS;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_GETCHGREQ_V8;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_GETCHGREPLY_V5;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_GETCHGREPLY_V6;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_ADDENTRYREPLY_V3;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_GETCHGREPLY_V7;
	our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_VERIFY_OBJECT;
	if (lp_parm_bool(tctx->lp_ctx, NULL, "dssync", "xpress", false)) {
		our_bind_info28->supported_extensions	|= DRSUAPI_SUPPORTED_EXTENSION_XPRESS_COMPRESS;
	}
	our_bind_info28->site_guid		= GUID_zero();
	our_bind_info28->pid			= 0;
	our_bind_info28->repl_epoch		= 0;

	our_bind_info_ctr			= &ctx->new_dc.drsuapi.our_bind_info_ctr;
	our_bind_info_ctr->length		= 28;
	our_bind_info_ctr->info.info28		= *our_bind_info28;

	GUID_from_string(DRSUAPI_DS_BIND_GUID_W2K3, &ctx->new_dc.drsuapi.bind_guid);

	ctx->new_dc.drsuapi.req.in.bind_guid		= &ctx->new_dc.drsuapi.bind_guid;
	ctx->new_dc.drsuapi.req.in.bind_info		= our_bind_info_ctr;
	ctx->new_dc.drsuapi.req.out.bind_handle		= &ctx->new_dc.drsuapi.bind_handle;

	ctx->new_dc.invocation_id			= ctx->new_dc.drsuapi.bind_guid;

	/* ctx->old_dc ...*/

	return ctx;
}

static bool _test_DsBind(struct torture_context *tctx,
			 struct DsSyncTest *ctx, struct cli_credentials *credentials, struct DsSyncBindInfo *b)
{
	NTSTATUS status;
	bool ret = true;

	status = dcerpc_pipe_connect_b(ctx,
				       &b->drs_pipe, ctx->drsuapi_binding,
				       &ndr_table_drsuapi,
				       credentials, tctx->ev, tctx->lp_ctx);
	
	if (!NT_STATUS_IS_OK(status)) {
		printf("Failed to connect to server as a BDC: %s\n", nt_errstr(status));
		return false;
	}

	status = dcerpc_drsuapi_DsBind(b->drs_pipe, ctx, &b->req);
	if (!NT_STATUS_IS_OK(status)) {
		const char *errstr = nt_errstr(status);
		if (NT_STATUS_EQUAL(status, NT_STATUS_NET_WRITE_FAULT)) {
			errstr = dcerpc_errstr(ctx, b->drs_pipe->last_fault_code);
		}
		printf("dcerpc_drsuapi_DsBind failed - %s\n", errstr);
		ret = false;
	} else if (!W_ERROR_IS_OK(b->req.out.result)) {
		printf("DsBind failed - %s\n", win_errstr(b->req.out.result));
		ret = false;
	}

	ZERO_STRUCT(b->peer_bind_info28);
	if (b->req.out.bind_info) {
		switch (b->req.out.bind_info->length) {
		case 24: {
			struct drsuapi_DsBindInfo24 *info24;
			info24 = &b->req.out.bind_info->info.info24;
			b->peer_bind_info28.supported_extensions= info24->supported_extensions;
			b->peer_bind_info28.site_guid		= info24->site_guid;
			b->peer_bind_info28.pid			= info24->pid;
			b->peer_bind_info28.repl_epoch		= 0;
			break;
		}
		case 48: {
			struct drsuapi_DsBindInfo48 *info48;
			info48 = &b->req.out.bind_info->info.info48;
			b->peer_bind_info28.supported_extensions= info48->supported_extensions;
			b->peer_bind_info28.site_guid		= info48->site_guid;
			b->peer_bind_info28.pid			= info48->pid;
			b->peer_bind_info28.repl_epoch		= info48->repl_epoch;
			break;
		}
		case 28:
			b->peer_bind_info28 = b->req.out.bind_info->info.info28;
			break;
		default:
			printf("DsBind - warning: unknown BindInfo length: %u\n",
			       b->req.out.bind_info->length);
		}
	}

	return ret;
}

static bool test_LDAPBind(struct torture_context *tctx, struct DsSyncTest *ctx, 
			  struct cli_credentials *credentials, struct DsSyncLDAPInfo *l)
{
	bool ret = true;

	struct ldb_context *ldb;

	const char *modules_option[] = { "modules:paged_searches", NULL };
	ctx->admin.ldap.ldb = ldb = ldb_init(ctx, tctx->ev);
	if (ldb == NULL) {
		return false;
	}

	/* Despite us loading the schema from the AD server, we need
	 * the samba handlers to get the extended DN syntax stuff */
	ret = ldb_register_samba_handlers(ldb);
	if (ret == -1) {
		talloc_free(ldb);
		return NULL;
	}

	ldb_set_modules_dir(ldb,
			    talloc_asprintf(ldb,
					    "%s/ldb",
					    lp_modulesdir(tctx->lp_ctx)));

	if (ldb_set_opaque(ldb, "credentials", credentials)) {
		talloc_free(ldb);
		return NULL;
	}

	if (ldb_set_opaque(ldb, "loadparm", tctx->lp_ctx)) {
		talloc_free(ldb);
		return NULL;
	}

	ret = ldb_connect(ldb, ctx->ldap_url, 0, modules_option);
	if (ret != LDB_SUCCESS) {
		talloc_free(ldb);
		torture_assert_int_equal(tctx, ret, LDB_SUCCESS, "Failed to make LDB connection to target");
	}
	
	printf("connected to LDAP: %s\n", ctx->ldap_url);

	return true;
}

static bool test_GetInfo(struct torture_context *tctx, struct DsSyncTest *ctx)
{
	NTSTATUS status;
	struct drsuapi_DsCrackNames r;
	union drsuapi_DsNameRequest req;
	union drsuapi_DsNameCtr ctr;
	int32_t level_out = 0;
	struct drsuapi_DsNameString names[1];
	bool ret = true;
	struct cldap_socket *cldap;
	struct cldap_netlogon search;

	status = cldap_socket_init(ctx, NULL, NULL, NULL, &cldap);
	if (!NT_STATUS_IS_OK(status)) {
		printf("failed to setup cldap socket - %s\n",
			nt_errstr(status));
		return false;
	}

	r.in.bind_handle		= &ctx->admin.drsuapi.bind_handle;
	r.in.level			= 1;
	r.in.req			= &req;
	r.in.req->req1.codepage		= 1252; /* western european */
	r.in.req->req1.language		= 0x00000407; /* german */
	r.in.req->req1.count		= 1;
	r.in.req->req1.names		= names;
	r.in.req->req1.format_flags	= DRSUAPI_DS_NAME_FLAG_NO_FLAGS;
	r.in.req->req1.format_offered	= DRSUAPI_DS_NAME_FORMAT_NT4_ACCOUNT;
	r.in.req->req1.format_desired	= DRSUAPI_DS_NAME_FORMAT_FQDN_1779;
	names[0].str = talloc_asprintf(ctx, "%s\\", lp_workgroup(tctx->lp_ctx));

	r.out.level_out			= &level_out;
	r.out.ctr			= &ctr;

	status = dcerpc_drsuapi_DsCrackNames(ctx->admin.drsuapi.drs_pipe, ctx, &r);
	if (!NT_STATUS_IS_OK(status)) {
		const char *errstr = nt_errstr(status);
		if (NT_STATUS_EQUAL(status, NT_STATUS_NET_WRITE_FAULT)) {
			errstr = dcerpc_errstr(ctx, ctx->admin.drsuapi.drs_pipe->last_fault_code);
		}
		printf("dcerpc_drsuapi_DsCrackNames failed - %s\n", errstr);
		return false;
	} else if (!W_ERROR_IS_OK(r.out.result)) {
		printf("DsCrackNames failed - %s\n", win_errstr(r.out.result));
		return false;
	}

	ctx->domain_dn = r.out.ctr->ctr1->array[0].result_name;
	
	ZERO_STRUCT(search);
	search.in.dest_address = ctx->drsuapi_binding->host;
	search.in.dest_port = lp_cldap_port(tctx->lp_ctx);
	search.in.acct_control = -1;
	search.in.version		= NETLOGON_NT_VERSION_5 | NETLOGON_NT_VERSION_5EX;
	search.in.map_response = true;
	status = cldap_netlogon(cldap, lp_iconv_convenience(tctx->lp_ctx), ctx, &search);
	if (!NT_STATUS_IS_OK(status)) {
		const char *errstr = nt_errstr(status);
		ctx->site_name = talloc_asprintf(ctx, "%s", "Default-First-Site-Name");
		printf("cldap_netlogon() returned %s. Defaulting to Site-Name: %s\n", errstr, ctx->site_name);		
	} else {
		ctx->site_name = talloc_steal(ctx, search.out.netlogon.data.nt5_ex.client_site);
		printf("cldap_netlogon() returned Client Site-Name: %s.\n",ctx->site_name);
		printf("cldap_netlogon() returned Server Site-Name: %s.\n",search.out.netlogon.data.nt5_ex.server_site);
	}

	if (!ctx->domain_dn) {
		struct ldb_context *ldb = ldb_init(ctx, tctx->ev);
		struct ldb_dn *dn = samdb_dns_domain_to_dn(ldb, ctx, search.out.netlogon.data.nt5_ex.dns_domain);
		ctx->domain_dn = ldb_dn_alloc_linearized(ctx, dn);
		talloc_free(dn);
		talloc_free(ldb);
	}

	return ret;
}

static bool test_analyse_objects(struct torture_context *tctx, 
				 struct DsSyncTest *ctx,
				 const char *partition, 
				 const struct drsuapi_DsReplicaOIDMapping_Ctr *mapping_ctr,
				 uint32_t object_count,
				 const struct drsuapi_DsReplicaObjectListItemEx *first_object,
				 const DATA_BLOB *gensec_skey)
{
	static uint32_t object_id;
	const char *save_values_dir;
	const struct drsuapi_DsReplicaObjectListItemEx *cur;
	struct ldb_context *ldb = ctx->admin.ldap.ldb;
	struct ldb_dn *deleted_dn;
	WERROR status;
	int i, j, ret;
	struct dsdb_extended_replicated_objects *objs;
	struct ldb_extended_dn_control *extended_dn_ctrl;
	const char *err_msg;
	
	if (!dsdb_get_schema(ldb)) {
		struct dsdb_schema *ldap_schema;
		struct ldb_result *a_res;
		struct ldb_result *c_res;
		struct ldb_dn *schema_dn = ldb_get_schema_basedn(ldb);
		ldap_schema = dsdb_new_schema(ctx, lp_iconv_convenience(tctx->lp_ctx));
		if (!ldap_schema) {
			return false;
		}
		status = dsdb_load_prefixmap_from_drsuapi(ldap_schema, mapping_ctr);

		/*
		 * load the attribute definitions
		 */
		ret = ldb_search(ldb, ldap_schema, &a_res,
				 schema_dn, LDB_SCOPE_ONELEVEL, NULL,
				 "(objectClass=attributeSchema)");
		if (ret != LDB_SUCCESS) {
			err_msg = talloc_asprintf(tctx,
						  "failed to search attributeSchema objects: %s",
						  ldb_errstring(ldb));
			torture_fail(tctx, err_msg);
		}

		/*
		 * load the objectClass definitions
		 */
		ret = ldb_search(ldb, ldap_schema, &c_res,
				 schema_dn, LDB_SCOPE_ONELEVEL, NULL,
				 "(objectClass=classSchema)");
		if (ret != LDB_SUCCESS) {
			err_msg = talloc_asprintf(tctx,
						  "failed to search classSchema objects: %s",
						  ldb_errstring(ldb));
			torture_fail(tctx, err_msg);
		}

		/* Build schema */
		for (i=0; i < a_res->count; i++) {
			status = dsdb_attribute_from_ldb(ldb, ldap_schema, a_res->msgs[i]);
			torture_assert_werr_ok(tctx, status,
					       talloc_asprintf(tctx,
							       "dsdb_attribute_from_ldb() failed for: %s",
							       ldb_dn_get_linearized(a_res->msgs[i]->dn)));
		}
		
		for (i=0; i < c_res->count; i++) {
			status = dsdb_class_from_ldb(ldap_schema, c_res->msgs[i]);
			torture_assert_werr_ok(tctx, status,
					       talloc_asprintf(tctx,
							       "dsdb_class_from_ldb() failed for: %s",
							       ldb_dn_get_linearized(c_res->msgs[i]->dn)));
		}
		talloc_free(a_res);
		talloc_free(c_res);
		ret = dsdb_set_schema(ldb, ldap_schema);
		if (ret != LDB_SUCCESS) {
			torture_fail(tctx,
				     talloc_asprintf(tctx, "dsdb_set_schema() failed: %s", ldb_strerror(ret)));
		}
	}

	status = dsdb_extended_replicated_objects_convert(ldb,
							  partition,
							  mapping_ctr,
							  object_count,
							  first_object,
							  0, NULL, 
							  NULL, NULL, 
							  gensec_skey,
							  ctx, &objs);
	torture_assert_werr_ok(tctx, status, "dsdb_extended_replicated_objects_convert() failed!");

	extended_dn_ctrl = talloc(objs, struct ldb_extended_dn_control);
	extended_dn_ctrl->type = 1;

	deleted_dn = ldb_dn_new(objs, ldb, partition);
	ldb_dn_add_child_fmt(deleted_dn, "CN=Deleted Objects");
		
	for (i=0; i < object_count; i++) {
		struct ldb_request *search_req;
		struct ldb_result *res;
		struct ldb_message *new_msg, *drs_msg, *ldap_msg;
		const char **attrs = talloc_array(objs, const char *, objs->objects[i].msg->num_elements+1);
		for (j=0; j < objs->objects[i].msg->num_elements; j++) {
			attrs[j] = objs->objects[i].msg->elements[j].name;
		}
		attrs[j] = NULL;
		res = talloc_zero(objs, struct ldb_result);
		if (!res) {
			return LDB_ERR_OPERATIONS_ERROR;
		}
		ret = ldb_build_search_req(&search_req, ldb, objs, 
					   objs->objects[i].msg->dn,
					   LDB_SCOPE_BASE,
					   NULL,
					   attrs,
					   NULL,
					   res,
					   ldb_search_default_callback,
					   NULL);
		if (ret != LDB_SUCCESS) {
			return false;
		}
		talloc_steal(search_req, res);
		ret = ldb_request_add_control(search_req, LDB_CONTROL_SHOW_DELETED_OID, true, NULL);
		if (ret != LDB_SUCCESS) {
			return false;
		}

		ret = ldb_request_add_control(search_req, LDB_CONTROL_EXTENDED_DN_OID, true, extended_dn_ctrl);
		if (ret != LDB_SUCCESS) {
			return false;
		}

		ret = ldb_request(ldb, search_req);
		if (ret == LDB_SUCCESS) {
			ret = ldb_wait(search_req->handle, LDB_WAIT_ALL);
		}

		torture_assert_int_equal(tctx, ret, LDB_SUCCESS,
					 talloc_asprintf(tctx,
							 "Could not re-fetch object just delivered over DRS: %s",
							 ldb_errstring(ldb)));
		torture_assert_int_equal(tctx, res->count, 1, "Could not re-fetch object just delivered over DRS");
		ldap_msg = res->msgs[0];
		for (j=0; j < ldap_msg->num_elements; j++) {
			ldap_msg->elements[j].flags = LDB_FLAG_MOD_ADD;
			/* For unknown reasons, there is no nTSecurityDescriptor on cn=deleted objects over LDAP, but there is over DRS!  Skip it on both transports for now here so */
			if ((ldb_attr_cmp(ldap_msg->elements[j].name, "nTSecurityDescriptor") == 0) && 
			    (ldb_dn_compare(ldap_msg->dn, deleted_dn) == 0)) {
				ldb_msg_remove_element(ldap_msg, &ldap_msg->elements[j]);
				/* Don't skip one */
				j--;
			}
		}

		drs_msg = ldb_msg_canonicalize(ldb, objs->objects[i].msg);
		talloc_steal(search_req, drs_msg);

		for (j=0; j < drs_msg->num_elements; j++) {
			if (drs_msg->elements[j].num_values == 0) {
				ldb_msg_remove_element(drs_msg, &drs_msg->elements[j]);
				/* Don't skip one */
				j--;
				
				/* For unknown reasons, there is no nTSecurityDescriptor on cn=deleted objects over LDAP, but there is over DRS! */
			} else if ((ldb_attr_cmp(drs_msg->elements[j].name, "nTSecurityDescriptor") == 0) && 
				   (ldb_dn_compare(drs_msg->dn, deleted_dn) == 0)) {
				ldb_msg_remove_element(drs_msg, &drs_msg->elements[j]);
				/* Don't skip one */
				j--;
			} else if (ldb_attr_cmp(drs_msg->elements[j].name, "unicodePwd") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "dBCSPwd") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "ntPwdHistory") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "lmPwdHistory") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "supplementalCredentials") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "priorValue") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "currentValue") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "trustAuthOutgoing") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "trustAuthIncoming") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "initialAuthOutgoing") == 0 ||
				   ldb_attr_cmp(drs_msg->elements[j].name, "initialAuthIncoming") == 0) {

				/* These are not shown over LDAP, so we need to skip them for the comparison */
				ldb_msg_remove_element(drs_msg, &drs_msg->elements[j]);
				/* Don't skip one */
				j--;
			} else {
				drs_msg->elements[j].flags = LDB_FLAG_MOD_ADD;
			}
		}
		
		
		new_msg = ldb_msg_diff(ldb, drs_msg, ldap_msg);
		talloc_steal(search_req, new_msg);
		if (new_msg->num_elements != 0) {
			char *s;
			struct ldb_ldif ldif;
			ldif.changetype = LDB_CHANGETYPE_MODIFY;
			ldif.msg = new_msg;
			s = ldb_ldif_write_string(ldb, new_msg, &ldif);
			s = talloc_asprintf(tctx, "\n# Difference in between DRS and LDAP objects: \n%s\n", s);

			ldif.msg = ldb_msg_diff(ldb, ldap_msg, drs_msg);
			s = talloc_asprintf_append(s,
						   "\n# Difference in between LDAP and DRS objects: \n%s\n",
						   ldb_ldif_write_string(ldb, new_msg, &ldif));

			s = talloc_asprintf_append(s,
						   "# Should have no objects in 'difference' message. Diff elements: %d",
						   new_msg->num_elements);
			torture_fail(tctx, s);
		}

		/* search_req is used as a tmp talloc context in the above */
		talloc_free(search_req);
	}

	if (!lp_parm_bool(tctx->lp_ctx, NULL, "dssync", "print_pwd_blobs", false)) {
		talloc_free(objs);
		return true;	
	}

	save_values_dir = lp_parm_string(tctx->lp_ctx, NULL, "dssync", "save_pwd_blobs_dir");

	for (cur = first_object; cur; cur = cur->next_object) {
		const char *dn;
		struct dom_sid *sid = NULL;
		uint32_t rid = 0;
		bool dn_printed = false;

		if (!cur->object.identifier) continue;

		dn = cur->object.identifier->dn;
		if (cur->object.identifier->sid.num_auths > 0) {
			sid = &cur->object.identifier->sid;
			rid = sid->sub_auths[sid->num_auths - 1];
		}

		for (i=0; i < cur->object.attribute_ctr.num_attributes; i++) {
			WERROR werr;
			const char *name = NULL;
			bool rcrypt = false;
			DATA_BLOB *enc_data = NULL;
			DATA_BLOB plain_data;
			struct drsuapi_DsReplicaAttribute *attr;
			ndr_pull_flags_fn_t pull_fn = NULL;
			ndr_print_fn_t print_fn = NULL;
			void *ptr = NULL;
			attr = &cur->object.attribute_ctr.attributes[i];

			switch (attr->attid) {
			case DRSUAPI_ATTRIBUTE_dBCSPwd:
				name	= "dBCSPwd";
				rcrypt	= true;
				break;
			case DRSUAPI_ATTRIBUTE_unicodePwd:
				name	= "unicodePwd";
				rcrypt	= true;
				break;
			case DRSUAPI_ATTRIBUTE_ntPwdHistory:
				name	= "ntPwdHistory";
				rcrypt	= true;
				break;
			case DRSUAPI_ATTRIBUTE_lmPwdHistory:
				name	= "lmPwdHistory";
				rcrypt	= true;
				break;
			case DRSUAPI_ATTRIBUTE_supplementalCredentials:
				name	= "supplementalCredentials";
				pull_fn = (ndr_pull_flags_fn_t)ndr_pull_supplementalCredentialsBlob;
				print_fn = (ndr_print_fn_t)ndr_print_supplementalCredentialsBlob;
				ptr = talloc(ctx, struct supplementalCredentialsBlob);
				break;
			case DRSUAPI_ATTRIBUTE_priorValue:
				name	= "priorValue";
				break;
			case DRSUAPI_ATTRIBUTE_currentValue:
				name	= "currentValue";
				break;
			case DRSUAPI_ATTRIBUTE_trustAuthOutgoing:
				name	= "trustAuthOutgoing";
				pull_fn = (ndr_pull_flags_fn_t)ndr_pull_trustAuthInOutBlob;
				print_fn = (ndr_print_fn_t)ndr_print_trustAuthInOutBlob;
				ptr = talloc(ctx, struct trustAuthInOutBlob);
				break;
			case DRSUAPI_ATTRIBUTE_trustAuthIncoming:
				name	= "trustAuthIncoming";
				pull_fn = (ndr_pull_flags_fn_t)ndr_pull_trustAuthInOutBlob;
				print_fn = (ndr_print_fn_t)ndr_print_trustAuthInOutBlob;
				ptr = talloc(ctx, struct trustAuthInOutBlob);
				break;
			case DRSUAPI_ATTRIBUTE_initialAuthOutgoing:
				name	= "initialAuthOutgoing";
				break;
			case DRSUAPI_ATTRIBUTE_initialAuthIncoming:
				name	= "initialAuthIncoming";
				break;
			default:
				continue;
			}

			if (attr->value_ctr.num_values != 1) continue;

			if (!attr->value_ctr.values[0].blob) continue;

			enc_data = attr->value_ctr.values[0].blob;
			ZERO_STRUCT(plain_data);

			werr = drsuapi_decrypt_attribute_value(ctx, gensec_skey, rcrypt,
							       rid,
							       enc_data, &plain_data);
			if (!W_ERROR_IS_OK(werr)) {
				DEBUG(0, ("Failed to decrypt %s\n", name));
				continue;
			}
			if (!dn_printed) {
				object_id++;
				DEBUG(0,("DN[%u] %s\n", object_id, dn));
				dn_printed = true;
			}
			DEBUGADD(0,("ATTR: %s enc.length=%lu plain.length=%lu\n",
				    name, (long)enc_data->length, (long)plain_data.length));
			if (plain_data.length) {
				enum ndr_err_code ndr_err;
				dump_data(0, plain_data.data, plain_data.length);
				if (save_values_dir) {
					char *fname;
					fname = talloc_asprintf(ctx, "%s/%s%02d",
								save_values_dir,
								name, object_id);
					if (fname) {
						bool ok;
						ok = file_save(fname, plain_data.data, plain_data.length);
						if (!ok) {
							DEBUGADD(0,("Failed to save '%s'\n", fname));
						}
					}
					talloc_free(fname);
				}

				if (pull_fn) {
					/* Can't use '_all' because of PIDL bugs with relative pointers */
					ndr_err = ndr_pull_struct_blob(&plain_data, ptr,
								       lp_iconv_convenience(tctx->lp_ctx), ptr,
								       pull_fn);
					if (NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
						ndr_print_debug(print_fn, name, ptr);
					} else {
						DEBUG(0, ("Failed to decode %s\n", name));
					}
				}
			} else {
				dump_data(0, enc_data->data, enc_data->length);
			}
			talloc_free(ptr);
		}
	}
	talloc_free(objs);
	return true;
}

static bool test_FetchData(struct torture_context *tctx, struct DsSyncTest *ctx)
{
	NTSTATUS status;
	bool ret = true;
	int i, y = 0;
	uint64_t highest_usn = 0;
	const char *partition = NULL;
	struct drsuapi_DsGetNCChanges r;
	union drsuapi_DsGetNCChangesRequest req;
	struct drsuapi_DsReplicaObjectIdentifier nc;
	struct drsuapi_DsGetNCChangesCtr1 *ctr1 = NULL;
	struct drsuapi_DsGetNCChangesCtr6 *ctr6 = NULL;
	int32_t out_level = 0;
	struct GUID null_guid;
	struct dom_sid null_sid;
	DATA_BLOB gensec_skey;
	struct {
		int32_t level;
	} array[] = {
/*		{
			5
		},
*/		{
			8
		}
	};

	ZERO_STRUCT(null_guid);
	ZERO_STRUCT(null_sid);

	partition = lp_parm_string(tctx->lp_ctx, NULL, "dssync", "partition");
	if (partition == NULL) {
		partition = ctx->domain_dn;
		printf("dssync:partition not specified, defaulting to %s.\n", ctx->domain_dn);
	}

	highest_usn = lp_parm_int(tctx->lp_ctx, NULL, "dssync", "highest_usn", 0);

	array[0].level = lp_parm_int(tctx->lp_ctx, NULL, "dssync", "get_nc_changes_level", array[0].level);

	if (lp_parm_bool(tctx->lp_ctx, NULL, "dssync", "print_pwd_blobs", false)) {
		const struct samr_Password *nthash;
		nthash = cli_credentials_get_nt_hash(ctx->new_dc.credentials, ctx);
		if (nthash) {
			dump_data_pw("CREDENTIALS nthash:", nthash->hash, sizeof(nthash->hash));
		}
	}
	status = gensec_session_key(ctx->new_dc.drsuapi.drs_pipe->conn->security_state.generic_state,
				    &gensec_skey);
	if (!NT_STATUS_IS_OK(status)) {
		printf("failed to get gensec session key: %s\n", nt_errstr(status));
		return false;
	}

	for (i=0; i < ARRAY_SIZE(array); i++) {
		printf("testing DsGetNCChanges level %d\n",
			array[i].level);

		r.in.bind_handle	= &ctx->new_dc.drsuapi.bind_handle;
		r.in.level		= array[i].level;

		switch (r.in.level) {
		case 5:
			nc.guid	= null_guid;
			nc.sid	= null_sid;
			nc.dn	= partition; 

			r.in.req					= &req;
			r.in.req->req5.destination_dsa_guid		= ctx->new_dc.invocation_id;
			r.in.req->req5.source_dsa_invocation_id		= null_guid;
			r.in.req->req5.naming_context			= &nc;
			r.in.req->req5.highwatermark.tmp_highest_usn	= highest_usn;
			r.in.req->req5.highwatermark.reserved_usn	= 0;
			r.in.req->req5.highwatermark.highest_usn	= highest_usn;
			r.in.req->req5.uptodateness_vector		= NULL;
			r.in.req->req5.replica_flags			= 0;
			if (lp_parm_bool(tctx->lp_ctx, NULL, "dssync", "compression", false)) {
				r.in.req->req5.replica_flags		|= DRSUAPI_DS_REPLICA_NEIGHBOUR_COMPRESS_CHANGES;
			}
			if (lp_parm_bool(tctx->lp_ctx, NULL, "dssync", "neighbour_writeable", true)) {
				r.in.req->req5.replica_flags		|= DRSUAPI_DS_REPLICA_NEIGHBOUR_WRITEABLE;
			}
			r.in.req->req5.replica_flags			|= DRSUAPI_DS_REPLICA_NEIGHBOUR_SYNC_ON_STARTUP
									| DRSUAPI_DS_REPLICA_NEIGHBOUR_DO_SCHEDULED_SYNCS
									| DRSUAPI_DS_REPLICA_NEIGHBOUR_RETURN_OBJECT_PARENTS
									| DRSUAPI_DS_REPLICA_NEIGHBOUR_NEVER_SYNCED
									;
			r.in.req->req5.max_object_count			= 133;
			r.in.req->req5.max_ndr_size			= 1336770;
			r.in.req->req5.extended_op			= DRSUAPI_EXOP_NONE;
			r.in.req->req5.fsmo_info			= 0;

			break;
		case 8:
			nc.guid	= null_guid;
			nc.sid	= null_sid;
			nc.dn	= partition; 
			/* nc.dn can be set to any other ad partition */

			r.in.req					= &req;
			r.in.req->req8.destination_dsa_guid		= ctx->new_dc.invocation_id;
			r.in.req->req8.source_dsa_invocation_id		= null_guid;
			r.in.req->req8.naming_context			= &nc;
			r.in.req->req8.highwatermark.tmp_highest_usn	= highest_usn;
			r.in.req->req8.highwatermark.reserved_usn	= 0;
			r.in.req->req8.highwatermark.highest_usn	= highest_usn;
			r.in.req->req8.uptodateness_vector		= NULL;
			r.in.req->req8.replica_flags			= 0;
			if (lp_parm_bool(tctx->lp_ctx, NULL, "dssync", "compression", false)) {
				r.in.req->req8.replica_flags		|= DRSUAPI_DS_REPLICA_NEIGHBOUR_COMPRESS_CHANGES;
			}
			if (lp_parm_bool(tctx->lp_ctx, NULL, "dssync", "neighbour_writeable", true)) {
				r.in.req->req8.replica_flags		|= DRSUAPI_DS_REPLICA_NEIGHBOUR_WRITEABLE;
			}
			r.in.req->req8.replica_flags			|= DRSUAPI_DS_REPLICA_NEIGHBOUR_SYNC_ON_STARTUP
									| DRSUAPI_DS_REPLICA_NEIGHBOUR_DO_SCHEDULED_SYNCS
									| DRSUAPI_DS_REPLICA_NEIGHBOUR_RETURN_OBJECT_PARENTS
									| DRSUAPI_DS_REPLICA_NEIGHBOUR_NEVER_SYNCED
									;
			r.in.req->req8.max_object_count			= 402;
			r.in.req->req8.max_ndr_size			= 402116;

			r.in.req->req8.extended_op			= DRSUAPI_EXOP_NONE;
			r.in.req->req8.fsmo_info			= 0;
			r.in.req->req8.partial_attribute_set		= NULL;
			r.in.req->req8.partial_attribute_set_ex		= NULL;
			r.in.req->req8.mapping_ctr.num_mappings		= 0;
			r.in.req->req8.mapping_ctr.mappings		= NULL;

			break;
		}
		
		printf("Dumping AD partition: %s\n", nc.dn);
		for (y=0; ;y++) {
			int32_t _level = 0;
			union drsuapi_DsGetNCChangesCtr ctr;

			ZERO_STRUCT(r.out);

			r.out.level_out = &_level;
			r.out.ctr	= &ctr;

			if (r.in.level == 5) {
				torture_comment(tctx,
						"start[%d] tmp_higest_usn: %llu , highest_usn: %llu\n",
						y,
						r.in.req->req5.highwatermark.tmp_highest_usn,
						r.in.req->req5.highwatermark.highest_usn);
			}

			if (r.in.level == 8) {
				torture_comment(tctx,
						"start[%d] tmp_higest_usn: %llu , highest_usn: %llu\n",
						y,
						r.in.req->req8.highwatermark.tmp_highest_usn,
						r.in.req->req8.highwatermark.highest_usn);
			}

			status = dcerpc_drsuapi_DsGetNCChanges(ctx->new_dc.drsuapi.drs_pipe, ctx, &r);
			torture_drsuapi_assert_call(tctx, ctx->new_dc.drsuapi.drs_pipe, status,
						    &r, "dcerpc_drsuapi_DsGetNCChanges");

			if (ret == true && *r.out.level_out == 1) {
				out_level = 1;
				ctr1 = &r.out.ctr->ctr1;
			} else if (ret == true && *r.out.level_out == 2 &&
				   r.out.ctr->ctr2.mszip1.ts) {
				out_level = 1;
				ctr1 = &r.out.ctr->ctr2.mszip1.ts->ctr1;
			}

			if (out_level == 1) {
				torture_comment(tctx,
						"end[%d] tmp_highest_usn: %llu , highest_usn: %llu\n",
						y,
						ctr1->new_highwatermark.tmp_highest_usn,
						ctr1->new_highwatermark.highest_usn);

				if (!test_analyse_objects(tctx, ctx, partition, &ctr1->mapping_ctr,  ctr1->object_count, 
							  ctr1->first_object, &gensec_skey)) {
					return false;
				}

				if (ctr1->more_data) {
					r.in.req->req5.highwatermark = ctr1->new_highwatermark;
					continue;
				}
			}

			if (ret == true && *r.out.level_out == 6) {
				out_level = 6;
				ctr6 = &r.out.ctr->ctr6;
			} else if (ret == true && *r.out.level_out == 7
				   && r.out.ctr->ctr7.level == 6
				   && r.out.ctr->ctr7.type == DRSUAPI_COMPRESSION_TYPE_MSZIP
				   && r.out.ctr->ctr7.ctr.mszip6.ts) {
				out_level = 6;
				ctr6 = &r.out.ctr->ctr7.ctr.mszip6.ts->ctr6;
			} else if (ret == true && *r.out.level_out == 7
				   && r.out.ctr->ctr7.level == 6
				   && r.out.ctr->ctr7.type == DRSUAPI_COMPRESSION_TYPE_XPRESS
				   && r.out.ctr->ctr7.ctr.xpress6.ts) {
				out_level = 6;
				ctr6 = &r.out.ctr->ctr7.ctr.xpress6.ts->ctr6;
			}

			if (out_level == 6) {
				torture_comment(tctx,
						"end[%d] tmp_highest_usn: %llu , highest_usn: %llu\n",
						y,
						ctr6->new_highwatermark.tmp_highest_usn,
						ctr6->new_highwatermark.highest_usn);

				if (!test_analyse_objects(tctx, ctx, partition, &ctr6->mapping_ctr,  ctr6->object_count, 
							  ctr6->first_object, &gensec_skey)) {
					return false;
				}

				if (ctr6->more_data) {
					r.in.req->req8.highwatermark = ctr6->new_highwatermark;
					continue;
				}
			}

			break;
		}
	}

	return ret;
}

static bool test_FetchNT4Data(struct torture_context *tctx, 
			      struct DsSyncTest *ctx)
{
	NTSTATUS status;
	struct drsuapi_DsGetNT4ChangeLog r;
	union drsuapi_DsGetNT4ChangeLogRequest req;
	union drsuapi_DsGetNT4ChangeLogInfo info;
	uint32_t level_out = 0;
	struct GUID null_guid;
	struct dom_sid null_sid;
	DATA_BLOB cookie;

	ZERO_STRUCT(null_guid);
	ZERO_STRUCT(null_sid);
	ZERO_STRUCT(cookie);

	ZERO_STRUCT(r);
	r.in.bind_handle	= &ctx->new_dc.drsuapi.bind_handle;
	r.in.level		= 1;
	r.out.info		= &info;
	r.out.level_out		= &level_out;

	req.req1.unknown1	= lp_parm_int(tctx->lp_ctx, NULL, "dssync", "nt4-1", 3);
	req.req1.unknown2	= lp_parm_int(tctx->lp_ctx, NULL, "dssync", "nt4-2", 0x00004000);

	while (1) {
		req.req1.length	= cookie.length;
		req.req1.data	= cookie.data;

		r.in.req = &req;

		status = dcerpc_drsuapi_DsGetNT4ChangeLog(ctx->new_dc.drsuapi.drs_pipe, ctx, &r);
		if (NT_STATUS_EQUAL(status, NT_STATUS_NOT_IMPLEMENTED)) {
			torture_skip(tctx, "DsGetNT4ChangeLog not supported by target server");
		} else if (!NT_STATUS_IS_OK(status)) {
			const char *errstr = nt_errstr(status);
			if (NT_STATUS_EQUAL(status, NT_STATUS_NET_WRITE_FAULT)) {
				errstr = dcerpc_errstr(ctx, ctx->new_dc.drsuapi.drs_pipe->last_fault_code);
			}
			torture_fail(tctx,
				     talloc_asprintf(tctx, "dcerpc_drsuapi_DsGetNT4ChangeLog failed - %s\n",
						     errstr));
		} else if (W_ERROR_EQUAL(r.out.result, WERR_INVALID_DOMAIN_ROLE)) {
			torture_skip(tctx, "DsGetNT4ChangeLog not supported by target server");
		} else if (!W_ERROR_IS_OK(r.out.result)) {
			torture_fail(tctx,
				     talloc_asprintf(tctx, "DsGetNT4ChangeLog failed - %s\n",
						     win_errstr(r.out.result)));
		} else if (*r.out.level_out != 1) {
			torture_fail(tctx,
				     talloc_asprintf(tctx, "DsGetNT4ChangeLog unknown level - %u\n",
						     *r.out.level_out));
		} else if (NT_STATUS_IS_OK(r.out.info->info1.status)) {
		} else if (NT_STATUS_EQUAL(r.out.info->info1.status, STATUS_MORE_ENTRIES)) {
			cookie.length	= r.out.info->info1.length1;
			cookie.data	= r.out.info->info1.data1;
			continue;
		} else {
			torture_fail(tctx,
				     talloc_asprintf(tctx, "DsGetNT4ChangeLog failed - %s\n",
						     nt_errstr(r.out.info->info1.status)));
		}

		break;
	}

	return true;
}

/**
 * DSSYNC test case setup
 */
static bool torture_dssync_tcase_setup(struct torture_context *tctx, void **data)
{
	bool bret;
	struct DsSyncTest *ctx;

	*data = ctx = test_create_context(tctx);
	torture_assert(tctx, ctx, "test_create_context() failed");

	bret = _test_DsBind(tctx, ctx, ctx->admin.credentials, &ctx->admin.drsuapi);
	torture_assert(tctx, bret, "_test_DsBind() failed");

	bret = test_LDAPBind(tctx, ctx, ctx->admin.credentials, &ctx->admin.ldap);
	torture_assert(tctx, bret, "test_LDAPBind() failed");

	bret = test_GetInfo(tctx, ctx);
	torture_assert(tctx, bret, "test_GetInfo() failed");

	bret = _test_DsBind(tctx, ctx, ctx->new_dc.credentials, &ctx->new_dc.drsuapi);
	torture_assert(tctx, bret, "_test_DsBind() failed");

	return true;
}

/**
 * DSSYNC test case cleanup
 */
static bool torture_dssync_tcase_teardown(struct torture_context *tctx, void *data)
{
	struct DsSyncTest *ctx;
	struct drsuapi_DsUnbind r;
	struct policy_handle bind_handle;

	ctx = talloc_get_type(data, struct DsSyncTest);

	ZERO_STRUCT(r);
	r.out.bind_handle = &bind_handle;

	/* Unbing admin handle */
	r.in.bind_handle = &ctx->admin.drsuapi.bind_handle;
	dcerpc_drsuapi_DsUnbind(ctx->admin.drsuapi.drs_pipe, ctx, &r);

	/* Unbing new_dc handle */
	r.in.bind_handle = &ctx->new_dc.drsuapi.bind_handle;
	dcerpc_drsuapi_DsUnbind(ctx->new_dc.drsuapi.drs_pipe, ctx, &r);

	talloc_free(ctx);

	return true;
}

/**
 * DSSYNC test case implementation
 */
void torture_drs_rpc_dssync_tcase(struct torture_suite *suite)
{
	typedef bool (*run_func) (struct torture_context *test, void *tcase_data);

	struct torture_test *test;
	struct torture_tcase *tcase = torture_suite_add_tcase(suite, "DSSYNC");

	torture_tcase_set_fixture(tcase,
				  torture_dssync_tcase_setup,
				  torture_dssync_tcase_teardown);

	test = torture_tcase_add_simple_test(tcase, "DC_FetchData", (run_func)test_FetchData);
	test = torture_tcase_add_simple_test(tcase, "FetchNT4Data", (run_func)test_FetchNT4Data);
}

