using System;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.IO.Compression;
using System.Web.Script.Serialization;
class Tests {
 static int count;
 static void Check(bool condition,string message){if(!condition)throw new Exception(message);count++;}
 static void Reject(Action action){bool failed=false;try{action();}catch{failed=true;}Check(failed,"Expected rejection");}
 static void Main(){
  Check(UpdateCore.Compare("0.10.0","0.9.0")>0,"Numeric version ordering");
  Reject(()=>UpdateCore.Compare("1.2.3-beta","1.2.3"));
  using(var key=new RSACryptoServiceProvider(2048)){
   key.PersistKeyInCsp=false;
   var m=new Manifest{version="0.3.1",notes="Test",sha256=new string('a',64),size=123,url="https://github.com/contentriumkorea/flowblur/releases/download/v0.3.1/FlowBlur-0.3.1-Windows.zip"};
   var bytes=Encoding.UTF8.GetBytes(new JavaScriptSerializer().Serialize(m));
   var envelope=new Envelope{payload=Convert.ToBase64String(bytes),signature=Convert.ToBase64String(key.SignData(bytes,"SHA256"))};
   string encoded=new JavaScriptSerializer().Serialize(envelope);
   Check(UpdateCore.VerifyManifest(encoded,key.ToXmlString(false)).version=="0.3.1","Signed manifest");
   envelope.payload=Convert.ToBase64String(Encoding.UTF8.GetBytes("tampered"));
   Reject(()=>UpdateCore.VerifyManifest(new JavaScriptSerializer().Serialize(envelope),key.ToXmlString(false)));
   m.url="https://example.com/evil.zip";Reject(()=>UpdateCore.Validate(m));
  }
  var root=Path.Combine(Path.GetTempPath(),"FlowBlur-tests-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
  try{
   var package=Path.Combine(root,"package.zip");
   using(var z=ZipFile.Open(package,ZipArchiveMode.Create))foreach(var name in UpdateCore.Files){var e=z.CreateEntry(name);using(var w=new StreamWriter(e.Open()))w.Write("new-"+name);}
   var manifest=new Manifest{size=new FileInfo(package).Length,sha256=UpdateCore.Hash(package)};
   UpdateCore.VerifyPackage(package,manifest);count++;
   manifest.sha256=new string('0',64);Reject(()=>UpdateCore.VerifyPackage(package,manifest));
   var stage=Path.Combine(root,"stage");UpdateCore.Extract(package,stage);count++;
   var dest=Path.Combine(root,"install");Directory.CreateDirectory(dest);
   foreach(var name in UpdateCore.Files)File.WriteAllText(Path.Combine(dest,name),"old-"+name);
   File.WriteAllText(Path.Combine(dest,"activation-v1.dat"),"untouched");
   Reject(()=>UpdateCore.Install(stage,dest,()=>true,null));
   Check(File.ReadAllText(Path.Combine(dest,"FlowBlur.aex"))=="old-FlowBlur.aex","Busy host preservation");
   Reject(()=>UpdateCore.Install(stage,dest,()=>false,i=>{if(i==1)throw new IOException("injected failure");}));
   foreach(var name in UpdateCore.Files)Check(File.ReadAllText(Path.Combine(dest,name))=="old-"+name,"Rollback "+name);
   UpdateCore.Install(stage,dest,()=>false,null);
   Check(File.ReadAllText(Path.Combine(dest,"FlowBlur.aex"))=="new-FlowBlur.aex","Install");
   Check(File.ReadAllText(Path.Combine(dest,"activation-v1.dat"))=="untouched","Activation preserved");
   using(var z=ZipFile.Open(Path.Combine(root,"bad.zip"),ZipArchiveMode.Create)){z.CreateEntry("../escape");}
   Reject(()=>UpdateCore.Extract(Path.Combine(root,"bad.zip"),Path.Combine(root,"bad-stage")));
  }finally{Directory.Delete(root,true);}
  Console.WriteLine("PASS: "+count+" updater assertions (signatures, versions, hash, traversal, busy host, rollback, activation preservation)");
 }
}
