# AdvCiv-SAS DLL Compilation Guide

This documents the tested local command-line Release-build workflow primarily for AI/LLM helpers operating through a terminal. Human modders can usually open [`AdvCiv.vcxproj`](/CvGameCoreDLL/Project/AdvCiv.vcxproj) directly with tools like Visual C++ 2010 Express and build the Release configuration through the IDE instead, as used for AdvCiv-SAS development. The clean-target, output-verification, installation, testing and symbol-retention rules below still apply to either approach.

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

Stale fast-build intermediates have produced unreliable DLL behavior in testing. A one-file retry may be useful only to diagnose a compiler failure; afterward, delete that target folder before retrying the full build. Do not accept a DLL resumed from partial intermediates after a failed attempt. Retaining `temp_files/Debug-opt` is useful after a successful build, but it must be deleted before the next Debug-opt compile; it does not affect a clean Release build.

## Release compile command

Run this PowerShell command with `CvGameCoreDLL\Project` as the working directory:

```powershell
cmd.exe /d /s /c 'call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat" >nul && set "TARGET=Release" && "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\nmake.exe" source_list /NOLOGO && "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\nmake.exe" fastdep /NOLOGO && "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\nmake.exe" dll /NOLOGO'
```

Verification on 2026-08-14: from a missing/clean `temp_files/Release` target, this completed a full Release compile and link in about 106 seconds. Full builds on this machine generally take about 1 minute 30 seconds to 1 minute 42 seconds; allow about 2 minutes before treating quiet/buffered output as hung or timing it out.

The command writes `CvGameCoreDLL\Project\Release\CvGameCoreDLL.dll`. Verify successful command output/exit status and the updated DLL. Visual Studio/MSBuild builds also write `CvGameCoreDLL\Project\Release\AdvCiv.log`, where success is shown by `Build succeeded.`, but do not assume that direct `nmake` refreshes the IDE build log.

## Install, test, and clean

A successful project build does not by itself install or test the DLL used by the mod.

1. Copy `CvGameCoreDLL\Project\Release\CvGameCoreDLL.dll` to [`Assets/CvGameCoreDLL.dll`](/Assets/CvGameCoreDLL.dll) if the new build should be installed.
2. Verify the copied file (for example, compare source/destination SHA-256 hashes).
3. Launch and test Civ4 as appropriate for the change.
4. After verifying the generated DLL and, when installing it, copying/verifying the installed DLL, the cleanup command below may remove the generated Release target. Keeping another target such as Debug-opt does not affect Release.
5. Before the next Release build, ensure `temp_files/Release` is absent; the parent `.gitkeep` and retained Debug-opt symbols may remain.

```powershell
cmd.exe /d /s /c 'call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat" >nul && set "TARGET=Release" && "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\nmake.exe" clean /NOLOGO'
```

## Retained Debug-opt crash symbols

A clean Debug-opt build writes both `CvGameCoreDLL.dll` and its exact matching `CvGameCoreDLL.pdb` under `CvGameCoreDLL/Project/temp_files/Debug-opt`. The linked DLL records that build-time PDB path. Installing only the DLL in `Assets` while retaining the successful build folder therefore lets automatic WinDbg dump analysis resolve private symbols, source files and line numbers without copying the PDB into `Assets` or manually extending the debugger's symbol path. This was confirmed empirically during KI#475.2: after `temp_files/Debug-opt` was removed, a later matching Debug-opt crash report lost the useful source frames despite copying a PDB beside the installed Assets DLL; retaining the freshly rebuilt target restored them.

Keep the successful `temp_files/Debug-opt` folder while its installed DLL may need crash analysis. The folder is locally Git-ignored because retaining its exact PDB is now the normal Debug-opt workflow. This is not permission to reuse its `.obj`, `.pch`, dependency or linker files for another accepted Debug-opt build: delete the whole `temp_files/Debug-opt` target before every subsequent Debug-opt compile or retry, then create a fresh DLL/PDB pair.

Other `temp_files/<target>` folders remain unignored so stale Release, Debug, Profile or other intermediates are visible in local status. The tracked `.gitkeep` remains for fresh clones and CI, while `.gitattributes` excludes the entire parent from release archives. The light-source ZIP includes only the placeholder, never retained compiler intermediates or private symbols. To preserve symbols beyond the next Debug-opt build, archive the exact DLL/PDB pair outside the repository and explicitly add that archive directory to WinDbg's symbol path when analyzing its matching dump.

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

During the 2026-08-14 verification, the first full attempt had a silent VC2003 `cl.exe` code-1 failure at unchanged `CyGlobalContextInterface4.cpp`. The exact correctly pathed one-file diagnostic target compiled immediately afterward. Deleting the complete Release target and rerunning the full command then produced a successful DLL.

For a similarly unexplained compiler failure: use a narrow retry only for diagnosis, discard that configuration's partial intermediates, delete its target folder and retry the full build from scratch.
