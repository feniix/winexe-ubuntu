/*
  ldb database library

  Copyright (C) Simo Sorce 2006-2008
  Copyright (C) Nadezhda Ivanova 2009
  Copyright (C) Anatoliy Atanasov  2009

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

/*
 *  Name: ldb
 *
 *  Component: ldb ACL module
 *
 *  Description: Module that performs authorisation access checks based on the
 *               account's security context and the DACL of the object being polled.
 *               Only DACL checks implemented at this point
 *
 *  Authors: Nadezhda Ivanova, Anatoliy Atanasov
 */

#include "includes.h"
#include "ldb_module.h"
#include "auth/auth.h"
#include "libcli/security/security.h"
#include "librpc/gen_ndr/ndr_security.h"
#include "dsdb/samdb/samdb.h"
#include "librpc/gen_ndr/ndr_security.h"
#include "param/param.h"

struct extended_access_check_attribute {
	const char *oa_name;
	const uint32_t requires_rights;
};

struct acl_private {
	bool acl_perform;
	const char **password_attrs;
};

struct acl_context {
	struct ldb_module *module;
	struct ldb_request *req;
	enum security_user_level user_type;
	bool allowedAttributes;
	bool allowedAttributesEffective;
	bool allowedChildClasses;
	bool allowedChildClassesEffective;
	bool sDRightsEffective;
	const char * const *attrs;
};

bool is_root_base_dn(struct ldb_context *ldb, struct ldb_dn *dn_to_check)
{
	int result;
	struct ldb_dn *root_base_dn = ldb_get_root_basedn(ldb);
	result = ldb_dn_compare(root_base_dn,dn_to_check);
	return (result==0);
}

static enum security_user_level what_is_user(struct ldb_module *module)
{
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	struct auth_session_info *session_info
		= (struct auth_session_info *)ldb_get_opaque(ldb, "sessionInfo");
	return security_session_user_level(session_info);
}

static struct security_token *acl_user_token(struct ldb_module *module)
{
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	struct auth_session_info *session_info
		= (struct auth_session_info *)ldb_get_opaque(ldb, "sessionInfo");
	if(!session_info) {
		return NULL;
	}
	return session_info->security_token;
}

static int acl_module_init(struct ldb_module *module)
{
	struct ldb_context *ldb;
	struct acl_private *data;
	int ret, i;
	TALLOC_CTX *mem_ctx = talloc_new(module);
	static const char *attrs[] = { "passwordAttribute", NULL };
	struct ldb_result *res;
	struct ldb_message *msg;
	struct ldb_message_element *password_attributes;

	ldb = ldb_module_get_ctx(module);

	ret = ldb_mod_register_control(module, LDB_CONTROL_SD_FLAGS_OID);
	if (ret != LDB_SUCCESS) {
		ldb_debug(ldb, LDB_DEBUG_ERROR,
			  "acl_module_init: Unable to register control with rootdse!\n");
		return LDB_ERR_OPERATIONS_ERROR;
	}

	data = talloc(module, struct acl_private);
	if (data == NULL) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	data->password_attrs = NULL;
	data->acl_perform = lp_parm_bool(ldb_get_opaque(ldb, "loadparm"),
					 NULL, "acl", "perform", false);
	ldb_module_set_private(module, data);

	if (!mem_ctx) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	ret = ldb_search(ldb, mem_ctx, &res,
			 ldb_dn_new(mem_ctx, ldb, "@KLUDGEACL"),
			 LDB_SCOPE_BASE, attrs, NULL);
	if (ret != LDB_SUCCESS) {
		goto done;
	}
	if (res->count == 0) {
		goto done;
	}

	if (res->count > 1) {
		talloc_free(mem_ctx);
		return LDB_ERR_CONSTRAINT_VIOLATION;
	}

	msg = res->msgs[0];

	password_attributes = ldb_msg_find_element(msg, "passwordAttribute");
	if (!password_attributes) {
		goto done;
	}
	data->password_attrs = talloc_array(data, const char *, password_attributes->num_values + 1);
	if (!data->password_attrs) {
		talloc_free(mem_ctx);
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}
	for (i=0; i < password_attributes->num_values; i++) {
		data->password_attrs[i] = (const char *)password_attributes->values[i].data;
		talloc_steal(data->password_attrs, password_attributes->values[i].data);
	}
	data->password_attrs[i] = NULL;

done:
	talloc_free(mem_ctx);
	return ldb_next_init(module);
}

static int get_sd_from_ldb_message(TALLOC_CTX *mem_ctx,
				   struct ldb_message *acl_res,
				   struct security_descriptor **sd)
{
	struct ldb_message_element *sd_element;
	enum ndr_err_code ndr_err;

	sd_element = ldb_msg_find_element(acl_res, "nTSecurityDescriptor");
	if (!sd_element) {
		*sd = NULL;
		return LDB_SUCCESS;
	}
	*sd = talloc(mem_ctx, struct security_descriptor);
	if(!*sd) {
		return LDB_ERR_OPERATIONS_ERROR;
	}
	ndr_err = ndr_pull_struct_blob(&sd_element->values[0], *sd, NULL, *sd,
				       (ndr_pull_flags_fn_t)ndr_pull_security_descriptor);

	if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	return LDB_SUCCESS;
}

