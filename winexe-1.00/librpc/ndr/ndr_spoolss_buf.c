/*
   Unix SMB/CIFS implementation.

   routines for marshalling/unmarshalling spoolss subcontext buffer structures

   Copyright (C) Andrew Tridgell 2003
   Copyright (C) Tim Potter 2003
   Copyright (C) Guenther Deschner 2009

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
#include "librpc/gen_ndr/ndr_spoolss.h"
#if (_SAMBA_BUILD_ >= 4)
#include "param/param.h"
#endif

#define NDR_SPOOLSS_PUSH_ENUM_IN(fn) do { \
	if (!r->in.buffer && r->in.offered != 0) {\
		return ndr_push_error(ndr, NDR_ERR_BUFSIZE,\
			"SPOOLSS Buffer: r->in.offered[%u] but there's no buffer",\
			(unsigned)r->in.offered);\
	} else if (r->in.buffer && r->in.buffer->length != r->in.offered) {\
		return ndr_push_error(ndr, NDR_ERR_BUFSIZE,\
			"SPOOLSS Buffer: r->in.offered[%u] doesn't match length of r->in.buffer[%u]",\
			(unsigned)r->in.offered, (unsigned)r->in.buffer->length);\
	}\
	_r.in.level	= r->in.level;\
	_r.in.buffer	= r->in.buffer;\
	_r.in.offered	= r->in.offered;\
	NDR_CHECK(ndr_push__##fn(ndr, flags, &_r));\
} while(0)

#define NDR_SPOOLSS_PUSH_ENUM_OUT(fn) do { \
	struct ndr_push *_ndr_info;\
	_r.in.level	= r->in.level;\
	_r.in.buffer	= r->in.buffer;\
	_r.in.offered	= r->in.offered;\
	_r.out.info	= NULL;\
	_r.out.needed	= r->out.needed;\
	_r.out.count	= r->out.count;\
	_r.out.result	= r->out.result;\
	if (r->out.info && *r->out.info && !r->in.buffer) {\
		return ndr_push_error(ndr, NDR_ERR_BUFSIZE,\
			"SPOOLSS Buffer: *r->out.info but there's no r->in.buffer");\
	}\
	if (r->in.buffer) {\
		DATA_BLOB _data_blob_info;\
		_ndr_info = ndr_push_init_ctx(ndr, ndr->iconv_convenience);\
		NDR_ERR_HAVE_NO_MEMORY(_ndr_info);\
		_ndr_info->flags= ndr->flags;\
		if (r->out.info) {\
			struct __##fn __r;\
			__r.in.level	= r->in.level;\
			__r.in.count	= *r->out.count;\
			__r.out.info	= *r->out.info;\
			NDR_CHECK(ndr_push___##fn(_ndr_info, flags, &__r)); \
		}\
		if (r->in.offered > _ndr_info->offset) {\
			uint32_t _padding_len = r->in.offered - _ndr_info->offset;\
			NDR_CHECK(ndr_push_zero(_ndr_info, _padding_len));\
		} else if (r->in.offered < _ndr_info->offset) {\
			return ndr_push_error(ndr, NDR_ERR_BUFSIZE,\
				"SPOOLSS Buffer: r->in.offered[%u] doesn't match length of out buffer[%u]!",\
				(unsigned)r->in.offered, (unsigned)_ndr_info->offset);\
		}\
		_data_blob_info = ndr_push_blob(_ndr_info);\
		_r.out.info	= &_data_blob_info;\
	}\
	NDR_CHECK(ndr_push__##fn(ndr, flags, &_r));\
} while(0)

#define NDR_SPOOLSS_PUSH_ENUM(fn,in,out) do { \
	struct _##fn _r;\
	if (flags & NDR_IN) {\
		in;\
		NDR_SPOOLSS_PUSH_ENUM_IN(fn);\
	}\
	if (flags & NDR_OUT) {\
		out;\
		NDR_SPOOLSS_PUSH_ENUM_OUT(fn);\
	}\
} while(0)

#define NDR_SPOOLSS_PULL_ENUM_IN(fn) do { \
	ZERO_STRUCT(r->out);\
	NDR_CHECK(ndr_pull__##fn(ndr, flags, &_r));\
	r->in.level	= _r.in.level;\
	r->in.buffer	= _r.in.buffer;\
	r->in.offered	= _r.in.offered;\
	r->out.needed	= _r.out.needed;\
	r->out.count	= _r.out.count;\
	if (!r->in.buffer && r->in.offered != 0) {\
		return ndr_pull_error(ndr, NDR_ERR_BUFSIZE,\
			"SPOOLSS Buffer: r->in.offered[%u] but there's no buffer",\
			(unsigned)r->in.offered);\
	} else if (r->in.buffer && r->in.buffer->length != r->in.offered) {\
		return ndr_pull_error(ndr, NDR_ERR_BUFSIZE,\
			"SPOOLSS Buffer: r->in.offered[%u] doesn't match length of r->in.buffer[%u]",\
			(unsigned)r->in.offered, (unsigned)r->in.buffer->length);\
	}\
	NDR_PULL_ALLOC(ndr, r->out.info);\
	ZERO_STRUCTP(r->out.info);\
} while(0)

#define NDR_SPOOLSS_PULL_ENUM_OUT(fn) do { \
	_r.in.level	= r->in.level;\
	_r.in.buffer	= r->in.buffer;\
	_r.in.offered	= r->in.offered;\
	_r.out.needed	= r->out.needed;\
	_r.out.count	= r->out.count;\
	NDR_CHECK(ndr_pull__##fn(ndr, flags, &_r));\
	if (ndr->flags & LIBNDR_FLAG_REF_ALLOC) {\
		NDR_PULL_ALLOC(ndr, r->out.info);\
	}\
	*r->out.info = NULL;\
	r->out.needed	= _r.out.needed;\
	r->out.count	= _r.out.count;\
	r->out.result	= _r.out.result;\
	if (_r.out.info) {\
		struct ndr_pull *_ndr_info;\
		NDR_PULL_ALLOC(ndr, *r->out.info);\
		_ndr_info = ndr_pull_init_blob(_r.out.info, *r->out.info, ndr->iconv_convenience);\
		NDR_ERR_HAVE_NO_MEMORY(_ndr_info);\
		_ndr_info->flags= ndr->flags;\
		if (r->in.offered != _ndr_info->data_size) {\
			return ndr_pull_error(ndr, NDR_ERR_BUFSIZE,\
				"SPOOLSS Buffer: offered[%u] doesn't match length of buffer[%u]",\
				(unsigned)r->in.offered, (unsigned)_ndr_info->data_size);\
		}\
		if (*r->out.needed <= _ndr_info->data_size) {\
			struct __##fn __r;\
			__r.in.level	= r->in.level;\
			__r.in.count	= *r->out.count;\
			__r.out.info	= NULL;\
			NDR_CHECK(ndr_pull___##fn(_ndr_info, flags, &__r));\
			*r->out.info	= __r.out.info;\
		}\
	}\
} while(0)

#define NDR_SPOOLSS_PULL_ENUM(fn,in,out) do { \
	struct _##fn _r;\
	if (flags & NDR_IN) {\
		out;\
		NDR_SPOOLSS_PULL_ENUM_IN(fn);\
		in;\
	}\
	if (flags & NDR_OUT) {\
		out;\
		NDR_SPOOLSS_PULL_ENUM_OUT(fn);\
	}\
} while(0)

#define _NDR_CHECK_UINT32(call) do {\
	enum ndr_err_code _ndr_err; \
        _ndr_err = call; \
	if (!NDR_ERR_CODE_IS_SUCCESS(_ndr_err)) { \
		return 0; \
	}\
} while (0)

/* TODO: set _ndr_info->flags correct */
#define NDR_SPOOLSS_SIZE_ENUM_LEVEL(fn) do { \
	struct __##fn __r;\
	DATA_BLOB _data_blob_info;\
	struct ndr_push *_ndr_info = ndr_push_init_ctx(mem_ctx, iconv_convenience);\
	if (!_ndr_info) return 0;\
	_ndr_info->flags|=0;\
	__r.in.level	= level;\
	__r.in.count	= count;\
	__r.out.info	= info;\
	_NDR_CHECK_UINT32(ndr_push___##fn(_ndr_info, NDR_OUT, &__r)); \
	_data_blob_info = ndr_push_blob(_ndr_info);\
	return _data_blob_info.length;\
} while(0)

