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

These are compatibility fixes, not a claim that every future upstream change
is automatically incorporated. The existing Windows/macOS build and test
workflows remain the verification gate for the fork.