static const struct GUID *get_oc_guid_from_message(struct ldb_module *module,
						   struct ldb_message *msg)
{
	struct ldb_message_element *oc_el;
	struct ldb_context *ldb = ldb_module_get_ctx(module);

	oc_el = ldb_msg_find_element(msg, "objectClass");
	if (!oc_el) {
		return NULL;
	}

	return class_schemaid_guid_by_lDAPDisplayName(dsdb_get_schema(ldb),
						      (char *)oc_el->values[oc_el->num_values-1].data);
}

static int get_dom_sid_from_ldb_message(TALLOC_CTX *mem_ctx,
				   struct ldb_message *acl_res,
				   struct dom_sid **sid)
{
	struct ldb_message_element *sid_element;
	enum ndr_err_code ndr_err;

	sid_element = ldb_msg_find_element(acl_res, "objectSid");
	if (!sid_element) {
		*sid = NULL;
		return LDB_SUCCESS;
	}
	*sid = talloc(mem_ctx, struct dom_sid);
	if(!*sid) {
		return LDB_ERR_OPERATIONS_ERROR;
	}
	ndr_err = ndr_pull_struct_blob(&sid_element->values[0], *sid, NULL, *sid,
				       (ndr_pull_flags_fn_t)ndr_pull_dom_sid);

	if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	return LDB_SUCCESS;
}


static void acl_debug(struct security_descriptor *sd,
		      struct security_token *token,
		      struct ldb_dn *dn,
		      bool denied,
		      int level)
{
	if (denied) {
		DEBUG(level, ("Access on %s denied", ldb_dn_get_linearized(dn)));
	} else {
		DEBUG(level, ("Access on %s granted", ldb_dn_get_linearized(dn)));
	}

	DEBUG(level,("Security context: %s\n",
		     ndr_print_struct_string(0,(ndr_print_fn_t)ndr_print_security_token,"", token)));
	DEBUG(level,("Security descriptor: %s\n",
		     ndr_print_struct_string(0,(ndr_print_fn_t)ndr_print_security_descriptor,"", sd)));
}

static int check_access_on_dn(struct ldb_module *module,
			      TALLOC_CTX *mem_ctx,
			      struct ldb_dn *dn,
			      uint32_t access,
			      struct object_tree *tree)
{
	int ret;
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	struct ldb_result *acl_res;
	struct security_descriptor *sd = NULL;
	struct dom_sid *sid = NULL;
	NTSTATUS status;
	uint32_t access_granted;
	static const char *acl_attrs[] = {
		"nTSecurityDescriptor",
		"objectSid",
		NULL
	};

	ret = ldb_search(ldb, mem_ctx, &acl_res, dn, LDB_SCOPE_BASE, acl_attrs, NULL);
	/* we sould be able to find the parent */
	if (ret != LDB_SUCCESS) {
		DEBUG(10,("acl: failed to find object %s\n", ldb_dn_get_linearized(dn)));
		return ret;
	}

	ret = get_sd_from_ldb_message(mem_ctx, acl_res->msgs[0], &sd);
	if (ret != LDB_SUCCESS) {
		return LDB_ERR_OPERATIONS_ERROR;
	}
	/* Theoretically we pass the check if the object has no sd */
	if (!sd) {
		return LDB_SUCCESS;
	}
	ret = get_dom_sid_from_ldb_message(mem_ctx, acl_res->msgs[0], &sid);
	if (ret != LDB_SUCCESS) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	status = sec_access_check_ds(sd, acl_user_token(module),
				     access,
				     &access_granted,
				     tree,
				     sid);
	if (!NT_STATUS_IS_OK(status)) {
		acl_debug(sd,
			  acl_user_token(module),
			  dn,
			  true,
			  10);
		return LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS;
	}
	return LDB_SUCCESS;
}

static int acl_check_access_on_attribute(struct ldb_module *module,
					 TALLOC_CTX *mem_ctx,
					 struct security_descriptor *sd,
					 struct dom_sid *rp_sid,
					 uint32_t access,
					 struct dsdb_attribute *attr)
{
	int ret;
	NTSTATUS status;
	uint32_t access_granted;
	struct object_tree *root = NULL;
	struct object_tree *new_node = NULL;
	TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);
	struct security_token *token = acl_user_token(module);
	if (attr) {
		if (!GUID_all_zero(&attr->attributeSecurityGUID)) {
			if (!insert_in_object_tree(tmp_ctx,
						   &attr->attributeSecurityGUID, access,
						   &root, &new_node)) {
				DEBUG(10, ("acl_search: cannot add to object tree securityGUID\n"));
				goto fail;
			}

			if (!insert_in_object_tree(tmp_ctx,
						   &attr->schemaIDGUID, access, &new_node, &new_node)) {
				DEBUG(10, ("acl_search: cannot add to object tree attributeGUID\n"));
				goto fail;
			}
		}
		else {
			if (!insert_in_object_tree(tmp_ctx,
						   &attr->schemaIDGUID, access, &root, &new_node)) {
				DEBUG(10, ("acl_search: cannot add to object tree attributeGUID\n"));
				goto fail;
			}
		}
	}
	status = sec_access_check_ds(sd, token,
				     access,
				     &access_granted,
				     root,
				     rp_sid);
	if (!NT_STATUS_IS_OK(status)) {
		ret = LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS;
	}
	else {
		ret = LDB_SUCCESS;
	}
	return ret;
