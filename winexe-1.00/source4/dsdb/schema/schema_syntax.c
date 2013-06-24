/* 
   Unix SMB/CIFS mplementation.
   DSDB schema syntaxes
   
   Copyright (C) Stefan Metzmacher <metze@samba.org> 2006
   Copyright (C) Simo Sorce 2005
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2008

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
#include "dsdb/samdb/samdb.h"
#include "librpc/gen_ndr/ndr_drsuapi.h"
#include "librpc/gen_ndr/ndr_security.h"
#include "librpc/gen_ndr/ndr_misc.h"
#include "lib/ldb/include/ldb.h"
#include "lib/ldb/include/ldb_errors.h"
#include "system/time.h"
#include "../lib/util/charset/charset.h"
#include "librpc/ndr/libndr.h"

static WERROR dsdb_syntax_FOOBAR_drsuapi_to_ldb(struct ldb_context *ldb, 
						const struct dsdb_schema *schema,
						const struct dsdb_attribute *attr,
						const struct drsuapi_DsReplicaAttribute *in,
						TALLOC_CTX *mem_ctx,
						struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		str = talloc_asprintf(out->values, "%s: not implemented",
				      attr->syntax->name);
		W_ERROR_HAVE_NO_MEMORY(str);

		out->values[i] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_FOOBAR_ldb_to_drsuapi(struct ldb_context *ldb, 
						const struct dsdb_schema *schema,
						const struct dsdb_attribute *attr,
						const struct ldb_message_element *in,
						TALLOC_CTX *mem_ctx,
						struct drsuapi_DsReplicaAttribute *out)
{
	return WERR_FOOBAR;
}

static WERROR dsdb_syntax_BOOL_drsuapi_to_ldb(struct ldb_context *ldb, 
					      const struct dsdb_schema *schema,
					      const struct dsdb_attribute *attr,
					      const struct drsuapi_DsReplicaAttribute *in,
					      TALLOC_CTX *mem_ctx,
					      struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		uint32_t v;
		char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length != 4) {
			return WERR_FOOBAR;
		}

		v = IVAL(in->value_ctr.values[i].blob->data, 0);

		if (v != 0) {
			str = talloc_strdup(out->values, "TRUE");
			W_ERROR_HAVE_NO_MEMORY(str);
		} else {
			str = talloc_strdup(out->values, "FALSE");
			W_ERROR_HAVE_NO_MEMORY(str);
		}

		out->values[i] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_BOOL_ldb_to_drsuapi(struct ldb_context *ldb, 
					      const struct dsdb_schema *schema,
					      const struct dsdb_attribute *attr,
					      const struct ldb_message_element *in,
					      TALLOC_CTX *mem_ctx,
					      struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		out->value_ctr.values[i].blob	= &blobs[i];

		blobs[i] = data_blob_talloc(blobs, NULL, 4);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		if (strcmp("TRUE", (const char *)in->values[i].data) == 0) {
			SIVAL(blobs[i].data, 0, 0x00000001);
		} else if (strcmp("FALSE", (const char *)in->values[i].data) == 0) {
			SIVAL(blobs[i].data, 0, 0x00000000);
		} else {
			return WERR_FOOBAR;
		}
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_INT32_drsuapi_to_ldb(struct ldb_context *ldb, 
					       const struct dsdb_schema *schema,
					       const struct dsdb_attribute *attr,
					       const struct drsuapi_DsReplicaAttribute *in,
					       TALLOC_CTX *mem_ctx,
					       struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		int32_t v;
		char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length != 4) {
			return WERR_FOOBAR;
		}

		v = IVALS(in->value_ctr.values[i].blob->data, 0);

		str = talloc_asprintf(out->values, "%d", v);
		W_ERROR_HAVE_NO_MEMORY(str);

		out->values[i] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_INT32_ldb_to_drsuapi(struct ldb_context *ldb, 
					       const struct dsdb_schema *schema,
					       const struct dsdb_attribute *attr,
					       const struct ldb_message_element *in,
					       TALLOC_CTX *mem_ctx,
					       struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		int32_t v;

		out->value_ctr.values[i].blob	= &blobs[i];

		blobs[i] = data_blob_talloc(blobs, NULL, 4);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		/* We've to use "strtoll" here to have the intended overflows.
		 * Otherwise we may get "LONG_MAX" and the conversion is wrong. */
		v = (int32_t) strtoll((char *)in->values[i].data, NULL, 0);

		SIVALS(blobs[i].data, 0, v);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_INT64_drsuapi_to_ldb(struct ldb_context *ldb, 
					       const struct dsdb_schema *schema,
					       const struct dsdb_attribute *attr,
					       const struct drsuapi_DsReplicaAttribute *in,
					       TALLOC_CTX *mem_ctx,
					       struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		int64_t v;
		char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length != 8) {
			return WERR_FOOBAR;
		}

		v = BVALS(in->value_ctr.values[i].blob->data, 0);

		str = talloc_asprintf(out->values, "%lld", (long long int)v);
		W_ERROR_HAVE_NO_MEMORY(str);

		out->values[i] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_INT64_ldb_to_drsuapi(struct ldb_context *ldb, 
					       const struct dsdb_schema *schema,
					       const struct dsdb_attribute *attr,
					       const struct ldb_message_element *in,
					       TALLOC_CTX *mem_ctx,
					       struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		int64_t v;

		out->value_ctr.values[i].blob	= &blobs[i];

		blobs[i] = data_blob_talloc(blobs, NULL, 8);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		v = strtoll((const char *)in->values[i].data, NULL, 10);

		SBVALS(blobs[i].data, 0, v);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_NTTIME_UTC_drsuapi_to_ldb(struct ldb_context *ldb, 
						    const struct dsdb_schema *schema,
						    const struct dsdb_attribute *attr,
						    const struct drsuapi_DsReplicaAttribute *in,
						    TALLOC_CTX *mem_ctx,
						    struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		NTTIME v;
		time_t t;
		char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length != 8) {
			return WERR_FOOBAR;
		}

		v = BVAL(in->value_ctr.values[i].blob->data, 0);
		v *= 10000000;
		t = nt_time_to_unix(v);

		/* 
		 * NOTE: On a w2k3 server you can set a GeneralizedTime string
		 *       via LDAP, but you get back an UTCTime string,
		 *       but via DRSUAPI you get back the NTTIME_1sec value
		 *       that represents the GeneralizedTime value!
		 *
		 *       So if we store the UTCTime string in our ldb
		 *       we'll loose information!
		 */
		str = ldb_timestring_utc(out->values, t); 
		W_ERROR_HAVE_NO_MEMORY(str);
		out->values[i] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_NTTIME_UTC_ldb_to_drsuapi(struct ldb_context *ldb, 
						    const struct dsdb_schema *schema,
						    const struct dsdb_attribute *attr,
						    const struct ldb_message_element *in,
						    TALLOC_CTX *mem_ctx,
						    struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		NTTIME v;
		time_t t;

		out->value_ctr.values[i].blob	= &blobs[i];

		blobs[i] = data_blob_talloc(blobs, NULL, 8);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		t = ldb_string_utc_to_time((const char *)in->values[i].data);
		unix_to_nt_time(&v, t);
		v /= 10000000;

		SBVAL(blobs[i].data, 0, v);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_NTTIME_drsuapi_to_ldb(struct ldb_context *ldb, 
						const struct dsdb_schema *schema,
						const struct dsdb_attribute *attr,
						const struct drsuapi_DsReplicaAttribute *in,
						TALLOC_CTX *mem_ctx,
						struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		NTTIME v;
		time_t t;
		char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length != 8) {
			return WERR_FOOBAR;
		}

		v = BVAL(in->value_ctr.values[i].blob->data, 0);
		v *= 10000000;
		t = nt_time_to_unix(v);

		str = ldb_timestring(out->values, t); 
		W_ERROR_HAVE_NO_MEMORY(str);

		out->values[i] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_NTTIME_ldb_to_drsuapi(struct ldb_context *ldb, 
						const struct dsdb_schema *schema,
						const struct dsdb_attribute *attr,
						const struct ldb_message_element *in,
						TALLOC_CTX *mem_ctx,
						struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		NTTIME v;
		time_t t;

		out->value_ctr.values[i].blob	= &blobs[i];

		blobs[i] = data_blob_talloc(blobs, NULL, 8);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		t = ldb_string_to_time((const char *)in->values[i].data);
		unix_to_nt_time(&v, t);
		v /= 10000000;

		SBVAL(blobs[i].data, 0, v);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_DATA_BLOB_drsuapi_to_ldb(struct ldb_context *ldb, 
						   const struct dsdb_schema *schema,
						   const struct dsdb_attribute *attr,
						   const struct drsuapi_DsReplicaAttribute *in,
						   TALLOC_CTX *mem_ctx,
						   struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length == 0) {
			return WERR_FOOBAR;
		}

		out->values[i] = data_blob_dup_talloc(out->values,
						      in->value_ctr.values[i].blob);
		W_ERROR_HAVE_NO_MEMORY(out->values[i].data);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_DATA_BLOB_ldb_to_drsuapi(struct ldb_context *ldb, 
						   const struct dsdb_schema *schema,
						   const struct dsdb_attribute *attr,
						   const struct ldb_message_element *in,
						   TALLOC_CTX *mem_ctx,
						   struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		out->value_ctr.values[i].blob	= &blobs[i];

		blobs[i] = data_blob_dup_talloc(blobs, &in->values[i]);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);
	}

	return WERR_OK;
}

static WERROR _dsdb_syntax_OID_obj_drsuapi_to_ldb(struct ldb_context *ldb, 
						  const struct dsdb_schema *schema,
						  const struct dsdb_attribute *attr,
						  const struct drsuapi_DsReplicaAttribute *in,
						  TALLOC_CTX *mem_ctx,
						  struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		uint32_t v;
		const struct dsdb_class *c;
		const char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length != 4) {
			return WERR_FOOBAR;
		}

		v = IVAL(in->value_ctr.values[i].blob->data, 0);

		c = dsdb_class_by_governsID_id(schema, v);
		if (!c) {
			return WERR_FOOBAR;
		}

		str = talloc_strdup(out->values, c->lDAPDisplayName);
		W_ERROR_HAVE_NO_MEMORY(str);

		/* the values need to be reversed */
		out->values[out->num_values - (i + 1)] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR _dsdb_syntax_OID_attr_drsuapi_to_ldb(struct ldb_context *ldb, 
						   const struct dsdb_schema *schema,
						   const struct dsdb_attribute *attr,
						   const struct drsuapi_DsReplicaAttribute *in,
						   TALLOC_CTX *mem_ctx,
						   struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		uint32_t v;
		const struct dsdb_attribute *a;
		const char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length != 4) {
			return WERR_FOOBAR;
		}

		v = IVAL(in->value_ctr.values[i].blob->data, 0);

		a = dsdb_attribute_by_attributeID_id(schema, v);
		if (!a) {
			return WERR_FOOBAR;
		}

		str = talloc_strdup(out->values, a->lDAPDisplayName);
		W_ERROR_HAVE_NO_MEMORY(str);

		/* the values need to be reversed */
		out->values[out->num_values - (i + 1)] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR _dsdb_syntax_OID_oid_drsuapi_to_ldb(struct ldb_context *ldb, 
						  const struct dsdb_schema *schema,
						  const struct dsdb_attribute *attr,
						  const struct drsuapi_DsReplicaAttribute *in,
						  TALLOC_CTX *mem_ctx,
						  struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		uint32_t attid;
		WERROR status;
		const char *oid;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length != 4) {
			return WERR_FOOBAR;
		}

		attid = IVAL(in->value_ctr.values[i].blob->data, 0);

		status = dsdb_schema_pfm_oid_from_attid(schema->prefixmap, attid, out->values, &oid);
		W_ERROR_NOT_OK_RETURN(status);

		out->values[i] = data_blob_string_const(oid);
	}

	return WERR_OK;
}

