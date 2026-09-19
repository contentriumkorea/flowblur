# FlowBlur Motion Blur (experimental)

Premiere Pro용 CPU 모션 블러 네이티브 효과. 효과 목록의 **Video Effects → Blur & Sharpen → FlowBlur Motion Blur**에서 사용합니다.

## 현재 상태

Windows Premiere Pro 2026에서 효과 검색, 클립 적용, 설정 표시와 on/off 전환을 확인했습니다. C++ 코어 테스트 6개를 통과했습니다. 전반적인 화질·안정성 검증이나 RSMB와의 동등성 검증은 완료하지 않았습니다. macOS 빌드는 아직 없습니다.

## Windows 빌드

Python 3와 공식 Adobe After Effects SDK 26.5 Windows 버전이 필요합니다. SDK는 포함하지 않습니다. [Adobe 개발자 페이지](https://developer.adobe.com/after-effects/)에서 직접 받으세요.

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-build.txt
$env:AE_SDK_PATH = 'C:\path\to\AfterEffectsSDK_26.5_win\Examples'
.\.venv\Scripts\python.exe build_native.py
```

기본 SDK 위치는 sdk/AfterEffectsSDK_26.5_win/Examples입니다. 산출물은 native/build/FlowBlur.aex입니다. 첫 빌드에는 Zig의 컴파일러 의존성 확보를 위한 네트워크 접근이 필요할 수 있습니다.

## 설치

Premiere를 종료하고 install-native.cmd를 관리자 권한으로 실행합니다. 설치 위치는 C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\FlowBlur 입니다. Premiere를 다시 열어 효과 이름을 검색하세요. 제거하려면 Premiere 종료 후 이 FlowBlur 폴더만 제거합니다.

## 설정

| 설정 | 기본값 | 의미 |
|---|---:|---|
| Shutter Angle | 180° | 블러 샘플링 구간 |
| Blur Amount | 100% | 블러 배율. 0이면 bypass |
| Quality | Standard | 움직임 분석·샘플 수 |
| Max Motion | 64px | 추정 이동 거리 제한 |

## 코어 테스트

C++17 컴파일러와 CMake가 있으면 Adobe SDK 없이 테스트 가능합니다.

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

이는 순수 C++ 엔진 검사이며 Premiere 호스트 통합 검사를 대체하지 않습니다.

## 제한 사항

- CPU 다중 해상도 패치 매칭 방식. GPU 실시간 처리가 아닙니다.
- 빠른 움직임, 가려짐, 회전, 반복 무늬, 장면 전환에서 오추정 가능.
- Premiere BGRA 8-bit/32-bit float 및 ARGB 8-bit 버퍼 처리. HDR·알파·복잡한 효과 조합은 미검증.
- 이전/다음 프레임 체크아웃과 현재 프레임에 선행 효과가 다르게 반영될 수 있습니다. 효과 순서에 따른 결과 확인이 필요합니다.
- macOS 지원 계획은 [MACOS.md](MACOS.md)를 참고하세요.

## 저장소 범위

자체 C++ 코드와 빌드/설치 스크립트만 포함합니다. Adobe SDK, 개인 영상, Premiere 프로젝트, 계정 정보, 빌드 결과물은 포함하지 않습니다. RSMB 또는 RE:Vision Effects와 무관한 독립 실험입니다.

## 라이선스

아직 오픈소스 라이선스를 선택하지 않았습니다. 공개 저장소라는 이유만으로 재배포·상업적 이용 허가를 부여하지 않습니다. 공유 정책을 정한 뒤 별도 LICENSE를 추가하세요. Adobe SDK 사용에는 Adobe의 별도 조건이 적용됩니다.