fail:
	return LDB_ERR_OPERATIONS_ERROR;
}

static int acl_check_access_on_class(struct ldb_module *module,
				     TALLOC_CTX *mem_ctx,
				     struct security_descriptor *sd,
				     struct dom_sid *rp_sid,
				     uint32_t access,
				     const char *class_name)
{
	int ret;
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	NTSTATUS status;
	uint32_t access_granted;
	struct object_tree *root = NULL;
	struct object_tree *new_node = NULL;
	struct GUID *guid;
	const struct dsdb_schema *schema = dsdb_get_schema(ldb);
	TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);
	struct security_token *token = acl_user_token(module);
	if (class_name) {
		guid = class_schemaid_guid_by_lDAPDisplayName(schema, class_name);
		if (!guid) {
			DEBUG(10, ("acl_search: cannot find class %s\n",
				   class_name));
			goto fail;
		}
		if (!insert_in_object_tree(tmp_ctx,
					   guid, access,
					   &root, &new_node)) {
			DEBUG(10, ("acl_search: cannot add to object tree guid\n"));
			goto fail;
		}
	}
	status = sec_access_check_ds(sd, token,
				     access,
				     &access_granted,
				     root,
				     rp_sid);
	if (!NT_STATUS_IS_OK(status)) {
		ret = LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS;
	}
	else {
		ret = LDB_SUCCESS;
	}
	return ret;
fail:
	return LDB_ERR_OPERATIONS_ERROR;
}

static int acl_allowedAttributes(struct ldb_module *module,
				 struct ldb_message *sd_msg,
				 struct ldb_message *msg,
				 struct acl_context *ac)
{
	struct ldb_message_element *oc_el;
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	const struct dsdb_schema *schema = dsdb_get_schema(ldb);
	TALLOC_CTX *mem_ctx;
	const char **attr_list;
	int i, ret;

	/* If we don't have a schema yet, we can't do anything... */
	if (schema == NULL) {
		return LDB_SUCCESS;
	}

	/* Must remove any existing attribute */
	if (ac->allowedAttributes) {
		ldb_msg_remove_attr(msg, "allowedAttributes");
	}

	mem_ctx = talloc_new(msg);
	if (!mem_ctx) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}

	oc_el = ldb_msg_find_element(sd_msg, "objectClass");
	attr_list = dsdb_full_attribute_list(mem_ctx, schema, oc_el, DSDB_SCHEMA_ALL);
	if (!attr_list) {
		ldb_asprintf_errstring(ldb, "acl: Failed to get list of attributes");
		talloc_free(mem_ctx);
		return LDB_ERR_OPERATIONS_ERROR;
	}
	if (ac->allowedAttributes) {
		for (i=0; attr_list && attr_list[i]; i++) {
			ldb_msg_add_string(msg, "allowedAttributes", attr_list[i]);
		}
	}
	if (ac->allowedAttributesEffective) {
		struct security_descriptor *sd;
		struct dom_sid *sid = NULL;
		ldb_msg_remove_attr(msg, "allowedAttributesEffective");
		if (ac->user_type == SECURITY_SYSTEM) {
			for (i=0; attr_list && attr_list[i]; i++) {
				ldb_msg_add_string(msg, "allowedAttributesEffective", attr_list[i]);
			}
			return LDB_SUCCESS;
		}

		ret = get_sd_from_ldb_message(mem_ctx, sd_msg, &sd);

		if (ret != LDB_SUCCESS) {
			return ret;
		}
		ret = get_dom_sid_from_ldb_message(mem_ctx, sd_msg, &sid);

		if (ret != LDB_SUCCESS) {
			return ret;
		}
		for (i=0; attr_list && attr_list[i]; i++) {
			struct dsdb_attribute *attr = dsdb_attribute_by_lDAPDisplayName(schema,
											attr_list[i]);
			if (!attr) {
				return LDB_ERR_OPERATIONS_ERROR;
			}
			/* remove constructed attributes */
			if (attr->systemFlags & DS_FLAG_ATTR_IS_CONSTRUCTED
			    || attr->systemOnly
			    || (attr->linkID != 0 && attr->linkID % 2 != 0 )) {
				continue;
			}
			ret = acl_check_access_on_attribute(module,
							    msg,
							    sd,
							    sid,
							    SEC_ADS_WRITE_PROP,
							    attr);
			if (ret == LDB_SUCCESS) {
				ldb_msg_add_string(msg, "allowedAttributesEffective", attr_list[i]);
			}
		}
	}
	return LDB_SUCCESS;
}

