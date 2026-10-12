# AdvCiv-SAS DLL Compilation Guide

This documents the tested local command-line Release-build workflow primarily for AI/LLM helpers operating through a terminal. Human modders can usually open [`AdvCiv.vcxproj`](/CvGameCoreDLL/Project/AdvCiv.vcxproj) directly with tools like Visual C++ 2010 Express and build the Release configuration through the IDE instead, as used for AdvCiv-SAS development.

The clean-target, output-verification, installation, testing and symbol-retention rules below still apply to either approach.

By default, AI/LLM helpers should let the user compile unless compilation is needed or the user agrees that the helper may do it.

## Toolchain layout

Visual C++ 2010 Express supplies the IDE, `vcvars32.bat` environment, and `nmake.exe` build driver. The actual Civ4 DLL is compiled and linked with Microsoft Visual C++ Toolkit 2003 for Civ4 compatibility.

[`AdvCiv.vcxproj`](/CvGameCoreDLL/Project/AdvCiv.vcxproj) is mainly an NMake wrapper, so it does not list every SDK/library path directly:

- [`Makefile.user`](/CvGameCoreDLL/Project/Makefile.user) sets `TOOLKIT` to `C:\Program Files (x86)\Civ4SDK\Microsoft Visual C++ Toolkit 2003` and `PSDK` to `C:\Program Files (x86)\Civ4SDK\WindowsSDK`.
- The main [`Makefile`](/CvGameCoreDLL/Project/Makefile) uses the Toolkit 2003 `cl.exe` and `link.exe`, plus the Windows SDK resource compiler.
- The system environment variable `CIV4_LIB_INSTALL_PATH=C:\Program Files (x86)\Civ4Libs` supplies the Boost 1.32.0 and Python24 headers/libraries.

Calling VS2010 `vcvars32.bat` before `nmake` is required for the remaining Visual Studio include environment. A direct `nmake` attempt without it failed on missing `sal.h`.

## Before every full compile attempt

This applies to the first attempt and every retry after a failure:

1. Resolve the exact target folder under [`CvGameCoreDLL/Project/temp_files`](/CvGameCoreDLL/Project/temp_files), e.g. `temp_files/Release` or `temp_files/Debug-opt`.
2. Delete that target folder before compiling or retrying the same configuration. Do not broaden deletion beyond the verified target folder; other configurations are isolated.
3. Keep the parent directory's tracked zero-byte `.gitkeep`. It preserves the empty workflow folder in fresh Git clones and anchors the repository hygiene check.

Stale fast-build intermediates have produced unreliable DLL behavior in testing. A one-file retry may be useful only to diagnose a compiler failure; afterward, delete that target folder before retrying the full build.

Do not accept a DLL resumed from partial intermediates after a failed attempt. Retaining `temp_files/Debug-opt` is useful after a successful build, but it must be deleted before the next Debug-opt compile; it does not affect a clean Release build.

## Release compile command

Run this PowerShell command with `CvGameCoreDLL\Project` as the working directory:

```powershell
cmd.exe /d /s /c 'call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat" >nul && set "TARGET=Release" && "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\nmake.exe" source_list /NOLOGO && "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\nmake.exe" fastdep /NOLOGO && "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\nmake.exe" dll /NOLOGO'
```

Verification on 2026-08-14: from a missing/clean `temp_files/Release` target, this completed a full Release compile and link in about 106 seconds. Full builds on this machine generally take about 1 minute 30 seconds to 1 minute 42 seconds; allow about 2 minutes before treating quiet/buffered output as hung or timing it out.

The command writes `CvGameCoreDLL\Project\Release\CvGameCoreDLL.dll`. Verify successful command output/exit status and the updated DLL.

Visual Studio/MSBuild builds also write `CvGameCoreDLL\Project\Release\AdvCiv.log`, where success is shown by `Build succeeded.`, but do not assume that direct `nmake` refreshes the IDE build log.

## Persistent build reports

Visual C++ 2010 Express Build, Rebuild and Clean commands automatically call [`build_with_report.ps1`](/CvGameCoreDLL/Project/build_with_report.ps1). Continue using the IDE normally.

Each configuration keeps one current pair beside its DLL, under `CvGameCoreDLL/Project/<target>/`:

| File | Contents |
| --- | --- |
| `AdvCiv.log` | Full command output, including uncoded messages; MSBuild writes it during IDE builds, while the wrapper writes it for terminal builds. |
| `CvGameCoreDLL.dll.build.json` | Full Git/build/debug context, timestamps, result/duration, diagnostics, temporary-artifact state and before/after DLL hashes. |

