#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>
namespace flowblur_update {
inline void show(){
 HMODULE module=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&show),&module))return;
 wchar_t filename[32768]{};DWORD length=GetModuleFileNameW(module,filename,32768);
 if(!length||length>=32768)return;
 std::wstring path(filename);path=path.substr(0,path.find_last_of(L"\\/"))+L"\\FlowBlurUpdater.exe";
 if(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES){MessageBoxW(GetActiveWindow(),L"The FlowBlur updater is missing. Please run the latest FlowBlur Setup installer.",L"FlowBlur Update",MB_OK|MB_ICONINFORMATION);return;}
 if(reinterpret_cast<INT_PTR>(ShellExecuteW(GetActiveWindow(),L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)MessageBoxW(GetActiveWindow(),L"Could not start the FlowBlur updater.",L"FlowBlur Update",MB_OK|MB_ICONERROR);
}
}
