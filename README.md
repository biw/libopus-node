# libopus-js [![Build](https://github.com/biw/libopus-node/workflows/Build/badge.svg)](https://github.com/biw/libopus-node/actions?query=workflow%3ABuild) [![Prebuild](https://github.com/biw/libopus-node/workflows/Prebuild/badge.svg)](https://github.com/biw/libopus-node/actions?query=workflow%3APrebuild)

> Native bindings to libopus v1.5

This is a fork of [github.com/discordjs/opus](https://github.com/discordjs/opus) with support for ESM.

## Usage

```js
const { OpusEncoder } = require('libopus-node');

// Create the encoder.
// Specify 48kHz sampling rate and 2 channel size.
const encoder = new OpusEncoder(48000, 2);

// Encode and decode.
const encoded = encoder.encode(buffer);
const decoded = encoder.decode(encoded);
```

## Platform support

> ⚠ **Node.js 20.0.0 or newer is required.**

| Operating System | Architectures         | Notes       |
| ---------------- | --------------------- | ----------- |
| Linux            | x64, ia32, arm, arm64 | RPi 1, 2, 3 |
| macOS            | x64, arm64            |             |
| Windows          | x64                   |             |
