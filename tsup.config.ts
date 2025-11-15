import { defineConfig } from "tsup";

export default defineConfig((opts) => ({
  clean: true,
  dts: true,
  entry: ["src/index.ts"],
  format: "esm",
  platform: "node",
  target: "node18",
  minify: !opts.watch,
  sourcemap: true,
}));
