/*
   Unix SMB/CIFS implementation.

   DRS::prefixMap implementation

   Copyright (C) Kamen Mazdrashki <kamen.mazdrashki@postpath.com> 2009

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
#include "librpc/gen_ndr/ndr_drsblobs.h"
#include "../lib/util/asn1.h"


/**
 * Allocates schema_prefixMap object in supplied memory context
 */
static struct dsdb_schema_prefixmap *_dsdb_schema_prefixmap_talloc(TALLOC_CTX *mem_ctx,
								   uint32_t length)
{
	struct dsdb_schema_prefixmap *pfm;

	pfm = talloc_zero(mem_ctx, struct dsdb_schema_prefixmap);
	if (!pfm) {
		return NULL;
	}

	pfm->length = length;
	pfm->prefixes = talloc_zero_array(pfm, struct dsdb_schema_prefixmap_oid,
					  pfm->length);
	if (!pfm->prefixes) {
		talloc_free(pfm);
		return NULL;
	}

	return pfm;
}

/**
 * Initial prefixMap creation according to:
 * [MS-DRSR] section 5.12.2
 */
WERROR dsdb_schema_pfm_new(TALLOC_CTX *mem_ctx, struct dsdb_schema_prefixmap **_pfm)
{
	uint32_t i;
	struct dsdb_schema_prefixmap *pfm;
	const struct {
		uint32_t	id;
		const char	*oid_prefix;
	} pfm_init_data[] = {
		{.id=0x00000000, .oid_prefix="2.5.4"},
		{.id=0x00000001, .oid_prefix="2.5.6"},
		{.id=0x00000002, .oid_prefix="1.2.840.113556.1.2"},
		{.id=0x00000003, .oid_prefix="1.2.840.113556.1.3"},
		{.id=0x00000004, .oid_prefix="2.16.840.1.101.2.2.1"},
		{.id=0x00000005, .oid_prefix="2.16.840.1.101.2.2.3"},
		{.id=0x00000006, .oid_prefix="2.16.840.1.101.2.1.5"},
		{.id=0x00000007, .oid_prefix="2.16.840.1.101.2.1.4"},
		{.id=0x00000008, .oid_prefix="2.5.5"},
		{.id=0x00000009, .oid_prefix="1.2.840.113556.1.4"},
		{.id=0x0000000A, .oid_prefix="1.2.840.113556.1.5"},
		{.id=0x00000013, .oid_prefix="0.9.2342.19200300.100"},
		{.id=0x00000014, .oid_prefix="2.16.840.1.113730.3"},
		{.id=0x00000015, .oid_prefix="0.9.2342.19200300.100.1"},
		{.id=0x00000016, .oid_prefix="2.16.840.1.113730.3.1"},
		{.id=0x00000017, .oid_prefix="1.2.840.113556.1.5.7000"},
		{.id=0x00000018, .oid_prefix="2.5.21"},
		{.id=0x00000019, .oid_prefix="2.5.18"},
		{.id=0x0000001A, .oid_prefix="2.5.20"},
	};

	/* allocate mem for prefix map */
	pfm = _dsdb_schema_prefixmap_talloc(mem_ctx, ARRAY_SIZE(pfm_init_data));
	W_ERROR_HAVE_NO_MEMORY(pfm);

	/* build prefixes */
	for (i = 0; i < pfm->length; i++) {
		if (!ber_write_partial_OID_String(pfm, &pfm->prefixes[i].bin_oid, pfm_init_data[i].oid_prefix)) {
			talloc_free(pfm);
			return WERR_INTERNAL_ERROR;
		}
		pfm->prefixes[i].id = pfm_init_data[i].id;
	}

	*_pfm = pfm;

	return WERR_OK;
}


/**
 * Adds oid to prefix map.
 * On success returns ID for newly added index
 * or ID of existing entry that matches oid
 * Reference: [MS-DRSR] section 5.12.2
 *
 * \param pfm prefixMap
 * \param bin_oid OID prefix to be added to prefixMap
 * \param pfm_id Location where to store prefixMap entry ID
 */
