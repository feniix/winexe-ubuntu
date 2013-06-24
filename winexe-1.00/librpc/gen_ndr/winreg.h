/* header auto-generated by pidl */

#include <stdint.h>

#include "libcli/util/ntstatus.h"

#include "librpc/gen_ndr/lsa.h"
#include "librpc/gen_ndr/security.h"
#include "librpc/gen_ndr/misc.h"
#ifndef _HEADER_winreg
#define _HEADER_winreg

#define REG_KEY_READ	( (STANDARD_RIGHTS_READ_ACCESS|KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS|KEY_NOTIFY) )
#define REG_KEY_EXECUTE	( REG_KEY_READ )
#define REG_KEY_WRITE	( (STANDARD_RIGHTS_WRITE_ACCESS|KEY_SET_VALUE|KEY_CREATE_SUB_KEY) )
#define REG_KEY_ALL	( (STANDARD_RIGHTS_REQUIRED_ACCESS|REG_KEY_READ|REG_KEY_WRITE|KEY_CREATE_LINK) )
/* bitmap winreg_AccessMask */
#define KEY_QUERY_VALUE ( 0x00001 )
#define KEY_SET_VALUE ( 0x00002 )
#define KEY_CREATE_SUB_KEY ( 0x00004 )
#define KEY_ENUMERATE_SUB_KEYS ( 0x00008 )
#define KEY_NOTIFY ( 0x00010 )
#define KEY_CREATE_LINK ( 0x00020 )
#define KEY_WOW64_64KEY ( 0x00100 )
#define KEY_WOW64_32KEY ( 0x00200 )

struct winreg_String {
	uint16_t name_len;/* [value(strlen_m_term(name)*2)] */
	uint16_t name_size;/* [value(strlen_m_term(name)*2)] */
	const char *name;/* [unique,charset(UTF16)] */
}/* [public] */;

struct KeySecurityData {
	uint8_t *data;/* [unique,length_is(len),size_is(size)] */
	uint32_t size;
	uint32_t len;
};

struct winreg_SecBuf {
	uint32_t length;
	struct KeySecurityData sd;
	uint8_t inherit;
};

enum winreg_CreateAction
#ifndef USE_UINT_ENUMS
 {
	REG_ACTION_NONE=(int)(0),
	REG_CREATED_NEW_KEY=(int)(1),
	REG_OPENED_EXISTING_KEY=(int)(2)
}
#else
 { __donnot_use_enum_winreg_CreateAction=0x7FFFFFFF}
#define REG_ACTION_NONE ( 0 )
#define REG_CREATED_NEW_KEY ( 1 )
#define REG_OPENED_EXISTING_KEY ( 2 )
#endif
;

struct winreg_StringBuf {
	uint16_t length;/* [value(strlen_m_term_null(name)*2)] */
	uint16_t size;
	const char *name;/* [unique,length_is(length/2),charset(UTF16),size_is(size/2)] */
};

struct winreg_ValNameBuf {
	uint16_t length;/* [value(strlen_m_term(name)*2)] */
	uint16_t size;
	const char *name;/* [unique,length_is(length/2),charset(UTF16),size_is(size/2)] */
};

/* bitmap winreg_NotifyChangeType */
#define REG_NOTIFY_CHANGE_NAME ( 0x00000001 )
#define REG_NOTIFY_CHANGE_ATTRIBUTES ( 0x00000002 )
#define REG_NOTIFY_CHANGE_LAST_SET ( 0x00000004 )
#define REG_NOTIFY_CHANGE_SECURITY ( 0x00000008 )

struct KeySecurityAttribute {
	uint32_t data_size;
	struct KeySecurityData sec_data;
	uint8_t inherit;
};

struct QueryMultipleValue {
	struct winreg_String *name;/* [unique] */
	enum winreg_Type type;
	uint32_t offset;
	uint32_t length;
};


