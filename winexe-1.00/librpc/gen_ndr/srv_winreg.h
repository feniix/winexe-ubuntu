#include "../librpc/gen_ndr/ndr_winreg.h"
#ifndef __SRV_WINREG__
#define __SRV_WINREG__
WERROR _winreg_OpenHKCR(pipes_struct *p, struct winreg_OpenHKCR *r);
WERROR _winreg_OpenHKCU(pipes_struct *p, struct winreg_OpenHKCU *r);
WERROR _winreg_OpenHKLM(pipes_struct *p, struct winreg_OpenHKLM *r);
WERROR _winreg_OpenHKPD(pipes_struct *p, struct winreg_OpenHKPD *r);
WERROR _winreg_OpenHKU(pipes_struct *p, struct winreg_OpenHKU *r);
WERROR _winreg_CloseKey(pipes_struct *p, struct winreg_CloseKey *r);
WERROR _winreg_CreateKey(pipes_struct *p, struct winreg_CreateKey *r);
WERROR _winreg_DeleteKey(pipes_struct *p, struct winreg_DeleteKey *r);
WERROR _winreg_DeleteValue(pipes_struct *p, struct winreg_DeleteValue *r);
WERROR _winreg_EnumKey(pipes_struct *p, struct winreg_EnumKey *r);
WERROR _winreg_EnumValue(pipes_struct *p, struct winreg_EnumValue *r);
WERROR _winreg_FlushKey(pipes_struct *p, struct winreg_FlushKey *r);
WERROR _winreg_GetKeySecurity(pipes_struct *p, struct winreg_GetKeySecurity *r);
WERROR _winreg_LoadKey(pipes_struct *p, struct winreg_LoadKey *r);
WERROR _winreg_NotifyChangeKeyValue(pipes_struct *p, struct winreg_NotifyChangeKeyValue *r);
WERROR _winreg_OpenKey(pipes_struct *p, struct winreg_OpenKey *r);
WERROR _winreg_QueryInfoKey(pipes_struct *p, struct winreg_QueryInfoKey *r);
WERROR _winreg_QueryValue(pipes_struct *p, struct winreg_QueryValue *r);
WERROR _winreg_ReplaceKey(pipes_struct *p, struct winreg_ReplaceKey *r);
WERROR _winreg_RestoreKey(pipes_struct *p, struct winreg_RestoreKey *r);
WERROR _winreg_SaveKey(pipes_struct *p, struct winreg_SaveKey *r);
WERROR _winreg_SetKeySecurity(pipes_struct *p, struct winreg_SetKeySecurity *r);
WERROR _winreg_SetValue(pipes_struct *p, struct winreg_SetValue *r);
WERROR _winreg_UnLoadKey(pipes_struct *p, struct winreg_UnLoadKey *r);
WERROR _winreg_InitiateSystemShutdown(pipes_struct *p, struct winreg_InitiateSystemShutdown *r);
WERROR _winreg_AbortSystemShutdown(pipes_struct *p, struct winreg_AbortSystemShutdown *r);
WERROR _winreg_GetVersion(pipes_struct *p, struct winreg_GetVersion *r);
WERROR _winreg_OpenHKCC(pipes_struct *p, struct winreg_OpenHKCC *r);
WERROR _winreg_OpenHKDD(pipes_struct *p, struct winreg_OpenHKDD *r);
WERROR _winreg_QueryMultipleValues(pipes_struct *p, struct winreg_QueryMultipleValues *r);
WERROR _winreg_InitiateSystemShutdownEx(pipes_struct *p, struct winreg_InitiateSystemShutdownEx *r);
WERROR _winreg_SaveKeyEx(pipes_struct *p, struct winreg_SaveKeyEx *r);
WERROR _winreg_OpenHKPT(pipes_struct *p, struct winreg_OpenHKPT *r);
WERROR _winreg_OpenHKPN(pipes_struct *p, struct winreg_OpenHKPN *r);
WERROR _winreg_QueryMultipleValues2(pipes_struct *p, struct winreg_QueryMultipleValues2 *r);
void winreg_get_pipe_fns(struct api_struct **fns, int *n_fns);
NTSTATUS rpc_winreg_dispatch(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, const struct ndr_interface_table *table, uint32_t opnum, void *r);
WERROR _winreg_OpenHKCR(pipes_struct *p, struct winreg_OpenHKCR *r);
WERROR _winreg_OpenHKCU(pipes_struct *p, struct winreg_OpenHKCU *r);
WERROR _winreg_OpenHKLM(pipes_struct *p, struct winreg_OpenHKLM *r);
WERROR _winreg_OpenHKPD(pipes_struct *p, struct winreg_OpenHKPD *r);
WERROR _winreg_OpenHKU(pipes_struct *p, struct winreg_OpenHKU *r);
WERROR _winreg_CloseKey(pipes_struct *p, struct winreg_CloseKey *r);
WERROR _winreg_CreateKey(pipes_struct *p, struct winreg_CreateKey *r);
WERROR _winreg_DeleteKey(pipes_struct *p, struct winreg_DeleteKey *r);
WERROR _winreg_DeleteValue(pipes_struct *p, struct winreg_DeleteValue *r);
WERROR _winreg_EnumKey(pipes_struct *p, struct winreg_EnumKey *r);
WERROR _winreg_EnumValue(pipes_struct *p, struct winreg_EnumValue *r);
WERROR _winreg_FlushKey(pipes_struct *p, struct winreg_FlushKey *r);
WERROR _winreg_GetKeySecurity(pipes_struct *p, struct winreg_GetKeySecurity *r);
WERROR _winreg_LoadKey(pipes_struct *p, struct winreg_LoadKey *r);
WERROR _winreg_NotifyChangeKeyValue(pipes_struct *p, struct winreg_NotifyChangeKeyValue *r);
WERROR _winreg_OpenKey(pipes_struct *p, struct winreg_OpenKey *r);
WERROR _winreg_QueryInfoKey(pipes_struct *p, struct winreg_QueryInfoKey *r);
WERROR _winreg_QueryValue(pipes_struct *p, struct winreg_QueryValue *r);
WERROR _winreg_ReplaceKey(pipes_struct *p, struct winreg_ReplaceKey *r);
WERROR _winreg_RestoreKey(pipes_struct *p, struct winreg_RestoreKey *r);
WERROR _winreg_SaveKey(pipes_struct *p, struct winreg_SaveKey *r);
WERROR _winreg_SetKeySecurity(pipes_struct *p, struct winreg_SetKeySecurity *r);
WERROR _winreg_SetValue(pipes_struct *p, struct winreg_SetValue *r);
WERROR _winreg_UnLoadKey(pipes_struct *p, struct winreg_UnLoadKey *r);
WERROR _winreg_InitiateSystemShutdown(pipes_struct *p, struct winreg_InitiateSystemShutdown *r);
WERROR _winreg_AbortSystemShutdown(pipes_struct *p, struct winreg_AbortSystemShutdown *r);
WERROR _winreg_GetVersion(pipes_struct *p, struct winreg_GetVersion *r);
WERROR _winreg_OpenHKCC(pipes_struct *p, struct winreg_OpenHKCC *r);
WERROR _winreg_OpenHKDD(pipes_struct *p, struct winreg_OpenHKDD *r);
WERROR _winreg_QueryMultipleValues(pipes_struct *p, struct winreg_QueryMultipleValues *r);
WERROR _winreg_InitiateSystemShutdownEx(pipes_struct *p, struct winreg_InitiateSystemShutdownEx *r);
WERROR _winreg_SaveKeyEx(pipes_struct *p, struct winreg_SaveKeyEx *r);
WERROR _winreg_OpenHKPT(pipes_struct *p, struct winreg_OpenHKPT *r);
WERROR _winreg_OpenHKPN(pipes_struct *p, struct winreg_OpenHKPN *r);
WERROR _winreg_QueryMultipleValues2(pipes_struct *p, struct winreg_QueryMultipleValues2 *r);
NTSTATUS rpc_winreg_init(void);
#endif /* __SRV_WINREG__ */