static WERROR _dsdb_schema_pfm_add_entry(struct dsdb_schema_prefixmap *pfm, DATA_BLOB bin_oid, uint32_t *_idx)
{
	uint32_t i;
	struct dsdb_schema_prefixmap_oid * pfm_entry;
	struct dsdb_schema_prefixmap_oid * prefixes_new;

	/* dup memory for bin-oid prefix to be added */
	bin_oid = data_blob_dup_talloc(pfm, &bin_oid);
	W_ERROR_HAVE_NO_MEMORY(bin_oid.data);

	/* make room for new entry */
	prefixes_new = talloc_realloc(pfm, pfm->prefixes, struct dsdb_schema_prefixmap_oid, pfm->length + 1);
	if (!prefixes_new) {
		talloc_free(bin_oid.data);
		return WERR_NOMEM;
	}
	pfm->prefixes = prefixes_new;

	/* make new unique ID in prefixMap */
	pfm_entry = &pfm->prefixes[pfm->length];
	pfm_entry->id = 0;
	for (i = 0; i < pfm->length; i++) {
		if (pfm_entry->id < pfm->prefixes[i].id)
			pfm_entry->id = pfm->prefixes[i].id;
	}

	/* add new bin-oid prefix */
	pfm_entry->id++;
	pfm_entry->bin_oid = bin_oid;

	*_idx = pfm->length;
	pfm->length++;

	return WERR_OK;
}


/**
 * Make partial binary OID for supplied OID.
 * Reference: [MS-DRSR] section 5.12.2
 */
static WERROR _dsdb_pfm_make_binary_oid(const char *full_oid, TALLOC_CTX *mem_ctx,
					DATA_BLOB *_bin_oid, uint32_t *_last_subid)
{
	uint32_t last_subid;
	const char *oid_subid;

	/* make last sub-identifier value */
	oid_subid = strrchr(full_oid, '.');
	if (!oid_subid) {
		return WERR_INVALID_PARAMETER;
	}
	oid_subid++;
	last_subid = strtoul(oid_subid, NULL, 10);

	/* encode oid in BER format */
	if (!ber_write_OID_String(mem_ctx, _bin_oid, full_oid)) {
		return WERR_INTERNAL_ERROR;
	}

	/* get the prefix of the OID */
	if (last_subid < 128) {
		_bin_oid->length -= 1;
	} else {
		_bin_oid->length -= 2;
	}

	/* return last_value if requested */
	if (_last_subid) {
		*_last_subid = last_subid;
	}

	return WERR_OK;
}

/**
 * Lookup partial-binary-oid in prefixMap
 */
WERROR dsdb_schema_pfm_find_binary_oid(const struct dsdb_schema_prefixmap *pfm,
				       DATA_BLOB bin_oid,
				       uint32_t *_idx)
{
	uint32_t i;

	for (i = 0; i < pfm->length; i++) {
		if (pfm->prefixes[i].bin_oid.length != bin_oid.length) {
			continue;
		}

		if (memcmp(pfm->prefixes[i].bin_oid.data, bin_oid.data, bin_oid.length) == 0) {
			if (_idx) {
				*_idx = i;
			}
			return WERR_OK;
		}
	}

	return WERR_DS_NO_MSDS_INTID;
}

/**
 * Lookup full-oid in prefixMap
 * Note: this may be slow.
 */
WERROR dsdb_schema_pfm_find_oid(const struct dsdb_schema_prefixmap *pfm,
				const char *full_oid,
				uint32_t *_idx)
{
	WERROR werr;
	DATA_BLOB bin_oid;

	ZERO_STRUCT(bin_oid);

	/* make partial-binary-oid to look for */
	werr = _dsdb_pfm_make_binary_oid(full_oid, NULL, &bin_oid, NULL);
	W_ERROR_NOT_OK_RETURN(werr);

	/* lookup the partial-oid */
	werr = dsdb_schema_pfm_find_binary_oid(pfm, bin_oid, _idx);

	data_blob_free(&bin_oid);

	return werr;
}

/**
 * Make ATTID for given OID
 * Reference: [MS-DRSR] section 5.12.2
 */
