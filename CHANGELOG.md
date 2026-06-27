# Changelog

This file is the source of truth for tagged release notes.

## [0.1.2] - 2026-06-27

### Changed
- Fixed Quest package metadata so generated `mod.json` matches the next release version and current public repository metadata
- Updated the packaged mod description to reflect the in-game scan/delete UI instead of the old dry-run helper wording
- Removed the tool-generated `porter` field from the public qmod manifest template

## [0.1.1] - 2026-06-27

### Added
- Added GitHub Actions automation for `Quest mod CI`, `Nightly release`, and `Versioned release`
- Added CI / nightly / latest-release badges and quick links near the top of the README
- Added a bilingual README layout with English as the default and a linked Korean version

### Changed
- Improved tagged release notes to include install steps, target Beat Saber version, and changelog-driven summaries
- Updated nightly releases to publish as prereleases with compare links in the release body

## [0.1.0] - 2026-06-27

### Added
- Initial Meta Quest Beat Saber implementation of the `Keep Favorites` mod
- Favorites-based scan, non-favorite deletion flow, and direct main-menu entry UI
- Initial Quest build/package flow using SongCore refresh and batch-delete behavior
