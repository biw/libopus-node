# libopus-node

[![Build](https://github.com/biw/libopus-node/workflows/CI/badge.svg)](https://github.com/biw/libopus-node/actions?query=workflow%3ACI)

`libopus-node` is a thin, modern wrapper around the official [libopus 1.5.2](https://opus-codec.org/release/stable/2024/04/12/libopus-1_5_2.html) reference implementation.

It exposes a small JavaScript API backed by N-API native bindings so you can encode or decode Opus frames directly in Node.js without spawning external processes or shipping a separate binary.

**Key traits:**

- **ESM-only**: the package uses native ES modules; you must `import` it (no `require()`).
- **Prebuilt binaries**: GitHub Actions publishes binaries for Linux, macOS, Windows (including musl) so install usually finishes without a compiler.
- **Node 20+**: built against current Node headers and includes prebuilds for the active LTS line.
- **Minimal surface**: exposes an `OpusEncoder` class with encode/decode helpers plus passthrough CTL access—ideal for bots, streaming services, or testing harnesses that want full control over buffers.

Prebuilt binaries are provided for the platforms listed below; you do not need a system libopus installed.

If no prebuild is available for your target, npm will fall back to `node-gyp-build`, so you can still compile from source as long as a C/C++ toolchain is present.

---

## Installation

```bash
yarn add libopus-node
# or
npm install libopus-node
# or
pnpm add libopus-node
```

---

## Usage

> [!NOTE]
> This package is ESM-only. Use `import`/`export` syntax or dynamic `import()` - `require('libopus-node')` is not supported.

```js
import { OpusEncoder } from "libopus-node";

const encoder = new OpusEncoder(48000, 2);

const encoded = encoder.encode(pcm);
const decoded = encoder.decode(encoded);
```

---

## API

### `new OpusEncoder(sampleRate: number, channels: number)`

Create a new encoder/decoder instance.

- `sampleRate` – one of the Opus sample rates, typically **48000**.
  (Valid Opus sample rates are 8000, 12000, 16000, 24000, 48000.)
- `channels` – number of channels:

  - `1` = mono
  - `2` = stereo

If the arguments are invalid (e.g. channels ≤ 0, sampleRate ≤ 0 or > 48000), the constructor throws.

Each `OpusEncoder` instance maintains its own internal encoder/decoder state and can be reused across many frames.

---

### `encoder.encode(pcm: Buffer): Buffer`

Encode a single frame of PCM into an Opus packet.

#### Input (encode)

- `pcm` – Node `Buffer` containing **signed 16-bit PCM** samples:

  - Interleaved by channel (LRLRLR… for stereo).
  - Little-endian.

- The number of samples **must be a multiple of `channels`**.
- Maximum frame size:

  - Up to **5760 samples per channel** per call
    (≈120 ms at 48 kHz).

If the input length is invalid (e.g. not a multiple of channels, or too many samples), an error is thrown.

#### Output (encode)

- Returns a `Buffer` containing a single Opus packet.
- The buffer length is the exact size of the encoded packet.

Typical usage is to call `encode` on small frames, e.g. 20 ms of audio at 48 kHz:

```js
// 20 ms @ 48 kHz = 960 samples per channel
const samplesPerFrame = 960;
const channels = 2;
const frameSamples = samplesPerFrame * channels;

// Int16 PCM -> Buffer
const int16 = new Int16Array(frameSamples);
// ... fill int16 with samples ...

const pcm = Buffer.from(int16.buffer);
const packet = encoder.encode(pcm);
```

---

### `encoder.decode(packet: Buffer): Buffer`

Decode a single Opus packet into PCM.

#### Input (decode)

- `packet` – `Buffer` containing a single Opus packet (as returned by `encode`, or received from elsewhere).

#### Output (decode)

- Returns a `Buffer` containing **signed 16-bit PCM**:

  - Interleaved by channel.
  - Little-endian.

- The number of samples produced depends on the packet and encoder configuration.

You can wrap the result in a `Int16Array` if you prefer:

```js
const decoded = encoder.decode(packet);
const samples = new Int16Array(
  decoded.buffer,
  decoded.byteOffset,
  decoded.byteLength / Int16Array.BYTES_PER_ELEMENT
);

// `samples.length` is frames * channels
```

---

### `encoder.setBitrate(bitrate: number): number`

Set the target bitrate for the encoder.

- `bitrate` – bits per second (e.g. `64000`, `96000`, `128000`).

Returns the libopus status code (`0` on success). On failure, throws an error.

The actual effective bitrate also depends on the application mode and audio content, as per libopus.

### `encoder.getBitrate(): number`

Get the current encoder bitrate in bits per second.

- Returns a `number` (e.g. `64000` for 64 kbps).
- On error, throws.

---

### `encoder.applyEncoderCTL(ctl: number, value: number): number`

Low-level access to libopus encoder CTL (control) options.

- `ctl` – numeric Opus encoder CTL constant.
- `value` – integer value for that CTL.

Returns the libopus status code (`0` on success). On error, throws.

> ⚠ **Note:** This helper is intended for advanced users. Only CTLs that take a single integer argument are supported. CTLs that expect pointers or no argument are **not** supported through this method.

Typical uses include options like complexity, VBR mode, etc. Refer to the libopus documentation for valid CTL constants and values.

---

### `encoder.applyDecoderCTL(ctl: number, value: number): number`

Low-level access to libopus decoder CTL options.

- Same semantics as `applyEncoderCTL`, but operates on the decoder side.

---

## Error handling

All methods throw JavaScript `Error` instances when something goes wrong, for example:

- Invalid arguments (wrong buffer size, wrong types).
- libopus errors (invalid packet, internal error, etc).

You should wrap encode/decode in try/catch if working with untrusted or external data:

```js
try {
  const packet = encoder.encode(pcm);
  const decoded = encoder.decode(packet);
} catch (err) {
  console.error("Opus error:", err);
}
```

---

## Platform support

> ⚠ **Node.js 20.0.0 or newer is required.**

Prebuilt binaries are provided for:

| Operating System | Architectures |
| ---------------- | ------------- |
| Linux            | x64, arm64    |
| macOS            | x64, arm64    |
| Windows          | x64           |

If a prebuilt binary is not available for your platform, Node will attempt to build from source during installation; this requires a working C/C++ toolchain and build tools for your OS.

---

## Relationship to `discordjs/opus`

This project is a complete rewrite of [`discordjs/opus`](https://github.com/discordjs/opus) with only the [`libopus/config`](https://github.com/biw/libopus-node/tree/main/libopus/config) and [`libopus/binding.gyp`](https://github.com/biw/libopus-node/blob/main/libopus/binding.gyp) being shared:

- **ESM-first**: the original package targets CommonJS, while this fork is ESM-only with modern TypeScript definitions emitted from the source.
- **Updated toolchain**: build scripts, GitHub workflows, and packaging have been rewritten to supply musl binaries, automated npm publishing, and Node 20+ headers.
- **Rewritten native wrapper**: the C++ addon (`opus_encoder.cpp`) was rewritten for RAII-style memory management, stricter parameter validation, lazy encoder/decoder init, and clearer error propagation (mapping libopus error codes to JS exceptions). The encode/decode paths now guard frame sizes, buffer alignment, and expose CTL helpers with bit-rate accessors, which keeps behavior deterministic across platforms.
- **Native surface tweaks**: additional libopus APIs (packet extensions, CTL helpers) and better error reporting have been integrated into the binding layer.
- **Repository layout**: dependencies, config headers, and prebuild infrastructure were heavily edited to streamline maintenance and make cross-platform builds reproducible.

If you need CommonJS compatibility or want a drop-in replacement for older Discord bot stacks, the upstream `discordjs/opus` may still be a better fit. For modern Node environments that prefer ESM and automated prebuilds, `libopus-node` is the recommended path.