/* TODO: set _ndr_info->flags correct */
#define NDR_SPOOLSS_SIZE_ENUM(fn) do { \
	struct __##fn __r;\
	DATA_BLOB _data_blob_info;\
	struct ndr_push *_ndr_info = ndr_push_init_ctx(mem_ctx, iconv_convenience);\
	if (!_ndr_info) return 0;\
	_ndr_info->flags|=0;\
	__r.in.count	= count;\
	__r.out.info	= info;\
	_NDR_CHECK_UINT32(ndr_push___##fn(_ndr_info, NDR_OUT, &__r)); \
	_data_blob_info = ndr_push_blob(_ndr_info);\
	return _data_blob_info.length;\
} while(0)


/*
  spoolss_EnumPrinters
*/
enum ndr_err_code ndr_push_spoolss_EnumPrinters(struct ndr_push *ndr, int flags, const struct spoolss_EnumPrinters *r)
{
	NDR_SPOOLSS_PUSH_ENUM(spoolss_EnumPrinters,{
		_r.in.flags	= r->in.flags;
		_r.in.server	= r->in.server;
	},{
		_r.in.flags	= r->in.flags;
		_r.in.server	= r->in.server;
	});
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_EnumPrinters(struct ndr_pull *ndr, int flags, struct spoolss_EnumPrinters *r)
{
	NDR_SPOOLSS_PULL_ENUM(spoolss_EnumPrinters,{
		r->in.flags	= _r.in.flags;
		r->in.server	= _r.in.server;
	},{
		_r.in.flags	= r->in.flags;
		_r.in.server	= r->in.server;
	});
	return NDR_ERR_SUCCESS;
}

uint32_t ndr_size_spoolss_EnumPrinters_info(TALLOC_CTX *mem_ctx, struct smb_iconv_convenience *iconv_convenience, uint32_t level, uint32_t count, union spoolss_PrinterInfo *info)
{
	NDR_SPOOLSS_SIZE_ENUM_LEVEL(spoolss_EnumPrinters);
}

/*
  spoolss_EnumJobs
*/
enum ndr_err_code ndr_push_spoolss_EnumJobs(struct ndr_push *ndr, int flags, const struct spoolss_EnumJobs *r)
{
	NDR_SPOOLSS_PUSH_ENUM(spoolss_EnumJobs,{
		_r.in.handle	= r->in.handle;
		_r.in.firstjob	= r->in.firstjob;
		_r.in.numjobs	= r->in.numjobs;
	},{
		_r.in.handle	= r->in.handle;
		_r.in.firstjob	= r->in.firstjob;
		_r.in.numjobs	= r->in.numjobs;
	});
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_EnumJobs(struct ndr_pull *ndr, int flags, struct spoolss_EnumJobs *r)
{
	NDR_SPOOLSS_PULL_ENUM(spoolss_EnumJobs,{
		r->in.handle	= _r.in.handle;
		r->in.firstjob	= _r.in.firstjob;
		r->in.numjobs	= _r.in.numjobs;
	},{
		_r.in.handle	= r->in.handle;
		_r.in.firstjob	= r->in.firstjob;
		_r.in.numjobs	= r->in.numjobs;
	});
	return NDR_ERR_SUCCESS;
}

uint32_t ndr_size_spoolss_EnumJobs_info(TALLOC_CTX *mem_ctx, struct smb_iconv_convenience *iconv_convenience, uint32_t level, uint32_t count, union spoolss_JobInfo *info)
{
	NDR_SPOOLSS_SIZE_ENUM_LEVEL(spoolss_EnumJobs);
}

/*
  spoolss_EnumPrinterDrivers
*/
enum ndr_err_code ndr_push_spoolss_EnumPrinterDrivers(struct ndr_push *ndr, int flags, const struct spoolss_EnumPrinterDrivers *r)
{
	NDR_SPOOLSS_PUSH_ENUM(spoolss_EnumPrinterDrivers,{
		_r.in.server		= r->in.server;
		_r.in.environment	= r->in.environment;
	},{
		_r.in.server		= r->in.server;
		_r.in.environment	= r->in.environment;
	});
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_EnumPrinterDrivers(struct ndr_pull *ndr, int flags, struct spoolss_EnumPrinterDrivers *r)
{
	NDR_SPOOLSS_PULL_ENUM(spoolss_EnumPrinterDrivers,{
		r->in.server		= _r.in.server;
		r->in.environment	= _r.in.environment;
	},{
		_r.in.server		= r->in.server;
		_r.in.environment	= r->in.environment;
	});
	return NDR_ERR_SUCCESS;
}

uint32_t ndr_size_spoolss_EnumPrinterDrivers_info(TALLOC_CTX *mem_ctx, struct smb_iconv_convenience *iconv_convenience, uint32_t level, uint32_t count, union spoolss_DriverInfo *info)
{
	NDR_SPOOLSS_SIZE_ENUM_LEVEL(spoolss_EnumPrinterDrivers);
}

/*
  spoolss_EnumForms
*/
enum ndr_err_code ndr_push_spoolss_EnumForms(struct ndr_push *ndr, int flags, const struct spoolss_EnumForms *r)
{
	NDR_SPOOLSS_PUSH_ENUM(spoolss_EnumForms,{
		_r.in.handle	= r->in.handle;
	},{
		_r.in.handle	= r->in.handle;
	});
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_EnumForms(struct ndr_pull *ndr, int flags, struct spoolss_EnumForms *r)
{
	NDR_SPOOLSS_PULL_ENUM(spoolss_EnumForms,{
		r->in.handle	= _r.in.handle;
	},{
		_r.in.handle	= r->in.handle;
	});
	return NDR_ERR_SUCCESS;
}

uint32_t ndr_size_spoolss_EnumForms_info(TALLOC_CTX *mem_ctx, struct smb_iconv_convenience *iconv_convenience, uint32_t level, uint32_t count, union spoolss_FormInfo *info)
{
	NDR_SPOOLSS_SIZE_ENUM_LEVEL(spoolss_EnumForms);
}

/*
  spoolss_EnumPorts
*/
enum ndr_err_code ndr_push_spoolss_EnumPorts(struct ndr_push *ndr, int flags, const struct spoolss_EnumPorts *r)
{
	NDR_SPOOLSS_PUSH_ENUM(spoolss_EnumPorts,{
		_r.in.servername= r->in.servername;
	},{
		_r.in.servername= r->in.servername;
	});
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_EnumPorts(struct ndr_pull *ndr, int flags, struct spoolss_EnumPorts *r)
{
	NDR_SPOOLSS_PULL_ENUM(spoolss_EnumPorts,{
		r->in.servername= _r.in.servername;
	},{
		_r.in.servername= r->in.servername;
	});
	return NDR_ERR_SUCCESS;
}

uint32_t ndr_size_spoolss_EnumPorts_info(TALLOC_CTX *mem_ctx, struct smb_iconv_convenience *iconv_convenience, uint32_t level, uint32_t count, union spoolss_PortInfo *info)
{
	NDR_SPOOLSS_SIZE_ENUM_LEVEL(spoolss_EnumPorts);
}

/*
  spoolss_EnumMonitors
*/
enum ndr_err_code ndr_push_spoolss_EnumMonitors(struct ndr_push *ndr, int flags, const struct spoolss_EnumMonitors *r)
{
	NDR_SPOOLSS_PUSH_ENUM(spoolss_EnumMonitors,{
		_r.in.servername= r->in.servername;
	},{
		_r.in.servername= r->in.servername;
	});
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_EnumMonitors(struct ndr_pull *ndr, int flags, struct spoolss_EnumMonitors *r)
{
	NDR_SPOOLSS_PULL_ENUM(spoolss_EnumMonitors,{
		r->in.servername= _r.in.servername;
	},{
		_r.in.servername= r->in.servername;
	});
	return NDR_ERR_SUCCESS;
}

uint32_t ndr_size_spoolss_EnumMonitors_info(TALLOC_CTX *mem_ctx, struct smb_iconv_convenience *iconv_convenience, uint32_t level, uint32_t count, union spoolss_MonitorInfo *info)
{
	NDR_SPOOLSS_SIZE_ENUM_LEVEL(spoolss_EnumMonitors);
}

/*
  spoolss_EnumPrintProcessors
*/
enum ndr_err_code ndr_push_spoolss_EnumPrintProcessors(struct ndr_push *ndr, int flags, const struct spoolss_EnumPrintProcessors *r)
{
	NDR_SPOOLSS_PUSH_ENUM(spoolss_EnumPrintProcessors,{
		_r.in.servername	= r->in.servername;
		_r.in.environment	= r->in.environment;
	},{
		_r.in.servername	= r->in.servername;
		_r.in.environment	= r->in.environment;
	});
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_EnumPrintProcessors(struct ndr_pull *ndr, int flags, struct spoolss_EnumPrintProcessors *r)
{
	NDR_SPOOLSS_PULL_ENUM(spoolss_EnumPrintProcessors,{
		r->in.servername	= _r.in.servername;
		r->in.environment	= _r.in.environment;
	},{
		_r.in.servername	= r->in.servername;
		_r.in.environment	= r->in.environment;
	});
	return NDR_ERR_SUCCESS;
}

uint32_t ndr_size_spoolss_EnumPrintProcessors_info(TALLOC_CTX *mem_ctx, struct smb_iconv_convenience *iconv_convenience,
						   uint32_t level, uint32_t count, union spoolss_PrintProcessorInfo *info)
{
	NDR_SPOOLSS_SIZE_ENUM_LEVEL(spoolss_EnumPrintProcessors);
}

/*
  spoolss_EnumPrintProcessors
*/
enum ndr_err_code ndr_push_spoolss_EnumPrintProcDataTypes(struct ndr_push *ndr, int flags, const struct spoolss_EnumPrintProcDataTypes *r)
{
	NDR_SPOOLSS_PUSH_ENUM(spoolss_EnumPrintProcDataTypes,{
		_r.in.servername		= r->in.servername;
		_r.in.print_processor_name	= r->in.print_processor_name;
	},{
		_r.in.servername		= r->in.servername;
		_r.in.print_processor_name	= r->in.print_processor_name;
	});
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_EnumPrintProcDataTypes(struct ndr_pull *ndr, int flags, struct spoolss_EnumPrintProcDataTypes *r)
{
	NDR_SPOOLSS_PULL_ENUM(spoolss_EnumPrintProcDataTypes,{
		r->in.servername		= _r.in.servername;
		r->in.print_processor_name	= _r.in.print_processor_name;
	},{
		_r.in.servername		= r->in.servername;
		_r.in.print_processor_name	= r->in.print_processor_name;
	});
	return NDR_ERR_SUCCESS;
}

uint32_t ndr_size_spoolss_EnumPrintProcDataTypes_info(TALLOC_CTX *mem_ctx, struct smb_iconv_convenience *iconv_convenience,
						      uint32_t level, uint32_t count, union spoolss_PrintProcDataTypesInfo *info)
{
	NDR_SPOOLSS_SIZE_ENUM_LEVEL(spoolss_EnumPrintProcDataTypes);
}

/*
  spoolss_EnumPrinterDataEx
*/

enum ndr_err_code ndr_push_spoolss_EnumPrinterDataEx(struct ndr_push *ndr, int flags, const struct spoolss_EnumPrinterDataEx *r)
{
	struct _spoolss_EnumPrinterDataEx _r;
	if (flags & NDR_IN) {
		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		_r.in.offered	= r->in.offered;
		NDR_CHECK(ndr_push__spoolss_EnumPrinterDataEx(ndr, flags, &_r));
	}
	if (flags & NDR_OUT) {
		struct ndr_push *_ndr_info;
		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		_r.in.offered	= r->in.offered;
		_r.out.count	= r->out.count;
		_r.out.needed	= r->out.needed;
		_r.out.result	= r->out.result;
		_r.out.info 	= data_blob(NULL, 0);
		if (r->in.offered >= *r->out.needed) {
			struct __spoolss_EnumPrinterDataEx __r;
			_ndr_info = ndr_push_init_ctx(ndr, ndr->iconv_convenience);
			NDR_ERR_HAVE_NO_MEMORY(_ndr_info);
			_ndr_info->flags= ndr->flags;
			__r.in.count	= *r->out.count;
			__r.out.info	= *r->out.info;
			NDR_CHECK(ndr_push___spoolss_EnumPrinterDataEx(_ndr_info, flags, &__r));
			if (r->in.offered > _ndr_info->offset) {
				uint32_t _padding_len = r->in.offered - _ndr_info->offset;
				NDR_CHECK(ndr_push_zero(_ndr_info, _padding_len));
			}
			_r.out.info = ndr_push_blob(_ndr_info);
		}
		NDR_CHECK(ndr_push__spoolss_EnumPrinterDataEx(ndr, flags, &_r));
	}
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_EnumPrinterDataEx(struct ndr_pull *ndr, int flags, struct spoolss_EnumPrinterDataEx *r)
{
	struct _spoolss_EnumPrinterDataEx _r;
	if (flags & NDR_IN) {
		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		ZERO_STRUCT(r->out);
		NDR_CHECK(ndr_pull__spoolss_EnumPrinterDataEx(ndr, flags, &_r));
		r->in.handle	= _r.in.handle;
		r->in.key_name	= _r.in.key_name;
		r->in.offered	= _r.in.offered;
		r->out.needed	= _r.out.needed;
		r->out.count	= _r.out.count;
		NDR_PULL_ALLOC(ndr, r->out.info);
		ZERO_STRUCTP(r->out.info);
	}
	if (flags & NDR_OUT) {
		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		_r.in.offered	= r->in.offered;
		_r.out.count	= r->out.count;
		_r.out.needed	= r->out.needed;
		NDR_CHECK(ndr_pull__spoolss_EnumPrinterDataEx(ndr, flags, &_r));
		if (ndr->flags & LIBNDR_FLAG_REF_ALLOC) {
			NDR_PULL_ALLOC(ndr, r->out.info);
		}
		*r->out.info	= NULL;
		r->out.needed	= _r.out.needed;
		r->out.count	= _r.out.count;
		r->out.result	= _r.out.result;
		if (_r.out.info.length) {
			struct ndr_pull *_ndr_info;
			NDR_PULL_ALLOC(ndr, *r->out.info);
			_ndr_info = ndr_pull_init_blob(&_r.out.info, *r->out.info, ndr->iconv_convenience);
			NDR_ERR_HAVE_NO_MEMORY(_ndr_info);
			_ndr_info->flags= ndr->flags;
			if (r->in.offered != _ndr_info->data_size) {
				return ndr_pull_error(ndr, NDR_ERR_BUFSIZE,
					"SPOOLSS Buffer: offered[%u] doesn't match length of buffer[%u]",
					(unsigned)r->in.offered, (unsigned)_ndr_info->data_size);
			}
			if (*r->out.needed <= _ndr_info->data_size) {
				struct __spoolss_EnumPrinterDataEx __r;
				__r.in.count	= *r->out.count;
				__r.out.info	= NULL;
				NDR_CHECK(ndr_pull___spoolss_EnumPrinterDataEx(_ndr_info, flags, &__r));
				*r->out.info	= __r.out.info;
			}
		}
	}
	return NDR_ERR_SUCCESS;
}

uint32_t ndr_size_spoolss_EnumPrinterDataEx_info(TALLOC_CTX *mem_ctx, struct smb_iconv_convenience *iconv_convenience,
						 uint32_t count, struct spoolss_PrinterEnumValues *info)
{
	NDR_SPOOLSS_SIZE_ENUM(spoolss_EnumPrinterDataEx);
}

/*
  spoolss_GetPrinterData
*/
enum ndr_err_code ndr_push_spoolss_GetPrinterData(struct ndr_push *ndr, int flags, const struct spoolss_GetPrinterData *r)
{
	struct _spoolss_GetPrinterData _r;
	if (flags & NDR_IN) {
		_r.in.handle	= r->in.handle;
		_r.in.value_name= r->in.value_name;
		_r.in.offered	= r->in.offered;
		NDR_CHECK(ndr_push__spoolss_GetPrinterData(ndr, flags, &_r));
	}
	if (flags & NDR_OUT) {
		struct ndr_push *_ndr_info;
		DATA_BLOB blob = data_blob(NULL, 0);
		_r.in.handle	= r->in.handle;
		_r.in.value_name= r->in.value_name;
		_r.in.offered	= r->in.offered;
		_r.out.type	= r->out.type;
		_r.out.data	= &blob;
		_r.out.needed	= r->out.needed;
		_r.out.result	= r->out.result;
		{
			struct __spoolss_GetPrinterData __r;
			DATA_BLOB _blob;
			_ndr_info = ndr_push_init_ctx(ndr, ndr->iconv_convenience);
			NDR_ERR_HAVE_NO_MEMORY(_ndr_info);
			_ndr_info->flags= ndr->flags;
			__r.in.type	= *r->out.type;
			__r.out.data	= r->out.data;
			NDR_CHECK(ndr_push___spoolss_GetPrinterData(_ndr_info, flags, &__r));
			if (r->in.offered > _ndr_info->offset) {
				uint32_t _padding_len = r->in.offered - _ndr_info->offset;
				NDR_CHECK(ndr_push_zero(_ndr_info, _padding_len));
			}
			_blob = ndr_push_blob(_ndr_info);
			_r.out.data = &_blob;
		}
		NDR_CHECK(ndr_push__spoolss_GetPrinterData(ndr, flags, &_r));
	}
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_GetPrinterData(struct ndr_pull *ndr, int flags, struct spoolss_GetPrinterData *r)
{
	struct _spoolss_GetPrinterData _r;
	if (flags & NDR_IN) {
		DATA_BLOB blob = data_blob(NULL,0);
		ZERO_STRUCT(r->out);

		_r.in.handle	= r->in.handle;
		_r.in.value_name= r->in.value_name;
		_r.in.offered	= r->in.offered;
		_r.out.type	= r->out.type;
		_r.out.data	= &blob;
		_r.out.needed	= r->out.needed;
		NDR_CHECK(ndr_pull__spoolss_GetPrinterData(ndr, flags, &_r));
		r->in.handle	= _r.in.handle;
		r->in.value_name= _r.in.value_name;
		r->in.offered	= _r.in.offered;
		r->out.needed	= _r.out.needed;
	}
	if (flags & NDR_OUT) {
		DATA_BLOB blob = data_blob_talloc(ndr,NULL,0);
		_r.in.handle	= r->in.handle;
		_r.in.value_name= r->in.value_name;
		_r.in.offered	= r->in.offered;
		_r.out.type	= r->out.type;
		_r.out.data	= &blob;
		_r.out.needed	= r->out.needed;
		_r.out.result	= r->out.result;
		NDR_CHECK(ndr_pull__spoolss_GetPrinterData(ndr, flags, &_r));
		r->out.type	= _r.out.type;
		if (ndr->flags & LIBNDR_FLAG_REF_ALLOC) {
			NDR_PULL_ALLOC(ndr, r->out.data);
		}
		ZERO_STRUCTP(r->out.data);
		r->out.needed	= _r.out.needed;
		r->out.result	= _r.out.result;
		if (_r.out.data && _r.out.data->length != r->in.offered) {
			return ndr_pull_error(ndr, NDR_ERR_BUFSIZE,
				"SPOOLSS Buffer: r->in.offered[%u] doesn't match length of out buffer[%u]",
				(unsigned)r->in.offered, (unsigned)_r.out.data->length);
		}
		if (_r.out.data && _r.out.data->length > 0 && *r->out.needed <= _r.out.data->length) {
			struct __spoolss_GetPrinterData __r;
			struct ndr_pull *_ndr_data = ndr_pull_init_blob(_r.out.data, ndr, ndr->iconv_convenience);
			NDR_ERR_HAVE_NO_MEMORY(_ndr_data);
			_ndr_data->flags= ndr->flags;
			__r.in.type	= *r->out.type;
			__r.out.data	= r->out.data;
			NDR_CHECK(ndr_pull___spoolss_GetPrinterData(_ndr_data, flags, &__r));
			r->out.data	= __r.out.data;
		} else {
			*r->out.type	= REG_NONE;
		}
	}
	return NDR_ERR_SUCCESS;
}

/*
  spoolss_SetPrinterData
*/
enum ndr_err_code ndr_push_spoolss_SetPrinterData(struct ndr_push *ndr, int flags, const struct spoolss_SetPrinterData *r)
{
	struct _spoolss_SetPrinterData _r;
	if (flags & NDR_IN) {
		struct ndr_push *_ndr_data;
		struct __spoolss_SetPrinterData __r;
		DATA_BLOB _data_blob_data;

		_ndr_data = ndr_push_init_ctx(ndr, ndr->iconv_convenience);
		NDR_ERR_HAVE_NO_MEMORY(_ndr_data);
		_ndr_data->flags= ndr->flags;

		__r.in.type	= r->in.type;
		__r.out.data	= discard_const_p(union spoolss_PrinterData, &r->in.data);
		NDR_CHECK(ndr_push___spoolss_SetPrinterData(_ndr_data, NDR_OUT, &__r));
		_data_blob_data = ndr_push_blob(_ndr_data);

		_r.in.handle	= r->in.handle;
		_r.in.value_name= r->in.value_name;
		_r.in.type	= r->in.type;
		_r.in.data	= _data_blob_data;
		_r.in._offered	= _data_blob_data.length;
		_r.out.result	= r->out.result;
		NDR_CHECK(ndr_push__spoolss_SetPrinterData(ndr, flags, &_r));
	}
	if (flags & NDR_OUT) {
		_r.in.handle	= r->in.handle;
		_r.in.value_name= r->in.value_name;
		_r.in.type	= r->in.type;
		_r.in.data	= data_blob(NULL,0),
		_r.in._offered	= r->in._offered;
		_r.out.result	= r->out.result;
		NDR_CHECK(ndr_push__spoolss_SetPrinterData(ndr, flags, &_r));
	}
	return NDR_ERR_SUCCESS;
}

/*
  spoolss_GetPrinterDataEx
*/
enum ndr_err_code ndr_push_spoolss_GetPrinterDataEx(struct ndr_push *ndr, int flags, const struct spoolss_GetPrinterDataEx *r)
{
	struct _spoolss_GetPrinterDataEx _r;
	if (flags & NDR_IN) {
		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		_r.in.value_name= r->in.value_name;
		_r.in.offered	= r->in.offered;
		NDR_CHECK(ndr_push__spoolss_GetPrinterDataEx(ndr, flags, &_r));
	}
	if (flags & NDR_OUT) {
		struct ndr_push *_ndr_info;
		DATA_BLOB blob = data_blob(NULL, 0);
		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		_r.in.value_name= r->in.value_name;
		_r.in.offered	= r->in.offered;
		_r.out.type	= r->out.type;
		_r.out.data	= &blob;
		_r.out.needed	= r->out.needed;
		_r.out.result	= r->out.result;
		{
			struct __spoolss_GetPrinterDataEx __r;
			DATA_BLOB _blob;
			_ndr_info = ndr_push_init_ctx(ndr, ndr->iconv_convenience);
			NDR_ERR_HAVE_NO_MEMORY(_ndr_info);
			_ndr_info->flags= ndr->flags;
			__r.in.type	= *r->out.type;
			__r.out.data	= r->out.data;
			NDR_CHECK(ndr_push___spoolss_GetPrinterDataEx(_ndr_info, flags, &__r));
			if (r->in.offered > _ndr_info->offset) {
				uint32_t _padding_len = r->in.offered - _ndr_info->offset;
				NDR_CHECK(ndr_push_zero(_ndr_info, _padding_len));
			}
			_blob = ndr_push_blob(_ndr_info);
			_r.out.data = &_blob;
		}
		NDR_CHECK(ndr_push__spoolss_GetPrinterDataEx(ndr, flags, &_r));
	}
	return NDR_ERR_SUCCESS;
}

enum ndr_err_code ndr_pull_spoolss_GetPrinterDataEx(struct ndr_pull *ndr, int flags, struct spoolss_GetPrinterDataEx *r)
{
	struct _spoolss_GetPrinterDataEx _r;
	if (flags & NDR_IN) {
		DATA_BLOB blob = data_blob(NULL,0);
		ZERO_STRUCT(r->out);

		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		_r.in.value_name= r->in.value_name;
		_r.in.offered	= r->in.offered;
		_r.out.type	= r->out.type;
		_r.out.data	= &blob;
		_r.out.needed	= r->out.needed;
		NDR_CHECK(ndr_pull__spoolss_GetPrinterDataEx(ndr, flags, &_r));
		r->in.handle	= _r.in.handle;
		r->in.key_name	= _r.in.key_name;
		r->in.value_name= _r.in.value_name;
		r->in.offered	= _r.in.offered;
		r->out.needed	= _r.out.needed;
	}
	if (flags & NDR_OUT) {
		DATA_BLOB blob = data_blob_talloc(ndr,NULL,0);
		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		_r.in.value_name= r->in.value_name;
		_r.in.offered	= r->in.offered;
		_r.out.type	= r->out.type;
		_r.out.data	= &blob;
		_r.out.needed	= r->out.needed;
		_r.out.result	= r->out.result;
		NDR_CHECK(ndr_pull__spoolss_GetPrinterDataEx(ndr, flags, &_r));
		r->out.type	= _r.out.type;
		if (ndr->flags & LIBNDR_FLAG_REF_ALLOC) {
			NDR_PULL_ALLOC(ndr, r->out.data);
		}
		ZERO_STRUCTP(r->out.data);
		r->out.needed	= _r.out.needed;
		r->out.result	= _r.out.result;
		if (_r.out.data && _r.out.data->length != r->in.offered) {
			return ndr_pull_error(ndr, NDR_ERR_BUFSIZE,
				"SPOOLSS Buffer: r->in.offered[%u] doesn't match length of out buffer[%u]",
				(unsigned)r->in.offered, (unsigned)_r.out.data->length);
		}
		if (_r.out.data && _r.out.data->length > 0 && *r->out.needed <= _r.out.data->length) {
			struct __spoolss_GetPrinterDataEx __r;
			struct ndr_pull *_ndr_data = ndr_pull_init_blob(_r.out.data, ndr, ndr->iconv_convenience);
			NDR_ERR_HAVE_NO_MEMORY(_ndr_data);
			_ndr_data->flags= ndr->flags;
			__r.in.type	= *r->out.type;
			__r.out.data	= r->out.data;
			NDR_CHECK(ndr_pull___spoolss_GetPrinterDataEx(_ndr_data, flags, &__r));
			r->out.data	= __r.out.data;
		} else {
			*r->out.type	= REG_NONE;
		}
	}
	return NDR_ERR_SUCCESS;
}

/*
  spoolss_SetPrinterDataEx
*/
enum ndr_err_code ndr_push_spoolss_SetPrinterDataEx(struct ndr_push *ndr, int flags, const struct spoolss_SetPrinterDataEx *r)
{
	struct _spoolss_SetPrinterDataEx _r;
	if (flags & NDR_IN) {
		struct ndr_push *_ndr_data;
		struct __spoolss_SetPrinterDataEx __r;
		DATA_BLOB _data_blob_data;

		_ndr_data = ndr_push_init_ctx(ndr, ndr->iconv_convenience);
		NDR_ERR_HAVE_NO_MEMORY(_ndr_data);
		_ndr_data->flags= ndr->flags;

		__r.in.type	= r->in.type;
		__r.out.data	= discard_const_p(union spoolss_PrinterData, &r->in.data);
		NDR_CHECK(ndr_push___spoolss_SetPrinterDataEx(_ndr_data, NDR_OUT, &__r));
		_data_blob_data = ndr_push_blob(_ndr_data);

		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		_r.in.value_name= r->in.value_name;
		_r.in.type	= r->in.type;
		_r.in.data	= _data_blob_data;
		_r.in._offered	= _data_blob_data.length;
		_r.out.result	= r->out.result;
		NDR_CHECK(ndr_push__spoolss_SetPrinterDataEx(ndr, flags, &_r));
	}
	if (flags & NDR_OUT) {
		_r.in.handle	= r->in.handle;
		_r.in.key_name	= r->in.key_name;
		_r.in.value_name= r->in.value_name;
		_r.in.type	= r->in.type;
		_r.in.data	= data_blob(NULL,0),
		_r.in._offered	= r->in._offered;
		_r.out.result	= r->out.result;
		NDR_CHECK(ndr_push__spoolss_SetPrinterDataEx(ndr, flags, &_r));
	}
	return NDR_ERR_SUCCESS;
}

uint32_t _ndr_size_spoolss_DeviceMode(struct spoolss_DeviceMode *devmode, struct smb_iconv_convenience *ic, uint32_t flags)
{
	if (!devmode) return 0;
	return ndr_size_spoolss_DeviceMode(devmode,ic,flags);
}

_PUBLIC_ size_t ndr_size_spoolss_StringArray(const struct spoolss_StringArray *r, struct smb_iconv_convenience *ic, int flags)
{
	if (!r) {
		return 4;
	}

	return ndr_size_struct(r, flags, (ndr_push_flags_fn_t)ndr_push_spoolss_StringArray, ic);
}

/* hand marshall as pidl cannot (yet) generate a relative pointer to a fixed array of
 * structs */

_PUBLIC_ enum ndr_err_code ndr_push_spoolss_DriverInfo101(struct ndr_push *ndr, int ndr_flags, const struct spoolss_DriverInfo101 *r)
{
	uint32_t cntr_file_info_1;
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_push_align(ndr, 8));
		NDR_CHECK(ndr_push_spoolss_DriverOSVersion(ndr, NDR_SCALARS, r->version));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_relative_ptr1(ndr, r->driver_name));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_relative_ptr1(ndr, r->architecture));
			ndr->flags = _flags_save_string;
		}
		NDR_CHECK(ndr_push_relative_ptr1(ndr, r->file_info));
		NDR_CHECK(ndr_push_uint32(ndr, NDR_SCALARS, r->file_count));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_relative_ptr1(ndr, r->monitor_name));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_relative_ptr1(ndr, r->default_datatype));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string_array = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_relative_ptr1(ndr, r->previous_names));
			ndr->flags = _flags_save_string_array;
		}
		NDR_CHECK(ndr_push_NTTIME(ndr, NDR_SCALARS, r->driver_date));
		NDR_CHECK(ndr_push_hyper(ndr, NDR_SCALARS, r->driver_version));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_relative_ptr1(ndr, r->manufacturer_name));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_relative_ptr1(ndr, r->manufacturer_url));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_relative_ptr1(ndr, r->hardware_id));
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_push_relative_ptr1(ndr, r->provider));
			ndr->flags = _flags_save_string;
		}
	}
	if (ndr_flags & NDR_BUFFERS) {
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->driver_name) {
				NDR_CHECK(ndr_push_relative_ptr2(ndr, r->driver_name));
				NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->driver_name));
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->architecture) {
				NDR_CHECK(ndr_push_relative_ptr2(ndr, r->architecture));
				NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->architecture));
			}
			ndr->flags = _flags_save_string;
		}
		if (r->file_info) {
			NDR_CHECK(ndr_push_relative_ptr2(ndr, r->file_info));
#if 0
			NDR_CHECK(ndr_push_uint32(ndr, NDR_SCALARS, r->file_count));
#endif
			for (cntr_file_info_1 = 0; cntr_file_info_1 < r->file_count; cntr_file_info_1++) {
				NDR_CHECK(ndr_push_spoolss_DriverFileInfo(ndr, NDR_SCALARS, &r->file_info[cntr_file_info_1]));
			}
			for (cntr_file_info_1 = 0; cntr_file_info_1 < r->file_count; cntr_file_info_1++) {
				NDR_CHECK(ndr_push_spoolss_DriverFileInfo(ndr, NDR_BUFFERS, &r->file_info[cntr_file_info_1]));
			}
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->monitor_name) {
				NDR_CHECK(ndr_push_relative_ptr2(ndr, r->monitor_name));
				NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->monitor_name));
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->default_datatype) {
				NDR_CHECK(ndr_push_relative_ptr2(ndr, r->default_datatype));
				NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->default_datatype));
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string_array = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->previous_names) {
				NDR_CHECK(ndr_push_relative_ptr2(ndr, r->previous_names));
				NDR_CHECK(ndr_push_string_array(ndr, NDR_SCALARS, r->previous_names));
			}
			ndr->flags = _flags_save_string_array;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->manufacturer_name) {
				NDR_CHECK(ndr_push_relative_ptr2(ndr, r->manufacturer_name));
				NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->manufacturer_name));
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->manufacturer_url) {
				NDR_CHECK(ndr_push_relative_ptr2(ndr, r->manufacturer_url));
				NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->manufacturer_url));
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->hardware_id) {
				NDR_CHECK(ndr_push_relative_ptr2(ndr, r->hardware_id));
				NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->hardware_id));
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->provider) {
				NDR_CHECK(ndr_push_relative_ptr2(ndr, r->provider));
				NDR_CHECK(ndr_push_string(ndr, NDR_SCALARS, r->provider));
			}
			ndr->flags = _flags_save_string;
		}
	}
	return NDR_ERR_SUCCESS;
}

