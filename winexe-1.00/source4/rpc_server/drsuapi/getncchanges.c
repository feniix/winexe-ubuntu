/* 
   Unix SMB/CIFS implementation.

   implement the DRSUpdateRefs call

   Copyright (C) Anatoliy Atanasov 2009
   Copyright (C) Andrew Tridgell 2009
   
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
#include "rpc_server/dcerpc_server.h"
#include "dsdb/samdb/samdb.h"
#include "param/param.h"
#include "librpc/gen_ndr/ndr_drsblobs.h"
#include "rpc_server/drsuapi/dcesrv_drsuapi.h"
#include "rpc_server/dcerpc_server_proto.h"
#include "../libcli/drsuapi/drsuapi.h"
#include "libcli/security/security.h"

/* 
  drsuapi_DsGetNCChanges for one object
*/
static WERROR get_nc_changes_build_object(struct drsuapi_DsReplicaObjectListItemEx *obj,
					  struct ldb_message *msg,
					  struct ldb_context *sam_ctx,
					  struct ldb_dn *ncRoot_dn,
					  struct dsdb_schema *schema,
					  DATA_BLOB *session_key,
					  uint64_t highest_usn,
					  uint32_t replica_flags)
{
	const struct ldb_val *md_value;
	int i, n;
	struct ldb_dn *obj_dn;
	struct replPropertyMetaDataBlob md;
	struct dom_sid *sid;
	uint32_t rid = 0;
	enum ndr_err_code ndr_err;
	uint32_t *attids;
	const char *rdn;
	const struct dsdb_attribute *rdn_sa;

	if (ldb_dn_compare(ncRoot_dn, msg->dn) == 0) {
		obj->is_nc_prefix = true;
		obj->parent_object_guid = NULL;
	} else {
		obj->is_nc_prefix = false;
		obj->parent_object_guid = talloc(obj, struct GUID);
		if (obj->parent_object_guid == NULL) {
			return WERR_DS_DRA_INTERNAL_ERROR;
		}
		*obj->parent_object_guid = samdb_result_guid(msg, "parentGUID");
		if (GUID_all_zero(obj->parent_object_guid)) {
			DEBUG(0,(__location__ ": missing parentGUID for %s\n",
				 ldb_dn_get_linearized(msg->dn)));
			return WERR_DS_DRA_INTERNAL_ERROR;
		}
	}
	obj->next_object = NULL;
	
	md_value = ldb_msg_find_ldb_val(msg, "replPropertyMetaData");
	if (!md_value) {
		/* nothing to send */
		return WERR_OK;
	}

	ndr_err = ndr_pull_struct_blob(md_value, obj,
				       lp_iconv_convenience(ldb_get_opaque(sam_ctx, "loadparm")), &md,
				       (ndr_pull_flags_fn_t)ndr_pull_replPropertyMetaDataBlob);
	if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
		return WERR_DS_DRA_INTERNAL_ERROR;
	}
	
	if (md.version != 1) {
		return WERR_DS_DRA_INTERNAL_ERROR;
	}

	rdn = ldb_dn_get_rdn_name(msg->dn);
	if (rdn == NULL) {
		DEBUG(0,(__location__ ": No rDN for %s\n", ldb_dn_get_linearized(msg->dn)));
		return WERR_DS_DRA_INTERNAL_ERROR;
	}

	rdn_sa = dsdb_attribute_by_lDAPDisplayName(schema, rdn);
	if (rdn_sa == NULL) {
		DEBUG(0,(__location__ ": Can't find dsds_attribute for rDN %s in %s\n", 
			 rdn, ldb_dn_get_linearized(msg->dn)));
		return WERR_DS_DRA_INTERNAL_ERROR;
	}

	obj->meta_data_ctr = talloc(obj, struct drsuapi_DsReplicaMetaDataCtr);
	attids = talloc_array(obj, uint32_t, md.ctr.ctr1.count);

	obj->object.identifier = talloc(obj, struct drsuapi_DsReplicaObjectIdentifier);
	obj_dn = ldb_msg_find_attr_as_dn(sam_ctx, obj, msg, "distinguishedName");
	obj->object.identifier->dn = ldb_dn_get_linearized(obj_dn);
	obj->object.identifier->guid = samdb_result_guid(msg, "objectGUID");
	sid = samdb_result_dom_sid(obj, msg, "objectSid");
	if (sid) {
		dom_sid_split_rid(NULL, sid, NULL, &rid);
		obj->object.identifier->sid = *sid;
	} else {
		ZERO_STRUCT(obj->object.identifier->sid);
	}
	
	obj->meta_data_ctr->meta_data = talloc_array(obj, struct drsuapi_DsReplicaMetaData, md.ctr.ctr1.count);
	for (n=i=0; i<md.ctr.ctr1.count; i++) {
		/* if the attribute has not changed, and it is not the
		   instanceType then don't include it */
		if (md.ctr.ctr1.array[i].local_usn < highest_usn &&
		    md.ctr.ctr1.array[i].attid != DRSUAPI_ATTRIBUTE_instanceType) continue;

		/* don't include the rDN */
		if (md.ctr.ctr1.array[i].attid == rdn_sa->attributeID_id) continue;

		obj->meta_data_ctr->meta_data[n].originating_change_time = md.ctr.ctr1.array[i].originating_change_time;
		obj->meta_data_ctr->meta_data[n].version = md.ctr.ctr1.array[i].version;
		obj->meta_data_ctr->meta_data[n].originating_invocation_id = md.ctr.ctr1.array[i].originating_invocation_id;
		obj->meta_data_ctr->meta_data[n].originating_usn = md.ctr.ctr1.array[i].originating_usn;
		attids[n] = md.ctr.ctr1.array[i].attid;
		n++;
	}

	/*
	  note that if n==0 we still need to send the change, as it
	  could be a rename, which changes the uSNChanged, but not any
	  of the replicated attributes
	 */

	obj->meta_data_ctr->count = n;

	obj->object.flags = DRSUAPI_DS_REPLICA_OBJECT_FROM_MASTER;
	obj->object.attribute_ctr.num_attributes = obj->meta_data_ctr->count;
	obj->object.attribute_ctr.attributes = talloc_array(obj, struct drsuapi_DsReplicaAttribute,
							    obj->object.attribute_ctr.num_attributes);

	/*
	 * Note that the meta_data array and the attributes array must
	 * be the same size and in the same order
	 */
	for (i=0; i<obj->object.attribute_ctr.num_attributes; i++) {
		struct ldb_message_element *el;
		WERROR werr;
		const struct dsdb_attribute *sa;
	
		sa = dsdb_attribute_by_attributeID_id(schema, attids[i]);
		if (!sa) {
			DEBUG(0,("Unable to find attributeID %u in schema\n", attids[i]));
			return WERR_DS_DRA_INTERNAL_ERROR;
		}

		el = ldb_msg_find_element(msg, sa->lDAPDisplayName);
		if (el == NULL) {
			/* this happens for attributes that have been removed */
			DEBUG(5,("No element '%s' for attributeID %u in message\n",
				 sa->lDAPDisplayName, attids[i]));
			ZERO_STRUCT(obj->object.attribute_ctr.attributes[i]);
			obj->object.attribute_ctr.attributes[i].attid = attids[i];
		} else {
			werr = dsdb_attribute_ldb_to_drsuapi(sam_ctx, schema, el, obj,
							     &obj->object.attribute_ctr.attributes[i]);
			if (!W_ERROR_IS_OK(werr)) {
				DEBUG(0,("Unable to convert %s to DRS object - %s\n", 
					 sa->lDAPDisplayName, win_errstr(werr)));
				return werr;
			}
			/* if DRSUAPI_DRS_SPECIAL_SECRET_PROCESSING is set
			 * check if attribute is secret and send a null value
			 */
			if (replica_flags & DRSUAPI_DRS_SPECIAL_SECRET_PROCESSING) {
				drsuapi_process_secret_attribute(&obj->object.attribute_ctr.attributes[i],
								 &obj->meta_data_ctr->meta_data[i]);
			}
			/* some attributes needs to be encrypted
			   before being sent */
			werr = drsuapi_encrypt_attribute(obj, session_key, rid, 
							 &obj->object.attribute_ctr.attributes[i]);
			if (!W_ERROR_IS_OK(werr)) {
				DEBUG(0,("Unable to encrypt %s in DRS object - %s\n", 
					 sa->lDAPDisplayName, win_errstr(werr)));
				return werr;
			}
		}
	}

	return WERR_OK;
}

