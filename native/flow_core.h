#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <stdexcept>
namespace flowblur {
using Pixel=std::array<float,4>;
struct Image {
 int w,h; std::vector<Pixel> p;
 Image(int width,int height):w(width),h(height),p(size_t(w)*h){}
 Pixel& at(int x,int y){return p[size_t(y)*w+x];}
 const Pixel& at(int x,int y)const{return p[size_t(y)*w+x];}
 Pixel sample(float x,float y)const{
  x=std::clamp(x,0.f,float(w-1));y=std::clamp(y,0.f,float(h-1));
  int ix=int(x),iy=int(y),jx=std::min(ix+1,w-1),jy=std::min(iy+1,h-1);
  float tx=x-ix,ty=y-iy;Pixel r{};
  for(int k=0;k<4;k++)r[k]=(at(ix,iy)[k]*(1-tx)+at(jx,iy)[k]*tx)*(1-ty)+(at(ix,jy)[k]*(1-tx)+at(jx,jy)[k]*tx)*ty;
  return r;
 }
};
struct Settings{float shutter=180,amount=1,maxMotion=64;int quality=2;};
struct Gray{
 int w,h;std::vector<float> p;
 Gray(int a,int b):w(a),h(b),p(size_t(a)*b){}
 float at(int x,int y)const{return p[size_t(std::clamp(y,0,h-1))*w+std::clamp(x,0,w-1)];}
};
inline Gray gray(const Image& im,int scale){
 Gray g(std::max(1,im.w/scale),std::max(1,im.h/scale));
 for(int y=0;y<g.h;y++)for(int x=0;x<g.w;x++){
  float v=0;int n=0;
  for(int dy=0;dy<scale;dy++)for(int dx=0;dx<scale;dx++){
   const auto&p=im.at(std::min(im.w-1,x*scale+dx),std::min(im.h-1,y*scale+dy));
   v+=std::clamp(.0722f*p[0]+.7152f*p[1]+.2126f*p[2],0.f,1.f);n++;
  }g.p[size_t(y)*g.w+x]=v/n;
 }return g;
}
inline Gray half(const Gray& a){
 Gray b(std::max(1,a.w/2),std::max(1,a.h/2));
 for(int y=0;y<b.h;y++)for(int x=0;x<b.w;x++)b.p[size_t(y)*b.w+x]=(a.at(x*2,y*2)+a.at(x*2+1,y*2)+a.at(x*2,y*2+1)+a.at(x*2+1,y*2+1))*.25f;
 return b;
}
inline bool cut(const Gray& a,const Gray& b){
 float ha[32]={},hb[32]={};
 for(float v:a.p)ha[std::min(31,int(v*32))]++;
 for(float v:b.p)hb[std::min(31,int(v*32))]++;
 float d=0;for(int i=0;i<32;i++)d+=std::sqrt(ha[i]*hb[i]);
 return d/std::sqrt(float(a.p.size())*b.p.size())<.70f;
}
struct Vec{float x=0,y=0;};
struct Field{
 int w,h,step,scale;std::vector<Vec> v;
 Field(int width,int height,int st,int sc):w((width+st-1)/st+1),h((height+st-1)/st+1),step(st),scale(sc),v(size_t(w)*h){}
 Vec sample(float x,float y)const{
  x=std::clamp(x/(step*scale),0.f,float(w-1));y=std::clamp(y/(step*scale),0.f,float(h-1));
  int ix=int(x),iy=int(y);float tx=x-ix,ty=y-iy;Vec r;
  for(int j=0;j<2;j++)for(int i=0;i<2;i++){
   float z=(i?tx:1-tx)*(j?ty:1-ty);auto q=v[size_t(std::min(iy+j,h-1))*w+std::min(ix+i,w-1)];r.x+=q.x*z;r.y+=q.y*z;
  }return r;
 }
};
inline float cost(const Gray&a,const Gray&b,int x,int y,int dx,int dy){
 float e=0;
 for(int j=-3;j<=3;j++)for(int i=-3;i<=3;i++)e+=std::abs(a.at(x+i,y+j)-b.at(x+i+dx,y+j+dy));
 return e/49.f;
}
inline Field motion(const Image&cur,const Image*neighbor,const Settings&s){
 int scale=std::max(1,(std::max(cur.w,cur.h)+639)/640),step=s.quality==1?8:4;
 auto a=gray(cur,scale);Field f(a.w,a.h,step,scale);
 if(!neighbor)return f;
 if(neighbor->w!=cur.w||neighbor->h!=cur.h) return f;
 if(cur.p==neighbor->p)return f;
 auto b=gray(*neighbor,scale);
 if(cut(a,b))return f;
 std::vector<Gray> pa{a},pb{b};
 while(pa.size()<4 && pa.back().w>=48 && pa.back().h>=48){pa.push_back(half(pa.back()));pb.push_back(half(pb.back()));}
 for(int gy=0;gy<f.h;gy++)for(int gx=0;gx<f.w;gx++){
  int x=std::min(a.w-1,gx*step),y=std::min(a.h-1,gy*step);
  float zero=cost(a,b,x,y,0,0);
  if(zero<.001f)continue;
  int dx=0,dy=0;
  for(int l=int(pa.size())-1;l>=0;l--){
   if(l!=int(pa.size())-1){dx*=2;dy*=2;}
   int factor=1<<l,px=x/factor,py=y/factor;
   int rad=l==int(pa.size())-1?int(std::ceil(s.maxMotion/(scale*factor))):3;
   rad=std::min(rad,20);int bx=dx,by=dy;float best=1e9f;
   for(int sy=dy-rad;sy<=dy+rad;sy++)for(int sx=dx-rad;sx<=dx+rad;sx++){
    if(std::hypot(float(sx),float(sy))*factor*scale>s.maxMotion)continue;
    float e=cost(pa[l],pb[l],px,py,sx,sy)+.00005f*std::hypot(float(sx),float(sy));
    if(e<best){best=e;bx=sx;by=sy;}
   }dx=bx;dy=by;
  }
  float err=cost(a,b,x,y,dx,dy);
  float confidence=std::clamp((zero-err)/std::max(zero,.001f)*2.f,0.f,1.f)*std::clamp(1.f-err/.20f,0.f,1.f);
  f.v[size_t(gy)*f.w+gx]={dx*scale*confidence,dy*scale*confidence};
 }return f;
}
inline Image render(const Image&cur,const Image*prev,const Image*next,const Settings&s){
 if(s.shutter<0||s.shutter>360||s.amount<0||s.amount>2||s.maxMotion<1||s.maxMotion>256)throw std::invalid_argument("Invalid settings");
 if(s.amount==0||s.shutter==0||(!prev&&!next))return cur;
 if((!prev||prev->p==cur.p)&&(!next||next->p==cur.p))return cur;
 auto fp=motion(cur,prev?prev:next,s),fn=motion(cur,next?next:prev,s);
 Image out(cur.w,cur.h);int n=s.quality==1?9:s.quality==2?17:33;
 for(int y=0;y<cur.h;y++)for(int x=0;x<cur.w;x++){
  Vec vp=fp.sample(float(x),float(y)),vn=fn.sample(float(x),float(y));
  if(!prev){vp.x=-vp.x;vp.y=-vp.y;}if(!next){vn.x=-vn.x;vn.y=-vn.y;}
  if(std::hypot(vp.x,vp.y)+std::hypot(vn.x,vn.y)<.05f){out.at(x,y)=cur.at(x,y);continue;}
  Pixel sum{};
  for(int i=0;i<n;i++){
   float t=(float(i)/(n-1)-.5f)*s.shutter/360.f*s.amount;
   Vec v=t<0?vp:vn;float z=std::abs(t);auto p=cur.sample(x+z*v.x,y+z*v.y);
   for(int k=0;k<4;k++)sum[k]+=p[k]/n;
  }out.at(x,y)=sum;
 }return out;
}
}
