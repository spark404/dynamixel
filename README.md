# dynamixel

A small, dependency-free C library for the [Dynamixel 2.0 protocol](https://emanual.robotis.com/docs/en/dxl/protocol2/), targeted at the Robotis XL430 servo. The library handles packet construction, CRC validation, response parsing, and synchronous read/write across one or more servos. **It does not include a serial driver** — the caller plugs in their own bus implementation through three function pointers.

## Status

- **Supported servos:** XL430 only. Adding a new model means extending `DYNAMIXEL_*` types in `include/dynamixel/dynamixel.h` and the type check in `validate_servo`.
- **Language:** C11 library, C++20 test harness.
- **Tested platforms:** Linux (Ubuntu CI), macOS (local). No hardware-specific code; should compile anywhere with a C11 toolchain.
- **Coverage:** ~91 % lines, 100 % functions exercised by 91 unit tests.

## Building

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

A `Coverage` build type is provided for `gcovr`:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Coverage
cmake --build build
ctest --test-dir build -T Test -C Coverage
gcovr --root . --filter 'src/' --filter 'include/'
```

The library compiles with `-Wall -Wextra -pedantic -Werror`. Note that this also applies to vendored GoogleTest sources, which trip `-Wcharacter-conversion` on recent clang; the `tests/` subtree may need `-DCMAKE_CXX_FLAGS=-Wno-character-conversion` until that gets scoped to the library target only.

## Layout

```
include/dynamixel/      Public headers (dynamixel.h, dynamixel_bus.h, dynamixel_error.h)
priv_include/dynamixel/ Internal headers (api.h, protocol.h, crc.h)
src/                    Implementation
tests/                  GoogleTest unit tests
```

## Public API at a glance

```c
// Setup
dynamixel_bus_init(&bus, read_fn, write_fn, flush_fn, ctx);
dynamixel_init(&servo, id, DYNAMIXEL_XL430, &bus);

// Single-servo control
dynamixel_ping(&servo);
dynamixel_set_torque_enable(&servo, 1);
dynamixel_set_long_parameter(&servo, XL430_CT_RAM_GOAL_POSITION, 2048);
dynamixel_get_long_parameter(&servo, XL430_CT_RAM_PRESENT_POSITION, &pos);

// Multi-servo (sync write / sync read, 32-bit values)
dynamixel_set_long_parameter_multiple(servos, n, XL430_CT_RAM_GOAL_POSITION, values);
dynamixel_get_long_parameter_multiple(servos, n, XL430_CT_RAM_PRESENT_POSITION, values);

// Multi-servo, any length: adjacent registers in one packet
// Profile Velocity (112) + Goal Position (116): one 8 byte write per servo
dynamixel_set_bytes_multiple(servos, n, XL430_CT_RAM_PROFILE_VEL, 8, data);
// Present Load (126) + Velocity (128) + Position (132): one 10 byte read per servo,
// with a result per servo in status[] (may be NULL)
dynamixel_get_bytes_multiple(servos, n, XL430_CT_RAM_PRESENT_LOAD, 10, data, status);
load     = dynamixel_decode_i16(&data[i * 10 + 0]);
velocity = dynamixel_decode_i32(&data[i * 10 + 2]);
position = dynamixel_decode_u32(&data[i * 10 + 6]);
```

`data` holds `n * length` bytes, the bytes of `servos[i]` at `data[i * length]`.

**Reading with a result per servo.** `dynamixel_get_bytes_multiple` (and `dynamixel_get_long_parameter_multiple`) keep reading after a damaged reply, a servo error or a reply from the wrong servo: that servo gets its own code in `status[i]` and its data is left unchanged, so you can keep the previous value. A hardware alert (`STATUS_ALERT_FLAG`) is reported in `status[i]` but the data is valid and stored. A timeout, bus error or lost packet header ends the read: that servo and all following ones get the error, because the remaining servos may be waiting for the missing reply. The return value is the first error, or `DNM_OK`.

All functions return `dynamixel_result_t` (a `uint8_t`). `DNM_OK` (0) means success; non-zero values are documented in `include/dynamixel/dynamixel_error.h`.

## Bus interface — implementation contract

The library never touches your serial port. You provide three functions when calling `dynamixel_bus_init`:

```c
typedef ssize_t (*dynamixelWriteFunc_t)(const uint8_t *txBuffer, size_t size, void *pvArgument);
typedef ssize_t (*dynamixelReadFunc_t)(uint8_t *rxBuffer, size_t size, void *pvArgument);
typedef dynamixel_bus_result_t (*dynamixelFlushFunc_t)(void *pvArgument);
```

`pvArgument` is the `pvContext` you passed to `dynamixel_bus_init`; the library stores it and passes it back unchanged. Use it for the file descriptor, port handle, mutex, anything you need.

### Write function contract

| Return value | Meaning to the library |
|---|---|
| `n == size` | Full write — proceed |
| `n == 0` | Timeout — `dynamixel_*` returns `DNM_LL_TIMEOUT` |
| `n < 0` | Error — `dynamixel_*` returns `DNM_LL_ERR` |
| `0 < n < size` | **Partial write — `dynamixel_*` returns `DNM_LL_ERR`** |

The write function **must transmit all `size` bytes or report failure**. The library does not retry partial writes. If your underlying driver can return early (e.g. a non-blocking `write(2)` hitting EAGAIN), wrap it so the bus function only returns once all bytes are out, the timeout has expired, or an error has occurred.

### Read function contract

| Return value | Meaning to the library |
|---|---|
| `0 < n ≤ size` | `n` bytes were placed in `rxBuffer`; loop continues if more are needed |
| `n == 0` | Timeout — caller maps to `DNM_LL_TIMEOUT` |
| `n < 0` | Error — caller maps to `DNM_LL_ERR` |
| `n > size` | **Protocol violation — rejected with `DNM_LL_ERR`** |

Partial reads are fine: the library calls the read function in a loop until the whole header (7 bytes) and then the declared payload have arrived. Each call should block up to whatever timeout you configure for the bus and return as soon as at least one byte is available, or zero on timeout.

The read function **must not return more bytes than `size`**. A misbehaving driver doing so used to be able to corrupt internal counters; the library now rejects it defensively.

### Flush function contract

`flushFunc` is **optional** — pass `NULL` if you don't have one. When non-NULL, it should drop any unread bytes still in the receive buffer (e.g. `tcflush(fd, TCIFLUSH)` on POSIX) and return `DNM_BUS_OK`. The library calls it only when explicitly invoked via `dynamixel_bus_flush`; it is *not* called automatically before transactions. If your application can interleave ping/scan/recovery routines, call `dynamixel_bus_flush` after a `DNM_LL_TIMEOUT` or `DNM_RECV_CRC_FAIL` to discard stale fragments before the next operation.

### Threading and external synchronization

**The library is not thread-safe.** All public functions are synchronous and take a `const dynamixel_bus_t *`; nothing inside the library protects shared state. The Dynamixel 2.0 protocol is half-duplex on a single TTL bus, so concurrent transactions on the same bus are physically impossible anyway — the constraint is on you, the integrator, not just on the library.

What this means in practice:

1. **One thread per bus.** If only one thread ever calls `dynamixel_*` against a given `dynamixel_bus_t`, you need no synchronization. Multiple servos on the same bus is fine; multiple threads on the same bus is not.
2. **If multiple threads must share a bus, serialize externally.** Wrap every `dynamixel_*` call in a mutex held *inside `pvContext`*. The library will not lock for you. Holding the lock across the whole call (write + read) is required — releasing between the request and the response will let another thread inject a packet and corrupt the response framing.
3. **Multiple buses are independent.** Distinct `dynamixel_bus_t` instances on distinct UARTs can be operated from distinct threads with no further coordination, provided each thread sticks to its own bus.
4. **Reentrancy.** `dynamixel_*` calls do not call back into your code other than through the bus function pointers, so signal-handler reentrancy is not a concern unless your bus functions touch global state.

The bus functions themselves are called sequentially within a single library call — `write` then one or more `read`s. They do not need to be thread-safe with respect to each other; they need to be safe against concurrent invocation only if you call `dynamixel_*` from multiple threads.

### Timeouts

The library has no internal timeout. Every wait is delegated to your `read` function returning `0`. Pick a per-call timeout that comfortably exceeds the servo's `Return Delay Time` (XL430 default: 250 µs) plus packet on-wire time at your baud rate; tens of milliseconds is typical for direct USB-to-TTL.

### Buffer sizing

The library uses fixed-size stack buffers internally. Practical caller-visible limits:

- `DYNAMIXEL_MAX_PACKET_SIZE` (256 bytes, `priv_include/dynamixel/api.h`, overridable at compile time) is the size of the packet buffers on the caller's stack, two per sync call.
- `dynamixel_sync_write_bytes` needs `14 + count * (length + 1)` bytes, so 18 servos can take up to 12 bytes each (a 248 byte packet).
- `dynamixel_sync_read_bytes` needs `14 + count` bytes for the request and `11 + length` bytes per reply, plus up to `length / 3` bytes of stuffing (so `length` is at most 184).
- A request that does not fit is rejected with `DNM_API_ERR` before anything is sent.
- The multi-servo wrappers (`dynamixel_*_multiple`) further cap the list at 64 servos.

## Error codes

Defined in `include/dynamixel/dynamixel_error.h`:

| Code | Cause |
|---|---|
| `DNM_OK` | Success. |
| `DNM_API_ERR` | Invalid argument or internal validation failure. |
| `DNM_RECV_CRC_FAIL` | Response CRC did not match. |
| `DNM_RECV_NOT_STATUS` | Response was a non-status instruction. |
| `DNM_RECV_ID_MISMATCH` | (sync read) Reply came from a different servo than the one asked. |
| `DNM_RECV_LENGTH_MISMATCH` | (sync read) Reply carries a different amount of data than requested. |
| `DNM_NOT_SUPPORTED` | Unsupported servo type. |
| `DNM_LL_ERR` | Bus link error: write incomplete / read error / malformed packet. |
| `DNM_LL_TIMEOUT` | Bus write or read returned 0. |

A successful round-trip can also return a Dynamixel servo status byte directly (e.g. hardware alert flag `0x80`); these have values outside the `DNM_*` set above.

## Limitations and gotchas worth knowing

- **No retry logic.** Any single dropped byte aborts the call with `DNM_LL_TIMEOUT` or `DNM_LL_ERR`. If you need retries, wrap at the application level.
- **Byte stuffing is handled, and costs buffer space.** Protocol 2.0 inserts an extra `0xFD` after any `FF FF FD` in the parameters of a packet. The library adds it to the packets it sends and removes it from the replies it receives (the CRC is checked on the stuffed bytes first), so callers always see the real data. A reply can be up to one byte per three data bytes longer, and the buffer limits below include that; a request whose stuffed form does not fit returns `DNM_API_ERR`. A stuffing sequence that is not followed by its `0xFD` in a reply returns `DNM_RECV_LENGTH_MISMATCH`.
- **Sequential `sync_read`.** The library issues one SYNC_READ packet but reads each servo's response in turn, so the latency is roughly *N × (servo delay + on-wire time)*. For high-rate multi-servo control this matters.
- **No bulk read/write.** The Protocol 2.0 BULK opcodes are defined in `priv_include/dynamixel/protocol.h` but not exposed.
- **XL430 only.** Other Dynamixel models compile but are rejected at runtime.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

The mock under `tests/` exercises the wire format byte-for-byte against canned packets. There is no real-hardware test in the tree.

## License

See repository for license terms.