/*
  load replUpToDateVector from a DN
 */
static WERROR load_udv(struct ldb_context *sam_ctx, TALLOC_CTX *mem_ctx,
		       struct ldb_dn *dn, struct replUpToDateVectorBlob *ouv)
{
	const char *attrs[] = { "replUpToDateVector", NULL };
	struct ldb_result *res = NULL;
	TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);
	struct ldb_message_element *el;
	enum ndr_err_code ndr_err;

	ZERO_STRUCTP(ouv);

	if (ldb_search(sam_ctx, tmp_ctx, &res, dn, LDB_SCOPE_BASE, attrs, NULL) != LDB_SUCCESS ||
	    res->count < 1) {
		DEBUG(0,("load_udv: failed to read partition object\n"));
		talloc_free(tmp_ctx);
		return WERR_DS_DRA_INTERNAL_ERROR;
	}

	el = ldb_msg_find_element(res->msgs[0], "replUpToDateVector");
	if (el == NULL || el->num_values < 1) {
		talloc_free(tmp_ctx);
		ouv->version = 2;
		return WERR_OK;
	}

	ndr_err = ndr_pull_struct_blob(&el->values[0], 
				       mem_ctx, lp_iconv_convenience(ldb_get_opaque(sam_ctx, "loadparm")),
				       ouv, 
				       (ndr_pull_flags_fn_t)ndr_pull_replUpToDateVectorBlob);
	talloc_free(tmp_ctx);
	if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
		DEBUG(0,(__location__ ": Failed to parse replUpToDateVector for %s\n",
			 ldb_dn_get_linearized(dn)));
		return WERR_DS_DRA_INTERNAL_ERROR;
	}
	
	return WERR_OK;
	
}

