# macOS 이식 범위

현재 macOS용 빌드/호스트 검증은 미완료입니다. Windows .aex의 확장자 변경으로는 사용할 수 없습니다.

재사용 가능한 부분은 native/flow_core.h의 표준 C++17 계산 엔진입니다. Windows 의존성은 native/FlowBlur.cpp의 windows.h와 __declspec(dllexport), build_native.py의 Windows 리소스 처리와 PiPLtool.exe, 설치 스크립트에 있습니다.

필요 작업:
1. Mac과 Xcode, macOS용 공식 Adobe SDK 준비.
2. 플랫폼별 include/export 선언 분리.
3. SDK Mac 샘플을 기준으로 .plugin 번들, PiPL, Info.plist 및 빌드 타깃 구성.
4. Apple Silicon 빌드부터 Premiere 실제 로딩·파라미터·프레임 체크아웃·픽셀 포맷·렌더링 확인.
5. Intel 지원이 필요하면 별도 아키텍처 빌드와 검증.
6. 외부 배포에는 Developer ID 서명 및 공증 작업 검토.

엔진 재작성보다는 패키징과 호스트 검증이 중심입니다. Mac 테스트 환경과 SDK가 준비된 경우 초기 로딩까지 수 시간~1일, 사용 가능한 테스트 빌드까지 1~3일을 계획용 추정으로 잡을 수 있습니다. 실제 측정이 아니며 호환성 문제·서명 환경·Intel 지원에 따라 더 걸릴 수 있습니다.

참고:
- https://developer.adobe.com/after-effects/
- https://developer.apple.com/developer-id/
