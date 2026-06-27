# mod

Beat Saber 내부 helper mod 트랙이에요.

목표:
- favorites 읽기
- keep/delete dry-run 계산
- 확인 후 non-favorite 곡 삭제

초기 범위:
- 런타임 favorites source 사용
- SongCore runtime level 목록 사용
- 기본은 dry-run

현재 구현 상태:
- 1.40.8용 템플릿/QPM 설정 추가
- Main menu 활성화 시 준비가 될 때까지 dry-run summary를 로그로 재시도 출력하는 helper mod 스켈레톤 추가
- 실제 삭제 executor와 UI는 아직 미연결

빌드 메모:
- fresh checkout에서는 먼저 `qpm restore`
- NDK 경로에 공백이 있으면 안 돼서, 필요하면 공백 없는 symlink 경로를 `ndkpath.txt`에 적어야 함
