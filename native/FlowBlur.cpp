#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <memory>
#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "SPBasic.h"
#include "PrSDKAESupport.h"
#include "flow_core.h"
#include "flow_activation.h"
#include "flow_update.h"
#include "build/version.h"
using flowblur::Image;
enum {SOURCE,SHUTTER,AMOUNT,QUALITY,MAX_MOTION,ACTIVATION,UPDATE,PARAM_COUNT};
static constexpr PF_OutFlags FLAGS=PF_OutFlag_WIDE_TIME_INPUT;

struct FormatSuite{
 PF_InData* in;const PF_PixelFormatSuite1* p=nullptr;
 explicit FormatSuite(PF_InData* i):in(i){if(i->appl_id==kAppID_Premiere)i->pica_basicP->AcquireSuite(kPFPixelFormatSuite,1,reinterpret_cast<const void**>(&p));}
 ~FormatSuite(){if(p)in->pica_basicP->ReleaseSuite(kPFPixelFormatSuite,1);}
 PrPixelFormat get(PF_LayerDef* w){PrPixelFormat f=PrPixelFormat_ARGB_4444_8u;if(p&&p->GetPixelFormat(w,&f))throw PF_Err_BAD_CALLBACK_PARAM;return f;}
};
static Image readImage(PF_LayerDef* w,PrPixelFormat f){
 Image im(w->width,w->height);
 bool fp=f==PrPixelFormat_BGRA_4444_32f;
 bool bgra=fp||f==PrPixelFormat_BGRA_4444_8u;
 if(!bgra&&f!=PrPixelFormat_ARGB_4444_8u)throw PF_Err_BAD_CALLBACK_PARAM;
 for(int y=0;y<w->height;y++){
  const auto* row=reinterpret_cast<const unsigned char*>(w->data)+ptrdiff_t(y)*w->rowbytes;
  for(int x=0;x<w->width;x++){
   if(fp){const float*p=reinterpret_cast<const float*>(row)+x*4;im.at(x,y)={p[0],p[1],p[2],p[3]};}
   else{const auto*p=row+x*4;if(bgra)im.at(x,y)={p[0]/255.f,p[1]/255.f,p[2]/255.f,p[3]/255.f};else im.at(x,y)={p[3]/255.f,p[2]/255.f,p[1]/255.f,p[0]/255.f};}
  }
 }return im;
}
static void writeImage(const Image&im,PF_LayerDef*w,PrPixelFormat f){
 bool fp=f==PrPixelFormat_BGRA_4444_32f,bgra=fp||f==PrPixelFormat_BGRA_4444_8u;
 for(int y=0;y<im.h;y++){
  auto* row=reinterpret_cast<unsigned char*>(w->data)+ptrdiff_t(y)*w->rowbytes;
  for(int x=0;x<im.w;x++){
   auto v=im.at(x,y);
   if(fp){float*p=reinterpret_cast<float*>(row)+x*4;for(int k=0;k<4;k++)p[k]=v[k];}
   else{auto*p=row+x*4;for(int k=0;k<4;k++)p[bgra?k:3-k]=static_cast<unsigned char>(std::clamp(std::lround(v[k]*255),0l,255l));}
  }
 }
}
struct Checkout{
 PF_InData* in;PF_ParamDef param{};PF_Err err;bool acquired;
 Checkout(PF_InData*i,A_long time):in(i),err(PF_CHECKOUT_PARAM(i,0,time,i->time_step,i->time_scale,&param)),acquired(err==PF_Err_NONE){}
 ~Checkout(){if(acquired)PF_CHECKIN_PARAM(in,&param);}
 bool valid(int w,int h)const{return acquired&&param.u.ld.data&&param.u.ld.width==w&&param.u.ld.height==h;}
};
static PF_Err setup(PF_InData*in_data,PF_OutData*out_data){
 PF_ParamDef def{};
 PF_ADD_FLOAT_SLIDERX("Shutter Angle",0,360,0,360,180,PF_Precision_INTEGER,0,0,1);
 PF_ADD_FLOAT_SLIDERX("Blur Amount (%)",0,200,0,200,100,PF_Precision_INTEGER,0,0,2);
 AEFX_CLR_STRUCT(def);PF_ADD_POPUP("Quality",3,2,"Draft|Standard|High",3);
 PF_ADD_FLOAT_SLIDERX("Max Motion (pixels)",1,256,1,128,64,PF_Precision_INTEGER,0,0,4);
 PF_ADD_BUTTON("Activation", "Activate / Status", 0, PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY, 5);
 PF_ADD_BUTTON("Update", "Check for Updates", 0, PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY, 6);
 out_data->num_params=PARAM_COUNT;return PF_Err_NONE;
}
extern "C" __declspec(dllexport) PF_Err EffectMain(PF_Cmd cmd,PF_InData*in_data,PF_OutData*out_data,PF_ParamDef*params[],PF_LayerDef*output,void*extra){
 try{
  if(cmd==PF_Cmd_ABOUT){std::strcpy(out_data->return_msg,"FlowBlur Motion Blur " FLOWBLUR_VERSION "\rActivation > Activate / Status\rUpdate > Check for Updates\rIndependent of RE:Vision RSMB.");}
  else if(cmd==PF_Cmd_GLOBAL_SETUP){
   out_data->my_version=PF_VERSION(FLOWBLUR_MAJOR,FLOWBLUR_MINOR,FLOWBLUR_PATCH,PF_Stage_DEVELOP,1);out_data->out_flags=FLAGS;out_data->out_flags2=0;
   FormatSuite fs(in_data);if(fs.p){fs.p->ClearSupportedPixelFormats(in_data->effect_ref);fs.p->AddSupportedPixelFormat(in_data->effect_ref,PrPixelFormat_BGRA_4444_32f);fs.p->AddSupportedPixelFormat(in_data->effect_ref,PrPixelFormat_BGRA_4444_8u);}
  }
  else if(cmd==PF_Cmd_PARAMS_SETUP)return setup(in_data,out_data);
  else if(cmd==PF_Cmd_USER_CHANGED_PARAM){
   auto* changed=static_cast<PF_UserChangedParamExtra*>(extra);
   if(changed && changed->param_index==UPDATE)flowblur_update::show();
   if(changed && changed->param_index==ACTIVATION && flowblur_activation::showDialog())
    out_data->out_flags |= PF_OutFlag_FORCE_RERENDER | PF_OutFlag_REFRESH_UI;
  }
  else if(cmd==PF_Cmd_RENDER){
   auto*w=&params[SOURCE]->u.ld;
   if(!w->data||!output->data||w->width!=output->width||w->height!=output->height)return PF_Err_BAD_CALLBACK_PARAM;
   FormatSuite fs(in_data);auto format=fs.get(w),of=fs.get(output);
   flowblur::Settings s;s.shutter=float(params[SHUTTER]->u.fs_d.value);s.amount=float(params[AMOUNT]->u.fs_d.value/100);s.quality=params[QUALITY]->u.pd.value;s.maxMotion=float(params[MAX_MOTION]->u.fs_d.value);
   if(!flowblur_activation::active()||s.shutter==0||s.amount==0){
    if(format==of){int bytes=format==PrPixelFormat_BGRA_4444_32f?16:4;for(int y=0;y<w->height;y++)std::memcpy(reinterpret_cast<char*>(output->data)+ptrdiff_t(y)*output->rowbytes,reinterpret_cast<char*>(w->data)+ptrdiff_t(y)*w->rowbytes,size_t(w->width)*bytes);}
    else writeImage(readImage(w,format),output,of);
    return PF_Err_NONE;
   }
   auto current=readImage(w,format);
   Checkout previous(in_data,in_data->current_time-in_data->time_step),next(in_data,in_data->current_time+in_data->time_step);
   std::unique_ptr<Image> pi,ni;
   if(previous.valid(w->width,w->height))pi=std::make_unique<Image>(readImage(&previous.param.u.ld,fs.get(&previous.param.u.ld)));
   if(next.valid(w->width,w->height))ni=std::make_unique<Image>(readImage(&next.param.u.ld,fs.get(&next.param.u.ld)));
   if(auto aborted=PF_ABORT(in_data))return aborted;
   auto result=flowblur::render(current,pi.get(),ni.get(),s);
   if(auto aborted=PF_ABORT(in_data))return aborted;
   writeImage(result,output,of);
  }
  return PF_Err_NONE;
 }catch(PF_Err e){return e;}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
extern "C" __declspec(dllexport) PF_Err PluginDataEntryFunction2(PF_PluginDataPtr inPtr,PF_PluginDataCB2 cb,SPBasicSuite*,const char*,const char*){
 PF_Err result=PF_Err_NONE;
 PF_REGISTER_EFFECT_EXT2(inPtr,cb,"FlowBlur Motion Blur","JCS FlowBlur Motion Blur","Blur & Sharpen",AE_RESERVED_INFO,"EffectMain","");
 return result;
}
// Host-independent entry used to test the exact compiled rendering engine.
extern "C" __declspec(dllexport) int FB_Render(const float*cur,const float*prev,const float*next,float*out,int w,int h,float shutter,float amount,int quality,float maxMotion){
 try{
  if(!cur||!out||w<1||h<1)return 1;
  Image c(w,h);std::memcpy(c.p.data(),cur,size_t(w)*h*16);
  std::unique_ptr<Image>p,n;
  if(prev){p=std::make_unique<Image>(w,h);std::memcpy(p->p.data(),prev,size_t(w)*h*16);}
  if(next){n=std::make_unique<Image>(w,h);std::memcpy(n->p.data(),next,size_t(w)*h*16);}
  flowblur::Settings s{shutter,amount,maxMotion,quality};auto r=flowblur::render(c,p.get(),n.get(),s);std::memcpy(out,r.p.data(),size_t(w)*h*16);return 0;
 }catch(...){return 1;}
}