/*
  fill in the cursors return based on the replUpToDateVector for the ncRoot_dn
 */
static WERROR get_nc_changes_udv(struct ldb_context *sam_ctx,
				 struct ldb_dn *ncRoot_dn,
				 struct drsuapi_DsReplicaCursor2CtrEx *udv,
				 uint64_t highestUSN)
{
	WERROR werr;
	struct drsuapi_DsReplicaCursor2 *tmp_cursor;
	NTTIME now;
	time_t t = time(NULL);
	struct replUpToDateVectorBlob ouv;

	werr = load_udv(sam_ctx, udv, ncRoot_dn, &ouv);
	if (!W_ERROR_IS_OK(werr)) {
		return werr;
	}
	
	tmp_cursor = talloc(udv, struct drsuapi_DsReplicaCursor2);
	tmp_cursor->source_dsa_invocation_id = *(samdb_ntds_invocation_id(sam_ctx));
	tmp_cursor->highest_usn = highestUSN;
	unix_to_nt_time(&now, t);
	tmp_cursor->last_sync_success = now;

	udv->count = ouv.ctr.ctr2.count + 1;
	udv->cursors = talloc_steal(udv, ouv.ctr.ctr2.cursors);
	udv->cursors = talloc_realloc(udv, udv->cursors, struct drsuapi_DsReplicaCursor2, udv->count);
	if (!udv->cursors) {
		return WERR_DS_DRA_INTERNAL_ERROR;
	}
	udv->cursors[udv->count - 1] = *tmp_cursor;
	
	qsort(udv->cursors, udv->count,
	      sizeof(struct drsuapi_DsReplicaCursor2),
	      (comparison_fn_t)drsuapi_DsReplicaCursor2_compare);

	return WERR_OK;
}

/* state of a partially completed getncchanges call */
struct drsuapi_getncchanges_state {
	struct ldb_result *site_res;
	uint32_t num_sent;
	struct ldb_dn *ncRoot_dn;
	uint64_t min_usn;
	uint64_t highest_usn;
};

/* 
  drsuapi_DsGetNCChanges

  see MS-DRSR 4.1.10.5.2 for basic logic of this function
*/
WERROR dcesrv_drsuapi_DsGetNCChanges(struct dcesrv_call_state *dce_call, TALLOC_CTX *mem_ctx,
				     struct drsuapi_DsGetNCChanges *r)
{
	struct drsuapi_DsReplicaObjectIdentifier *ncRoot;
	int ret;
	int i;
	struct dsdb_schema *schema;
	struct drsuapi_DsReplicaOIDMapping_Ctr *ctr;
	struct drsuapi_DsReplicaObjectListItemEx **currentObject;
	NTSTATUS status;
	DATA_BLOB session_key;
	const char *attrs[] = { "*", "distinguishedName",
				"nTSecurityDescriptor",
				"parentGUID",
				"replPropertyMetaData",
				"unicodePwd",
				"dBCSPwd",
				"ntPwdHistory",
				"lmPwdHistory",
				"supplementalCredentials",
				NULL };
	WERROR werr;
	struct dcesrv_handle *h;
	struct drsuapi_bind_state *b_state;	
	struct drsuapi_getncchanges_state *getnc_state;
	struct drsuapi_DsGetNCChangesRequest8 *req8;
	uint32_t options;

