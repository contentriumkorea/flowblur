from pathlib import Path
import subprocess,sys,os
root=Path(__file__).parent.resolve()
sdk=Path(os.environ.get('AE_SDK_PATH', str(root/'sdk/AfterEffectsSDK_26.5_win/Examples'))).resolve()
if not (sdk/'Headers/AE_Effect.h').is_file():
    raise SystemExit('Set AE_SDK_PATH to the Adobe SDK Examples directory.')
build=root/'native/build';build.mkdir(exist_ok=True)
env=dict(os.environ,ZIG_GLOBAL_CACHE_DIR=str(root/'.zig-cache'))
zig=[sys.executable,'-m','ziglang']
def run(args,output=None,cwd=root):
    p=subprocess.run(args,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if output:Path(output).write_bytes(p.stdout)
    elif p.stdout:print(p.stdout.decode(errors='replace')[-3000:])
    if p.returncode:
        (build/'error.log').write_bytes(p.stderr)
        print(p.stderr.decode(errors='replace')[-7000:]);raise SystemExit(p.returncode)
    if p.stderr:(build/'warnings.log').write_bytes(p.stderr)
includes=sum([['-I',str(sdk/p)]for p in ['Headers','Headers/SP','Headers/Win','Util','Resources']],[])
pipl=(sdk/'Template/Skeleton/SkeletonPiPL.r').read_text().replace('ADBE Skeleton','JCS FlowBlur Motion Blur').replace('Skeleton','FlowBlur Motion Blur').replace('Sample Plug-ins','Blur & Sharpen').replace('557057','65537').replace('0x02000000','0x00000002').replace('https://www.adobe.com','')
(build/'FlowBlur.r').write_text(pipl)
run(zig+['cc','-E','-P','-x','c','-DMSWindows']+includes+[str(build/'FlowBlur.r')],build/'FlowBlur.rr')
run([str(sdk/'Resources/PiPLtool.exe'),'FlowBlur.rr','FlowBlur.rrc'],cwd=build)
run(zig+['cc','-E','-P','-x','c','-DMSWindows',str(build/'FlowBlur.rrc')],build/'FlowBlur.rc')
run(zig+['rc','/fo',str(build/'FlowBlur.res'),str(build/'FlowBlur.rc')])
run(zig+['c++','-std=c++17','-O2','-w','-shared','-DMSWindows','-DWIN32','-D_WINDOWS']+includes+['native/FlowBlur.cpp',str(build/'FlowBlur.res'),'-o',str(build/'FlowBlur.aex')])
print('Built '+str(build/'FlowBlur.aex'))
