#include "flow_activation.h"
#include <cassert>
#include <iostream>
int main(){
  std::cout<<"Checking production activation state"<<std::endl;
  flowblur_activation::active();
  std::cout<<"Production activation state loaded"<<std::endl;
  const auto salt=std::string("test-salt");
  const auto hash=flowblur_activation::derive("test-password",salt,210000);
  assert(hash.size()==32);
  assert(flowblur_activation::matches("test-password",salt,210000,hash));
  assert(!flowblur_activation::matches("wrong",salt,210000,hash));
  assert(!flowblur_activation::matches("",salt,210000,hash));
  wchar_t tmp[MAX_PATH];assert(GetTempPathW(MAX_PATH,tmp));
  auto file=std::wstring(tmp)+L"flowblur-activation-test-"+std::to_wstring(GetCurrentProcessId())+L".dat";
  assert(!flowblur_activation::loadToken(file,"test-product-token"));
  assert(flowblur_activation::saveToken(file,"test-product-token"));
  assert(flowblur_activation::loadToken(file,"test-product-token"));
  assert(!flowblur_activation::loadToken(file,"other-product-token"));
  assert(!flowblur_activation::saveToken(file+L"/missing/file","token"));
  DeleteFileW(file.c_str());
  assert(!flowblur_activation::loadToken(file,"test-product-token"));
  std::cout<<"Activation: password, persistence, product binding and write failure passed\n";
}
