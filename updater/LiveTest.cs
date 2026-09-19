using System;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Reflection;
class LiveTest {
 static void Main(string[] args){
  ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;
  var assembly=Assembly.LoadFrom(Path.GetFullPath(args[0]));
  var clientFactory=assembly.GetType("UpdateWindow").GetMethod("Client",BindingFlags.Static|BindingFlags.NonPublic);
  var key=(string)assembly.GetType("ReleaseKey").GetField("PublicXml",BindingFlags.Static|BindingFlags.NonPublic).GetRawConstantValue();
  var root=Path.Combine(Path.GetTempPath(),"FlowBlur-live-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
  try{using(var client=(HttpClient)clientFactory.Invoke(null,null)){
   var json=client.GetStringAsync("https://github.com/contentriumkorea/flowblur/releases/latest/download/FlowBlur-update.json").GetAwaiter().GetResult();
   var m=UpdateCore.VerifyManifest(json,key);
   var bytes=client.GetByteArrayAsync(m.url).GetAwaiter().GetResult();var zip=Path.Combine(root,"package.zip");File.WriteAllBytes(zip,bytes);UpdateCore.VerifyPackage(zip,m);
   Console.WriteLine("PASS: released updater HTTP client connected to live feed and verified "+m.version+" download");
  }}finally{Directory.Delete(root,true);}
 }
}