	DCESRV_PULL_HANDLE_WERR(h, r->in.bind_handle, DRSUAPI_BIND_HANDLE);
	b_state = h->data;

	*r->out.level_out = 6;
	/* TODO: linked attributes*/
	r->out.ctr->ctr6.linked_attributes_count = 0;
	r->out.ctr->ctr6.linked_attributes = NULL;

	r->out.ctr->ctr6.object_count = 0;
	r->out.ctr->ctr6.nc_object_count = 0;
	r->out.ctr->ctr6.more_data = false;
	r->out.ctr->ctr6.uptodateness_vector = NULL;

	/* a RODC doesn't allow for any replication */
	if (samdb_rodc(ldb_get_opaque(b_state->sam_ctx, "loadparm"))) {
		DEBUG(0,(__location__ ": DsGetNCChanges attempt on RODC\n"));
		return WERR_DS_DRA_SOURCE_DISABLED;
	}

	/* Check request revision. 
	   TODO: Adding mappings to req8 from the other levels
	 */
	if (r->in.level != 8) {
		DEBUG(0,(__location__ ": Request for DsGetNCChanges with unsupported level %u\n",
			 r->in.level));
		return WERR_REVISION_MISMATCH;
	}

	req8 = &r->in.req->req8;

        /* Perform access checks. */
	/* TODO: we need to support a sync on a specific non-root
	 * DN. We'll need to find the real partition root here */
	ncRoot = req8->naming_context;
	if (ncRoot == NULL) {
		DEBUG(0,(__location__ ": Request for DsGetNCChanges with no NC\n"));
		return WERR_DS_DRA_INVALID_PARAMETER;
	}

	if (samdb_ntds_options(b_state->sam_ctx, &options) != LDB_SUCCESS) {
		return WERR_DS_DRA_INTERNAL_ERROR;
	}
	
	if ((options & DS_NTDSDSA_OPT_DISABLE_OUTBOUND_REPL) &&
	    !(req8->replica_flags & DRSUAPI_DRS_SYNC_FORCED)) {
		return WERR_DS_DRA_SOURCE_DISABLED;
	}


	if (req8->replica_flags & DRSUAPI_DS_REPLICA_NEIGHBOUR_FULL_SYNC_PACKET) {
		/* Ignore the _in_ uptpdateness vector*/
		req8->uptodateness_vector = NULL;
	} 

	werr = drs_security_level_check(dce_call, "DsGetNCChanges");
	if (!W_ERROR_IS_OK(werr)) {
		return werr;
	}

	getnc_state = b_state->getncchanges_state;

	/* see if a previous replication has been abandoned */
	if (getnc_state) {
		struct ldb_dn *new_dn = ldb_dn_new(getnc_state, b_state->sam_ctx, ncRoot->dn);
		if (ldb_dn_compare(new_dn, getnc_state->ncRoot_dn) != 0) {
			DEBUG(0,(__location__ ": DsGetNCChanges 2nd replication on different DN %s %s\n",
				 ldb_dn_get_linearized(new_dn),
				 ldb_dn_get_linearized(getnc_state->ncRoot_dn)));
			talloc_free(getnc_state);
			getnc_state = NULL;
		}
	}

	if (getnc_state == NULL) {
		getnc_state = talloc_zero(b_state, struct drsuapi_getncchanges_state);
		if (getnc_state == NULL) {
			return WERR_NOMEM;
		}
		b_state->getncchanges_state = getnc_state;
		getnc_state->ncRoot_dn = ldb_dn_new(getnc_state, b_state->sam_ctx, ncRoot->dn);
	}

	if (!ldb_dn_validate(getnc_state->ncRoot_dn) ||
	    ldb_dn_is_null(getnc_state->ncRoot_dn)) {
		DEBUG(0,(__location__ ": Bad DN '%s'\n", ncRoot->dn));
		return WERR_DS_DRA_INVALID_PARAMETER;
	}