static WERROR _dsdb_syntax_OID_obj_ldb_to_drsuapi(struct ldb_context *ldb,
						  const struct dsdb_schema *schema,
						  const struct dsdb_attribute *attr,
						  const struct ldb_message_element *in,
						  TALLOC_CTX *mem_ctx,
						  struct drsuapi_DsReplicaAttribute *out)
{
        uint32_t i;
        DATA_BLOB *blobs;

        out->attid= attr->attributeID_id;
        out->value_ctr.num_values= in->num_values;
        out->value_ctr.values= talloc_array(mem_ctx,
                                            struct drsuapi_DsAttributeValue,
                                            in->num_values);
        W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

        blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
        W_ERROR_HAVE_NO_MEMORY(blobs);

        for (i=0; i < in->num_values; i++) {
		const struct dsdb_class *obj_class;

		out->value_ctr.values[i].blob= &blobs[i];

		blobs[i] = data_blob_talloc(blobs, NULL, 4);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		obj_class = dsdb_class_by_lDAPDisplayName(schema, (const char *)in->values[i].data);
		if (!obj_class) {
			return WERR_FOOBAR;
		}
		SIVAL(blobs[i].data, 0, obj_class->governsID_id);
        }


        return WERR_OK;
}

static WERROR _dsdb_syntax_OID_attr_ldb_to_drsuapi(struct ldb_context *ldb,
						   const struct dsdb_schema *schema,
						   const struct dsdb_attribute *attr,
						   const struct ldb_message_element *in,
						   TALLOC_CTX *mem_ctx,
						   struct drsuapi_DsReplicaAttribute *out)
{
        uint32_t i;
        DATA_BLOB *blobs;

