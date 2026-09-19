#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>
#include "activation_config.local.h"

namespace flowblur_activation {
inline std::vector<unsigned char> derive(const std::string& password,const std::string& salt,ULONG iterations){
 BCRYPT_ALG_HANDLE algorithm=nullptr;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0)return {};
 std::vector<unsigned char> result(32);
 auto status=BCryptDeriveKeyPBKDF2(algorithm,(PUCHAR)password.data(),ULONG(password.size()),(PUCHAR)salt.data(),ULONG(salt.size()),iterations,result.data(),32,0);
 BCryptCloseAlgorithmProvider(algorithm,0);
 if(status<0)return {};return result;
}
inline bool matches(const std::string& password,const std::string& salt,ULONG iterations,const std::vector<unsigned char>& expected){
 if(password.empty()||password.size()>1024||expected.size()!=32)return false;
 auto actual=derive(password,salt,iterations);if(actual.size()!=32)return false;
 unsigned char difference=0;for(size_t i=0;i<32;i++)difference|=actual[i]^expected[i];
 SecureZeroMemory(actual.data(),actual.size());return difference==0;
}
inline std::string token(){return std::string("FlowBlur:activation:v1:")+FLOWBLUR_ACTIVATION_HASH;}
inline std::vector<unsigned char> expected(){
 std::vector<unsigned char> value;const std::string hex=FLOWBLUR_ACTIVATION_HASH;
 if(hex.size()!=64)return value;
 for(size_t i=0;i<hex.size();i+=2)value.push_back(static_cast<unsigned char>(std::stoul(hex.substr(i,2),nullptr,16)));
 return value;
}
inline std::wstring licensePath(){
 wchar_t folder[MAX_PATH];if(FAILED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,folder)))return {};
 std::wstring path=std::wstring(folder)+L"\\Contentrium";CreateDirectoryW(path.c_str(),nullptr);
 path+=L"\\FlowBlur";CreateDirectoryW(path.c_str(),nullptr);return path+L"\\activation-v1.dat";
}
inline bool loadToken(const std::wstring& path,const std::string& expectedToken){
 HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)return false;
 DWORD size=GetFileSize(file,nullptr),read=0;
 if(size==INVALID_FILE_SIZE||size==0||size>65536){CloseHandle(file);return false;}
 std::vector<unsigned char> data(size);bool ok=ReadFile(file,data.data(),size,&read,nullptr)&&read==size;CloseHandle(file);
 if(!ok)return false;
 DATA_BLOB input{size,data.data()},output{};
 if(!CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))return false;
 ok=output.cbData==expectedToken.size()&&std::equal(output.pbData,output.pbData+output.cbData,expectedToken.begin());
 SecureZeroMemory(output.pbData,output.cbData);LocalFree(output.pbData);return ok;
}
inline bool saveToken(const std::wstring& path,const std::string& value){
 if(path.empty())return false;
 DATA_BLOB input{DWORD(value.size()),(BYTE*)value.data()},output{};
 if(!CryptProtectData(&input,L"FlowBlur activation",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))return false;
 const auto temporary=path+L"."+std::to_wstring(GetCurrentProcessId())+L".tmp";
 HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 bool ok=false;DWORD written=0;
 if(file!=INVALID_HANDLE_VALUE){ok=WriteFile(file,output.pbData,output.cbData,&written,nullptr)&&written==output.cbData;ok=FlushFileBuffers(file)&&ok;CloseHandle(file);}
 LocalFree(output.pbData);
 if(ok)ok=MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
 if(!ok)DeleteFileW(temporary.c_str());return ok;
}
inline std::atomic<bool> unlocked{false};
inline std::once_flag initialized;
inline bool active(){std::call_once(initialized,[]{unlocked=loadToken(licensePath(),token());});return unlocked.load();}
inline INT_PTR CALLBACK dialogProc(HWND dialog,UINT message,WPARAM wParam,LPARAM){
 if(message==WM_INITDIALOG){
  SetWindowTextW(dialog,L"FlowBlur Activation");
  auto add=[&](LPCWSTR klass,LPCWSTR title,DWORD style,int x,int y,int w,int h,int id){
   HWND control=CreateWindowExW(0,klass,title,WS_CHILD|WS_VISIBLE|style,x,y,w,h,dialog,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);
   SendMessageW(control,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return control;
  };
  add(L"STATIC",L"Enter your activation password.",0,18,15,325,24,100);
  HWND edit=add(L"EDIT",L"",WS_TABSTOP|WS_BORDER|ES_PASSWORD|ES_AUTOHSCROLL,18,44,325,28,101);
  SendMessageW(edit,EM_SETLIMITTEXT,256,0);
  add(L"STATIC",L"",0,18,82,325,40,102);
  add(L"BUTTON",L"Activate",WS_TABSTOP|BS_DEFPUSHBUTTON,143,130,96,29,IDOK);
  add(L"BUTTON",L"Cancel",WS_TABSTOP,247,130,96,29,IDCANCEL);
  SetFocus(edit);return FALSE;
 }
 if(message==WM_COMMAND&&LOWORD(wParam)==IDOK){
  wchar_t wide[257]{};GetDlgItemTextW(dialog,101,wide,257);
  char bytes[1029]{};WideCharToMultiByte(CP_UTF8,0,wide,-1,bytes,sizeof(bytes),nullptr,nullptr);
  std::string password(bytes);bool valid=matches(password,FLOWBLUR_ACTIVATION_SALT,210000,expected());
  SecureZeroMemory(wide,sizeof(wide));SecureZeroMemory(bytes,sizeof(bytes));SecureZeroMemory(password.data(),password.size());
  SetDlgItemTextW(dialog,101,L"");
  if(valid&&saveToken(licensePath(),token())){unlocked=true;EndDialog(dialog,IDOK);}
  else SetDlgItemTextW(dialog,102,valid?L"Could not save activation. Please try again.":L"Incorrect password. Please try again.");
  return TRUE;
 }
 if(message==WM_CLOSE||(message==WM_COMMAND&&LOWORD(wParam)==IDCANCEL)){EndDialog(dialog,IDCANCEL);return TRUE;}
 return FALSE;
}
inline bool showDialog(){
 if(active()){MessageBoxW(GetActiveWindow(),L"FlowBlur is activated for this Windows account.",L"FlowBlur Activation",MB_OK|MB_ICONINFORMATION);return true;}
 // A zero-control dialog; controls are created during WM_INITDIALOG.
 struct alignas(DWORD) EmptyDialog {DLGTEMPLATE header;WORD menu;WORD klass;WORD title;} layout{};
 layout.header.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER;
 layout.header.dwExtendedStyle=WS_EX_CONTROLPARENT;layout.header.cx=245;layout.header.cy=120;
 auto result=DialogBoxIndirectParamW(GetModuleHandleW(nullptr),&layout.header,GetActiveWindow(),dialogProc,0);
 if(result==-1)MessageBoxW(GetActiveWindow(),L"Could not open the activation dialog.",L"FlowBlur",MB_OK|MB_ICONERROR);
 return active();
}
}
