# mod

Beat Saber 내부 scan/delete UI 모드 트랙이에요.

목표:
- favorites 읽기
- keep/delete 스캔 결과 계산
- 확인 후 non-favorite 곡 삭제

현재 범위:
- 런타임 favorites source 사용
- SongCore runtime level 목록 사용
- 스캔 결과를 UI에 표시하고 확인 후 삭제 실행

현재 구현 상태:
- 1.40.8용 템플릿/QPM 설정 추가
- Main menu 진입에서 스캔을 트리거하고 결과를 UI 상태라인에 표시
- non-favorite 곡 폴더 삭제 executor와 SongCore 기반 재스캔 흐름 연결

빌드 메모:
- fresh checkout에서는 먼저 `qpm restore`
- NDK 경로에 공백이 있으면 안 돼서, 필요하면 공백 없는 symlink 경로를 `ndkpath.txt`에 적어야 함
