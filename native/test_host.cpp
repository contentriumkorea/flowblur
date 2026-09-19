#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <cassert>
#include <cstring>
#include "AEConfig.h"
#include "AE_Effect.h"
using Entry=PF_Err(*)(PF_Cmd,PF_InData*,PF_OutData*,PF_ParamDef*[],PF_LayerDef*,void*);
static int count=0;
static PF_Err addParam(PF_ProgPtr,PF_ParamIndex index,PF_ParamDefPtr parameter){
 assert(index==-1);++count;
 if(count==5){assert(parameter->param_type==PF_Param_BUTTON);assert(parameter->uu.id==5);assert(parameter->flags&PF_ParamFlag_SUPERVISE);}
 if(count==6){assert(parameter->param_type==PF_Param_BUTTON);assert(parameter->uu.id==6);assert(parameter->flags&PF_ParamFlag_SUPERVISE);}
 return PF_Err_NONE;
}
int main(){
 auto module=LoadLibraryW(L"native/build/FlowBlur.aex");assert(module);
 auto entry=reinterpret_cast<Entry>(GetProcAddress(module,"EffectMain"));assert(entry);
 PF_InData input{};PF_OutData result{};
 assert(entry(PF_Cmd_GLOBAL_SETUP,&input,&result,nullptr,nullptr,nullptr)==0);
 input.inter.add_param=addParam;
 assert(entry(PF_Cmd_PARAMS_SETUP,&input,&result,nullptr,nullptr,nullptr)==0);
 assert(count==6 && result.num_params==7);
 PF_ParamDef values[6]{};PF_ParamDef* parameters[6];for(int i=0;i<6;i++)parameters[i]=&values[i];
 unsigned char source[16]={255,10,20,30,255,40,50,60,255,70,80,90,255,100,110,120},dest[16]{};
 values[0].u.ld.width=2;values[0].u.ld.height=2;values[0].u.ld.rowbytes=8;values[0].u.ld.data=reinterpret_cast<PF_Pixel*>(source);
 PF_LayerDef output=values[0].u.ld;output.data=reinterpret_cast<PF_Pixel*>(dest);
 values[1].u.fs_d.value=180;values[2].u.fs_d.value=100;values[3].u.pd.value=2;values[4].u.fs_d.value=64;
 assert(entry(PF_Cmd_RENDER,&input,&result,parameters,&output,nullptr)==0);
 assert(std::memcmp(source,dest,sizeof(source))==0);
 std::cout<<"PASS: compiled AEX global setup, supervised activation button registration and locked render exact passthrough\n";
 FreeLibrary(module);
}