static int acl_childClasses(struct ldb_module *module,
			    struct ldb_message *sd_msg,
			    struct ldb_message *msg,
			    const char *attrName)
{
	struct ldb_message_element *oc_el;
	struct ldb_message_element *allowedClasses;
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	const struct dsdb_schema *schema = dsdb_get_schema(ldb);
	const struct dsdb_class *sclass;
	int i, j, ret;

	/* If we don't have a schema yet, we can't do anything... */
	if (schema == NULL) {
		return LDB_SUCCESS;
	}

	/* Must remove any existing attribute, or else confusion reins */
	ldb_msg_remove_attr(msg, attrName);
	ret = ldb_msg_add_empty(msg, attrName, 0, &allowedClasses);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	oc_el = ldb_msg_find_element(sd_msg, "objectClass");

	for (i=0; oc_el && i < oc_el->num_values; i++) {
		sclass = dsdb_class_by_lDAPDisplayName_ldb_val(schema, &oc_el->values[i]);
		if (!sclass) {
			/* We don't know this class?  what is going on? */
			continue;
		}

		for (j=0; sclass->possibleInferiors && sclass->possibleInferiors[j]; j++) {
			ldb_msg_add_string(msg, attrName, sclass->possibleInferiors[j]);
		}
	}
	if (allowedClasses->num_values > 1) {
		qsort(allowedClasses->values,
		      allowedClasses->num_values,
		      sizeof(*allowedClasses->values),
		      (comparison_fn_t)data_blob_cmp);

		for (i=1 ; i < allowedClasses->num_values; i++) {
			struct ldb_val *val1 = &allowedClasses->values[i-1];
			struct ldb_val *val2 = &allowedClasses->values[i];
			if (data_blob_cmp(val1, val2) == 0) {
				memmove(val1, val2, (allowedClasses->num_values - i) * sizeof(struct ldb_val));
				allowedClasses->num_values--;
				i--;
			}
		}
	}

	return LDB_SUCCESS;
}

static int acl_childClassesEffective(struct ldb_module *module,
				     struct ldb_message *sd_msg,
				     struct ldb_message *msg,
				     struct acl_context *ac)
{
	struct ldb_message_element *oc_el;
	struct ldb_message_element *allowedClasses = NULL;
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	const struct dsdb_schema *schema = dsdb_get_schema(ldb);
	const struct dsdb_class *sclass;
	struct security_descriptor *sd;
	struct dom_sid *sid = NULL;
	int i, j, ret;

	if (ac->user_type == SECURITY_SYSTEM) {
		return acl_childClasses(module, sd_msg, msg, "allowedChildClassesEffective");
	}

	/* If we don't have a schema yet, we can't do anything... */
	if (schema == NULL) {
		return LDB_SUCCESS;
	}

	/* Must remove any existing attribute, or else confusion reins */
	ldb_msg_remove_attr(msg, "allowedChildClassesEffective");

	oc_el = ldb_msg_find_element(sd_msg, "objectClass");
	ret = get_sd_from_ldb_message(msg, sd_msg, &sd);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	ret = get_dom_sid_from_ldb_message(msg, sd_msg, &sid);

	if (ret != LDB_SUCCESS) {
		return ret;
	}
	for (i=0; oc_el && i < oc_el->num_values; i++) {
		sclass = dsdb_class_by_lDAPDisplayName_ldb_val(schema, &oc_el->values[i]);
		if (!sclass) {
			/* We don't know this class?  what is going on? */
			continue;
		}

		for (j=0; sclass->possibleInferiors && sclass->possibleInferiors[j]; j++) {
			ret = acl_check_access_on_class(module,
							msg,
							sd,
							sid,
							SEC_ADS_CREATE_CHILD,
							sclass->possibleInferiors[j]);
			if (ret == LDB_SUCCESS) {
				ldb_msg_add_string(msg, "allowedChildClassesEffective",
						   sclass->possibleInferiors[j]);
			}
		}
	}
	allowedClasses = ldb_msg_find_element(msg, "allowedChildClassesEffective");
	if (!allowedClasses) {
		return LDB_SUCCESS;
	}

	if (allowedClasses->num_values > 1) {
		qsort(allowedClasses->values,
		      allowedClasses->num_values,
		      sizeof(*allowedClasses->values),
		      (comparison_fn_t)data_blob_cmp);
		for (i=1 ; i < allowedClasses->num_values; i++) {
			struct ldb_val *val1 = &allowedClasses->values[i-1];
			struct ldb_val *val2 = &allowedClasses->values[i];
			if (data_blob_cmp(val1, val2) == 0) {
				memmove(val1, val2, (allowedClasses->num_values - i) * sizeof( struct ldb_val));
				allowedClasses->num_values--;
				i--;
			}
		}
	}
	return LDB_SUCCESS;
}

static int acl_sDRightsEffective(struct ldb_module *module,
				 struct ldb_message *sd_msg,
				 struct ldb_message *msg,
				 struct acl_context *ac)
{
	struct ldb_message_element *rightsEffective;
	int ret;
	struct security_descriptor *sd;
	struct dom_sid *sid = NULL;
	uint32_t flags = 0;

