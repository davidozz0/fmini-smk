// Verifica il wasm ABI della pagina SMK fuori dal browser:
//   node web/emu/emu_test_smk.mjs [build/emu/smk-engine-fmini.wasm]
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const { instance } = await WebAssembly.instantiate(
  readFileSync(process.argv[2] || "build/emu/smk-engine-fmini.wasm"), {});
const ex = instance.exports;
const render = (n) => {
  let peak = 0;
  for (let i = 0; i < n; i++) {
    const p = ex.synth_render();
    const dv = new DataView(ex.memory.buffer);
    for (let k = 0; k < 64; k++)
      peak = Math.max(peak, Math.abs(dv.getInt32(p + 4 * k, true)));
  }
  return peak;
};
const status = () => new Uint32Array(ex.memory.buffer, ex.fw_status(), 16);

ex.synth_init();
assert.equal(render(5000), 0);              // boot: the 3 s splash, no sound
ex.fw_event(2, 12, 100);                    // keybed key 12 (C4)
assert.ok(render(100) > 0, "key silent");
assert.equal(status()[1], 1 << 12);
ex.fw_event(3, 0, 100);                     // pad 0 -> white key
assert.equal(status()[3], 1);
const fb = new Uint16Array(ex.memory.buffer, ex.fw_screen(), 240 * 240);
assert.ok(fb.some((v) => v), "screen black");
ex.fw_release();
console.log("ok");
