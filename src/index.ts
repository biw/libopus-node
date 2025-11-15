import type { Buffer } from "node:buffer";
import path from "node:path";
import { fileURLToPath } from "node:url";
import nodeGypBuild from "node-gyp-build";

export interface OpusEncoder {
  encode(buf: Buffer): Buffer;
  /**
   * Decodes the given Opus buffer to PCM signed 16-bit little-endian
   * @param buf Opus buffer
   */
  decode(buf: Buffer): Buffer;
  applyEncoderCTL(ctl: number, value: number): void;
  applyDecoderCTL(ctl: number, value: number): void;
  setBitrate(bitrate: number): void;
  getBitrate(): number;
}

export interface OpusBinding {
  OpusEncoder: new (rate: number, channels: number) => OpusEncoder;
}

// Pass the **package root** to node-gyp-build, not lib/
const moduleDir = path.dirname(fileURLToPath(import.meta.url));
const binding = nodeGypBuild(path.resolve(moduleDir, "..")) as OpusBinding;

export const { OpusEncoder } = binding;
export default binding;
