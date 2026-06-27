# Beat Saber Keep Favorites Mod-First Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Beat Saber Quest 1.40.8에서 즐겨찾기한 곡만 남기고 나머지 커스텀 곡을 정리할 수 있는 helper mod를 먼저 만들고, 같은 비즈니스 로직을 재사용하는 CLI 트랙은 보존한다.

**Architecture:** 비즈니스 로직은 `core` 계층으로 분리한다. `mod` 트랙은 Beat Saber 내부에서 `favoritesLevelIds`, SongCore 캐시, 삭제 API를 직접 사용한다. `cli` 트랙은 일단 삭제하지 않고 동일한 입력/출력 계약을 따르는 보조 도구로 남겨둔다.

**Tech Stack:** Quest Beat Saber modding (C++ / Scotland2 / SongCore), Markdown docs, optional Python CLI later.

---

## Bound facts already verified

- Favorites source on Quest:
  - `/sdcard/Android/data/com.beatgames.beatsaber/files/PlayerData.dat`
  - JSON path: `localPlayers[0].favoritesLevelIds`
- Song cache source:
  - `/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CachedSongData.json`
- Custom level roots seen in SongCore source:
  - `/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CustomLevels`
  - `/sdcard/ModData/com.beatgames.beatsaber/Mods/SongLoader/CustomLevels`
- ADB runtime dependency is undesired for final UX.
- CLI track must remain in the repo and must not be deleted.

## Public success criteria

1. Mod can compute a dry-run plan: keep count, delete count, unresolved count.
2. Mod uses Beat Saber runtime favorites, not a hardcoded export file.
3. Mod can delete non-favorited custom songs through SongCore-compatible paths.
4. CLI track remains present and documented as a secondary track.
5. No destructive action runs by default without explicit confirmation.

---

### Task 1: Reserve the two-track repo layout

**Objective:** Make the repo direction explicit: mod first, CLI preserved.

**Files:**
- Modify: `README.md`
- Create: `mod/README.md`
- Create: `cli/README.md`
- Create: `docs/plans/2026-06-27-mod-first-dual-track.md`

**Step 1: Update README direction**

Add:
- mod-first decision
- cli preserved decision
- track descriptions

**Step 2: Create track marker docs**

`mod/README.md`
```md
# mod

Beat Saber 내부 helper mod 트랙.

목표:
- favorites 읽기
- keep/delete dry-run 계산
- 확인 후 삭제 실행
```

`cli/README.md`
```md
# cli

외부 dry-run / 분석 / 백업용 CLI 트랙.

현재 상태:
- 보존 only
- mod와 동일한 core 계약을 나중에 재사용
```

**Step 3: Verify repo surface**

Run:
```bash
git -C /home/tigeryoo/workspace/beatsaber-keep-favorites status --short
```

Expected:
- `README.md`
- `mod/README.md`
- `cli/README.md`
- `docs/plans/...md`

---

### Task 2: Freeze the domain model before touching mod UI

**Objective:** UI보다 먼저 keep/delete 판정 계약을 고정한다.

**Files:**
- Create: `docs/domain-model.md`

**Step 1: Define the record types**

Document these shapes:

```text
FavoriteLevelId = "custom_level_<SHA1>"

CachedSongRecord {
  path: string
  sha1: string | null
  directoryHash: int | null
  songDuration: float | null
}

SongDecision {
  path: string
  sha1: string | null
  levelId: string | null
  decision: keep | delete | unresolved
  reason: string
}

SweepPlan {
  favoriteCount: int
  scannedCount: int
  keepCount: int
  deleteCount: int
  unresolvedCount: int
  items: SongDecision[]
}
```

**Step 2: Freeze the decision rules**

Document rules:
1. If `sha1` exists and `custom_level_<sha1>` is in favorites -> `keep`
2. If `sha1` exists and not in favorites -> `delete`
3. If `sha1` missing -> `unresolved`
4. Default action for unresolved is **do not delete**

**Step 3: Verify completeness**

Check the document contains:
- keep
- delete
- unresolved
- no-delete default for unresolved

---

### Task 3: Define the mod-side data providers

**Objective:** Identify the exact runtime adapters the mod needs.

**Files:**
- Create: `docs/mod-data-sources.md`

**Step 1: Record favorites provider**

