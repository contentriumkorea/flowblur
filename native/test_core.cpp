#include "flow_core.h"
#include <iostream>
#include <random>
#include <stdexcept>
using namespace flowblur;
void require(bool v,const char* msg){if(!v)throw std::runtime_error(msg);}
int main(){
 Image a(96,64),prev(96,64),next(96,64);
 std::mt19937 rng(7);
 for(auto& p:a.p){p={float(rng()%256)/255,float(rng()%256)/255,float(rng()%256)/255,1};}
 Settings s; s.shutter=0;
 require(render(a,&a,&a,s).p==a.p,"zero shutter must bypass exactly");
 s.shutter=270;
 require(render(a,&a,&a,s).p==a.p,"static frame must be exact");
 require(render(a,nullptr,nullptr,s).p==a.p,"no neighbors must bypass");
 for(int y=0;y<64;y++)for(int x=0;x<96;x++){
  prev.at(x,y)=a.at((x+6)%96,y);next.at(x,y)=a.at((x+90)%96,y);
 }
 Image out=render(a,&prev,&next,s);
 float before=0,after=0;
 for(int y=12;y<52;y++)for(int x=20;x<75;x++){
  before+=std::abs(a.at(x,y)[0]-a.at(x+1,y)[0]);
  after+=std::abs(out.at(x,y)[0]-out.at(x+1,y)[0]);
 }
 require(after<before*.8f,"translation must blur along motion");
 Image black(96,64),white(96,64);for(auto& p:white.p)p={1,1,1,1};
 require(render(white,&black,&white,s).p==white.p,"cut must not smear");
 for(const auto& p:out.p)for(float f:p)require(std::isfinite(f),"finite pixels");
 std::cout<<"6 native core tests passed; texture energy ratio="<<after/before<<"\n";
}
