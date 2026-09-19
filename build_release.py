from pathlib import Path
import base64,hashlib,json,os,re,shutil,subprocess,sys,zipfile
root=Path(__file__).resolve().parent
os.chdir(root)
version=(root/'VERSION').read_text().strip()
if not re.fullmatch(r'(0|[1-9]\d{0,3})\.(0|[1-9]\d{0,3})\.(0|[1-9]\d{0,3})',version):raise SystemExit('Invalid VERSION')
key=root/'.private/update-signing-key.xml'
if not key.exists():raise SystemExit('Missing private release key. Read docs/UPDATES.md before creating a distributor key.')
csc=Path(os.environ.get('WINDIR','C:/Windows'))/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
def run(args):
    p=subprocess.run([str(x) for x in args],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if p.returncode:
        print(p.stdout.decode(errors='replace'));raise SystemExit(p.returncode)
run([sys.executable,root/'build_native.py'])
build=root/'updater/build';build.mkdir(parents=True,exist_ok=True)
refs=['/r:'+s+'.dll' for s in ['System.Windows.Forms','System.Drawing','System.Net.Http','System.Web.Extensions','System.IO.Compression','System.IO.Compression.FileSystem']]
(build/'Version.cs').write_text('[assembly:System.Reflection.AssemblyVersion("'+version+'.0")]\n[assembly:System.Reflection.AssemblyFileVersion("'+version+'.0")]\n')
run([csc,'/nologo','/target:winexe','/platform:x64','/out:'+str(build/'FlowBlurUpdater.exe')]+refs+[root/'updater'/s for s in ['Program.cs','UpdateCore.cs','ReleaseKey.cs']]+[build/'Version.cs'])
run([csc,'/nologo','/out:'+str(build/'Sign.exe'),root/'updater/Sign.cs',root/'updater/ReleaseKey.cs'])
release=root/'release'/version;release.mkdir(parents=True,exist_ok=True)
package=release/f'FlowBlur-{version}-Windows.zip'
with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED) as z:
    z.write(root/'native/build/FlowBlur.aex','FlowBlur.aex')
    z.write(build/'FlowBlurUpdater.exe','FlowBlurUpdater.exe')
    z.writestr('version.txt',version+'\n')
notes=(root/'docs/release-notes.txt').read_text(encoding='utf-8')
manifest=dict(version=version,notes=notes,size=package.stat().st_size,sha256=hashlib.sha256(package.read_bytes()).hexdigest(),url=f'https://github.com/contentriumkorea/flowblur/releases/download/v{version}/{package.name}')
payload=json.dumps(manifest,ensure_ascii=False,separators=(',',':')).encode()
(build/'payload.json').write_bytes(payload)
run([build/'Sign.exe',key,build/'payload.json',build/'signature.txt'])
envelope=dict(payload=base64.b64encode(payload).decode(),signature=(build/'signature.txt').read_text(encoding='utf-8-sig'))
(release/'FlowBlur-update.json').write_text(json.dumps(envelope),encoding='utf-8')
shutil.copy2(build/'FlowBlurUpdater.exe',release/'FlowBlurSetup.exe')
print('Built signed release '+version+' in '+str(release))
