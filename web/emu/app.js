// SPDX-License-Identifier: GPL-3.0-only
// The firmware's own part of its browser page (web/emu/index.html is niwa's and the same for all; docs/EMULATOR.md). A firmware built
// on niwa replaces this file: its name, the words under the buttons, the help, the credits. Every field is optional.
export default {
  title: "niwa",                                   // the tab, and the logo's alt
  description: "niwa, the base of the FM-1 firmwares of this family, running in your browser: the same code the instrument runs.",
  wasm: "niwa.wasm",                               // build/emu/<APP>.wasm (web/emu/build.sh)
  store: "niwa-emu-flash",                         // where this browser keeps the settings (localStorage)
  logo: "logo.svg",                                // an image next to the device (build.sh copies assets/logo.svg), or leave out
  verbs: {},                                       // the word under a button's name, by label: { FX: "SINK", HOME: "NEW" }
  // help: ["<kbd>Z</kbd> keys", ...],             // the lines under the device; the default tells the keyboard
  credits: 'niwa, <a href="https://github.com/okumurari2/niwa">source</a>',
};