        out->attid= attr->attributeID_id;
        out->value_ctr.num_values= in->num_values;
        out->value_ctr.values= talloc_array(mem_ctx,
                                            struct drsuapi_DsAttributeValue,
                                            in->num_values);
        W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

        blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
        W_ERROR_HAVE_NO_MEMORY(blobs);

        for (i=0; i < in->num_values; i++) {
		const struct dsdb_attribute *obj_attr;

		out->value_ctr.values[i].blob= &blobs[i];

		blobs[i] = data_blob_talloc(blobs, NULL, 4);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		obj_attr = dsdb_attribute_by_lDAPDisplayName(schema, (const char *)in->values[i].data);
		if (!obj_attr) {
			return WERR_FOOBAR;
		}
		SIVAL(blobs[i].data, 0, obj_attr->attributeID_id);
        }


        return WERR_OK;
}

static WERROR _dsdb_syntax_OID_oid_ldb_to_drsuapi(struct ldb_context *ldb,
						  const struct dsdb_schema *schema,
						  const struct dsdb_attribute *attr,
						  const struct ldb_message_element *in,
						  TALLOC_CTX *mem_ctx,
						  struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	out->attid= attr->attributeID_id;
	out->value_ctr.num_values= in->num_values;
	out->value_ctr.values= talloc_array(mem_ctx,
					    struct drsuapi_DsAttributeValue,
					    in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		uint32_t attid;
		WERROR status;

		out->value_ctr.values[i].blob= &blobs[i];

		blobs[i] = data_blob_talloc(blobs, NULL, 4);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		status = dsdb_schema_pfm_make_attid(schema->prefixmap,
						    (const char *)in->values[i].data,
						    &attid);
		W_ERROR_NOT_OK_RETURN(status);

		SIVAL(blobs[i].data, 0, attid);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_OID_drsuapi_to_ldb(struct ldb_context *ldb, 
					     const struct dsdb_schema *schema,
					     const struct dsdb_attribute *attr,
					     const struct drsuapi_DsReplicaAttribute *in,
					     TALLOC_CTX *mem_ctx,
					     struct ldb_message_element *out)
{
	uint32_t i;

	switch (attr->attributeID_id) {
	case DRSUAPI_ATTRIBUTE_objectClass:
	case DRSUAPI_ATTRIBUTE_subClassOf:
	case DRSUAPI_ATTRIBUTE_auxiliaryClass:
	case DRSUAPI_ATTRIBUTE_systemPossSuperiors:
	case DRSUAPI_ATTRIBUTE_possSuperiors:
		return _dsdb_syntax_OID_obj_drsuapi_to_ldb(ldb, schema, attr, in, mem_ctx, out);
	case DRSUAPI_ATTRIBUTE_systemMustContain:
	case DRSUAPI_ATTRIBUTE_systemMayContain:	
	case DRSUAPI_ATTRIBUTE_mustContain:
	case DRSUAPI_ATTRIBUTE_mayContain:
		return _dsdb_syntax_OID_attr_drsuapi_to_ldb(ldb, schema, attr, in, mem_ctx, out);
	case DRSUAPI_ATTRIBUTE_governsID:
	case DRSUAPI_ATTRIBUTE_attributeID:
	case DRSUAPI_ATTRIBUTE_attributeSyntax:
		return _dsdb_syntax_OID_oid_drsuapi_to_ldb(ldb, schema, attr, in, mem_ctx, out);
	}

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		uint32_t v;
		const char *name;
		char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length != 4) {
			return WERR_FOOBAR;
		}

		v = IVAL(in->value_ctr.values[i].blob->data, 0);

		name = dsdb_lDAPDisplayName_by_id(schema, v);
		if (!name) {
			return WERR_FOOBAR;
		}

		str = talloc_strdup(out->values, name);
		W_ERROR_HAVE_NO_MEMORY(str);

		out->values[i] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_OID_ldb_to_drsuapi(struct ldb_context *ldb, 
					     const struct dsdb_schema *schema,
					     const struct dsdb_attribute *attr,
					     const struct ldb_message_element *in,
					     TALLOC_CTX *mem_ctx,
					     struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	switch (attr->attributeID_id) {
	case DRSUAPI_ATTRIBUTE_objectClass:
	case DRSUAPI_ATTRIBUTE_subClassOf:
	case DRSUAPI_ATTRIBUTE_auxiliaryClass:
	case DRSUAPI_ATTRIBUTE_systemPossSuperiors:
	case DRSUAPI_ATTRIBUTE_possSuperiors:
		return _dsdb_syntax_OID_obj_ldb_to_drsuapi(ldb, schema, attr, in, mem_ctx, out);
	case DRSUAPI_ATTRIBUTE_systemMustContain:
	case DRSUAPI_ATTRIBUTE_systemMayContain:	
	case DRSUAPI_ATTRIBUTE_mustContain:
	case DRSUAPI_ATTRIBUTE_mayContain:
		return _dsdb_syntax_OID_attr_ldb_to_drsuapi(ldb, schema, attr, in, mem_ctx, out);
	case DRSUAPI_ATTRIBUTE_governsID:
	case DRSUAPI_ATTRIBUTE_attributeID:
	case DRSUAPI_ATTRIBUTE_attributeSyntax:
		return _dsdb_syntax_OID_oid_ldb_to_drsuapi(ldb, schema, attr, in, mem_ctx, out);
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		uint32_t v;

		out->value_ctr.values[i].blob	= &blobs[i];

		blobs[i] = data_blob_talloc(blobs, NULL, 4);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		v = strtol((const char *)in->values[i].data, NULL, 10);

		SIVAL(blobs[i].data, 0, v);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_UNICODE_drsuapi_to_ldb(struct ldb_context *ldb, 
						 const struct dsdb_schema *schema,
						 const struct dsdb_attribute *attr,
						 const struct drsuapi_DsReplicaAttribute *in,
						 TALLOC_CTX *mem_ctx,
						 struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length == 0) {
			return WERR_FOOBAR;
		}

		if (!convert_string_talloc_convenience(out->values, 
						schema->iconv_convenience, 
									CH_UTF16, CH_UNIX,
					    in->value_ctr.values[i].blob->data,
					    in->value_ctr.values[i].blob->length,
					    (void **)&str, NULL, false)) {
			return WERR_FOOBAR;
		}

		out->values[i] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_UNICODE_ldb_to_drsuapi(struct ldb_context *ldb, 
						 const struct dsdb_schema *schema,
						 const struct dsdb_attribute *attr,
						 const struct ldb_message_element *in,
						 TALLOC_CTX *mem_ctx,
						 struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		out->value_ctr.values[i].blob	= &blobs[i];

		if (!convert_string_talloc_convenience(blobs,
			schema->iconv_convenience, CH_UNIX, CH_UTF16,
			in->values[i].data, in->values[i].length,
			(void **)&blobs[i].data, &blobs[i].length, false)) {
				return WERR_FOOBAR;
		}
	}

	return WERR_OK;
}


WERROR dsdb_syntax_one_DN_drsuapi_to_ldb(TALLOC_CTX *mem_ctx, struct ldb_context *ldb, 
					 const struct dsdb_syntax *syntax, 
					 struct smb_iconv_convenience *iconv_convenience,
					 const DATA_BLOB *in, DATA_BLOB *out)
{
	struct drsuapi_DsReplicaObjectIdentifier3 id3;
	enum ndr_err_code ndr_err;
	DATA_BLOB guid_blob;
	struct ldb_dn *dn;
	TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);
	int ret;
	NTSTATUS status;

	if (!tmp_ctx) {
		W_ERROR_HAVE_NO_MEMORY(tmp_ctx);
	}
	
	if (in == NULL) {
		talloc_free(tmp_ctx);
		return WERR_FOOBAR;
	}
	
	if (in->length == 0) {
		talloc_free(tmp_ctx);
		return WERR_FOOBAR;
	}
	
	
	/* windows sometimes sends an extra two pad bytes here */
	ndr_err = ndr_pull_struct_blob(in,
				       tmp_ctx, iconv_convenience, &id3,
				       (ndr_pull_flags_fn_t)ndr_pull_drsuapi_DsReplicaObjectIdentifier3);
	if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
		status = ndr_map_error2ntstatus(ndr_err);
		talloc_free(tmp_ctx);
		return ntstatus_to_werror(status);
	}
	
	dn = ldb_dn_new(tmp_ctx, ldb, id3.dn);
	if (!dn) {
		talloc_free(tmp_ctx);
		/* If this fails, it must be out of memory, as it does not do much parsing */
		W_ERROR_HAVE_NO_MEMORY(dn);
	}

	status = GUID_to_ndr_blob(&id3.guid, tmp_ctx, &guid_blob);
	if (!NT_STATUS_IS_OK(status)) {
		talloc_free(tmp_ctx);
		return ntstatus_to_werror(status);
	}
	
	ret = ldb_dn_set_extended_component(dn, "GUID", &guid_blob);
	if (ret != LDB_SUCCESS) {
		talloc_free(tmp_ctx);
		return WERR_FOOBAR;
	}
	
	talloc_free(guid_blob.data);
	
	if (id3.__ndr_size_sid) {
		DATA_BLOB sid_blob;
		ndr_err = ndr_push_struct_blob(&sid_blob, tmp_ctx, iconv_convenience, &id3.sid,
					       (ndr_push_flags_fn_t)ndr_push_dom_sid);
		if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
			status = ndr_map_error2ntstatus(ndr_err);
			talloc_free(tmp_ctx);
			return ntstatus_to_werror(status);
		}
		
		ret = ldb_dn_set_extended_component(dn, "SID", &sid_blob);
		if (ret != LDB_SUCCESS) {
			talloc_free(tmp_ctx);
			return WERR_FOOBAR;
		}
	}
	
	*out = data_blob_string_const(ldb_dn_get_extended_linearized(mem_ctx, dn, 1));
	talloc_free(tmp_ctx);
	return WERR_OK;
}

