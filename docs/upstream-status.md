# Upstream compatibility status

Catch3 retains Catch2 compatibility while maintaining its own extension surface.
Upstream fixes should be reviewed rather than blindly replacing fork-owned code.

The upstream `catchorg/Catch2` `devel` branch was reviewed through
`222e233903d287cda4713fb08e1a959e8d71fd6d` on 2026-09-29. Two changes since the
previous reviewed revision were backported without changing upstream naming,
licenses or compiler options:

- `99ea5e97aa4d60998cfd2d9bde886de69c708ee4`: include `<cassert>` directly in
  the range-generator header instead of relying on a transitive include.
- `222e233903d287cda4713fb08e1a959e8d71fd6d`: isolate Clang's pointer-to-bool
  diagnostic at the upstream reusable-string-stream compatibility boundary.

The branch was reviewed again through
`08092139210881f01a34e60c5f2fd6afa0ea4024`, the commit of Catch2 3.16.1, on
2026-10-11. The eleven changes since the previous reviewed revision were all
backported, in upstream order, and the compatibility engine is now 3.16.1:

- `d7683484`: a better message when a reporter cannot be initialized.
- `ea5020f0`: every first-party reporter validates its custom arguments.
- `aa01559e`: the throwing-reporter test also throws in `assertionEnded`.
- `45cd3987`: `handleUnexpectedInflightException` is `noexcept`.
- `6adbd539`: MSVC optimizations are enabled again after the benchmark
  headers (upstream issue 3212).
- `e87f2882`: test registration with non-type template parameters no longer
  scales quadratically.
- `73d546cd`: `REQUIRE` expands its arguments fewer times under GCC and
  Clang.
- `b710039a`: the decomposer operators are plain functions and no longer
  hidden friends.
- `f899467b`: streaming of reconstructed expressions is type-erased.
- `8f0e70ca`: the retired `macos-14` runner is dropped from the macOS builds.
- `08092139`: version 3.16.1, with its release notes.

Each applied without a conflict in the sources. The amalgamated distribution
under `extras/` did conflict, because it is generated; it was generated again
from the sources of this repository with `generateAmalgamatedFiles.py`, which
also brought it up to date with fork changes it had not yet picked up. No
fork-owned file under `src/Progmasoft` needed a change.

These are compatibility fixes, not a claim that every future upstream change
is automatically incorporated. The build and test workflows remain the
verification gate for the fork: Windows, macOS 15 and 26, Ubuntu 26.04 and
Fedora 43.