	/* we need the session key for encrypting password attributes */
	status = dcesrv_inherited_session_key(dce_call->conn, &session_key);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(0,(__location__ ": Failed to get session key\n"));
		return WERR_DS_DRA_INTERNAL_ERROR;		
	}

	/* we don't yet support extended operations */
	if (req8->extended_op != DRSUAPI_EXOP_NONE) {
		DEBUG(0,(__location__ ": Request for DsGetNCChanges extended op 0x%x\n",
			 (unsigned)req8->extended_op));
		return WERR_DS_DRA_NOT_SUPPORTED;
	}

	/* 
	   TODO: MS-DRSR section 4.1.10.1.1
	   Work out if this is the start of a new cycle */

	if (getnc_state->site_res == NULL) {
		char* search_filter;
		enum ldb_scope scope = LDB_SCOPE_SUBTREE;

		getnc_state->min_usn = req8->highwatermark.highest_usn;

		/* Construct response. */
		search_filter = talloc_asprintf(mem_ctx,
						"(uSNChanged>=%llu)",
						(unsigned long long)(getnc_state->min_usn+1));
	
		if (req8->replica_flags & DRSUAPI_DS_REPLICA_NEIGHBOUR_CRITICAL_ONLY) {
			search_filter = talloc_asprintf(mem_ctx,
							"(&%s(isCriticalSystemObject=TRUE))",
							search_filter);
		}
		
		if (req8->replica_flags & DRSUAPI_DS_REPLICA_NEIGHBOUR_ASYNC_REP) {
			scope = LDB_SCOPE_BASE;
		}
		
		DEBUG(6,(__location__ ": getncchanges on %s using filter %s\n",
			 ldb_dn_get_linearized(getnc_state->ncRoot_dn), search_filter));
		ret = drsuapi_search_with_extended_dn(b_state->sam_ctx, getnc_state, &getnc_state->site_res,
						      getnc_state->ncRoot_dn, scope, attrs,
						      "uSNChanged",
						      search_filter);
		if (ret != LDB_SUCCESS) {
			return WERR_DS_DRA_INTERNAL_ERROR;
		}
	}

	/* Prefix mapping */
	schema = dsdb_get_schema(b_state->sam_ctx);
	if (!schema) {
		DEBUG(0,("No schema in sam_ctx\n"));
		return WERR_DS_DRA_INTERNAL_ERROR;
	}

	r->out.ctr->ctr6.naming_context = talloc(mem_ctx, struct drsuapi_DsReplicaObjectIdentifier);
	*r->out.ctr->ctr6.naming_context = *ncRoot;

	if (dsdb_find_guid_by_dn(b_state->sam_ctx, getnc_state->ncRoot_dn, 
				 &r->out.ctr->ctr6.naming_context->guid) != LDB_SUCCESS) {
		DEBUG(0,(__location__ ": Failed to find GUID of ncRoot_dn %s\n",
			 ldb_dn_get_linearized(getnc_state->ncRoot_dn)));
		return WERR_DS_DRA_INTERNAL_ERROR;
	}

	/* find the SID if there is one */
	dsdb_find_sid_by_dn(b_state->sam_ctx, getnc_state->ncRoot_dn, &r->out.ctr->ctr6.naming_context->sid);

	dsdb_get_oid_mappings_drsuapi(schema, true, mem_ctx, &ctr);
	r->out.ctr->ctr6.mapping_ctr = *ctr;

	r->out.ctr->ctr6.source_dsa_guid = *(samdb_ntds_objectGUID(b_state->sam_ctx));
	r->out.ctr->ctr6.source_dsa_invocation_id = *(samdb_ntds_invocation_id(b_state->sam_ctx));

	r->out.ctr->ctr6.old_highwatermark = req8->highwatermark;
	r->out.ctr->ctr6.new_highwatermark = req8->highwatermark;

	r->out.ctr->ctr6.first_object = NULL;
	currentObject = &r->out.ctr->ctr6.first_object;

	for(i=getnc_state->num_sent; 
	    i<getnc_state->site_res->count && 
		    (r->out.ctr->ctr6.object_count < req8->max_object_count);
	    i++) {
		int uSN;
		struct drsuapi_DsReplicaObjectListItemEx *obj;
		obj = talloc_zero(mem_ctx, struct drsuapi_DsReplicaObjectListItemEx);

		uSN = ldb_msg_find_attr_as_int(getnc_state->site_res->msgs[i], "uSNChanged", -1);
		if (uSN > r->out.ctr->ctr6.new_highwatermark.tmp_highest_usn) {
			r->out.ctr->ctr6.new_highwatermark.tmp_highest_usn = uSN;
		}
		if (uSN > getnc_state->highest_usn) {
			getnc_state->highest_usn = uSN;
		}

		werr = get_nc_changes_build_object(obj, getnc_state->site_res->msgs[i], 
						   b_state->sam_ctx, getnc_state->ncRoot_dn, 
						   schema, &session_key, getnc_state->min_usn,
						   req8->replica_flags);
		if (!W_ERROR_IS_OK(werr)) {
			return werr;
		}

		if (obj->meta_data_ctr == NULL) {
			DEBUG(0,(__location__ ": getncchanges skipping send of object %s\n",
				 ldb_dn_get_linearized(getnc_state->site_res->msgs[i]->dn)));
			/* no attributes to send */
			talloc_free(obj);
			continue;
		}

		r->out.ctr->ctr6.object_count++;
		
		*currentObject = obj;
		currentObject = &obj->next_object;
	}

	getnc_state->num_sent += r->out.ctr->ctr6.object_count;

	r->out.ctr->ctr6.nc_object_count = getnc_state->site_res->count;

	/* the client can us to call UpdateRefs on its behalf to
	   re-establish monitoring of the NC */
	if ((req8->replica_flags & DRSUAPI_DRS_ADD_REF) && 
	    !GUID_all_zero(&req8->destination_dsa_guid)) {
		struct drsuapi_DsReplicaUpdateRefsRequest1 ureq;
		ureq.naming_context = ncRoot;
		ureq.dest_dsa_dns_name = talloc_asprintf(mem_ctx, "%s._msdcs.%s",
							 GUID_string(mem_ctx, &req8->destination_dsa_guid),
							 lp_realm(dce_call->conn->dce_ctx->lp_ctx));
		if (!ureq.dest_dsa_dns_name) {
			return WERR_NOMEM;
		}
		ureq.dest_dsa_guid = req8->destination_dsa_guid;
		ureq.options = DRSUAPI_DS_REPLICA_UPDATE_ADD_REFERENCE |
			DRSUAPI_DS_REPLICA_UPDATE_ASYNCHRONOUS_OPERATION |
			DRSUAPI_DS_REPLICA_UPDATE_GETCHG_CHECK;
		werr = drsuapi_UpdateRefs(b_state, mem_ctx, &ureq);
		if (!W_ERROR_IS_OK(werr)) {
			DEBUG(0,(__location__ ": Failed UpdateRefs in DsGetNCChanges - %s\n",
				 win_errstr(werr)));
		}
	}

	if (i < getnc_state->site_res->count) {
		r->out.ctr->ctr6.more_data = true;
	} else {
		r->out.ctr->ctr6.uptodateness_vector = talloc(mem_ctx, struct drsuapi_DsReplicaCursor2CtrEx);
		r->out.ctr->ctr6.uptodateness_vector->version = 2;
		r->out.ctr->ctr6.uptodateness_vector->reserved1 = 0;
		r->out.ctr->ctr6.uptodateness_vector->reserved2 = 0;

		r->out.ctr->ctr6.new_highwatermark.highest_usn = r->out.ctr->ctr6.new_highwatermark.tmp_highest_usn;

		werr = get_nc_changes_udv(b_state->sam_ctx, getnc_state->ncRoot_dn, 
					  r->out.ctr->ctr6.uptodateness_vector,
					  getnc_state->highest_usn);
		if (!W_ERROR_IS_OK(werr)) {
			return werr;
		}

		talloc_free(getnc_state);
		b_state->getncchanges_state = NULL;
	}

	DEBUG(2,("DsGetNCChanges with uSNChanged >= %llu flags 0x%08x on %s gave %u objects (done %d/%d)\n",
		 (unsigned long long)(req8->highwatermark.highest_usn+1),
		 req8->replica_flags,
		 ncRoot->dn, r->out.ctr->ctr6.object_count,
		 i, r->out.ctr->ctr6.more_data?getnc_state->site_res->count:i));

	return WERR_OK;
}
