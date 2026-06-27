![Keep Favorites cover](assets/readme-cover.png)

# beatsaber-keep-favorites

[English README](README.md)

[![Quest mod CI](https://github.com/MTGVim/beatsaber-keep-favorites/actions/workflows/ci.yml/badge.svg)](https://github.com/MTGVim/beatsaber-keep-favorites/actions/workflows/ci.yml)
[![Nightly release](https://github.com/MTGVim/beatsaber-keep-favorites/actions/workflows/nightly-release.yml/badge.svg)](https://github.com/MTGVim/beatsaber-keep-favorites/actions/workflows/nightly-release.yml)
[![Latest release](https://img.shields.io/github/v/release/MTGVim/beatsaber-keep-favorites?display_name=tag&label=latest%20release)](https://github.com/MTGVim/beatsaber-keep-favorites/releases/latest)

- [Latest release 받기](https://github.com/MTGVim/beatsaber-keep-favorites/releases/latest)
- [Nightly prerelease 받기](https://github.com/MTGVim/beatsaber-keep-favorites/releases/tag/nightly)
- [CI 실행 기록 보기](https://github.com/MTGVim/beatsaber-keep-favorites/actions)

Quest Beat Saber에서 **즐겨찾기한 곡만 남기고 나머지 커스텀 곡을 정리**하기 위한 리포지토리예요.

현재 방향은 **모드 우선**, **CLI는 보존**이에요.
- 1차 구현: Beat Saber 내부 helper mod
- 보존 트랙: 같은 비즈니스 로직을 쓰는 CLI / dry-run 도구

## 현재 확인된 사실

### Favorites 저장 위치
- Quest의 Beat Saber favorites는 `PlayerData.dat`에서 확인됨
- 실제 pull 경로:
  - `/sdcard/Android/data/com.beatgames.beatsaber/files/PlayerData.dat`
- JSON 키:
  - `localPlayers[0].favoritesLevelIds`

### 커스텀 곡 경로 후보
- `/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CustomLevels`
- `/sdcard/ModData/com.beatgames.beatsaber/Mods/SongLoader/CustomLevels`
- WIP:
  - `/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CustomWIPLevels`
  - `/sdcard/ModData/com.beatgames.beatsaber/Mods/SongLoader/CustomWIPLevels`

### SongCore 캐시
- 캐시 파일:
  - `/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CachedSongData.json`
- 캐시에 `path -> sha1` 정보가 있어 곡 폴더와 favorites hash를 매핑하는 데 활용 가능

## 현재 수집된 샘플
- `tmp/PlayerData.dat`
- `tmp/CachedSongData.json`

## 목표 로직
1. `PlayerData.dat`에서 `favoritesLevelIds` 읽기
2. `CachedSongData.json` 또는 fallback hash 계산으로 `song path -> sha1` 확보
3. `custom_level_<SHA1>` 기준으로 keep/delete 판정
4. dry-run으로 삭제 예정 목록 출력
5. `--apply`에서만 실제 삭제

## 안전 원칙
- 기본 동작은 `dry-run`
- 실제 삭제 전 `PlayerData.dat` 백업
- Beat Saber 완전히 종료한 상태에서 실행
- 삭제 단위는 개별 파일이 아니라 **곡 폴더 전체**

## 트랙
- `mod/`: Beat Saber 내부 helper mod 구현 트랙
- `cli/`: 외부 dry-run / 분석 / 백업용 CLI 트랙
- `docs/plans/`: 구현 계획 문서
