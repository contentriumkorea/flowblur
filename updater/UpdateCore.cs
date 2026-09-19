using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

public class Manifest { public string version; public string notes; public string url; public string sha256; public long size; }
public class Envelope { public string payload; public string signature; }
public static class UpdateCore {
 public static readonly string[] Files={"FlowBlur.aex","FlowBlurUpdater.exe","version.txt"};
 public const long MaxPackage=64*1024*1024;
 public static Version Parse(string value){if(value==null||!Regex.IsMatch(value,@"^(0|[1-9]\d{0,3})\.(0|[1-9]\d{0,3})\.(0|[1-9]\d{0,3})$"))throw new InvalidDataException("올바르지 않은 버전입니다.");return new Version(value);}
 public static int Compare(string a,string b){return Parse(a).CompareTo(Parse(b));}
 public static void Validate(Manifest m){
  if(m==null)throw new InvalidDataException("업데이트 정보가 없습니다.");Parse(m.version);
  if(m.url!="https://github.com/contentriumkorea/flowblur/releases/download/v"+m.version+"/FlowBlur-"+m.version+"-Windows.zip"||m.sha256==null||!Regex.IsMatch(m.sha256,"^[a-f0-9]{64}$")||m.size<1||m.size>MaxPackage||m.notes==null||m.notes.Length>8000)throw new InvalidDataException("업데이트 정보가 올바르지 않습니다.");
 }
 public static Manifest VerifyManifest(string json,string publicKey){
  if(json==null||json.Length>65536)throw new InvalidDataException("업데이트 정보 크기 오류");
  var serializer=new JavaScriptSerializer();var e=serializer.Deserialize<Envelope>(json);
  var payload=Convert.FromBase64String(e.payload);var signature=Convert.FromBase64String(e.signature);
  using(var rsa=new RSACryptoServiceProvider()){rsa.PersistKeyInCsp=false;rsa.FromXmlString(publicKey);if(!rsa.VerifyData(payload,"SHA256",signature))throw new InvalidDataException("업데이트 서명 검증에 실패했습니다.");}
  var m=serializer.Deserialize<Manifest>(Encoding.UTF8.GetString(payload));Validate(m);return m;
 }
 public static string Hash(string file){using(var sha=SHA256.Create())using(var stream=File.OpenRead(file))return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-","").ToLowerInvariant();}
 public static void VerifyPackage(string file,Manifest m){if(new FileInfo(file).Length!=m.size||Hash(file)!=m.sha256)throw new InvalidDataException("다운로드 파일 검증에 실패했습니다. 다시 다운로드해 주세요.");}
 public static void Extract(string zip,string stage){
  Directory.CreateDirectory(stage);
  using(var archive=ZipFile.OpenRead(zip)){
   if(archive.Entries.Count!=Files.Length||archive.Entries.Select(e=>e.FullName).Distinct(StringComparer.OrdinalIgnoreCase).Count()!=Files.Length||archive.Entries.Any(e=>!Files.Contains(e.FullName)||e.Length<1||e.Length>MaxPackage)||archive.Entries.Sum(e=>e.Length)>MaxPackage)throw new InvalidDataException("설치 패키지 구성이 올바르지 않습니다.");
   foreach(var entry in archive.Entries)entry.ExtractToFile(Path.Combine(stage,entry.FullName),false);
  }
 }
 public static void CheckPath(string path){
  for(var dir=new DirectoryInfo(Path.GetFullPath(path));dir!=null;dir=dir.Parent)if(dir.Exists&&(dir.Attributes&FileAttributes.ReparsePoint)!=0)throw new IOException("연결된 폴더에는 설치할 수 없습니다.");
 }
 public static void Install(string stage,string target,Func<bool> hostRunning,Action<int> afterCopy){
  if(hostRunning())throw new IOException("Premiere, After Effects, Media Encoder에서 작업을 저장하고 종료해 주세요.");
  CheckPath(target);Directory.CreateDirectory(target);
  foreach(var file in Files){if(!File.Exists(Path.Combine(stage,file)))throw new IOException("설치 파일 누락");var dest=Path.Combine(target,file);if(File.Exists(dest)&&(File.GetAttributes(dest)&FileAttributes.ReparsePoint)!=0)throw new IOException("연결된 파일에는 설치할 수 없습니다.");}
  var backup=Path.Combine(target,"rollback-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(backup);
  var changed=new System.Collections.Generic.List<string>();
  try{
   foreach(var file in Files)if(File.Exists(Path.Combine(target,file)))File.Copy(Path.Combine(target,file),Path.Combine(backup,file));
   for(int i=0;i<Files.Length;i++){
    if(hostRunning())throw new IOException("Adobe 프로그램이 실행되었습니다. 종료 후 다시 시도해 주세요.");
    var file=Files[i];var dest=Path.Combine(target,file);var temp=Path.Combine(target,"new-"+Guid.NewGuid().ToString("N"));
    try{File.Copy(Path.Combine(stage,file),temp);if(File.Exists(dest))File.Replace(temp,dest,null);else File.Move(temp,dest);changed.Add(file);}finally{if(File.Exists(temp))File.Delete(temp);}
    if(afterCopy!=null)afterCopy(i);
   }
  }catch(Exception original){
   try{foreach(var file in changed){var old=Path.Combine(backup,file);var dest=Path.Combine(target,file);if(File.Exists(old))File.Copy(old,dest,true);else File.Delete(dest);}}
   catch(Exception rollback){throw new IOException("설치와 복원에 실패했습니다. 백업: "+backup+" / "+rollback.Message,original);}
   throw;
  }
  // Keep the most recent transaction backup for manual recovery; never touch activation data.
 }
}