	/* Must remove any existing attribute, or else confusion reins */
	ldb_msg_remove_attr(msg, "sDRightsEffective");
	ret = ldb_msg_add_empty(msg, "sDRightsEffective", 0, &rightsEffective);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	if (ac->user_type == SECURITY_SYSTEM) {
		flags = SECINFO_OWNER | SECINFO_GROUP |  SECINFO_SACL |  SECINFO_DACL;
	}
	else {
		/* Get the security descriptor from the message */
		ret = get_sd_from_ldb_message(msg, sd_msg, &sd);
		if (ret != LDB_SUCCESS) {
			return ret;
		}
		ret = get_dom_sid_from_ldb_message(msg, sd_msg, &sid);

		if (ret != LDB_SUCCESS) {
			return ret;
		}
		ret = acl_check_access_on_attribute(module,
						    msg,
						    sd,
						    sid,
						    SEC_STD_WRITE_OWNER,
						    NULL);
		if (ret == LDB_SUCCESS) {
			flags |= SECINFO_OWNER | SECINFO_GROUP;
		}
		ret = acl_check_access_on_attribute(module,
						    msg,
						    sd,
						    sid,
						    SEC_STD_WRITE_DAC,
						    NULL);
		if (ret == LDB_SUCCESS) {
			flags |= SECINFO_DACL;
		}
		ret = acl_check_access_on_attribute(module,
						    msg,
						    sd,
						    sid,
						    SEC_FLAG_SYSTEM_SECURITY,
						    NULL);
		if (ret == LDB_SUCCESS) {
			flags |= SECINFO_SACL;
		}
	}
	ldb_msg_add_fmt(msg, "sDRightsEffective", "%u", flags);
	return LDB_SUCCESS;
}

static int acl_add(struct ldb_module *module, struct ldb_request *req)
{
	int ret;
	struct ldb_dn *parent = ldb_dn_get_parent(req, req->op.add.message->dn);
	struct ldb_context *ldb;
	struct ldb_message_element *oc_el;
	const struct GUID *guid;
	struct object_tree *root = NULL;
	struct object_tree *new_node = NULL;

	if (what_is_user(module) == SECURITY_SYSTEM) {
		return ldb_next_request(module, req);
	}

	if (ldb_dn_is_special(req->op.add.message->dn)) {
		return ldb_next_request(module, req);
	}
	ldb = ldb_module_get_ctx(module);
	/* Creating an NC. There is probably something we should do here,
	 * but we will establish that later */
	if ((ldb_dn_compare(req->op.add.message->dn, (ldb_get_schema_basedn(ldb))) == 0) ||
	    (ldb_dn_compare(req->op.add.message->dn, (ldb_get_config_basedn(ldb))) == 0) ||
	    (ldb_dn_compare(req->op.add.message->dn, (ldb_get_root_basedn(ldb))) == 0)) {
		return ldb_next_request(module, req);
	}

	oc_el = ldb_msg_find_element(req->op.add.message, "objectClass");
	if (!oc_el || oc_el->num_values == 0) {
		DEBUG(10,("acl:operation error %s\n", ldb_dn_get_linearized(req->op.add.message->dn)));
		return ldb_module_done(req, NULL, NULL, LDB_ERR_OPERATIONS_ERROR);
	}

	guid = class_schemaid_guid_by_lDAPDisplayName(dsdb_get_schema(ldb),
						      (char *)oc_el->values[oc_el->num_values-1].data);

	if (!insert_in_object_tree(req, guid, SEC_ADS_CREATE_CHILD, &root, &new_node)) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	ret = check_access_on_dn(module, req, parent, SEC_ADS_CREATE_CHILD, root);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	return ldb_next_request(module, req);
}