WERROR dsdb_schema_pfm_make_attid(struct dsdb_schema_prefixmap *pfm, const char *oid, uint32_t *attid)
{
	WERROR werr;
	uint32_t idx;
	uint32_t lo_word, hi_word;
	uint32_t last_subid;
	DATA_BLOB bin_oid;

	if (!pfm) {
		return WERR_INVALID_PARAMETER;
	}
	if (!oid) {
		return WERR_INVALID_PARAMETER;
	}

	werr = _dsdb_pfm_make_binary_oid(oid, pfm, &bin_oid, &last_subid);
	W_ERROR_NOT_OK_RETURN(werr);

	/* search the prefix in the prefix table, if none found, add
	 * one entry for new prefix.
	 */
	werr = dsdb_schema_pfm_find_binary_oid(pfm, bin_oid, &idx);
	if (W_ERROR_IS_OK(werr)) {
		/* free memory allocated for bin_oid */
		data_blob_free(&bin_oid);
	} else {
		/* entry does not exists, add it */
		werr = _dsdb_schema_pfm_add_entry(pfm, bin_oid, &idx);
		W_ERROR_NOT_OK_RETURN(werr);
	}

	/* compose the attid */
	lo_word = last_subid % 16384;	/* actually get lower 14 bits: lo_word & 0x3FFF */
	if (last_subid >= 16384) {
		/* mark it so that it is known to not be the whole lastValue
		 * This will raise 16-th bit*/
		lo_word += 32768;
	}
	hi_word = pfm->prefixes[idx].id;

	/* make ATTID:
	 * HIWORD is prefixMap id
	 * LOWORD is truncated binary-oid */
	*attid = (hi_word * 65536) + lo_word;

	return WERR_OK;
}


/**
 * Make OID for given ATTID.
 * Reference: [MS-DRSR] section 5.12.2
 */
WERROR dsdb_schema_pfm_oid_from_attid(struct dsdb_schema_prefixmap *pfm, uint32_t attid,
				      TALLOC_CTX *mem_ctx, const char **_oid)
{
	int i;
	uint32_t hi_word, lo_word;
	DATA_BLOB bin_oid = {NULL, 0};
	struct dsdb_schema_prefixmap_oid *pfm_entry;
	WERROR werr = WERR_OK;

	/* crack attid value */
	hi_word = attid >> 16;
	lo_word = attid & 0xFFFF;

	/* locate corRespoNding prefixMap entry */
	pfm_entry = NULL;
	for (i = 0; i < pfm->length; i++) {
		if (hi_word == pfm->prefixes[i].id) {
			pfm_entry = &pfm->prefixes[i];
			break;
		}
	}

	if (!pfm_entry) {
		return WERR_INTERNAL_ERROR;
	}

	/* copy oid prefix making enough room */
	bin_oid.length = pfm_entry->bin_oid.length + 2;
	bin_oid.data = talloc_array(mem_ctx, uint8_t, bin_oid.length);
	W_ERROR_HAVE_NO_MEMORY(bin_oid.data);
	memcpy(bin_oid.data, pfm_entry->bin_oid.data, pfm_entry->bin_oid.length);

	if (lo_word < 128) {
		bin_oid.length = bin_oid.length - 1;
		bin_oid.data[bin_oid.length-1] = lo_word;
	}
	else {
		if (lo_word >= 32768) {
			lo_word -= 32768;
		}
		bin_oid.data[bin_oid.length-2] = (0x80 | ((lo_word>>7) & 0x7f));
		bin_oid.data[bin_oid.length-1] = lo_word & 0x7f;
	}

	if (!ber_read_OID_String(mem_ctx, bin_oid, _oid)) {
		werr = WERR_INTERNAL_ERROR;
	}

	/* free locally allocated memory */
	talloc_free(bin_oid.data);

	return werr;
}


/**
 * Verifies drsuapi mappings.
 */
