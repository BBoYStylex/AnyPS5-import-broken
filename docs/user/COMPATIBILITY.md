# Tested compatibility

| Game           | ID        | Windows           | Linux | GTX 1050 Ti / i5-7500 3.4GHz | Intel(R) HD Graphic 620 / i5-7200 2.5GHz |
|----------------|-----------|-------------------|-------|------------------------------|------------------------------------------|
| Dreaming Sarah | PPSA02929 | In game, playable | ?     | 60 FPS                       | 36 FPS                                   |
| DOOM Shareware (homebrew) | PPSA99666 | In game (E1M1 and later maps) | In game (E1M1), software Vulkan only | ? | ? |
| Hollow Knight: Silksong | PPSA12544 | In game | ? | ? | ? |

DOOM Shareware is a public homebrew, [lowbit/ps5-doom](https://github.com/lowbit/ps5-doom), release v1.1.1 (content version 01.001.001).

- Windows 11, RTX 4070 (version 01.001.000, an integration build before [#553](https://github.com/boykopovar/AnyPS5/pull/553), [#683](https://github.com/boykopovar/AnyPS5/pull/683), [#684](https://github.com/boykopovar/AnyPS5/pull/684) and [#685](https://github.com/boykopovar/AnyPS5/pull/685) were merged): movement and shooting in E1M1. [In-game screenshots](https://github.com/boykopovar/AnyPS5/pull/685#issuecomment-5997547697).
- Windows 11, RTX 4070 SUPER (v1.1.1, `main` `2edaf7f4` plus open PRs, reported in [#690](https://github.com/boykopovar/AnyPS5/pull/690)): further maps, weapon switching, new games, level select, save/load round trip and audible audio.
- Linux, Ubuntu 24.04 container, Mesa llvmpipe, no hardware GPU (v1.1.1, `main` `0e173a46` plus [#1278](https://github.com/boykopovar/AnyPS5/pull/1278), [#1279](https://github.com/boykopovar/AnyPS5/pull/1279) and [#1307](https://github.com/boykopovar/AnyPS5/pull/1307), relinked with `--exclude-sce-module libc.prx`, see [#1497](https://github.com/boykopovar/AnyPS5/issues/1497)): E1M1, 35 FPS in game; audio and extended gameplay not tested. The `/download0` directory must exist beside the executable.
- On the first run without a shader cache, the presentation kernel is compiled at its first dispatch, frame 1 does not complete in time and the title switches to the CPU scaler. Later runs with the cached kernel stay on AGC compute presentation (measured on Windows 11, RTX 3080, `main` `d70b8998`).

Not verified: Linux with a hardware GPU, current `main` without extra PRs.

Hollow Knight: Silksong is content version 01.000.010, a Unity IL2CPP title, relinked with `--module-dir Media/Modules --module-dir Media/Plugins`. The title's original `.prx` files stay in `app0` beside the relinked modules: without them the Unity player starts `PSN.prx` without its arguments and the module faults at start-up.

- Windows 11, Radeon RX 550 (driver 31.0.21923.11000), unmodified `main` `a11c8f7f`: boot, menus, loading a saved profile and gameplay in Moss Grotto; 41 to 53 presented frames per second in the menus, 23 in game.
- Windows 11, AMD Radeon Graphics of a Ryzen 5 7600X (driver 32.0.21043.5001), unmodified `main` `a11c8f7f`: the same path; 21 to 26 presented frames per second in the menus, 12 in game.
- Windows 11, Radeon RX 550, `main` `a11c8f7f` with a local change to the masked-select pass that this path does not need: several minutes of play across rooms with combat on an Xbox controller, with music and effects audible and no fault.

Not verified: Linux, the reference machines of the table.
