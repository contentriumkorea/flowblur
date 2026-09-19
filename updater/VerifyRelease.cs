using System;
using System.IO;
class VerifyRelease {
 static void Main(string[] args){
  var m=UpdateCore.VerifyManifest(File.ReadAllText(args[0]),ReleaseKey.PublicXml);
  UpdateCore.VerifyPackage(args[1],m);
  var stage=Path.Combine(Path.GetTempPath(),"FlowBlur-release-check-"+Guid.NewGuid().ToString("N"));
  try{UpdateCore.Extract(args[1],stage);if(File.ReadAllText(Path.Combine(stage,"version.txt")).Trim()!=m.version)throw new Exception("Version mismatch");foreach(var name in new[]{"FlowBlur.aex","FlowBlurUpdater.exe"}){var b=File.ReadAllBytes(Path.Combine(stage,name));if(b.Length<2||b[0]!=77||b[1]!=90)throw new Exception("Not a PE binary");}}
  finally{if(Directory.Exists(stage))Directory.Delete(stage,true);}
  Console.WriteLine("PASS: signed release "+m.version+", package SHA-256, exact file set, binary headers and version");
 }
}