static WERROR dsdb_syntax_DN_drsuapi_to_ldb(struct ldb_context *ldb, 
					    const struct dsdb_schema *schema,
					    const struct dsdb_attribute *attr,
					    const struct drsuapi_DsReplicaAttribute *in,
					    TALLOC_CTX *mem_ctx,
					    struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		WERROR status = dsdb_syntax_one_DN_drsuapi_to_ldb(out->values, ldb, attr->syntax, 
								  schema->iconv_convenience, 
								  in->value_ctr.values[i].blob, 
								  &out->values[i]);
		if (!W_ERROR_IS_OK(status)) {
			return status;
		}
						  
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_DN_ldb_to_drsuapi(struct ldb_context *ldb, 
					    const struct dsdb_schema *schema,
					    const struct dsdb_attribute *attr,
					    const struct ldb_message_element *in,
					    TALLOC_CTX *mem_ctx,
					    struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		struct drsuapi_DsReplicaObjectIdentifier3 id3;
		enum ndr_err_code ndr_err;
		const DATA_BLOB *sid_blob;
		struct ldb_dn *dn;
		TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);
		NTSTATUS status;

		W_ERROR_HAVE_NO_MEMORY(tmp_ctx);

		out->value_ctr.values[i].blob	= &blobs[i];

		dn = ldb_dn_from_ldb_val(tmp_ctx, ldb, &in->values[i]);

		W_ERROR_HAVE_NO_MEMORY(dn);

		ZERO_STRUCT(id3);

		status = dsdb_get_extended_dn_guid(dn, &id3.guid);
		if (!NT_STATUS_IS_OK(status) &&
		    !NT_STATUS_EQUAL(status, NT_STATUS_OBJECT_NAME_NOT_FOUND)) {
			talloc_free(tmp_ctx);
			return ntstatus_to_werror(status);
		}

		sid_blob = ldb_dn_get_extended_component(dn, "SID");
		if (sid_blob) {
			
			ndr_err = ndr_pull_struct_blob_all(sid_blob, 
							   tmp_ctx, schema->iconv_convenience, &id3.sid,
							   (ndr_pull_flags_fn_t)ndr_pull_dom_sid);
			if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
				status = ndr_map_error2ntstatus(ndr_err);
				talloc_free(tmp_ctx);
				return ntstatus_to_werror(status);
			}
		}

		id3.dn = ldb_dn_get_linearized(dn);

		ndr_err = ndr_push_struct_blob(&blobs[i], blobs, schema->iconv_convenience, &id3, (ndr_push_flags_fn_t)ndr_push_drsuapi_DsReplicaObjectIdentifier3);
		if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
			status = ndr_map_error2ntstatus(ndr_err);
			talloc_free(tmp_ctx);
			return ntstatus_to_werror(status);
		}
		talloc_free(tmp_ctx);
	}

	return WERR_OK;
}