Document that mod reads runtime favorites from Beat Saber `PlayerData` / favorites set.

**Step 2: Record song inventory provider**

Document primary order:
1. SongCore cache
2. SongCore runtime level list
3. fallback path scan only if needed

**Step 3: Record delete executor**

Document delete path:
- preferred: SongCore delete API / runtime delete integration
- fallback: direct folder delete only if mod runtime path is insufficient

**Step 4: Verify**

Run manual review and confirm the doc explicitly separates:
- source of truth for favorites
- source of truth for song list
- delete executor

---

### Task 4: Plan a minimal helper mod API surface

**Objective:** Keep the first mod small and headless-friendly before rich UI.

**Files:**
- Create: `docs/mod-api.md`

**Step 1: Define the internal commands**

Document a minimal command set:

```text
scan() -> SweepPlan
apply(plan or current scan token) -> ApplyResult
backup() -> BackupResult
```

**Step 2: Add explicit confirmation contract**

Rules:
- `scan()` never deletes
- `apply()` requires explicit confirmation token or explicit UI confirmation
- unresolved songs are skipped by default

**Step 3: Define result shapes**

```text
ApplyResult {
  deletedCount: int
  skippedFavoriteCount: int
  skippedUnresolvedCount: int
  failedCount: int
  failures: [{path, reason}]
}
```

---

### Task 5: Plan the first user surface

**Objective:** Start with the smallest safe in-game UX.

**Files:**
- Create: `docs/mod-ui-v0.md`

**Step 1: Define the V0 UI**

V0 should only need:
- `Scan songs` button
- counts summary
- scrollable delete preview sample
- `Delete non-favorites` button
- confirmation modal

**Step 2: Explicitly exclude non-V0 work**

Not in V0:
- per-song manual selection
- playlist rules
- restore UI
- export/import favorites
- remote GUI bridge

**Step 3: Verify**

Check the document contains a `Not in V0` section.

---

### Task 6: Preserve the CLI track without implementing it now

**Objective:** Keep the CLI alive as a future adapter, not the first delivery.

**Files:**
- Create: `cli/contract.md`

**Step 1: Document shared contract**

CLI must eventually consume/produce the same shapes as `SweepPlan` and `ApplyResult`.

**Step 2: Document current non-goal**

State clearly:
- CLI is not the first shipping surface
- no deletion code written yet
- retained for later validation / batch / backup workflows

**Step 3: Verify**

Ensure the file does **not** promise a working CLI today.

---

### Task 7: Prepare implementation scaffolding for mod-first development

**Objective:** Create a code layout that allows later shared logic extraction.

**Files:**
- Create: `mod/src/`
- Create: `mod/include/`
- Create: `mod/docs/`
- Create: `mod/notes.md`

**Step 1: Add notes stub**

`mod/notes.md`
```md
# mod implementation notes

Planned components:
- favorites provider
- song inventory provider
- decision engine
- delete executor
- UI adapter
```

**Step 2: Verify layout**

Run:
```bash
find /home/tigeryoo/workspace/beatsaber-keep-favorites -maxdepth 2 -type d | sort
```

Expected to include:
- `mod`
- `mod/src`
- `mod/include`
- `cli`
- `docs/plans`

---

### Task 8: Stop and review before coding the mod

**Objective:** Make sure the architecture is acceptable before writing game-coupled code.

**Files:**
- Review only

**Step 1: Review these questions**

- Is unresolved-by-default acceptable?
- Is SongCore cache trusted as primary input?
- Should WIP folders be excluded in V0?
- Should backup happen automatically before apply?

**Step 2: Decide the first implementation target**

Preferred first code target:
- headless scan engine in the mod
- no UI deletion until scan output is trustworthy

---

## Suggested first implementation order after planning

1. favorites provider
2. SongCore cache reader
3. decision engine
4. dry-run logging / debug surface
5. confirmation-gated delete executor
6. minimal UI

## Verification commands for the planning stage

```bash
git -C /home/tigeryoo/workspace/beatsaber-keep-favorites status --short
find /home/tigeryoo/workspace/beatsaber-keep-favorites -maxdepth 2 -type d | sort
```

Expected:
- planning docs present
- both `mod/` and `cli/` present
- no CLI deletion implementation promised yet