static WERROR _dsdb_drsuapi_pfm_verify(const struct drsuapi_DsReplicaOIDMapping_Ctr *ctr,
				       bool have_schema_info)
{
	uint32_t i;
	uint32_t num_mappings;
	struct drsuapi_DsReplicaOIDMapping *mapping;

	/* check input params */
	if (!ctr) {
		return WERR_INVALID_PARAMETER;
	}
	if (!ctr->mappings) {
		return WERR_INVALID_PARAMETER;
	}
	num_mappings = ctr->num_mappings;

	if (have_schema_info) {
		if (ctr->num_mappings < 2) {
			return WERR_INVALID_PARAMETER;
		}

		/* check last entry for being special */
		mapping = &ctr->mappings[ctr->num_mappings - 1];
		if (!mapping->oid.binary_oid) {
			return WERR_INVALID_PARAMETER;
		}
		if (mapping->id_prefix != 0) {
			return WERR_INVALID_PARAMETER;
		}
		if (mapping->oid.length != 21) {
			return WERR_INVALID_PARAMETER;
		}
		if (*mapping->oid.binary_oid != 0xFF) {
			return WERR_INVALID_PARAMETER;
		}

		/* get number of read mappings in the map */
		num_mappings--;
	}

	/* now, verify rest of entries for being at least not null */
	for (i = 0; i < num_mappings; i++) {
		mapping = &ctr->mappings[i];
		if (!mapping->oid.length) {
			return WERR_INVALID_PARAMETER;
		}
		if (!mapping->oid.binary_oid) {
			return WERR_INVALID_PARAMETER;
		}
		/* check it is not the special entry */
		if (*mapping->oid.binary_oid == 0xFF) {
			return WERR_INVALID_PARAMETER;
		}
	}

	return WERR_OK;
}

/**
 * Convert drsuapi_ prefix map to prefixMap internal presentation.
 *
 * \param ctr Pointer to drsuapi_DsReplicaOIDMapping_Ctr which represents drsuapi_ prefixMap
 * \param have_schema_info if drsuapi_prefixMap have schem_info in it or not
 * \param mem_ctx TALLOC_CTX to make allocations in
 * \param _pfm Out pointer to hold newly created prefixMap
 * \param _schema_info Out param to store schema_info to. If NULL, schema_info is not decoded
 */
WERROR dsdb_schema_pfm_from_drsuapi_pfm(const struct drsuapi_DsReplicaOIDMapping_Ctr *ctr,
					bool have_schema_info,
					TALLOC_CTX *mem_ctx,
					struct dsdb_schema_prefixmap **_pfm,
					const char **_schema_info)
{
	WERROR werr;
	uint32_t i;
	DATA_BLOB blob;
	uint32_t num_mappings;
	struct dsdb_schema_prefixmap *pfm;

	if (!_pfm) {
		return WERR_INVALID_PARAMETER;
	}

	/*
	 * error out if schema_info is requested
	 * but it is not in the drsuapi_prefixMap
	 */
	if (_schema_info && !have_schema_info) {
		return WERR_INVALID_PARAMETER;
	}

	/* verify drsuapi_pefixMap */
	werr =_dsdb_drsuapi_pfm_verify(ctr, have_schema_info);
	W_ERROR_NOT_OK_RETURN(werr);

	/* allocate mem for prefix map */
	num_mappings = ctr->num_mappings;
	if (have_schema_info) {
		num_mappings--;
	}
	pfm = _dsdb_schema_prefixmap_talloc(mem_ctx, num_mappings);
	W_ERROR_HAVE_NO_MEMORY(pfm);

	/* copy entries from drsuapi_prefixMap */
	for (i = 0; i < pfm->length; i++) {
		blob = data_blob_talloc(pfm,
					ctr->mappings[i].oid.binary_oid,
					ctr->mappings[i].oid.length);
		if (!blob.data) {
			talloc_free(pfm);
			return WERR_NOMEM;
		}
		pfm->prefixes[i].id = ctr->mappings[i].id_prefix;
		pfm->prefixes[i].bin_oid = blob;
	}

	/* fetch schema_info if requested */
	if (_schema_info) {
		/* by this time, i should have this value,
		 *  but set it here for clarity */
		i = ctr->num_mappings - 1;

		*_schema_info = hex_encode_talloc(mem_ctx,
						  ctr->mappings[i].oid.binary_oid,
						  ctr->mappings[i].oid.length);
		if (!*_schema_info) {
			talloc_free(pfm);
			return WERR_NOMEM;
		}
	}

	/* schema_prefixMap created successfully */
	*_pfm = pfm;

	return WERR_OK;
}