static WERROR dsdb_syntax_DN_BINARY_drsuapi_to_ldb(struct ldb_context *ldb, 
						   const struct dsdb_schema *schema,
						   const struct dsdb_attribute *attr,
						   const struct drsuapi_DsReplicaAttribute *in,
						   TALLOC_CTX *mem_ctx,
						   struct ldb_message_element *out)
{
	uint32_t i;
	int ret;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		struct drsuapi_DsReplicaObjectIdentifier3Binary id3;
		enum ndr_err_code ndr_err;
		DATA_BLOB guid_blob;
		struct ldb_dn *dn;
		struct dsdb_dn *dsdb_dn;
		NTSTATUS status;
		TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);
		if (!tmp_ctx) {
			W_ERROR_HAVE_NO_MEMORY(tmp_ctx);
		}

		if (in->value_ctr.values[i].blob == NULL) {
			talloc_free(tmp_ctx);
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length == 0) {
			talloc_free(tmp_ctx);
			return WERR_FOOBAR;
		}

		
		/* windows sometimes sends an extra two pad bytes here */
		ndr_err = ndr_pull_struct_blob(in->value_ctr.values[i].blob,
					       tmp_ctx, schema->iconv_convenience, &id3,
					       (ndr_pull_flags_fn_t)ndr_pull_drsuapi_DsReplicaObjectIdentifier3Binary);
		if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
			status = ndr_map_error2ntstatus(ndr_err);
			talloc_free(tmp_ctx);
			return ntstatus_to_werror(status);
		}

		dn = ldb_dn_new(tmp_ctx, ldb, id3.dn);
		if (!dn) {
			talloc_free(tmp_ctx);
			/* If this fails, it must be out of memory, as it does not do much parsing */
			W_ERROR_HAVE_NO_MEMORY(dn);
		}

		status = GUID_to_ndr_blob(&id3.guid, tmp_ctx, &guid_blob);
		if (!NT_STATUS_IS_OK(status)) {
			talloc_free(tmp_ctx);
			return ntstatus_to_werror(status);
		}

		ret = ldb_dn_set_extended_component(dn, "GUID", &guid_blob);
		if (ret != LDB_SUCCESS) {
			talloc_free(tmp_ctx);
			return WERR_FOOBAR;
		}

		talloc_free(guid_blob.data);

		if (id3.__ndr_size_sid) {
			DATA_BLOB sid_blob;
			ndr_err = ndr_push_struct_blob(&sid_blob, tmp_ctx, schema->iconv_convenience, &id3.sid,
						       (ndr_push_flags_fn_t)ndr_push_dom_sid);
			if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
				status = ndr_map_error2ntstatus(ndr_err);
				talloc_free(tmp_ctx);
				return ntstatus_to_werror(status);
			}

			ret = ldb_dn_set_extended_component(dn, "SID", &sid_blob);
			if (ret != LDB_SUCCESS) {
				talloc_free(tmp_ctx);
				return WERR_FOOBAR;
			}
		}

		/* set binary stuff */
		dsdb_dn = dsdb_dn_construct(tmp_ctx, dn, id3.binary, attr->syntax->ldap_oid);
		if (!dsdb_dn) {
			/* If this fails, it must be out of memory, we know the ldap_oid is valid */
			talloc_free(tmp_ctx);
			W_ERROR_HAVE_NO_MEMORY(dsdb_dn);
		}
		out->values[i] = data_blob_string_const(dsdb_dn_get_extended_linearized(out->values, dsdb_dn, 1));
		talloc_free(tmp_ctx);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_DN_BINARY_ldb_to_drsuapi(struct ldb_context *ldb, 
						   const struct dsdb_schema *schema,
						   const struct dsdb_attribute *attr,
						   const struct ldb_message_element *in,
						   TALLOC_CTX *mem_ctx,
						   struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		struct drsuapi_DsReplicaObjectIdentifier3Binary id3;
		enum ndr_err_code ndr_err;
		const DATA_BLOB *sid_blob;
		struct dsdb_dn *dsdb_dn;
		TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);
		NTSTATUS status;

		W_ERROR_HAVE_NO_MEMORY(tmp_ctx);

		out->value_ctr.values[i].blob	= &blobs[i];

		dsdb_dn = dsdb_dn_parse(tmp_ctx, ldb, &in->values[i], attr->syntax->ldap_oid);

		if (!dsdb_dn) {
			talloc_free(tmp_ctx);
			return ntstatus_to_werror(NT_STATUS_INVALID_PARAMETER);
		}

		ZERO_STRUCT(id3);

		status = dsdb_get_extended_dn_guid(dsdb_dn->dn, &id3.guid);
		if (!NT_STATUS_IS_OK(status) &&
		    !NT_STATUS_EQUAL(status, NT_STATUS_OBJECT_NAME_NOT_FOUND)) {
			talloc_free(tmp_ctx);
			return ntstatus_to_werror(status);
		}

		sid_blob = ldb_dn_get_extended_component(dsdb_dn->dn, "SID");
		if (sid_blob) {
			
			ndr_err = ndr_pull_struct_blob_all(sid_blob, 
							   tmp_ctx, schema->iconv_convenience, &id3.sid,
							   (ndr_pull_flags_fn_t)ndr_pull_dom_sid);
			if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
				status = ndr_map_error2ntstatus(ndr_err);
				talloc_free(tmp_ctx);
				return ntstatus_to_werror(status);
			}
		}

		id3.dn = ldb_dn_get_linearized(dsdb_dn->dn);

		/* get binary stuff */
		id3.binary = dsdb_dn->extra_part;

		ndr_err = ndr_push_struct_blob(&blobs[i], blobs, schema->iconv_convenience, &id3, (ndr_push_flags_fn_t)ndr_push_drsuapi_DsReplicaObjectIdentifier3Binary);
		if (!NDR_ERR_CODE_IS_SUCCESS(ndr_err)) {
			status = ndr_map_error2ntstatus(ndr_err);
			talloc_free(tmp_ctx);
			return ntstatus_to_werror(status);
		}
		talloc_free(tmp_ctx);
	}

	return WERR_OK;
}