static int acl_modify(struct ldb_module *module, struct ldb_request *req)
{
	int ret;
	struct ldb_context *ldb = ldb_module_get_ctx(module);
	const struct dsdb_schema *schema = dsdb_get_schema(ldb);
	int i;
	bool modify_sd = false;
	const struct GUID *guid;
	uint32_t access_granted;
	struct object_tree *root = NULL;
	struct object_tree *new_node = NULL;
	NTSTATUS status;
	struct ldb_result *acl_res;
	struct security_descriptor *sd;
	struct dom_sid *sid = NULL;
	TALLOC_CTX *tmp_ctx = talloc_new(req);
	static const char *acl_attrs[] = {
		"nTSecurityDescriptor",
		"objectClass",
		"objectSid",
		NULL
	};

	/* Don't print this debug statement if elements[0].name is going to be NULL */
	if(req->op.mod.message->num_elements > 0)
	{
		DEBUG(10, ("ldb:acl_modify: %s\n", req->op.mod.message->elements[0].name));
	}
	if (what_is_user(module) == SECURITY_SYSTEM) {
		return ldb_next_request(module, req);
	}
	if (ldb_dn_is_special(req->op.mod.message->dn)) {
		return ldb_next_request(module, req);
	}
	ret = ldb_search(ldb, req, &acl_res, req->op.mod.message->dn,
			 LDB_SCOPE_BASE, acl_attrs, NULL);

	if (ret != LDB_SUCCESS) {
		return ret;
	}

	ret = get_sd_from_ldb_message(req, acl_res->msgs[0], &sd);
	if (ret != LDB_SUCCESS) {
		DEBUG(10, ("acl_modify: cannot get descriptor\n"));
		return ret;
	}
	/* Theoretically we pass the check if the object has no sd */
	if (!sd) {
		return LDB_SUCCESS;
	}

	guid = get_oc_guid_from_message(module,acl_res->msgs[0]);
	if (!guid) {
		DEBUG(10, ("acl_modify: cannot get guid\n"));
		goto fail;
	}

	ret = get_dom_sid_from_ldb_message(req, acl_res->msgs[0], &sid);
	if (ret != LDB_SUCCESS) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	if (!insert_in_object_tree(tmp_ctx, guid, SEC_ADS_WRITE_PROP,
				   &root, &new_node)) {
		DEBUG(10, ("acl_modify: cannot add to object tree\n"));
		goto fail;
	}
	for (i=0; i < req->op.mod.message->num_elements; i++){
		const struct dsdb_attribute *attr;
		/* clearTextPassword is not in schema */
		if (strcmp("clearTextPassword", req->op.mod.message->elements[i].name) == 0) {
			attr = dsdb_attribute_by_lDAPDisplayName(schema, "unicodePwd");
		} else {
			attr = dsdb_attribute_by_lDAPDisplayName(schema,
								 req->op.mod.message->elements[i].name);
		}
		if (strcmp("nTSecurityDescriptor", req->op.mod.message->elements[i].name) == 0) {
			modify_sd = true;
		} else {

			if (!attr) {
				DEBUG(10, ("acl_modify: cannot find attribute %s\n",
					   req->op.mod.message->elements[i].name));
				goto fail;
			}
			if (!insert_in_object_tree(tmp_ctx,
						   &attr->attributeSecurityGUID, SEC_ADS_WRITE_PROP,
						   &new_node, &new_node)) {
				DEBUG(10, ("acl_modify: cannot add to object tree securityGUID\n"));
				goto fail;
			}

			if (!insert_in_object_tree(tmp_ctx,
						   &attr->schemaIDGUID, SEC_ADS_WRITE_PROP, &new_node, &new_node)) {
				DEBUG(10, ("acl_modify: cannot add to object tree attributeGUID\n"));
				goto fail;
			}
		}
	}

	if (root->num_of_children > 0) {
		status = sec_access_check_ds(sd, acl_user_token(module),
					     SEC_ADS_WRITE_PROP,
					     &access_granted,
					     root,
					     sid);

		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(10, ("Object %s nas no write property access\n",
				   ldb_dn_get_linearized(req->op.mod.message->dn)));
			acl_debug(sd,
				  acl_user_token(module),
				  req->op.mod.message->dn,
				  true,
				  10);
			talloc_free(tmp_ctx);
			return LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS;
		}
	}
	if (modify_sd) {
		status = sec_access_check_ds(sd, acl_user_token(module),
					     SEC_STD_WRITE_DAC,
					     &access_granted,
					     NULL,
					     sid);

		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(10, ("Object %s nas no write dacl access\n",
				   ldb_dn_get_linearized(req->op.mod.message->dn)));
			acl_debug(sd,
				  acl_user_token(module),
				  req->op.mod.message->dn,
				  true,
				  10);
			talloc_free(tmp_ctx);
			return LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS;
		}
	}

	talloc_free(tmp_ctx);
	return ldb_next_request(module, req);
fail:
	talloc_free(tmp_ctx);
	return LDB_ERR_OPERATIONS_ERROR;
}

/* similar to the modify for the time being.
 * We need to concider the special delete tree case, though - TODO */
static int acl_delete(struct ldb_module *module, struct ldb_request *req)
{
	int ret;
	struct ldb_dn *parent = ldb_dn_get_parent(req, req->op.del.dn);
	struct ldb_context *ldb;

	DEBUG(10, ("ldb:acl_delete: %s\n", ldb_dn_get_linearized(req->op.del.dn)));
	if (what_is_user(module) == SECURITY_SYSTEM) {
		return ldb_next_request(module, req);
	}

	if (ldb_dn_is_special(req->op.del.dn)) {
		return ldb_next_request(module, req);
	}
	ldb = ldb_module_get_ctx(module);
	/* first check if we have delete object right */
	ret = check_access_on_dn(module, req, req->op.del.dn, SEC_STD_DELETE, NULL);
	if (ret == LDB_SUCCESS) {
		return ldb_next_request(module, req);
	}

	/* Nope, we don't have delete object. Lets check if we have delete child on the parent */
	/* No parent, so check fails */
	if ((ldb_dn_compare(req->op.del.dn, (ldb_get_schema_basedn(ldb))) == 0) ||
	    (ldb_dn_compare(req->op.del.dn, (ldb_get_config_basedn(ldb))) == 0) ||
	    (ldb_dn_compare(req->op.del.dn, (ldb_get_root_basedn(ldb))) == 0)) {
		DEBUG(10,("acl:deleting an NC\n"));
		return ldb_module_done(req, NULL, NULL, LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS);
	}

	ret = check_access_on_dn(module, req, parent, SEC_ADS_DELETE_CHILD, NULL);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	return ldb_next_request(module, req);
}