- Both files describe the current attempt and are replaced on the next attempt, including a failure or Clean. There is no automatic timestamped archive or separate reduced JSON format.
  - We rarely revisit ordinary local builds; retaining every attempt added cleanup and multiple overlapping artifacts. Git preserves deliberately shipped JSON reports, and a dated copy can be saved manually for a particular investigation.
- The JSON records branch/full commit SHA and dirty-file status at start/end, configuration/request, UTC timestamps to milliseconds, elapsed milliseconds, native step exit codes, object/PCH/PDB/incomplete-marker state and DLL size/timestamp/SHA-256.
  - Git status identifies dirty paths, not their contents or proof that source stayed unchanged during compilation. `??` means untracked, `M ` means staged modification, ` M` means unstaged modification, and `MM` means both.
  - Distinct recognized warning/error lines and counts describe captured coded diagnostics; repeated identical lines count once. The log preserves all emitted output.
- A failed command stops the sequence and returns its exit code to Visual Studio. The full JSON reports failure even if an older DLL remains; it never labels that old binary as a successful new build.
  - A forcibly interrupted process can leave an initial `RUNNING` report without final fields. Neither that report nor a Clean report qualifies as successful shipping evidence.
- For an intentional tracked Release DLL update, copy `CvGameCoreDLL.dll.build.json` with that DLL into `Assets/`, then stage both and include the exact phrase `Update DLL` in the commit message.
  - CI checks the full report in that same commit, requiring a successful Release-family Build/Rebuild and a size/SHA-256 match with its DLL. Recognized errors must be zero; warnings are recorded rather than automatically forbidden.
  - The current committed tree must retain the shipped DLL and matching JSON even on source-only commits. Ordinary source changes need no refresh; C++-used XML define name additions/removals/renames require a rebuilt Release DLL at default-branch integration. General commit approval still does not authorize including a compiled binary.
- Build folders stay Git-ignored; only deliberately copied shipped artifacts and selected documentation examples are tracked. Reports retain Git/debug context rather than discarding it for a compact shipping format; full compiler logs remain local unless deliberately included as documentation examples.
- These external files have no in-game effect or runtime cost. Embedded pre-build provenance is separate DLL metadata available to enabled SASGameRecord diagnostics; full compiler output and post-build results are not embedded.

The wrapper preserves the serial/fast build sequences and does not clean temporary folders automatically. Follow the clean-target rule above; local test installation still needs only the DLL.

