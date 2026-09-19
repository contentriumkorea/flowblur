using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

static class Program {
 internal static string Target { get {
  string adobe=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),"Adobe");
  var dirs=Directory.GetDirectories(adobe,"Adobe Premiere Pro*");Array.Sort(dirs,StringComparer.OrdinalIgnoreCase);Array.Reverse(dirs);
  foreach(var dir in dirs)if(File.Exists(Path.Combine(dir,"Adobe Premiere Pro.exe")))return Path.Combine(dir,"PlugIns","Common","FlowBlur");
  throw new IOException("설치된 Premiere Pro를 찾을 수 없습니다.");
 } }
 internal static readonly string RecoveryRoot=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),"Contentrium","FlowBlur Recovery");
 internal static readonly string LegacyTarget=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),@"Adobe\Common\Plug-ins\7.0\MediaCore\FlowBlur");
 internal static bool HostRunning(){foreach(var p in Process.GetProcesses())using(p){try{string n=p.ProcessName;if(n.Equals("Adobe Premiere Pro",StringComparison.OrdinalIgnoreCase)||n.Equals("AfterFX",StringComparison.OrdinalIgnoreCase)||n.Equals("Adobe Media Encoder",StringComparison.OrdinalIgnoreCase))return true;}catch{}}return false;}
 internal static string Installed(){var p=Path.Combine(Target,"version.txt");if(File.Exists(p)){var v=File.ReadAllText(p).Trim();UpdateCore.Parse(v);return v;}return File.Exists(Path.Combine(Target,"FlowBlur.aex"))?"0.2.0":"0.0.0";}
 [STAThread] static int Main(string[] args){
  Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
  if(args.Length==2 && args[0]=="--install")return Install(args[1]);
  if(args.Length==0){
   try{var dir=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Contentrium","FlowBlur","UpdateSessions",Guid.NewGuid().ToString("N"));Directory.CreateDirectory(dir);var copy=Path.Combine(dir,"FlowBlurUpdater.exe");File.Copy(Application.ExecutablePath,copy);Process.Start(new ProcessStartInfo(copy,"--ui"){UseShellExecute=true});return 0;}
   catch(Exception e){MessageBox.Show(e.Message,"FlowBlur 업데이트");return 1;}
  }
  if(args.Length!=1||args[0]!="--ui")return 2;
  using(var mutex=new Mutex(false,@"Local\Contentrium.FlowBlur.UpdateWindow")){
   bool locked=false;try{locked=mutex.WaitOne(0);}catch(AbandonedMutexException){locked=true;}
   if(!locked){MessageBox.Show("FlowBlur 업데이트 창이 이미 열려 있습니다.","FlowBlur");return 0;}
   try{Application.Run(new UpdateWindow());}finally{mutex.ReleaseMutex();}
  }return 0;
 }
 static int Install(string download){
  string stage=null;
  using(var mutex=new Mutex(false,@"Global\Contentrium.FlowBlur.Install")){
   bool locked=false;
   try{
    try{locked=mutex.WaitOne(0);}catch(AbandonedMutexException){locked=true;}
    if(!locked)throw new IOException("다른 설치가 진행 중입니다.");
    if(HostRunning())throw new IOException("Premiere, After Effects, Media Encoder에서 작업을 저장하고 종료해 주세요.");
    var manifest=UpdateCore.VerifyManifest(File.ReadAllText(Path.Combine(download,"update.json")),ReleaseKey.PublicXml);
    if(UpdateCore.Compare(manifest.version,Installed())<=0)throw new IOException("같거나 이전 버전은 설치하지 않습니다.");
    UpdateCore.CheckPath(Target);Directory.CreateDirectory(Target);
    UpdateCore.CheckPath(RecoveryRoot);
    stage=Path.Combine(RecoveryRoot,"staging-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(stage);
    var protectedZip=Path.Combine(stage,"package.zip");
    using(var source=File.OpenRead(Path.Combine(download,"package.zip"))){if(source.Length!=manifest.size)throw new IOException("패키지 크기 오류");using(var dest=File.Create(protectedZip))source.CopyTo(dest);}
    UpdateCore.VerifyPackage(protectedZip,manifest);
    var payload=Path.Combine(stage,"payload");UpdateCore.Extract(protectedZip,payload);
    if(File.ReadAllText(Path.Combine(payload,"version.txt")).Trim()!=manifest.version)throw new IOException("패키지 버전 오류");
    string legacyBackup=null;
    if(Directory.Exists(LegacyTarget)){
     UpdateCore.CheckPath(LegacyTarget);
     legacyBackup=Path.Combine(RecoveryRoot,"legacy-"+Guid.NewGuid().ToString("N"));
     Directory.Move(LegacyTarget,legacyBackup);
    }
    try{UpdateCore.Install(payload,Target,HostRunning,null);}
    catch{if(legacyBackup!=null && !Directory.Exists(LegacyTarget))Directory.Move(legacyBackup,LegacyTarget);throw;}
    return 0;
   }catch(Exception e){MessageBox.Show(e.Message,"FlowBlur 설치 실패",MessageBoxButtons.OK,MessageBoxIcon.Error);return 1;}
   finally{if(stage!=null){try{Directory.Delete(stage,true);}catch{}}if(locked)mutex.ReleaseMutex();}
  }
 }
}

sealed class UpdateWindow:Form {
 readonly Label status=new Label(); readonly Label versions=new Label(); readonly TextBox notes=new TextBox();
 readonly ProgressBar progress=new ProgressBar(); readonly Button action=new Button(); readonly Button later=new Button();
 readonly CancellationTokenSource cancellation=new CancellationTokenSource();
 Manifest manifest;string work;bool downloaded,busy,installing,complete;
 public UpdateWindow(){
  Text="FlowBlur 업데이트";ClientSize=new Size(530,365);FormBorderStyle=FormBorderStyle.FixedDialog;MaximizeBox=false;StartPosition=FormStartPosition.CenterScreen;
  Font=new Font("Malgun Gothic",10);BackColor=Color.FromArgb(20,23,30);ForeColor=Color.White;
  var title=new Label{Text="FlowBlur Motion Blur",Font=new Font("Segoe UI",19,FontStyle.Bold),Bounds=new Rectangle(24,20,480,37)};
  versions.SetBounds(26,65,475,25);status.SetBounds(26,101,475,47);status.Text="새 버전을 확인하고 있습니다…";
  notes.SetBounds(26,154,475,115);notes.Multiline=true;notes.ReadOnly=true;notes.ScrollBars=ScrollBars.Vertical;notes.BackColor=Color.FromArgb(30,34,44);notes.ForeColor=Color.White;notes.BorderStyle=BorderStyle.FixedSingle;
  progress.SetBounds(26,280,475,10);action.SetBounds(267,308,140,34);later.SetBounds(415,308,86,34);action.Text="확인 중";action.Enabled=false;later.Text="나중에";
  action.BackColor=Color.FromArgb(120,150,250);action.ForeColor=Color.Black;later.ForeColor=Color.Black;
  Controls.AddRange(new Control[]{title,versions,status,notes,progress,action,later});
  later.Click+=(s,e)=>Close();action.Click+=async(s,e)=>{if(complete){LaunchPremiere();return;}if(downloaded)await Install();else if(manifest!=null)await Download();else await Check();};
  Shown+=async(s,e)=>await Check();FormClosing+=(s,e)=>{if(installing){e.Cancel=true;return;}cancellation.Cancel();};
 }
 void LaunchPremiere(){
  try{var adobe=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),"Adobe");var dirs=Directory.GetDirectories(adobe,"Adobe Premiere Pro*");Array.Sort(dirs,StringComparer.OrdinalIgnoreCase);Array.Reverse(dirs);foreach(var dir in dirs){var exe=Path.Combine(dir,"Adobe Premiere Pro.exe");if(File.Exists(exe)){Process.Start(new ProcessStartInfo(exe){UseShellExecute=true});Close();return;}}MessageBox.Show("시작 메뉴에서 Premiere를 실행해 주세요.","FlowBlur");}catch(Exception e){MessageBox.Show(e.Message,"FlowBlur");}
 }
 static HttpClient Client(){var client=new HttpClient();client.Timeout=TimeSpan.FromMinutes(3);client.DefaultRequestHeaders.UserAgent.ParseAdd("Contentrium-FlowBlur-Updater/0.3");return client;}
 async Task<byte[]> Fetch(string url,long limit,string file){
  ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;
  using(var client=Client())using(var response=await client.GetAsync(url,HttpCompletionOption.ResponseHeadersRead,cancellation.Token)){
   if(response.StatusCode==HttpStatusCode.NotFound)throw new IOException("아직 배포된 업데이트가 없습니다. 나중에 다시 확인해 주세요.");
   response.EnsureSuccessStatusCode();if(response.Content.Headers.ContentLength>limit)throw new IOException("다운로드 크기 제한을 초과했습니다.");
   using(var input=await response.Content.ReadAsStreamAsync())using(Stream output=file==null?(Stream)new MemoryStream():File.Create(file)){
    byte[] buffer=new byte[65536];long total=0;int n;
    using(var timeout=CancellationTokenSource.CreateLinkedTokenSource(cancellation.Token)){
     timeout.CancelAfter(TimeSpan.FromMinutes(3));
     while((n=await input.ReadAsync(buffer,0,buffer.Length,timeout.Token))>0){total+=n;if(total>limit)throw new IOException("다운로드 크기 제한을 초과했습니다.");await output.WriteAsync(buffer,0,n,timeout.Token);if(file!=null)progress.Value=(int)Math.Min(100,total*100/limit);}
    }
    return file==null?((MemoryStream)output).ToArray():null;
   }
  }
 }
 void Error(Exception e){if(IsDisposed||cancellation.IsCancellationRequested)return;status.Text=e is OperationCanceledException?"연결 시간이 초과되었습니다. 다시 시도해 주세요.":e.Message;action.Text=downloaded?"설치":"다시 확인";action.Enabled=true;if(!downloaded)manifest=null;}
 async Task Check(){
  if(busy)return;busy=true;action.Enabled=false;
  try{
   var current=Program.Installed();versions.Text="현재 버전: "+(current=="0.0.0"?"미설치":current);
   var raw=await Fetch("https://github.com/contentriumkorea/flowblur/releases/latest/download/FlowBlur-update.json",65536,null);
   manifest=UpdateCore.VerifyManifest(Encoding.UTF8.GetString(raw),ReleaseKey.PublicXml);
   versions.Text+="    최신 버전: "+manifest.version;notes.Text=manifest.notes;
   if(UpdateCore.Compare(manifest.version,current)<=0){status.Text="최신 버전을 사용하고 있습니다.";action.Text="최신 버전";manifest=null;}
   else{status.Text="새 버전이 있습니다. 다운로드 후 설치할 수 있습니다.";action.Text="다운로드";action.Enabled=true;}
  }catch(Exception e){Error(e);}finally{busy=false;}
 }
 async Task Download(){
  if(busy)return;busy=true;action.Enabled=false;status.Text="업데이트를 다운로드하고 있습니다…";
  try{
   work=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Contentrium","FlowBlur","Updates",Guid.NewGuid().ToString("N"));Directory.CreateDirectory(work);
   // Retain the signed envelope, then validate it again before the privileged install.
   var raw=await Fetch("https://github.com/contentriumkorea/flowblur/releases/latest/download/FlowBlur-update.json",65536,null);
   var currentManifest=UpdateCore.VerifyManifest(Encoding.UTF8.GetString(raw),ReleaseKey.PublicXml);
   if(currentManifest.version!=manifest.version||currentManifest.sha256!=manifest.sha256)throw new IOException("새 배포가 감지되었습니다. 다시 확인해 주세요.");
   File.WriteAllBytes(Path.Combine(work,"update.json"),raw);
   var zip=Path.Combine(work,"package.zip");await Fetch(manifest.url,manifest.size,zip);await Task.Run(()=>UpdateCore.VerifyPackage(zip,manifest));
   downloaded=true;progress.Value=100;status.Text="다운로드 완료. Adobe 앱에서 작업을 저장하고 종료한 뒤 설치해 주세요.";action.Text="설치";action.Enabled=true;
  }catch(Exception e){Error(e);}finally{busy=false;}
 }
 async Task Install(){
  if(busy)return;
  if(Program.HostRunning()){status.Text="Premiere, After Effects, Media Encoder를 종료한 뒤 다시 눌러 주세요.";return;}
  busy=true;installing=true;action.Enabled=false;later.Enabled=false;status.Text="업데이트를 설치하고 있습니다…";
  try{
   // Copy the currently trusted updater so its installed copy can be replaced.
   string helper=Path.Combine(work,"FlowBlurInstall.exe");File.Copy(Application.ExecutablePath,helper,true);
   using(var process=Process.Start(new ProcessStartInfo(helper,"--install \""+work+"\""){UseShellExecute=true,Verb="runas"})){
    await Task.Run(()=>process.WaitForExit());if(process.ExitCode!=0)throw new IOException("설치를 완료하지 못했습니다. 기존 버전을 유지합니다.");
   }
   complete=true;status.Text="업데이트가 완료되었습니다. 기존 인증과 효과 설정이 유지됩니다.";versions.Text="설치 버전: "+manifest.version;action.Text="Premiere 실행";action.Enabled=true;later.Text="닫기";
  }catch(Exception e){Error(e);}finally{busy=false;installing=false;later.Enabled=true;}
 }
}
