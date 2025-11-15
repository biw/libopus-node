import assert from 'node:assert';
import fs from 'node:fs';
import path from 'node:path';
import { OpusEncoder } from '../../dist/index.js';

const opus = new OpusEncoder(16_000, 1);

const frame = fs.readFileSync(path.join(import.meta.dirname, 'frame.opus'));

const decoded = opus.decode(frame);

assert(decoded.length === 640, 'Decoded frame length is not 640');
console.log('Passed');
