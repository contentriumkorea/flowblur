# FlowBlur 업데이트

## 사용자

처음에는 GitHub Releases의 `FlowBlurSetup.exe`를 실행합니다. 새 버전 다운로드 후 Premiere, After Effects, Media Encoder에서 작업을 저장하고 종료한 뒤 `설치`를 누릅니다. Windows 관리자 권한 요청은 Program Files의 플러그인 파일을 교체할 때만 표시됩니다.

설치된 FlowBlur의 효과 컨트롤에서 **Update → Check for Updates**를 누르면 같은 업데이트 창이 열립니다. **나중에**를 누르면 설치하지 않습니다. 주기적 자동 팝업이나 강제 종료는 없습니다.

활성화 파일은 업데이트 대상이 아니므로 같은 Windows 계정의 기존 인증은 유지됩니다. 효과 match name과 기존 파라미터 ID도 유지합니다.

## 배포자: 다음 버전 배포

1. `VERSION`과 `docs/release-notes.txt`를 수정합니다. 현재 Adobe 버전 인코딩에서는 major 0–7, minor/patch 0–15를 지원합니다.
2. 기존 `native/activation_config.local.h`와 `.private/update-signing-key.xml`을 유지합니다. 비공개 설정과 서명키는 공개 저장소에 커밋하지 마세요. 서명키는 별도 안전한 위치에 백업해야 합니다. 키를 잃으면 기존 업데이터가 새 서명을 신뢰하지 못합니다.
3. Windows에서 빌드용 Python으로 `build_release.py`를 실행합니다. Adobe SDK, Zig, .NET Framework C# 컴파일러가 필요합니다. `updater/ReleaseKey.cs`의 공개키와 개인키가 일치하지 않으면 빌드가 실패합니다.
4. `release/<버전>/`의 ZIP, `FlowBlur-update.json`, `FlowBlurSetup.exe`를 GitHub `contentriumkorea/flowblur`의 태그 `v<버전>`에 첨부합니다. 자산은 모두 올린 뒤 정식 최신 릴리스로 게시합니다. 제목은 `FlowBlur <버전>`으로 지정합니다.
5. `https://github.com/contentriumkorea/flowblur/releases/latest/download/FlowBlur-update.json`과 서명·ZIP 해시를 확인합니다. 기존 버전의 업데이트 창에서 다운로드와 설치를 점검합니다.

서명된 메타데이터는 버전·고정 저장소의 다운로드 URL·크기·SHA-256·변경 사항을 포함합니다. RSA-SHA256 서명과 ZIP 해시를 검증한 후 고정된 FlowBlur 폴더에 설치합니다. Windows Authenticode 코드 서명 인증서를 사용한 배포는 아니므로 Windows가 별도의 게시자 확인 메시지를 표시할 수 있습니다.

## 복원과 검증

설치 시 기존 파일은 설치 폴더의 `rollback-<고유값>`에 보관됩니다. 설치 중 오류가 나면 변경된 파일을 복원합니다. 실행 중인 Adobe 앱, 사용 중인 파일, 권한 거절, 변조 파일은 설치 완료로 처리하지 않습니다.

`updater/Tests.cs`는 버전 비교, 서명, 파일 해시, 압축 경로, 실행 중 호스트, 중간 실패 복원과 인증 파일 보존을 검증합니다. `updater/VerifyRelease.cs`는 실제 릴리스 메타데이터 및 ZIP을 검증합니다. `native/test_host.cpp`는 빌드된 AEX의 초기화·버튼 등록·미인증 렌더 동작을 검증합니다.

## 0.3.1 Premiere-only installation

The installer uses the newest installed Premiere Pro folder: `PlugIns/Common/FlowBlur`. Legacy shared MediaCore installations move to `Program Files/Contentrium/FlowBlur Recovery` after Adobe hosts close. Rollback files use `.bak` extensions so Adobe cannot discover duplicate effects. After Effects is not a supported host. Separate Adobe Media Encoder queue rendering is not verified for this private Premiere installation.
