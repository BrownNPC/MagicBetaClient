# Copilot instructions for MagicBetaClient

This file gives concise, machine-friendly guidance for Copilot-style assistants working in this repository.

---

## 1) Build, test, and lint commands

Native (recommended):
- Install system deps: CMake (>=3.17), a C compiler, SDL3 dev headers, libcurl dev, OpenGL dev libs.
- Ensure the `so` translator is available on PATH (see notes).
- From repo root:
  - Full native build: `go run build.go -bootstrap=native`
  - Alternative manual flow: `so translate -o build/transpiled/ src` then `cmake -B build` then `cmake --build build --parallel`

PSP (PlayStation) cross-build:
- `go run build.go -bootstrap=psp` (requires PSP toolchain and `psp-cmake`).

Notes about `so` (translator):
- The build invokes the `so` translator (binary named `so`) to transpile `src` into C files under `build/transpiled/` before CMake runs. If `so` is not available, either install it or make it reachable in PATH. Example install (if the tool is distributed as a Go module): `go install solod.dev/so@latest` (adjust version as needed) or follow vendor instructions.

Go-specific:
- The Go source lives under `src/` and has its own go.mod (module `mbc`). You can also build or inspect packages with standard Go commands inside `src/`.

Tests and linters:
- There are currently no Go `_test.go` files in the repository. To run a single test (if added):
  - `cd src && go test ./... -run TestName`
- No project-wide linter is enforced; use your preferred Go linter when needed (e.g. `go vet`, `golangci-lint`).

Beta wiki (docs/site):
- `cd beta-wiki` (uses Bun/VitePress)
  - Dev server: `bun dev`
  - Build static site: `bun run build`
  - Preview built site: `bun preview`

---

## 2) High-level architecture (big picture)

- This repo contains a native game/client codebase implemented in a Go-like source tree under `src/` and a documentation/site project in `beta-wiki/` (VitePress + Bun).
- Build pipeline (native): source (`src/`) -> `so translate` -> transpiled C (`build/transpiled/*.c`) -> CMake builds executable linking SDL3, libcurl, OpenGL.
- Runtime expectations: the produced binary depends on SDL3, libcurl and OpenGL. Assets (images, icons, etc.) live in `assets/` and are copied into the build output by CMake.
- Cross-targets: CMake has PSP-specific branches guarded by `PSP` to support building for PSP with a `psp-cmake` toolchain.
- Key runtime packages: `mbc/gfx`, `mbc/sdl`, `mbc/net` (networking and curl wrapper) — these are implemented in `src/` and consumed by `src/main.go`.

---

## 3) Key conventions and repo-specific patterns

- Translator-first workflow: source under `src/` is not compiled directly by C; it is meant to be run through the `so` translator. The translator emits `build/transpiled/` which is the real input to CMake.
- CMake copies `assets/` into the target directory at POST_BUILD — expect runtime asset paths relative to the binary.
- Embedding C headers in Go-like source: files use `//so:embed` comments to include C headers (e.g. `//so:embed sdl/app.h`). Copilot should preserve these annotations when editing.
- Module naming: the Go module in `src/go.mod` is `mbc` — import paths in the code use `mbc/...`.
- Memory and allocation: code uses `solod.dev/so/mem` helpers (e.g., `mem.Alloc[...]` and `mem.Free`) — avoid replacing these with arbitrary allocators unless preparing a coordinated change across translator and runtime.
- Network/curl integration: native libcurl code exists under `src/net/curl/` (C and Go bindings). Be careful when refactoring network layers: there are mixed-language interfaces.
- PSP cross-build: CMake has PSP-specific compile flags and include paths. Changes to build options should be mirrored for PSP where relevant.

---

## Relevant files to reference quickly
- build.go — user-level build driver (bootstrap flags)
- CMakeLists.txt — native/CMake build rules and required libraries
- src/ — main application source (Go-like inputs to the translator)
- assets/ — runtime assets copied by CMake
- beta-wiki/ — documentation/site (Bun/VitePress)

---

If changes are made to transpiler annotations, translator usage, or CMake targets, update this file so Copilot agents keep accurate build guidance.

---

## 4) Solod skill and translator guidance

- Prefer invoking a dedicated "Solod" skill when available rather than running the `so` binary directly. The skill should provide operations to:
  - run the translator (translate src -> build/transpiled)
  - report the translator version (`so --version`)
  - perform dry-run translations and show diffs
- If the skill is unavailable, use `go install solod.dev/so@latest` or follow the local vendor instructions; ensure the runtime PATH contains `so`.
- When editing translator annotations (e.g. `//so:embed`), update build.go and any CMake/POST_BUILD asset copy commands in the same change.

## 5) Packet conventions (networking)

- Naming: Types named PacketXYZ (or with "Packet" prefix/suffix) are bi-directional and must implement both Step(...) and Write(io.Writer) where applicable.
- Reader/writer primitives to use:
  - Use net.SteppedReader{,16,32,64} for incremental reads and Reset/Step semantics.
  - Use String16Reader / WriteString16 for UCS-2 string fields.
  - Use binary.BigEndian and math.Float32frombits/Float64frombits for floats/doubles; use WriteFloat32/WriteFloat64 and WriteInteger/WriteLong helpers when writing.
  - Use mem.TryAllocSlice for variable-length arrays to avoid panics and support allocator patterns.
- Documentation source: authoritative packet formats are in beta-wiki/networking/packets/ — consult those markdown files when adding or modifying packet parsing/writing.
- TODO markers: add TODO comments in code for complex/variable-length packets (Chunk, EntityMetadata, Container/ItemData). Prefer incremental implementations and unit tests.

---

If you want, add a short CI job that runs `go build` in `src/` and a small packet-parsing unit test to exercise Step/Write symmetry; ask and I can add it.
