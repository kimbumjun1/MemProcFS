// m_fc.c : implementation of forensic info & startup module.
//
// (c) Ulf Frisk, 2020
// Author: Ulf Frisk, pcileech@frizk.net
//

#include "fc.h"
#include "pluginmanager.h"
#include "util.h"

LPCSTR szMFC_README =
"Information about MemProcFS forensics:                                      \n" \
"======================================                                      \n" \
"MemProcFS forensics is a collection of batch oriented comprehensive analysis\n" \
"tasks that may be initiated by the user if the prerequisites below are met: \n" \
" - static non-volatile memory being analyzed (i.e. only memory dump file).  \n" \
" - no live memory (first dump to file before running MemProcFS forensics).  \n" \
"                                                                            \n" \
"MemProcFS forensics performs the batch oriented comprehensive analysis tasks\n" \
"and outputs the result into a sqlite database and displays the result in the\n" \
"forensic sub-directory. Analysis tasks include (but are not limited to):    \n" \
" - NTFS MFT scanning.                                                       \n" \
" - Timeline analysis of Processes, Registry, NTFS MFT, Plugins and more.    \n" \
"                                                                            \n" \
"MemProcFS forensics is initialized by changing the file forensic_enable.txt.\n" \
"Once forensic_enable.txt is updated initialization of MemProcFS forensics   \n" \
"will take place. This may take some time due to scanning of the whole memory\n" \
"image and timeline construction.                                            \n" \
"If possible the file name of the sqlite will show in database.txt.          \n" \
"Valid initialization values for forensic_enable.txt are listed below:       \n" \
" 1 = in-memory sqlite database.                                             \n" \
" 2 = temporary sqlite database which will be deleted upon MemProcFS exit.   \n" \
" 3 = temporary sqlite database which will remain upon MemProcFS exit.       \n" \
" 4 = static named sqlite database (vmm.sqlite3).                            \n" \
"Note! MemProcFS forensics may take some time to initialize. Once intialized \n" \
"sub-directories under the forensic directory will show up automatically.    \n" \
"                                                                            \n" \
"For additional information about MemProcFS forensics check out the guide at:\n" \
"https://github.com/ufrisk/MemProcFS/wiki                                    \n";

NTSTATUS M_Fc_Read(_In_ PVMMDLL_PLUGIN_CONTEXT ctx, _Out_writes_to_(cb, *pcbRead) PBYTE pb, _In_ DWORD cb, _Out_ PDWORD pcbRead, _In_ QWORD cbOffset)
{
    BYTE btp;
    if(!wcscmp(ctx->wszPath, L"readme.txt")) {
        return Util_VfsReadFile_FromPBYTE((PBYTE)szMFC_README, strlen(szMFC_README), pb, cb, pcbRead, cbOffset);
    }
    if(!_wcsicmp(ctx->wszPath, L"forensic_enable.txt")) {
        btp = '0' + (ctxFc ? (BYTE)ctxFc->db.tp : 0);
        return Util_VfsReadFile_FromPBYTE(&btp, 1, pb, cb, pcbRead, cbOffset);
    }
    if(!_wcsicmp(ctx->wszPath, L"database.txt")) {
        if(ctxFc) {
            return Util_VfsReadFile_FromTextWtoU8(ctxFc->db.wszDatabaseWinPath, pb, cb, pcbRead, cbOffset);
        } else {
            return Util_VfsReadFile_FromPBYTE(NULL, 0, pb, cb, pcbRead, cbOffset);
        }
    }
    return VMMDLL_STATUS_FILE_INVALID;
}

NTSTATUS M_Fc_Write(_In_ PVMMDLL_PLUGIN_CONTEXT ctx, _In_reads_(cb) PBYTE pb, _In_ DWORD cb, _Out_ PDWORD pcbWrite, _In_ QWORD cbOffset)
{
    DWORD dwDatabaseType = 0;
    NTSTATUS nt;
    if(!_wcsicmp(ctx->wszPath, L"forensic_enable.txt")) {
        nt = Util_VfsWriteFile_09(&dwDatabaseType, pb, cb, pcbWrite, cbOffset);
        FcInitialize(dwDatabaseType, FALSE);
        return nt;
    }
    return VMMDLL_STATUS_FILE_INVALID;
}

BOOL M_Fc_List(_In_ PVMMDLL_PLUGIN_CONTEXT ctx, _Inout_ PHANDLE pFileList)
{
    VMMDLL_VfsList_AddFile(pFileList, L"forensic_enable.txt", 1, NULL);
    VMMDLL_VfsList_AddFile(pFileList, L"database.txt", ctxFc ? wcslen_u8(ctxFc->db.wszDatabaseWinPath) : 0, NULL);
    VMMDLL_VfsList_AddFile(pFileList, L"readme.txt", strlen(szMFC_README), NULL);
    return TRUE;
}

VOID M_Fc_Initialize(_Inout_ PVMMDLL_PLUGIN_REGINFO pRI)
{
    if((pRI->magic != VMMDLL_PLUGIN_REGINFO_MAGIC) || (pRI->wVersion != VMMDLL_PLUGIN_REGINFO_VERSION)) { return; }
    if((pRI->tpSystem != VMM_SYSTEM_WINDOWS_X64) && (pRI->tpSystem != VMM_SYSTEM_WINDOWS_X86)) { return; }
    if(ctxMain->dev.fVolatile) { return; }
    wcscpy_s(pRI->reg_info.wszPathName, 128, L"\\forensic");                    // module name
    pRI->reg_info.fRootModule = TRUE;                                           // module shows in root directory
    pRI->reg_fn.pfnList = M_Fc_List;                                            // List function supported
    pRI->reg_fn.pfnRead = M_Fc_Read;                                            // Read function supported
    pRI->reg_fn.pfnWrite = M_Fc_Write;                                          // Read function supported
    pRI->pfnPluginManager_Register(pRI);
}
