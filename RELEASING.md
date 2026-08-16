# Releasing

The library and the CLI ship independently, each with its own version and
release tag:

| Component | `MAJOR.MINOR` set in            | Tag prefix |
| --------- | ------------------------------- | ---------- |
| Library   | `project(VERSION ...)` in `CMakeLists.txt` | `lib-v`    |
| CLI       | `SQLITE_MANAGER_CLI_MAJOR` / `..._MINOR` in `cli/CMakeLists.txt` | `cli-v` |

`PATCH` is never written by hand: it is the number of commits touching the
component's sources since its most recent tag (see `cmake/GitVersion.cmake`).
Tagging a release resets it to 0; the next commit to that component makes it 1.

## Cutting a release

1. **Pick the new version.**
   - Patch release (bug fixes only): nothing to edit — `PATCH` already tracks
     the commits since the last tag. Read the current value with a configure:
     `cmake --preset linux-debug` prints `SQLITE_MANAGER_*_VERSION`.
   - Minor or major release: bump `MAJOR.MINOR` for the component in CMake (see
     the table above) and commit that change.

2. **Update the changelog.** In `CHANGELOG.md`, rename `## [Unreleased]` to
   `## [X.Y.Z] - YYYY-MM-DD` for the component being released, add a fresh empty
   `## [Unreleased]` above it, and update the link references at the bottom.

3. **Commit** the changelog (and any version bump) to `master` via a PR, and
   let it go green.

4. **Tag the release commit** and push the tag:

   ```bash
   git tag lib-v0.2.0        # or cli-v0.2.0
   git push origin lib-v0.2.0
   ```

   The tag name should match the version being released. Both components can be
   released together (two tags on the same commit) or independently.

5. **Let CI publish the release.** Pushing the tag triggers the `Release`
   workflow (`.github/workflows/release.yml`), which creates a GitHub Release
   with auto-generated notes. For a `cli-v*` tag it also builds the release
   `.deb` and attaches it; a `lib-v*` tag gets a notes-only release. No manual
   upload is needed.

## Building release artifacts

CI attaches the `.deb` to `cli-v*` releases automatically (step 5 above). To
build it by hand — for a local check or an off-CI release:

The CLI `.deb` is produced from a release build:

```bash
cmake --preset linux-release
cmake --build build/release
cd build/release && cpack -G DEB   # -> sqlite-manager_<version>_amd64.deb
```

The embedded version comes from the tag via `GitVersion.cmake`, so build the
package from the tagged commit.
