using System;
using System.IO;
using System.Security.Cryptography;
class Sign {
 static void Main(string[] args){
  using(var rsa=new RSACryptoServiceProvider()){
   rsa.PersistKeyInCsp=false;rsa.FromXmlString(File.ReadAllText(args[0]));
   if(rsa.ToXmlString(false)!=ReleaseKey.PublicXml)throw new Exception("Signing key does not match the updater public key.");
   File.WriteAllText(args[2],Convert.ToBase64String(rsa.SignData(File.ReadAllBytes(args[1]),"SHA256")));
  }
 }
}
