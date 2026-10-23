# Sources, credits, and licenses

ACID FM1's original application, DSP, studio page, and tests are MIT licensed.
No Arturia executable, source, preset, sample, image, logo, or other asset is
included. Acid V was a feature reference, not a DSP implementation source.

## Code actually used

- **fm1-base**, Charles Vestal and credited contributors, MPL-2.0, pinned to
  `6c0b47e4bfc56fd3cd5b34463210d0e7e0ce0431` in `base/`.
  https://github.com/charlesvestal/fm1-base
- **felucca-bsp**, Leo Kuroshita / Hügelton Instruments, Charles Vestal,
  keremimo and credited contributors, MPL-2.0, pinned by fm1-base to
  `0f08fcaa6610b7cabe8cfdcc5c50d6facdae0e7f` in `base/bsp/`.
  This is the hardware layer split out of Felucca. Its provenance and
  permissions are documented in `base/bsp/NOTICE` and `LICENSING.md`.
  https://github.com/hugelton/felucca-bsp
- **Barlow / Barlow Semi Condensed**, Jeremy Tribby and contributors,
  SIL Open Font License 1.1. The platform's generated firmware and hardware
  emulator embed rasterizations. Original font files and license notices are
  in `base/assets/fonts/`; license texts accompany binary releases.
- **JieLi AC79 SDK** package data, Apache-2.0. The `.fwsc` package includes
  three unmodified files: `uboot.boot`, `cfg_tool.bin`, and `eq_cfg_hw.bin`.
  They are fetched at build time from official commit
  `d179b4484759423312073f5fbb232501aa491047`. No radio SDK libraries are linked.
  Source: https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK

The copied/generated browser hardware emulator retains its MPL-2.0 notice.
`tools/prepare_hardware_web.py` records its adaptation: removing external font
requests, adding navigation and a local optional browser-speech adapter.
The original platform submodules are unmodified. This project's MIT license
does not relicense any MPL, OFL, or Apache files. Their source remains available
through the pinned submodules and full-source release archive.

## Examined as references, not incorporated

- Felucca: https://github.com/hugelton/Felucca (GPL-3.0-only app).
- M-VAVE FM1 Open Workbench: https://open.m-vave.com/fm1-workbench and
  https://github.com/m-vave/fm1_open.
- X0X: https://github.com/charlesvestal/fm1-x0x (acid-machine/browser design reference).
- Community catalog: https://thefirmwarehub.com/.
- Arturia Acid V public feature descriptions and manual:
  https://www.arturia.com/products/software-instruments/acid-v/overview and
  https://support.arturia.com/hc/en-us/articles/10105396302236-Acid-V-General-Questions.

Arturia, Acid V, M-VAVE, FM-1, Felucca, and other product names belong to their
respective owners. This independent project is not endorsed by those owners.