_PUBLIC_ enum ndr_err_code ndr_pull_spoolss_DriverInfo101(struct ndr_pull *ndr, int ndr_flags, struct spoolss_DriverInfo101 *r)
{
	uint32_t _ptr_driver_name;
	TALLOC_CTX *_mem_save_driver_name_0;
	uint32_t _ptr_architecture;
	TALLOC_CTX *_mem_save_architecture_0;
	uint32_t _ptr_file_info;
	uint32_t cntr_file_info_1;
	TALLOC_CTX *_mem_save_file_info_0;
	TALLOC_CTX *_mem_save_file_info_1;
	uint32_t _ptr_monitor_name;
	TALLOC_CTX *_mem_save_monitor_name_0;
	uint32_t _ptr_default_datatype;
	TALLOC_CTX *_mem_save_default_datatype_0;
	uint32_t _ptr_previous_names;
	TALLOC_CTX *_mem_save_previous_names_0;
	uint32_t _ptr_manufacturer_name;
	TALLOC_CTX *_mem_save_manufacturer_name_0;
	uint32_t _ptr_manufacturer_url;
	TALLOC_CTX *_mem_save_manufacturer_url_0;
	uint32_t _ptr_hardware_id;
	TALLOC_CTX *_mem_save_hardware_id_0;
	uint32_t _ptr_provider;
	TALLOC_CTX *_mem_save_provider_0;
	if (ndr_flags & NDR_SCALARS) {
		NDR_CHECK(ndr_pull_align(ndr, 8));
		NDR_CHECK(ndr_pull_spoolss_DriverOSVersion(ndr, NDR_SCALARS, &r->version));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_driver_name));
			if (_ptr_driver_name) {
				NDR_PULL_ALLOC(ndr, r->driver_name);
				NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->driver_name, _ptr_driver_name));
			} else {
				r->driver_name = NULL;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_architecture));
			if (_ptr_architecture) {
				NDR_PULL_ALLOC(ndr, r->architecture);
				NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->architecture, _ptr_architecture));
			} else {
				r->architecture = NULL;
			}
			ndr->flags = _flags_save_string;
		}
		NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_file_info));
		if (_ptr_file_info) {
			NDR_PULL_ALLOC(ndr, r->file_info);
			NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->file_info, _ptr_file_info));
		} else {
			r->file_info = NULL;
		}
		NDR_CHECK(ndr_pull_uint32(ndr, NDR_SCALARS, &r->file_count));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_monitor_name));
			if (_ptr_monitor_name) {
				NDR_PULL_ALLOC(ndr, r->monitor_name);
				NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->monitor_name, _ptr_monitor_name));
			} else {
				r->monitor_name = NULL;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_default_datatype));
			if (_ptr_default_datatype) {
				NDR_PULL_ALLOC(ndr, r->default_datatype);
				NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->default_datatype, _ptr_default_datatype));
			} else {
				r->default_datatype = NULL;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string_array = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_previous_names));
			if (_ptr_previous_names) {
				NDR_PULL_ALLOC(ndr, r->previous_names);
				NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->previous_names, _ptr_previous_names));
			} else {
				r->previous_names = NULL;
			}
			ndr->flags = _flags_save_string_array;
		}
		NDR_CHECK(ndr_pull_NTTIME(ndr, NDR_SCALARS, &r->driver_date));
		NDR_CHECK(ndr_pull_hyper(ndr, NDR_SCALARS, &r->driver_version));
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_manufacturer_name));
			if (_ptr_manufacturer_name) {
				NDR_PULL_ALLOC(ndr, r->manufacturer_name);
				NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->manufacturer_name, _ptr_manufacturer_name));
			} else {
				r->manufacturer_name = NULL;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_manufacturer_url));
			if (_ptr_manufacturer_url) {
				NDR_PULL_ALLOC(ndr, r->manufacturer_url);
				NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->manufacturer_url, _ptr_manufacturer_url));
			} else {
				r->manufacturer_url = NULL;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_hardware_id));
			if (_ptr_hardware_id) {
				NDR_PULL_ALLOC(ndr, r->hardware_id);
				NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->hardware_id, _ptr_hardware_id));
			} else {
				r->hardware_id = NULL;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			NDR_CHECK(ndr_pull_generic_ptr(ndr, &_ptr_provider));
			if (_ptr_provider) {
				NDR_PULL_ALLOC(ndr, r->provider);
				NDR_CHECK(ndr_pull_relative_ptr1(ndr, r->provider, _ptr_provider));
			} else {
				r->provider = NULL;
			}
			ndr->flags = _flags_save_string;
		}
	}
	if (ndr_flags & NDR_BUFFERS) {
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->driver_name) {
				uint32_t _relative_save_offset;
				_relative_save_offset = ndr->offset;
				NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->driver_name));
				_mem_save_driver_name_0 = NDR_PULL_GET_MEM_CTX(ndr);
				NDR_PULL_SET_MEM_CTX(ndr, r->driver_name, 0);
				NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->driver_name));
				NDR_PULL_SET_MEM_CTX(ndr, _mem_save_driver_name_0, 0);
				ndr->offset = _relative_save_offset;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->architecture) {
				uint32_t _relative_save_offset;
				_relative_save_offset = ndr->offset;
				NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->architecture));
				_mem_save_architecture_0 = NDR_PULL_GET_MEM_CTX(ndr);
				NDR_PULL_SET_MEM_CTX(ndr, r->architecture, 0);
				NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->architecture));
				NDR_PULL_SET_MEM_CTX(ndr, _mem_save_architecture_0, 0);
				ndr->offset = _relative_save_offset;
			}
			ndr->flags = _flags_save_string;
		}
		if (r->file_info) {
			uint32_t _relative_save_offset;
			_relative_save_offset = ndr->offset;
			NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->file_info));
			_mem_save_file_info_0 = NDR_PULL_GET_MEM_CTX(ndr);
			NDR_PULL_SET_MEM_CTX(ndr, r->file_info, 0);