static WERROR dsdb_syntax_PRESENTATION_ADDRESS_drsuapi_to_ldb(struct ldb_context *ldb, 
							      const struct dsdb_schema *schema,
							      const struct dsdb_attribute *attr,
							      const struct drsuapi_DsReplicaAttribute *in,
							      TALLOC_CTX *mem_ctx,
							      struct ldb_message_element *out)
{
	uint32_t i;

	out->flags	= 0;
	out->name	= talloc_strdup(mem_ctx, attr->lDAPDisplayName);
	W_ERROR_HAVE_NO_MEMORY(out->name);

	out->num_values	= in->value_ctr.num_values;
	out->values	= talloc_array(mem_ctx, struct ldb_val, out->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->values);

	for (i=0; i < out->num_values; i++) {
		uint32_t len;
		char *str;

		if (in->value_ctr.values[i].blob == NULL) {
			return WERR_FOOBAR;
		}

		if (in->value_ctr.values[i].blob->length < 4) {
			return WERR_FOOBAR;
		}

		len = IVAL(in->value_ctr.values[i].blob->data, 0);

		if (len != in->value_ctr.values[i].blob->length) {
			return WERR_FOOBAR;
		}

		if (!convert_string_talloc_convenience(out->values, schema->iconv_convenience, CH_UTF16, CH_UNIX,
					    in->value_ctr.values[i].blob->data+4,
					    in->value_ctr.values[i].blob->length-4,
					    (void **)&str, NULL, false)) {
			return WERR_FOOBAR;
		}

		out->values[i] = data_blob_string_const(str);
	}

	return WERR_OK;
}

static WERROR dsdb_syntax_PRESENTATION_ADDRESS_ldb_to_drsuapi(struct ldb_context *ldb, 
							      const struct dsdb_schema *schema,
							      const struct dsdb_attribute *attr,
							      const struct ldb_message_element *in,
							      TALLOC_CTX *mem_ctx,
							      struct drsuapi_DsReplicaAttribute *out)
{
	uint32_t i;
	DATA_BLOB *blobs;

	if (attr->attributeID_id == 0xFFFFFFFF) {
		return WERR_FOOBAR;
	}

	out->attid			= attr->attributeID_id;
	out->value_ctr.num_values	= in->num_values;
	out->value_ctr.values		= talloc_array(mem_ctx,
						       struct drsuapi_DsAttributeValue,
						       in->num_values);
	W_ERROR_HAVE_NO_MEMORY(out->value_ctr.values);

	blobs = talloc_array(mem_ctx, DATA_BLOB, in->num_values);
	W_ERROR_HAVE_NO_MEMORY(blobs);

	for (i=0; i < in->num_values; i++) {
		uint8_t *data;
		size_t ret;

		out->value_ctr.values[i].blob	= &blobs[i];

		if (!convert_string_talloc_convenience(blobs, schema->iconv_convenience, CH_UNIX, CH_UTF16,
					    in->values[i].data,
					    in->values[i].length,
					    (void **)&data, &ret, false)) {
			return WERR_FOOBAR;
		}

		blobs[i] = data_blob_talloc(blobs, NULL, 4 + ret);
		W_ERROR_HAVE_NO_MEMORY(blobs[i].data);

		SIVAL(blobs[i].data, 0, 4 + ret);

		if (ret > 0) {
			memcpy(blobs[i].data + 4, data, ret);
			talloc_free(data);
		}
	}

	return WERR_OK;
}

#define OMOBJECTCLASS(val) { .length = sizeof(val) - 1, .data = discard_const_p(uint8_t, val) }