struct winreg_OpenHKCR {
	struct {
		uint16_t *system_name;/* [unique] */
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_OpenHKCU {
	struct {
		uint16_t *system_name;/* [unique] */
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_OpenHKLM {
	struct {
		uint16_t *system_name;/* [unique] */
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_OpenHKPD {
	struct {
		uint16_t *system_name;/* [unique] */
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_OpenHKU {
	struct {
		uint16_t *system_name;/* [unique] */
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_CloseKey {
	struct {
		struct policy_handle *handle;/* [ref] */
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_CreateKey {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct winreg_String name;
		struct winreg_String keyclass;
		uint32_t options;
		uint32_t access_mask;
		struct winreg_SecBuf *secdesc;/* [unique] */
		enum winreg_CreateAction *action_taken;/* [unique] */
	} in;

	struct {
		struct policy_handle *new_handle;/* [ref] */
		enum winreg_CreateAction *action_taken;/* [unique] */
		WERROR result;
	} out;

};


struct winreg_DeleteKey {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct winreg_String key;
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_DeleteValue {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct winreg_String value;
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_EnumKey {
	struct {
		struct policy_handle *handle;/* [ref] */
		uint32_t enum_index;
		struct winreg_StringBuf *name;/* [ref] */
		struct winreg_StringBuf *keyclass;/* [unique] */
		NTTIME *last_changed_time;/* [unique] */
	} in;

	struct {
		struct winreg_StringBuf *name;/* [ref] */
		struct winreg_StringBuf *keyclass;/* [unique] */
		NTTIME *last_changed_time;/* [unique] */
		WERROR result;
	} out;

};


struct winreg_EnumValue {
	struct {
		struct policy_handle *handle;/* [ref] */
		uint32_t enum_index;
		struct winreg_ValNameBuf *name;/* [ref] */
		enum winreg_Type *type;/* [unique] */
		uint8_t *value;/* [unique,length_is(*length),size_is(*size)] */
		uint32_t *size;/* [unique] */
		uint32_t *length;/* [unique] */
	} in;

	struct {
		struct winreg_ValNameBuf *name;/* [ref] */
		enum winreg_Type *type;/* [unique] */
		uint8_t *value;/* [unique,length_is(*length),size_is(*size)] */
		uint32_t *size;/* [unique] */
		uint32_t *length;/* [unique] */
		WERROR result;
	} out;

};


struct winreg_FlushKey {
	struct {
		struct policy_handle *handle;/* [ref] */
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_GetKeySecurity {
	struct {
		struct policy_handle *handle;/* [ref] */
		uint32_t sec_info;
		struct KeySecurityData *sd;/* [ref] */
	} in;

	struct {
		struct KeySecurityData *sd;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_LoadKey {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct winreg_String *keyname;/* [unique] */
		struct winreg_String *filename;/* [unique] */
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_NotifyChangeKeyValue {
	struct {
		struct policy_handle *handle;/* [ref] */
		uint8_t watch_subtree;
		uint32_t notify_filter;
		uint32_t unknown;
		struct winreg_String string1;
		struct winreg_String string2;
		uint32_t unknown2;
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_OpenKey {
	struct {
		struct policy_handle *parent_handle;/* [ref] */
		struct winreg_String keyname;
		uint32_t unknown;
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_QueryInfoKey {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct winreg_String *classname;/* [ref] */
	} in;

	struct {
		uint32_t *num_subkeys;/* [ref] */
		uint32_t *max_subkeylen;/* [ref] */
		uint32_t *max_classlen;/* [ref] */
		uint32_t *num_values;/* [ref] */
		uint32_t *max_valnamelen;/* [ref] */
		uint32_t *max_valbufsize;/* [ref] */
		uint32_t *secdescsize;/* [ref] */
		NTTIME *last_changed_time;/* [ref] */
		struct winreg_String *classname;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_QueryValue {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct winreg_String *value_name;/* [ref] */
		enum winreg_Type *type;/* [unique] */
		uint8_t *data;/* [unique,length_is(*data_length),size_is(*data_size)] */
		uint32_t *data_size;/* [unique] */
		uint32_t *data_length;/* [unique] */
	} in;

	struct {
		enum winreg_Type *type;/* [unique] */
		uint8_t *data;/* [unique,length_is(*data_length),size_is(*data_size)] */
		uint32_t *data_size;/* [unique] */
		uint32_t *data_length;/* [unique] */
		WERROR result;
	} out;

};


struct winreg_ReplaceKey {
	struct {
		WERROR result;
	} out;

};


struct winreg_RestoreKey {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct winreg_String *filename;/* [ref] */
		uint32_t flags;
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_SaveKey {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct winreg_String *filename;/* [ref] */
		struct KeySecurityAttribute *sec_attrib;/* [unique] */
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_SetKeySecurity {
	struct {
		struct policy_handle *handle;/* [ref] */
		uint32_t sec_info;
		struct KeySecurityData *sd;/* [ref] */
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_SetValue {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct winreg_String name;
		enum winreg_Type type;
		uint8_t *data;/* [ref,size_is(size)] */
		uint32_t size;
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_UnLoadKey {
	struct {
		WERROR result;
	} out;

};


struct winreg_InitiateSystemShutdown {
	struct {
		uint16_t *hostname;/* [unique] */
		struct lsa_StringLarge *message;/* [unique] */
		uint32_t timeout;
		uint8_t force_apps;
		uint8_t do_reboot;
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_AbortSystemShutdown {
	struct {
		uint16_t *server;/* [unique] */
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_GetVersion {
	struct {
		struct policy_handle *handle;/* [ref] */
	} in;

	struct {
		uint32_t *version;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_OpenHKCC {
	struct {
		uint16_t *system_name;/* [unique] */
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_OpenHKDD {
	struct {
		uint16_t *system_name;/* [unique] */
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_QueryMultipleValues {
	struct {
		struct policy_handle *key_handle;/* [ref] */
		uint32_t num_values;
		struct QueryMultipleValue *values;/* [ref,length_is(num_values),size_is(num_values)] */
		uint8_t *buffer;/* [unique,length_is(*buffer_size),size_is(*buffer_size)] */
		uint32_t *buffer_size;/* [ref] */
	} in;

	struct {
		struct QueryMultipleValue *values;/* [ref,length_is(num_values),size_is(num_values)] */
		uint8_t *buffer;/* [unique,length_is(*buffer_size),size_is(*buffer_size)] */
		uint32_t *buffer_size;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_InitiateSystemShutdownEx {
	struct {
		uint16_t *hostname;/* [unique] */
		struct lsa_StringLarge *message;/* [unique] */
		uint32_t timeout;
		uint8_t force_apps;
		uint8_t do_reboot;
		uint32_t reason;
	} in;

	struct {
		WERROR result;
	} out;

};


struct winreg_SaveKeyEx {
	struct {
		WERROR result;
	} out;

};


struct winreg_OpenHKPT {
	struct {
		uint16_t *system_name;/* [unique] */
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_OpenHKPN {
	struct {
		uint16_t *system_name;/* [unique] */
		uint32_t access_mask;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		WERROR result;
	} out;

};


struct winreg_QueryMultipleValues2 {
	struct {
		WERROR result;
	} out;

};

#endif /* _HEADER_winreg */