static int acl_rename(struct ldb_module *module, struct ldb_request *req)
{
	int ret;
	struct ldb_dn *oldparent = ldb_dn_get_parent(req, req->op.rename.olddn);
	struct ldb_dn *newparent = ldb_dn_get_parent(req, req->op.rename.newdn);
	struct ldb_context *ldb;
	struct security_descriptor *sd = NULL;
	struct dom_sid *sid = NULL;
	struct ldb_result *acl_res;
	const struct GUID *guid;
	struct object_tree *root = NULL;
	struct object_tree *new_node = NULL;
	TALLOC_CTX *tmp_ctx = talloc_new(req);
	NTSTATUS status;
	uint32_t access_granted;
	static const char *acl_attrs[] = {
		"nTSecurityDescriptor",
		"objectClass",
		"objectSid",
		NULL
	};

	DEBUG(10, ("ldb:acl_rename: %s\n", ldb_dn_get_linearized(req->op.rename.olddn)));
	if (what_is_user(module) == SECURITY_SYSTEM) {
		return ldb_next_request(module, req);
	}
	if (ldb_dn_is_special(req->op.rename.olddn)) {
		return ldb_next_request(module, req);
	}
	ldb = ldb_module_get_ctx(module);

	/* TODO search to include deleted objects */
	ret = ldb_search(ldb, req, &acl_res, req->op.rename.olddn,
			 LDB_SCOPE_BASE, acl_attrs, NULL);
	/* we sould be able to find the parent */
	if (ret != LDB_SUCCESS) {
		DEBUG(10,("acl: failed to find object %s\n",
			  ldb_dn_get_linearized(req->op.rename.olddn)));
		return ret;
	}

	guid = get_oc_guid_from_message(module,acl_res->msgs[0]);
	if (!insert_in_object_tree(tmp_ctx, guid, SEC_ADS_WRITE_PROP,
				   &root, &new_node)) {
		return LDB_ERR_OPERATIONS_ERROR;
	};

	guid = attribute_schemaid_guid_by_lDAPDisplayName(dsdb_get_schema(ldb),
							  "name");
	if (!insert_in_object_tree(tmp_ctx, guid, SEC_ADS_WRITE_PROP,
				   &new_node, &new_node)) {
		return LDB_ERR_OPERATIONS_ERROR;
	};

	ret = get_sd_from_ldb_message(req, acl_res->msgs[0], &sd);

	if (ret != LDB_SUCCESS) {
		return LDB_ERR_OPERATIONS_ERROR;
	}
	/* Theoretically we pass the check if the object has no sd */
	if (!sd) {
		return LDB_SUCCESS;
	}
	ret = get_dom_sid_from_ldb_message(req, acl_res->msgs[0], &sid);
	if (ret != LDB_SUCCESS) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	status = sec_access_check_ds(sd, acl_user_token(module),
				     SEC_ADS_WRITE_PROP,
				     &access_granted,
				     root,
				     sid);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10, ("Object %s nas no wp on name\n",
			   ldb_dn_get_linearized(req->op.rename.olddn)));
		acl_debug(sd,
			  acl_user_token(module),
			  req->op.rename.olddn,
			  true,
			  10);
		return LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS;
	}

	if (ldb_dn_compare(oldparent, newparent) == 0) {
		/* regular rename, not move, nothing more to do */
		return ldb_next_request(module, req);
	}

	/* What exactly to do in this case? It would fail anyway.. */
	if ((ldb_dn_compare(req->op.rename.newdn, (ldb_get_schema_basedn(ldb))) == 0) ||
	    (ldb_dn_compare(req->op.rename.newdn, (ldb_get_config_basedn(ldb))) == 0) ||
	    (ldb_dn_compare(req->op.rename.newdn, (ldb_get_root_basedn(ldb))) == 0)) {
		DEBUG(10,("acl:moving as an NC\n"));
		return LDB_ERR_INSUFFICIENT_ACCESS_RIGHTS;
	}
	/* new parent should have create child */
	talloc_free(tmp_ctx);
	tmp_ctx = talloc_new(req);
	root = NULL;
	new_node = NULL;
	guid = get_oc_guid_from_message(module,acl_res->msgs[0]);
	if (!guid) {
		DEBUG(10,("acl:renamed object has no object class\n"));
		return ldb_module_done(req, NULL, NULL,  LDB_ERR_OPERATIONS_ERROR);
	}
	if (!insert_in_object_tree(tmp_ctx, guid, SEC_ADS_CREATE_CHILD,
				   &root, &new_node)) {
		return LDB_ERR_OPERATIONS_ERROR;
	}
	ret = check_access_on_dn(module, req, newparent, SEC_ADS_CREATE_CHILD, root);
	if (ret != LDB_SUCCESS) {
		DEBUG(10,("acl:access_denied renaming %s", ldb_dn_get_linearized(req->op.rename.olddn)));
		return ret;
	}
	/* do we have delete object on the object? */

	status = sec_access_check_ds(sd, acl_user_token(module),
				     SEC_STD_DELETE,
				     &access_granted,
				     NULL,
				     sid);

	if (NT_STATUS_IS_OK(status)) {
		return ldb_next_request(module, req);
	}
	/* what about delete child on the current parent */
	ret = check_access_on_dn(module, req, oldparent, SEC_ADS_DELETE_CHILD, NULL);
	if (ret != LDB_SUCCESS) {
		DEBUG(10,("acl:access_denied renaming %s", ldb_dn_get_linearized(req->op.rename.olddn)));
		return ldb_module_done(req, NULL, NULL, ret);
	}
	return ldb_next_request(module, req);
}