For the same reporting from a terminal, initialize `vcvars32.bat` as above, then run from `CvGameCoreDLL/Project`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build_with_report.ps1 -Configuration Release -Request BUILD
```

Use `-Request REBUILD` or `-Request CLEAN` for the corresponding NMake operations. Direct `nmake` bypasses the report wrapper. The project's `-IdeBuild` switch leaves log writing to MSBuild; omit it for terminal use.

### Example build output

The examples retain completed VC2010 Express builds from 2026-10-12:

| Configuration | Full compiler output | Full build JSON |
| --- | --- | --- |
| Release | [IDE log](/_1_AdvCiv-SAS/Docs/examples/build_reports/AdvCiv_Release_example.log) | [Build report](/_1_AdvCiv-SAS/Docs/examples/build_reports/AdvCiv_Release_example.build.json) |
| Debug-opt | [IDE log](/_1_AdvCiv-SAS/Docs/examples/build_reports/AdvCiv_Debug-opt_example.log) | [Build report](/_1_AdvCiv-SAS/Docs/examples/build_reports/AdvCiv_Debug-opt_example.build.json) |

These complete examples retain Git/build context; local absolute repository paths are replaced with `<MOD_ROOT>`, source commit identifiers are mapped to rewritten Git history, and unavailable identifiers use `-`. Introductory copyright/purpose metadata is included in the JSON, while retaining recorded binary identities and build results.

Each JSON hash identifies its corresponding build's DLL, including any recorded uncommitted source changes. The Debug-opt example is not eligible as a shipped Release DLL; the full logs are maintained documentation examples, not required files beside the shipped binary.

```text
Ended UTC: 2026-10-12T21:25:52.293Z; result: SUCCEEDED; exit code: 0; duration milliseconds: 105657
Distinct coded diagnostic lines: warnings=0; errors=0
```

## Install, test, and clean

A successful project build does not by itself install or test the DLL used by the mod.

1. Copy `CvGameCoreDLL\Project\Release\CvGameCoreDLL.dll` to [`Assets/CvGameCoreDLL.dll`](/Assets/CvGameCoreDLL.dll) if the new build should be installed.
   - Pre-build request/artifact evidence is embedded in the DLL automatically; no separate provenance file needs copying. The generated header stays in that configuration's temporary build folder.
2. Verify the copied file (for example, compare source/destination SHA-256 hashes).
3. Launch and test Civ4 as appropriate for the change.
4. After verifying the generated DLL and, when installing it, copying/verifying the installed DLL, the cleanup command below may remove the generated Release target. Keeping another target such as Debug-opt does not affect Release.
5. Before the next Release build, ensure `temp_files/Release` is absent; the parent `.gitkeep` and retained Debug-opt symbols may remain.

```powershell
cmd.exe /d /s /c 'call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat" >nul && set "TARGET=Release" && "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\nmake.exe" clean /NOLOGO'
```

## Retained Debug-opt crash symbols

A clean Debug-opt build writes both `CvGameCoreDLL.dll` and its exact matching `CvGameCoreDLL.pdb` under `CvGameCoreDLL/Project/temp_files/Debug-opt`. The linked DLL records that build-time PDB path.

Installing only the DLL in `Assets` while retaining the successful build folder therefore lets automatic WinDbg dump analysis resolve private symbols, source files and line numbers without copying the PDB into `Assets` or manually extending the debugger's symbol path.

This was confirmed empirically during KI#475.2: after `temp_files/Debug-opt` was removed, a later matching Debug-opt crash report lost the useful source frames despite copying a PDB beside the installed Assets DLL; retaining the freshly rebuilt target restored them.

Keep the successful `temp_files/Debug-opt` folder while its installed DLL may need crash analysis. The folder is locally Git-ignored because retaining its exact PDB is now the normal Debug-opt workflow.

This is not permission to reuse its `.obj`, `.pch`, dependency or linker files for another accepted Debug-opt build: delete the whole `temp_files/Debug-opt` target before every subsequent Debug-opt compile or retry, then create a fresh DLL/PDB pair.

Other `temp_files/<target>` folders remain unignored so stale Release, Debug, Profile or other intermediates are visible in local status. The tracked `.gitkeep` remains for fresh clones and CI, while `.gitattributes` excludes the entire parent from release archives.

The light-source ZIP includes only the placeholder, never retained compiler intermediates or private symbols. To preserve symbols beyond the next Debug-opt build, archive the exact DLL/PDB pair outside the repository and explicitly add that archive directory to WinDbg's symbol path when analyzing its matching dump.

### Inspecting a dump from the command line

The x86 console debugger installed with WinDbg can inspect Civ4's full-memory dump directly. This PowerShell command was tested from the repository root during the KI#475.2/KI#475.3 investigation; the matching Debug-opt folder resolved the DLL functions and source lines correctly:

```powershell
$cdbExe = "$env:LOCALAPPDATA\Microsoft\WindowsApps\cdbX86.exe"
$dumpFile = "$env:LOCALAPPDATA\Temp\Civ4BeyondSword.exe.dmp"
$symbolDir = (Resolve-Path 'CvGameCoreDLL\Project\temp_files\Debug-opt').Path
& $cdbExe -z $dumpFile -y "$symbolDir;srv*" -c '.ecxr; kb; q'
```

The useful debugger commands are compact:

- `.ecxr` switches to the crashing exception context.
- `kb` prints the symbolic call stack with arguments.
- `.frame N; dv /t` selects a stack frame and shows its typed locals. Debug-opt is optimized, so treat surprising local values as leads rather than authoritative state.
- `dt TypeName address` inspects a typed object, `dd address` displays raw memory and `u FunctionName` disassembles a function when the ordinary stack is insufficient.
- `.reload /f CvGameCoreDLL.dll` forces symbol reloading after correcting the symbol path.

`Civ4BeyondSword.exe.dmp` is replaced by a later crash, so preserve a valuable dump before reproducing another one. The dump, installed DLL and PDB must all belong to the same build; source-line information from a different build is not reliable.

## Transient legacy-compiler failures

During the 2026-08-14 verification, the first full attempt had a silent VC2003 `cl.exe` code-1 failure at unchanged `CyGlobalContextInterface4.cpp`. The exact correctly pathed one-file diagnostic target compiled immediately afterward.

Deleting the complete Release target and rerunning the full command then produced a successful DLL.

For a similarly unexplained compiler failure: use a narrow retry only for diagnosis, discard that configuration's partial intermediates, delete its target folder and retry the full build from scratch.
