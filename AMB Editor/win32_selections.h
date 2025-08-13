
// This file contains only the Win32 definitions needed by the AMB editor. Normally one would #include the needed header files, however that's not
// possible because they aren't available with TCC.

// Common Control definitions (normally from commctrl.h)
#define WC_LISTVIEW "SysListView32"
#define LVS_REPORT 0x0001
#define LVS_SHOWSELALWAYS 0x0008
#define LVS_SINGLESEL 0x0004
#define LVS_EDITLABELS 0x0200
#define LVS_NOHSCROLL 0x8000
#define LVM_INSERTCOLUMN (LVM_FIRST + 27)
#define LVM_INSERTITEM (LVM_FIRST + 7)
#define LVM_SETITEM (LVM_FIRST + 6)
#define LVM_SETITEMSTATE (LVM_FIRST + 43)
#define LVM_GETITEMCOUNT (LVM_FIRST + 4)
#define LVM_DELETEALLITEMS (LVM_FIRST + 9)
#define LVM_GETITEMSTATE (LVM_FIRST + 44)
#define LVM_FIRST 0x1000
#define LVCF_TEXT 0x0004
#define LVCF_WIDTH 0x0002
#define LVIF_TEXT 0x0001
#define LVIF_STATE 0x0008
#define LVIS_STATEIMAGEMASK 0xF000
#define LVIS_SELECTED 0x0002
#define LVS_EX_GRIDLINES 0x00000001
#define LVS_EX_FULLROWSELECT 0x00000020
#define LVS_EX_CHECKBOXES 0x00000004
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 55)
#define LVN_FIRST               (-100)
#define LVN_BEGINLABELEDIT      (LVN_FIRST-5)
#define LVN_ENDLABELEDIT        (LVN_FIRST-6)
#define LVN_COLUMNCLICK         (LVN_FIRST-8)
#define NM_CUSTOMDRAW           (NM_FIRST-12)
#define LVM_GETEDITCONTROL      (LVM_FIRST + 24)
#define LVM_EDITLABEL           (LVM_FIRST + 23)
#define LVM_GETITEMTEXT         (LVM_FIRST + 45)
#define LVM_GETSUBITEMRECT      (LVM_FIRST + 56)
#define LVM_GETCOLUMN           (LVM_FIRST + 25)
#define EM_SETSEL               0x00B1
#define LVIR_BOUNDS             0
#define LVIR_ICON               1
#define LVIR_LABEL              2
#define LVIR_SELECTBOUNDS       3

// Custom draw constants
#define CDDS_PREPAINT           0x00000001
#define CDDS_POSTPAINT          0x00000002
#define CDDS_PREERASE           0x00000003
#define CDDS_POSTERASE          0x00000004
#define CDDS_ITEM               0x00010000
#define CDDS_ITEMPREPAINT       (CDDS_ITEM | CDDS_PREPAINT)
#define CDDS_ITEMPOSTPAINT      (CDDS_ITEM | CDDS_POSTPAINT)
#define CDDS_SUBITEM            0x00020000
#define CDRF_DODEFAULT          0x00000000
#define CDRF_NEWFONT            0x00000002
#define CDRF_SKIPDEFAULT        0x00000004
#define CDRF_NOTIFYPOSTPAINT    0x00000010
#define CDRF_NOTIFYITEMDRAW     0x00000020
#define CDRF_NOTIFYSUBITEMDRAW  0x00000020

typedef struct tagLVCOLUMNA {
    UINT mask;
    int fmt;
    int cx;
    LPSTR pszText;
    int cchTextMax;
    int iSubItem;
    int iImage;
    int iOrder;
} LVCOLUMNA, *LPLVCOLUMNA;

typedef struct tagLVITEMA {
    UINT mask;
    int iItem;
    int iSubItem;
    UINT state;
    UINT stateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
    int iIndent;
} LVITEMA, *LPLVITEMA;

typedef struct tagNMLVDISPINFOA {
    NMHDR hdr;
    LVITEMA item;
} NMLVDISPINFOA, *LPNMLVDISPINFOA;

typedef struct tagNMITEMACTIVATE {
    NMHDR hdr;
    int iItem;
    int iSubItem;
    UINT uNewState;
    UINT uOldState;
    UINT uChanged;
    POINT ptAction;
    LPARAM lParam;
    UINT uKeyFlags;
} NMITEMACTIVATE, *LPNMITEMACTIVATE;

typedef struct tagNMCUSTOMDRAWINFO {
    NMHDR hdr;
    DWORD dwDrawStage;
    HDC hdc;
    RECT rc;
    DWORD_PTR dwItemSpec;
    UINT uItemState;
    LPARAM lItemlParam;
} NMCUSTOMDRAW, *LPNMCUSTOMDRAW;

typedef struct tagNMLVCUSTOMDRAW {
    NMCUSTOMDRAW nmcd;
    COLORREF clrText;
    COLORREF clrTextBk;
    int iSubItem;
} NMLVCUSTOMDRAW, *LPNMLVCUSTOMDRAW;

#define NM_FIRST        (0U-  0U)
#define NM_DBLCLK       (NM_FIRST-3)
#define EN_KILLFOCUS    0x0200

// Minimal declarations for common dialogs (normally from commdlg.h)
#define OFN_PATHMUSTEXIST    0x00000800
#define OFN_FILEMUSTEXIST    0x00001000
#define OFN_HIDEREADONLY     0x00000004
#define OFN_NOCHANGEDIR      0x00000008
#define OFN_EXPLORER         0x00080000

typedef struct tagOFNA {
    DWORD        lStructSize;
    HWND         hwndOwner;
    HINSTANCE    hInstance;
    LPCSTR       lpstrFilter;
    LPSTR        lpstrCustomFilter;
    DWORD        nMaxCustFilter;
    DWORD        nFilterIndex;
    LPSTR        lpstrFile;
    DWORD        nMaxFile;
    LPSTR        lpstrFileTitle;
    DWORD        nMaxFileTitle;
    LPCSTR       lpstrInitialDir;
    LPCSTR       lpstrTitle;
    DWORD        Flags;
    WORD         nFileOffset;
    WORD         nFileExtension;
    LPCSTR       lpstrDefExt;
    LPARAM       lCustData;
    LPVOID       lpfnHook;
    LPCSTR       lpTemplateName;
    void*        pvReserved;
    DWORD        dwReserved;
    DWORD        FlagsEx;
} OPENFILENAMEA, *LPOPENFILENAMEA;