static const struct dsdb_syntax dsdb_syntaxes[] = {
	{
		.name			= "Boolean",
		.ldap_oid		= LDB_SYNTAX_BOOLEAN,
		.oMSyntax		= 1,
		.attributeSyntax_oid	= "2.5.5.8",
		.drsuapi_to_ldb		= dsdb_syntax_BOOL_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_BOOL_ldb_to_drsuapi,
		.equality               = "booleanMatch",
		.comment                = "Boolean" 
	},{
		.name			= "Integer",
		.ldap_oid		= LDB_SYNTAX_INTEGER,
		.oMSyntax		= 2,
		.attributeSyntax_oid	= "2.5.5.9",
		.drsuapi_to_ldb		= dsdb_syntax_INT32_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_INT32_ldb_to_drsuapi,
		.equality               = "integerMatch",
		.comment                = "Integer",
		.ldb_syntax		= LDB_SYNTAX_SAMBA_INT32
	},{
		.name			= "String(Octet)",
		.ldap_oid		= LDB_SYNTAX_OCTET_STRING,
		.oMSyntax		= 4,
		.attributeSyntax_oid	= "2.5.5.10",
		.drsuapi_to_ldb		= dsdb_syntax_DATA_BLOB_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DATA_BLOB_ldb_to_drsuapi,
		.equality               = "octetStringMatch",
		.comment                = "Octet String",
	},{
		.name			= "String(Sid)",
		.ldap_oid		= LDB_SYNTAX_OCTET_STRING,
		.oMSyntax		= 4,
		.attributeSyntax_oid	= "2.5.5.17",
		.drsuapi_to_ldb		= dsdb_syntax_DATA_BLOB_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DATA_BLOB_ldb_to_drsuapi,
		.equality               = "octetStringMatch",
		.comment                = "Octet String - Security Identifier (SID)",
		.ldb_syntax             = LDB_SYNTAX_SAMBA_SID
	},{
		.name			= "String(Object-Identifier)",
		.ldap_oid		= "1.3.6.1.4.1.1466.115.121.1.38",
		.oMSyntax		= 6,
		.attributeSyntax_oid	= "2.5.5.2",
		.drsuapi_to_ldb		= dsdb_syntax_OID_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_OID_ldb_to_drsuapi,
		.equality               = "caseIgnoreMatch", /* Would use "objectIdentifierMatch" but most are ldap attribute/class names */
		.comment                = "OID String",
		.ldb_syntax             = LDB_SYNTAX_DIRECTORY_STRING
	},{
		.name			= "Enumeration",
		.ldap_oid		= LDB_SYNTAX_INTEGER,
		.oMSyntax		= 10,
		.attributeSyntax_oid	= "2.5.5.9",
		.drsuapi_to_ldb		= dsdb_syntax_INT32_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_INT32_ldb_to_drsuapi,
		.ldb_syntax		= LDB_SYNTAX_SAMBA_INT32
	},{
	/* not used in w2k3 forest */
		.name			= "String(Numeric)",
		.ldap_oid		= "1.3.6.1.4.1.1466.115.121.1.36",
		.oMSyntax		= 18,
		.attributeSyntax_oid	= "2.5.5.6",
		.drsuapi_to_ldb		= dsdb_syntax_DATA_BLOB_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DATA_BLOB_ldb_to_drsuapi,
		.equality               = "numericStringMatch",
		.substring              = "numericStringSubstringsMatch",
		.comment                = "Numeric String",
		.ldb_syntax             = LDB_SYNTAX_DIRECTORY_STRING,
	},{
		.name			= "String(Printable)",
		.ldap_oid		= "1.3.6.1.4.1.1466.115.121.1.44",
		.oMSyntax		= 19,
		.attributeSyntax_oid	= "2.5.5.5",
		.drsuapi_to_ldb		= dsdb_syntax_DATA_BLOB_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DATA_BLOB_ldb_to_drsuapi,
		.ldb_syntax		= LDB_SYNTAX_OCTET_STRING,
	},{
		.name			= "String(Teletex)",
		.ldap_oid		= "1.2.840.113556.1.4.905",
		.oMSyntax		= 20,
		.attributeSyntax_oid	= "2.5.5.4",
		.drsuapi_to_ldb		= dsdb_syntax_DATA_BLOB_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DATA_BLOB_ldb_to_drsuapi,
		.equality               = "caseIgnoreMatch",
		.substring              = "caseIgnoreSubstringsMatch",
		.comment                = "Case Insensitive String",
		.ldb_syntax             = LDB_SYNTAX_DIRECTORY_STRING,
	},{
		.name			= "String(IA5)",
		.ldap_oid		= "1.3.6.1.4.1.1466.115.121.1.26",
		.oMSyntax		= 22,
		.attributeSyntax_oid	= "2.5.5.5",
		.drsuapi_to_ldb		= dsdb_syntax_DATA_BLOB_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DATA_BLOB_ldb_to_drsuapi,
		.equality               = "caseExactIA5Match",
		.comment                = "Printable String",
		.ldb_syntax		= LDB_SYNTAX_OCTET_STRING,
	},{
		.name			= "String(UTC-Time)",
		.ldap_oid		= "1.3.6.1.4.1.1466.115.121.1.53",
		.oMSyntax		= 23,
		.attributeSyntax_oid	= "2.5.5.11",
		.drsuapi_to_ldb		= dsdb_syntax_NTTIME_UTC_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_NTTIME_UTC_ldb_to_drsuapi,
		.equality               = "generalizedTimeMatch",
		.comment                = "UTC Time",
	},{
		.name			= "String(Generalized-Time)",
		.ldap_oid		= "1.3.6.1.4.1.1466.115.121.1.24",
		.oMSyntax		= 24,
		.attributeSyntax_oid	= "2.5.5.11",
		.drsuapi_to_ldb		= dsdb_syntax_NTTIME_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_NTTIME_ldb_to_drsuapi,
		.equality               = "generalizedTimeMatch",
		.comment                = "Generalized Time",
		.ldb_syntax             = LDB_SYNTAX_UTC_TIME,
	},{
	/* not used in w2k3 schema */
		.name			= "String(Case Sensitive)",
		.ldap_oid		= "1.2.840.113556.1.4.1362",
		.oMSyntax		= 27,
		.attributeSyntax_oid	= "2.5.5.3",
		.drsuapi_to_ldb		= dsdb_syntax_FOOBAR_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_FOOBAR_ldb_to_drsuapi,
	},{
		.name			= "String(Unicode)",
		.ldap_oid		= LDB_SYNTAX_DIRECTORY_STRING,
		.oMSyntax		= 64,
		.attributeSyntax_oid	= "2.5.5.12",
		.drsuapi_to_ldb		= dsdb_syntax_UNICODE_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_UNICODE_ldb_to_drsuapi,
		.equality               = "caseIgnoreMatch",
		.substring              = "caseIgnoreSubstringsMatch",
		.comment                = "Directory String",
	},{
		.name			= "Interval/LargeInteger",
		.ldap_oid		= "1.2.840.113556.1.4.906",
		.oMSyntax		= 65,
		.attributeSyntax_oid	= "2.5.5.16",
		.drsuapi_to_ldb		= dsdb_syntax_INT64_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_INT64_ldb_to_drsuapi,
		.equality               = "integerMatch",
		.comment                = "Large Integer",
		.ldb_syntax             = LDB_SYNTAX_INTEGER,
	},{
		.name			= "String(NT-Sec-Desc)",
		.ldap_oid		= LDB_SYNTAX_SAMBA_SECURITY_DESCRIPTOR,
		.oMSyntax		= 66,
		.attributeSyntax_oid	= "2.5.5.15",
		.drsuapi_to_ldb		= dsdb_syntax_DATA_BLOB_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DATA_BLOB_ldb_to_drsuapi,
	},{
		.name			= "Object(DS-DN)",
		.ldap_oid		= LDB_SYNTAX_DN,
		.oMSyntax		= 127,
		.oMObjectClass		= OMOBJECTCLASS("\x2b\x0c\x02\x87\x73\x1c\x00\x85\x4a"),
		.attributeSyntax_oid	= "2.5.5.1",
		.drsuapi_to_ldb		= dsdb_syntax_DN_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DN_ldb_to_drsuapi,
		.equality               = "distinguishedNameMatch",
		.comment                = "Object(DS-DN) == a DN",
	},{
		.name			= "Object(DN-Binary)",
		.ldap_oid		= DSDB_SYNTAX_BINARY_DN,
		.oMSyntax		= 127,
		.oMObjectClass		= OMOBJECTCLASS("\x2a\x86\x48\x86\xf7\x14\x01\x01\x01\x0b"),
		.attributeSyntax_oid	= "2.5.5.7",
		.drsuapi_to_ldb		= dsdb_syntax_DN_BINARY_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DN_BINARY_ldb_to_drsuapi,
		.equality               = "octetStringMatch",
		.comment                = "OctetString: Binary+DN",
	},{
	/* not used in w2k3 schema, but used in Exchange schema*/
		.name			= "Object(OR-Name)",
		.ldap_oid		= DSDB_SYNTAX_OR_NAME,
		.oMSyntax		= 127,
		.oMObjectClass		= OMOBJECTCLASS("\x56\x06\x01\x02\x05\x0b\x1D"),
		.attributeSyntax_oid	= "2.5.5.7",
		.drsuapi_to_ldb		= dsdb_syntax_DN_BINARY_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DN_BINARY_ldb_to_drsuapi,
		.equality		= "caseIgnoreMatch",
		.ldb_syntax		= LDB_SYNTAX_DN,
	},{
	/* 
	 * TODO: verify if DATA_BLOB is correct here...!
	 *
	 *       repsFrom and repsTo are the only attributes using
	 *       this attribute syntax, but they're not replicated... 
	 */
		.name			= "Object(Replica-Link)",
		.ldap_oid		= "1.3.6.1.4.1.1466.115.121.1.40",
		.oMSyntax		= 127,
		.oMObjectClass		= OMOBJECTCLASS("\x2a\x86\x48\x86\xf7\x14\x01\x01\x01\x06"),
		.attributeSyntax_oid	= "2.5.5.10",
		.drsuapi_to_ldb		= dsdb_syntax_DATA_BLOB_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DATA_BLOB_ldb_to_drsuapi,
	},{
		.name			= "Object(Presentation-Address)",
		.ldap_oid		= "1.3.6.1.4.1.1466.115.121.1.43",
		.oMSyntax		= 127,
		.oMObjectClass		= OMOBJECTCLASS("\x2b\x0c\x02\x87\x73\x1c\x00\x85\x5c"),
		.attributeSyntax_oid	= "2.5.5.13",
		.drsuapi_to_ldb		= dsdb_syntax_PRESENTATION_ADDRESS_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_PRESENTATION_ADDRESS_ldb_to_drsuapi,
		.comment                = "Presentation Address",
		.ldb_syntax             = LDB_SYNTAX_DIRECTORY_STRING,
	},{
	/* not used in w2k3 schema */
		.name			= "Object(Access-Point)",
		.ldap_oid		= "1.3.6.1.4.1.1466.115.121.1.2",
		.oMSyntax		= 127,
		.oMObjectClass		= OMOBJECTCLASS("\x2b\x0c\x02\x87\x73\x1c\x00\x85\x3e"),
		.attributeSyntax_oid	= "2.5.5.14",
		.drsuapi_to_ldb		= dsdb_syntax_FOOBAR_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_FOOBAR_ldb_to_drsuapi,
		.ldb_syntax             = LDB_SYNTAX_DIRECTORY_STRING,
	},{
	/* not used in w2k3 schema */
		.name			= "Object(DN-String)",
		.ldap_oid		= DSDB_SYNTAX_STRING_DN,
		.oMSyntax		= 127,
		.oMObjectClass		= OMOBJECTCLASS("\x2a\x86\x48\x86\xf7\x14\x01\x01\x01\x0c"),
		.attributeSyntax_oid	= "2.5.5.14",
		.drsuapi_to_ldb		= dsdb_syntax_DN_BINARY_drsuapi_to_ldb,
		.ldb_to_drsuapi		= dsdb_syntax_DN_BINARY_ldb_to_drsuapi,
		.equality               = "octetStringMatch",
		.comment                = "OctetString: String+DN",
	}
};

