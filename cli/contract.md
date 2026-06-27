# CLI contract

이 트랙은 나중에 외부 분석 / dry-run / 백업 도구로 갈 수 있게 남겨둔 계약 문서예요.

## 공유 계약

CLI는 나중에 mod와 같은 비즈니스 로직 결과를 다뤄야 해요.

### SweepPlan
- favoriteCount
- scannedCount
- keepCount
- deleteCount
- unresolvedCount
- items[]

### ApplyResult
- deletedCount
- skippedFavoriteCount
- skippedUnresolvedCount
- failedCount
- failures[]

## 현재 비목표
- working CLI 제공 안 함
- 삭제 실행 코드 아직 없음
- adb 기반 운영 UX를 최종 목표로 삼지 않음