#if 0
			NDR_CHECK(ndr_pull_array_size(ndr, &r->file_info));
#else
			NDR_CHECK(ndr_token_store(ndr, &ndr->array_size_list, &r->file_info, r->file_count));
#endif
			NDR_PULL_ALLOC_N(ndr, r->file_info, ndr_get_array_size(ndr, &r->file_info));
			_mem_save_file_info_1 = NDR_PULL_GET_MEM_CTX(ndr);
			NDR_PULL_SET_MEM_CTX(ndr, r->file_info, 0);
			for (cntr_file_info_1 = 0; cntr_file_info_1 < r->file_count; cntr_file_info_1++) {
				NDR_CHECK(ndr_pull_spoolss_DriverFileInfo(ndr, NDR_SCALARS, &r->file_info[cntr_file_info_1]));
			}
			for (cntr_file_info_1 = 0; cntr_file_info_1 < r->file_count; cntr_file_info_1++) {
				NDR_CHECK(ndr_pull_spoolss_DriverFileInfo(ndr, NDR_BUFFERS, &r->file_info[cntr_file_info_1]));
			}
			NDR_PULL_SET_MEM_CTX(ndr, _mem_save_file_info_1, 0);
			NDR_PULL_SET_MEM_CTX(ndr, _mem_save_file_info_0, 0);
			ndr->offset = _relative_save_offset;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->monitor_name) {
				uint32_t _relative_save_offset;
				_relative_save_offset = ndr->offset;
				NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->monitor_name));
				_mem_save_monitor_name_0 = NDR_PULL_GET_MEM_CTX(ndr);
				NDR_PULL_SET_MEM_CTX(ndr, r->monitor_name, 0);
				NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->monitor_name));
				NDR_PULL_SET_MEM_CTX(ndr, _mem_save_monitor_name_0, 0);
				ndr->offset = _relative_save_offset;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->default_datatype) {
				uint32_t _relative_save_offset;
				_relative_save_offset = ndr->offset;
				NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->default_datatype));
				_mem_save_default_datatype_0 = NDR_PULL_GET_MEM_CTX(ndr);
				NDR_PULL_SET_MEM_CTX(ndr, r->default_datatype, 0);
				NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->default_datatype));
				NDR_PULL_SET_MEM_CTX(ndr, _mem_save_default_datatype_0, 0);
				ndr->offset = _relative_save_offset;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string_array = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->previous_names) {
				uint32_t _relative_save_offset;
				_relative_save_offset = ndr->offset;
				NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->previous_names));
				_mem_save_previous_names_0 = NDR_PULL_GET_MEM_CTX(ndr);
				NDR_PULL_SET_MEM_CTX(ndr, r->previous_names, 0);
				NDR_CHECK(ndr_pull_string_array(ndr, NDR_SCALARS, &r->previous_names));
				NDR_PULL_SET_MEM_CTX(ndr, _mem_save_previous_names_0, 0);
				ndr->offset = _relative_save_offset;
			}
			ndr->flags = _flags_save_string_array;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->manufacturer_name) {
				uint32_t _relative_save_offset;
				_relative_save_offset = ndr->offset;
				NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->manufacturer_name));
				_mem_save_manufacturer_name_0 = NDR_PULL_GET_MEM_CTX(ndr);
				NDR_PULL_SET_MEM_CTX(ndr, r->manufacturer_name, 0);
				NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->manufacturer_name));
				NDR_PULL_SET_MEM_CTX(ndr, _mem_save_manufacturer_name_0, 0);
				ndr->offset = _relative_save_offset;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->manufacturer_url) {
				uint32_t _relative_save_offset;
				_relative_save_offset = ndr->offset;
				NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->manufacturer_url));
				_mem_save_manufacturer_url_0 = NDR_PULL_GET_MEM_CTX(ndr);
				NDR_PULL_SET_MEM_CTX(ndr, r->manufacturer_url, 0);
				NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->manufacturer_url));
				NDR_PULL_SET_MEM_CTX(ndr, _mem_save_manufacturer_url_0, 0);
				ndr->offset = _relative_save_offset;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->hardware_id) {
				uint32_t _relative_save_offset;
				_relative_save_offset = ndr->offset;
				NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->hardware_id));
				_mem_save_hardware_id_0 = NDR_PULL_GET_MEM_CTX(ndr);
				NDR_PULL_SET_MEM_CTX(ndr, r->hardware_id, 0);
				NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->hardware_id));
				NDR_PULL_SET_MEM_CTX(ndr, _mem_save_hardware_id_0, 0);
				ndr->offset = _relative_save_offset;
			}
			ndr->flags = _flags_save_string;
		}
		{
			uint32_t _flags_save_string = ndr->flags;
			ndr_set_flags(&ndr->flags, LIBNDR_FLAG_STR_NULLTERM);
			if (r->provider) {
				uint32_t _relative_save_offset;
				_relative_save_offset = ndr->offset;
				NDR_CHECK(ndr_pull_relative_ptr2(ndr, r->provider));
				_mem_save_provider_0 = NDR_PULL_GET_MEM_CTX(ndr);
				NDR_PULL_SET_MEM_CTX(ndr, r->provider, 0);
				NDR_CHECK(ndr_pull_string(ndr, NDR_SCALARS, &r->provider));
				NDR_PULL_SET_MEM_CTX(ndr, _mem_save_provider_0, 0);
				ndr->offset = _relative_save_offset;
			}
			ndr->flags = _flags_save_string;
		}
		if (r->file_info) {
			NDR_CHECK(ndr_check_array_size(ndr, (void*)&r->file_info, r->file_count));
		}
	}
	return NDR_ERR_SUCCESS;
}

void ndr_print_spoolss_Field(struct ndr_print *ndr, const char *name, const union spoolss_Field *r)
{
	int level;
	level = ndr_print_get_switch_value(ndr, r);
	ndr_print_union(ndr, name, level, "spoolss_Field");
	switch (level) {
		case PRINTER_NOTIFY_TYPE:
			ndr_print_spoolss_PrintNotifyField(ndr, "field", r->field);
		break;

		case JOB_NOTIFY_TYPE:
			ndr_print_spoolss_JobNotifyField(ndr, "field", r->field);
		break;

		default:
			ndr_print_uint16(ndr, "field", r->field);
		break;

	}
}

_PUBLIC_ size_t ndr_size_spoolss_PrinterData(const union spoolss_PrinterData *r, uint32_t level, struct smb_iconv_convenience *ic, int flags)
{
	if (!r) {
		return 0;
	}
	return ndr_size_union(r, flags, level, (ndr_push_flags_fn_t)ndr_push_spoolss_PrinterData, ic);
}

