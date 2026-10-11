# CLAUDE.md

Guidance for Claude Code working in this repository. Read `README.md` first for the project overview and bus-interface contract — it is the authoritative source for the user-facing API. This file covers what's not in there: layout shorthand, build/test commands that actually work on this machine, and gotchas you'll hit otherwise.

## What this is

A C11 library implementing the Robotis Dynamixel 2.0 protocol against the XL430 servo. Hardware-agnostic: the caller injects read/write/flush function pointers via `dynamixel_bus_init`. There is no serial driver in this tree.

## Layout

- `include/dynamixel/` — public headers. `dynamixel.h` (servo API), `dynamixel_bus.h` (bus contract), `dynamixel_error.h` (return codes).
- `priv_include/dynamixel/` — internal headers. `api.h` (low-level packet send/receive), `protocol.h` (header struct, instruction opcodes, magic bytes), `crc.h`.
- `src/` — implementation. `dynamixel.c` is the high-level wrapper that validates and delegates to `api.c`. `api.c` builds packets, calls the bus, parses responses. `dynamixel_bus.c` is a thin trampoline around the function pointers.
- `tests/` — GoogleTest binaries: `api_TEST.cc`, `dynamixel_TEST.cc`, `dynamixel_bus_TEST.cc`, `protocol_TEST.cc`. Each links the library and a mocked bus.
- `.github/workflows/cmake.yml` — CI: builds with `-DCMAKE_BUILD_TYPE=Coverage`, runs ctest, enforces `gcovr --fail-under-line 78`.

## Build / test / coverage

```sh
# Plain build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure

# Coverage (matches CI)
cmake -B build -DCMAKE_BUILD_TYPE=Coverage
cmake --build build --config Coverage
ctest --test-dir build -T Test -C Coverage
gcovr --root . --filter 'src/' --filter 'include/'
```

If you change source between coverage runs, delete the stale `.gcda` files first or you'll get "corrupt arc tag" warnings:

```sh
find build -name '*.gcda' -delete
```

`gcovr` may also emit a "suspicious hits" parse error from a known gcov bug; pass `--gcov-ignore-parse-errors=suspicious_hits.warn_once_per_file` to suppress.

## Warnings and `-Werror`

`-Wall -Wextra -pedantic -Werror` (`DYNAMIXEL_WARNING_FLAGS`, set in the root `CMakeLists.txt` when the library is the top level project) is applied per target: the library and the four test binaries. It is deliberately not global, because the vendored GoogleTest does not build with `-Werror` on recent clang (`-Wcharacter-conversion` in `gtest-printers.h`). No `-DCMAKE_CXX_FLAGS` workaround is needed any more.

## Conventions

- C11 (`-std=c11`, `CMAKE_C_EXTENSIONS OFF`). C++20 for tests.
- No dynamic allocation in the library. All buffers are fixed-size on the stack. If you need a buffer, declare a fixed size and bounds-check.
- No threading primitives. The library is explicitly single-threaded; synchronization is the caller's job. Don't add locking without a deliberate design discussion.
- Every public function takes the servo / bus pointer as its first argument and returns `dynamixel_result_t`.
- `validate_servo` is the universal entry guard in `dynamixel.c`; new high-level wrappers should call it before doing anything else.
- Error codes from the bus map: `n == 0` → `DNM_LL_TIMEOUT`, anything else not equal to the requested size → `DNM_LL_ERR`. All five api functions follow this pattern; if you add a new one, match it.

## Tests: extending the mock

The `ApiTest` fixture in `tests/api_TEST.cc` has overrides for forcing bus error returns:

- `_forceWriteOverride` + `_forceWriteReturn` — make `mock_write` return a chosen value (use `0` for timeout, partial for partial-write).
- `_forceReadOverride` + `_forceReadReturn` + `_forceReadAfterCalls` — make `mock_read` return a chosen value, optionally after the first N normal reads. Used to exercise over-read defenses in `dynamixel_read_response`.

- `_maxReadChunk` — when > 0, one `mock_read` call returns at most that many bytes (fragmented replies).
- `_writeTimeout` — `mock_write` returns 0 (timeout).

Expected packets for the sync tests were generated with an independent CRC16 implementation (not the library's own `update_crc`) and checked against the older hand-written vectors, so they check the CRC too.

`_writesize = -1` and `_readsize = -1` still work as the original "force a negative bus return" hooks — kept for compatibility with the older tests.

## Known gaps and caveats

- **Byte-oriented sync calls.** `dynamixel_sync_read_bytes` / `dynamixel_sync_write_bytes` (`api.c`) are the core; the `uint32_t` `dynamixel_sync_read` / `dynamixel_sync_write` are wrappers that pack and unpack. All four size their buffers with `DYNAMIXEL_MAX_PACKET_SIZE` (256), which replaced the old 128 byte read limit.
- **Sync read keeps going after a damaged reply.** It records the result per servo and stops only when the stream cannot be trusted (timeout, bus error, bad header, oversized reply). Keep that distinction when changing it.
- **Byte stuffing** (`FF FF FD` followed by an extra `FD`) lives in `protocol.c`: `dynamixel_build_packet` stuffs the parameters of every request (and fixes the length field), `dynamixel_unstuff` restores the parameters of a reply in place after the CRC check (`dynamixel_parse_parameters`). The CRC always covers the stuffed packet. A reply with a velocity or load register that holds `FF FF` followed by a position byte `FD` is the real-world trigger; reading signed registers without un-stuffing gave `DNM_RECV_LENGTH_MISMATCH` (71) on a moving robot.
- **Protocol BULK opcodes** (`BULK_READ`, `BULK_WRITE`) are defined in `protocol.h` but no public function exposes them.
- **`dynamixel_init` rejects unsupported types up front.** If you're adding XL320 / XM540 / etc support, also extend `validate_servo` and the control-table macros in `dynamixel.h`. The macros are XL430-specific (prefixed `XL430_CT_*`).
- **Sequential sync_read.** `dynamixel_sync_read` issues one packet but reads responses one at a time. Not a bug, but the latency is roughly *N × per-servo response time*. Don't try to "optimize" by reading them concurrently — Dynamixel buses are half-duplex.
- **Defensive `len < 7` guard at `api.c:345`** is unreachable from any current caller (all pass 128-byte buffers). Don't bother trying to test it without exposing the static function.

## When working on this codebase

- Run `ctest` after any change in `src/`. Tests are fast (<2 s for the full suite).
- If you change `api.c`'s read or write code paths, mentally walk through each `dynamixel_result_t` mapping. The mock makes it easy to forget that real serial drivers can return values nobody tested for.
- Don't introduce features speculatively (retries, async, callbacks). The library's value is its smallness and the explicit bus contract; adding policy belongs in the caller's wrapper.
- This is a single-author personal project, not a published package. There is no semantic-versioning or compat layer to preserve. Renaming things is fine if the rename improves clarity.