static int acl_search_callback(struct ldb_request *req, struct ldb_reply *ares)
{
	struct ldb_context *ldb;
	struct acl_context *ac;
	struct acl_private *data;
	struct ldb_result *acl_res;
	static const char *acl_attrs[] = {
		"objectClass",
		"nTSecurityDescriptor",
		"objectSid",
		NULL
	};
	int ret, i;

	ac = talloc_get_type(req->context, struct acl_context);
	data = talloc_get_type(ldb_module_get_private(ac->module), struct acl_private);
	ldb = ldb_module_get_ctx(ac->module);

	if (!ares) {
		return ldb_module_done(ac->req, NULL, NULL,
				       LDB_ERR_OPERATIONS_ERROR);
	}
	if (ares->error != LDB_SUCCESS) {
		return ldb_module_done(ac->req, ares->controls,
				       ares->response, ares->error);
	}

	switch (ares->type) {
	case LDB_REPLY_ENTRY:
		if (ac->allowedAttributes 
		    || ac->allowedChildClasses
		    || ac->allowedChildClassesEffective
		    || ac->allowedAttributesEffective
		    || ac->sDRightsEffective) {
			ret = ldb_search(ldb, ac, &acl_res, ares->message->dn, LDB_SCOPE_BASE, acl_attrs, NULL);
			if (ret != LDB_SUCCESS) {
				return ldb_module_done(ac->req, NULL, NULL, ret);
			}
			if (ac->allowedAttributes || ac->allowedAttributesEffective) {
				ret = acl_allowedAttributes(ac->module, acl_res->msgs[0], ares->message, ac);
				if (ret != LDB_SUCCESS) {
					return ldb_module_done(ac->req, NULL, NULL, ret);
				}
			}
			if (ac->allowedChildClasses) {
				ret = acl_childClasses(ac->module, acl_res->msgs[0],
						       ares->message, "allowedChildClasses");
				if (ret != LDB_SUCCESS) {
					return ldb_module_done(ac->req, NULL, NULL, ret);
				}
			}
			if (ac->allowedChildClassesEffective) {
				ret = acl_childClassesEffective(ac->module,
								acl_res->msgs[0], ares->message, ac);
				if (ret != LDB_SUCCESS) {
					return ldb_module_done(ac->req, NULL, NULL, ret);
				}
			}
			if (ac->sDRightsEffective) {
				ret = acl_sDRightsEffective(ac->module,
							    acl_res->msgs[0], ares->message, ac);
				if (ret != LDB_SUCCESS) {
					return ldb_module_done(ac->req, NULL, NULL, ret);
				}
			}
		}
		if (data && data->password_attrs) {
			if (ac->user_type != SECURITY_SYSTEM) {
				for (i = 0; data->password_attrs[i]; i++) {
					ldb_msg_remove_attr(ares->message, data->password_attrs[i]);
				}
			}
		}
		return ldb_module_send_entry(ac->req, ares->message, ares->controls);

	case LDB_REPLY_REFERRAL:
		return ldb_module_send_referral(ac->req, ares->referral);

	case LDB_REPLY_DONE:
		return ldb_module_done(ac->req, ares->controls,
				       ares->response, LDB_SUCCESS);

	}
	return LDB_SUCCESS;
}

static int acl_search(struct ldb_module *module, struct ldb_request *req)
{
	struct ldb_context *ldb;
	struct acl_context *ac;
	struct ldb_request *down_req;
	struct acl_private *data;
	int ret, i;

	ldb = ldb_module_get_ctx(module);

	ac = talloc_zero(req, struct acl_context);
	if (ac == NULL) {
		ldb_oom(ldb);
		return LDB_ERR_OPERATIONS_ERROR;
	}
	data = talloc_get_type(ldb_module_get_private(module), struct acl_private);

	ac->module = module;
	ac->req = req;
	ac->user_type = what_is_user(module);
	ac->allowedAttributes = ldb_attr_in_list(req->op.search.attrs, "allowedAttributes");
	ac->allowedAttributesEffective = ldb_attr_in_list(req->op.search.attrs, "allowedAttributesEffective");
	ac->allowedChildClasses = ldb_attr_in_list(req->op.search.attrs, "allowedChildClasses");
	ac->allowedChildClassesEffective = ldb_attr_in_list(req->op.search.attrs, "allowedChildClassesEffective");
	ac->sDRightsEffective = ldb_attr_in_list(req->op.search.attrs, "sDRightsEffective");

	/* replace any attributes in the parse tree that are private,
	   so we don't allow a search for 'userPassword=penguin',
	   just as we would not allow that attribute to be returned */
	if (ac->user_type != SECURITY_SYSTEM) {
		/* FIXME: We should copy the tree and keep the original unmodified. */
		/* remove password attributes */
		if (data && data->password_attrs) {
			for (i = 0; data->password_attrs[i]; i++) {
				ldb_parse_tree_attr_replace(req->op.search.tree,
							    data->password_attrs[i],
							    "kludgeACLredactedattribute");
			}
		}
	}
	ret = ldb_build_search_req_ex(&down_req,
				      ldb, ac,
				      req->op.search.base,
				      req->op.search.scope,
				      req->op.search.tree,
				      req->op.search.attrs,
				      req->controls,
				      ac, acl_search_callback,
				      req);
	if (ret != LDB_SUCCESS) {
		return ret;
	}
	/* perform the search */
	return ldb_next_request(module, down_req);
}

_PUBLIC_ const struct ldb_module_ops ldb_acl_module_ops = {
	.name		   = "acl",
	.search            = acl_search,
	.add               = acl_add,
	.modify            = acl_modify,
	.del               = acl_delete,
	.rename            = acl_rename,
	.init_context	   = acl_module_init
};