/**
 * Convert drsuapi_ prefix map to prefixMap internal presentation.
 *
 * \param pfm Schema prefixMap to be converted
 * \param schema_info schema_info string - if NULL, we don't need it
 * \param mem_ctx TALLOC_CTX to make allocations in
 * \param _ctr Out pointer to drsuapi_DsReplicaOIDMapping_Ctr prefix map structure
 */
WERROR dsdb_drsuapi_pfm_from_schema_pfm(const struct dsdb_schema_prefixmap *pfm,
					const char *schema_info,
					TALLOC_CTX *mem_ctx,
					struct drsuapi_DsReplicaOIDMapping_Ctr **_ctr)
{
	uint32_t i;
	DATA_BLOB blob;
	struct drsuapi_DsReplicaOIDMapping_Ctr *ctr;

	if (!_ctr) {
		return WERR_INVALID_PARAMETER;
	}
	if (!pfm) {
		return WERR_INVALID_PARAMETER;
	}
	if (pfm->length == 0) {
		return WERR_INVALID_PARAMETER;
	}

	/* allocate memory for the structure */
	ctr = talloc_zero(mem_ctx, struct drsuapi_DsReplicaOIDMapping_Ctr);
	W_ERROR_HAVE_NO_MEMORY(ctr);

	ctr->num_mappings = (schema_info ? pfm->length + 1 : pfm->length);
	ctr->mappings = talloc_array(ctr, struct drsuapi_DsReplicaOIDMapping, ctr->num_mappings);
	if (!ctr->mappings) {
		talloc_free(ctr);
		return WERR_NOMEM;
	}

	/* copy entries from schema_prefixMap */
	for (i = 0; i < pfm->length; i++) {
		blob = data_blob_dup_talloc(ctr, &pfm->prefixes[i].bin_oid);
		if (!blob.data) {
			talloc_free(ctr);
			return WERR_NOMEM;
		}
		ctr->mappings[i].id_prefix = pfm->prefixes[i].id;
		ctr->mappings[i].oid.length = blob.length;
		ctr->mappings[i].oid.binary_oid = blob.data;
	}

	/* make schema_info entry if needed */
	if (schema_info) {
		/* by this time, i should have this value,
		 *  but set it here for clarity */
		i = ctr->num_mappings - 1;

		blob = strhex_to_data_blob(ctr, schema_info);
		if (!blob.data) {
			talloc_free(ctr);
			return WERR_NOMEM;
		}

		ctr->mappings[i].id_prefix = 0;
		ctr->mappings[i].oid.length = blob.length;
		ctr->mappings[i].oid.binary_oid = blob.data;
	}

	/* drsuapi_prefixMap constructed successfully */
	*_ctr = ctr;

	return WERR_OK;
}

/**
 * Verifies schema prefixMap and drsuapi prefixMap are same.
 * Note that we just need to verify pfm contains prefixes
 * from ctr, not that those prefixes has same id_prefix.
 */
WERROR dsdb_schema_pfm_contains_drsuapi_pfm(const struct dsdb_schema_prefixmap *pfm,
					    const struct drsuapi_DsReplicaOIDMapping_Ctr *ctr)
{
	WERROR werr;
	uint32_t i;
	uint32_t idx;
	DATA_BLOB bin_oid;

	/* verify drsuapi_pefixMap */
	werr = _dsdb_drsuapi_pfm_verify(ctr, true);
	W_ERROR_NOT_OK_RETURN(werr);

	/* check pfm contains every entry from ctr, except the last one */
	for (i = 0; i < ctr->num_mappings - 1; i++) {
		bin_oid.length = ctr->mappings[i].oid.length;
		bin_oid.data   = ctr->mappings[i].oid.binary_oid;

		werr = dsdb_schema_pfm_find_binary_oid(pfm, bin_oid, &idx);
		if (!W_ERROR_IS_OK(werr)) {
			return WERR_DS_DRA_SCHEMA_MISMATCH;
		}
	}

	return WERR_OK;
}