const struct dsdb_syntax *find_syntax_map_by_ad_oid(const char *ad_oid) 
{
	int i;
	for (i=0; dsdb_syntaxes[i].ldap_oid; i++) {
		if (strcasecmp(ad_oid, dsdb_syntaxes[i].attributeSyntax_oid) == 0) {
			return &dsdb_syntaxes[i];
		}
	}
	return NULL;
}

const struct dsdb_syntax *find_syntax_map_by_ad_syntax(int oMSyntax) 
{
	int i;
	for (i=0; dsdb_syntaxes[i].ldap_oid; i++) {
		if (oMSyntax == dsdb_syntaxes[i].oMSyntax) {
			return &dsdb_syntaxes[i];
		}
	}
	return NULL;
}

const struct dsdb_syntax *find_syntax_map_by_standard_oid(const char *standard_oid) 
{
	int i;
	for (i=0; dsdb_syntaxes[i].ldap_oid; i++) {
		if (strcasecmp(standard_oid, dsdb_syntaxes[i].ldap_oid) == 0) {
			return &dsdb_syntaxes[i];
		}
	}
	return NULL;
}
const struct dsdb_syntax *dsdb_syntax_for_attribute(const struct dsdb_attribute *attr)
{
	uint32_t i;

	for (i=0; i < ARRAY_SIZE(dsdb_syntaxes); i++) {
		if (attr->oMSyntax != dsdb_syntaxes[i].oMSyntax) continue;

		if (attr->oMObjectClass.length != dsdb_syntaxes[i].oMObjectClass.length) continue;

		if (attr->oMObjectClass.length) {
			int ret;
			ret = memcmp(attr->oMObjectClass.data,
				     dsdb_syntaxes[i].oMObjectClass.data,
				     attr->oMObjectClass.length);
			if (ret != 0) continue;
		}

		if (strcmp(attr->attributeSyntax_oid, dsdb_syntaxes[i].attributeSyntax_oid) != 0) continue;

		return &dsdb_syntaxes[i];
	}

	return NULL;
}

WERROR dsdb_attribute_drsuapi_to_ldb(struct ldb_context *ldb, 
				     const struct dsdb_schema *schema,
				     const struct drsuapi_DsReplicaAttribute *in,
				     TALLOC_CTX *mem_ctx,
				     struct ldb_message_element *out)
{
	const struct dsdb_attribute *sa;

	sa = dsdb_attribute_by_attributeID_id(schema, in->attid);
	if (!sa) {
		return WERR_FOOBAR;
	}

	return sa->syntax->drsuapi_to_ldb(ldb, schema, sa, in, mem_ctx, out);
}

WERROR dsdb_attribute_ldb_to_drsuapi(struct ldb_context *ldb, 
				     const struct dsdb_schema *schema,
				     const struct ldb_message_element *in,
				     TALLOC_CTX *mem_ctx,
				     struct drsuapi_DsReplicaAttribute *out)
{
	const struct dsdb_attribute *sa;

	sa = dsdb_attribute_by_lDAPDisplayName(schema, in->name);
	if (!sa) {
		return WERR_FOOBAR;
	}

	return sa->syntax->ldb_to_drsuapi(ldb, schema, sa, in, mem_ctx, out);
}
