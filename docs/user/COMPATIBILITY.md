# Tested compatibility

| Game           | ID        | Windows           | Linux | GTX 1050 Ti / i5-7500 3.4GHz | Intel(R) HD Graphic 620 / i5-7200 2.5GHz |
|----------------|-----------|-------------------|-------|------------------------------|------------------------------------------|
| Dreaming Sarah | PPSA02929 | In game, playable | ?     | 60 FPS                       | 36 FPS                                   |
| DOOM Shareware (homebrew) | PPSA99666 | In game (E1M1 and later maps) | In game (E1M1), software Vulkan only | ? | ? |
| Disney Epic Mickey: Rebrushed | PPSA15657 | Menus; crashes when the first level starts | Menus; the first level runs, but its 3D view is not drawn | ? | ? |

DOOM Shareware is a public homebrew, [lowbit/ps5-doom](https://github.com/lowbit/ps5-doom), release v1.1.1 (content version 01.001.001).

- Windows 11, RTX 4070 (version 01.001.000, an integration build before [#553](https://github.com/boykopovar/AnyPS5/pull/553), [#683](https://github.com/boykopovar/AnyPS5/pull/683), [#684](https://github.com/boykopovar/AnyPS5/pull/684) and [#685](https://github.com/boykopovar/AnyPS5/pull/685) were merged): movement and shooting in E1M1. [In-game screenshots](https://github.com/boykopovar/AnyPS5/pull/685#issuecomment-5997547697).
- Windows 11, RTX 4070 SUPER (v1.1.1, `main` `2edaf7f4` plus open PRs, reported in [#690](https://github.com/boykopovar/AnyPS5/pull/690)): further maps, weapon switching, new games, level select, save/load round trip and audible audio.
- Linux, Ubuntu 24.04 container, Mesa llvmpipe, no hardware GPU (v1.1.1, `main` `0e173a46` plus [#1278](https://github.com/boykopovar/AnyPS5/pull/1278), [#1279](https://github.com/boykopovar/AnyPS5/pull/1279) and [#1307](https://github.com/boykopovar/AnyPS5/pull/1307), relinked with `--exclude-sce-module libc.prx`, see [#1497](https://github.com/boykopovar/AnyPS5/issues/1497)): E1M1, 35 FPS in game; audio and extended gameplay not tested. The `/download0` directory must exist beside the executable.
- On the first run without a shader cache, the presentation kernel is compiled at its first dispatch, frame 1 does not complete in time and the title switches to the CPU scaler. Later runs with the cached kernel stay on AGC compute presentation (measured on Windows 11, RTX 3080, `main` `d70b8998`).

Not verified: Linux with a hardware GPU, current `main` without extra PRs.

Disney Epic Mickey: Rebrushed is PPSA15657 (EU, v01.000.000), an Unreal Engine 4 title. It needs pull requests that are still open and testing-only changes, all on the [`mickey/testing`](https://github.com/johnmaia/AnyPS5/tree/mickey/testing) branch (`main` `d70b8998` with 60 commits on top). Of the pull requests it used, [#423](https://github.com/boykopovar/AnyPS5/pull/423), [#425](https://github.com/boykopovar/AnyPS5/pull/425) and [#1810](https://github.com/boykopovar/AnyPS5/pull/1810) are still open. The testing-only changes stand in for [#2102](https://github.com/boykopovar/AnyPS5/issues/2102), [#2366](https://github.com/boykopovar/AnyPS5/issues/2366) and per-block DCC metadata. Details and progress are in [discussion #2255](https://github.com/boykopovar/AnyPS5/discussions/2255).

- Linux, Ubuntu 24.04.5, AMD Radeon RX 7600 (RADV, Mesa 25.2.8), 2026-10-10: warnings, logos, intro movies, title screen and main menu work. Continue loads the first level and the game runs (dialogue, quest start, damage), but Unreal never draws the 3D view: only the HUD over the last loading image or black. The first launch compiles about 4,100 shaders, and the title screen takes about 15 minutes. On kernels before 6.11, booting is much slower without [#1810](https://github.com/boykopovar/AnyPS5/pull/1810).
- Windows 11, RTX 4080, the same build plus [#2054](https://github.com/boykopovar/AnyPS5/pull/2054), 2026-10-10: crashes at the start of the first level, with a lost device or `AGC graphics: unaligned vertex buffer`.
