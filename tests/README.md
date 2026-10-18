# Local tests

These tests exercise mod code outside Civ4 using platform-specific development tools. Run the relevant test when changing that subsystem or investigating a failure; they do not run during gameplay or normal DLL builds.

CI describes automatic execution, not a different kind of test. Our [GitHub workflows](/.github/workflows/README.md) run repository checks and [workflow-helper regression tests](/.github/workflows/tests/README.md); those commands can also be run locally. The tests in this folder currently require a separate local setup.

## Diagnostic writer

A user or agent explicitly runs this test; Civ4's flushing works independently of it.

The [Python runner](/tests/diagnostics/run_writer_tests.py) organizes the check: it extracts the current writer from `CvGameCoreUtils.cpp`, invokes VC2003 to compile it, then runs the executable. Python is the launcher; VC2003 is the compiler.

The [C++ harness](/tests/diagnostics/sas_diagnostic_writer_harness.cpp) supplies minimal game scaffolding and the test cases. Compiling it with the extracted production writer tests the actual implementation rather than a separate reimplementation.

Run from the mod root:

```powershell
python tests/diagnostics/run_writer_tests.py
```

- Requires Python 3, Windows, Visual C++ Toolkit 2003 (VC2003), the Windows SDK and the Civ4 Boost headers. Defaults follow the local [DLL compilation setup](/_1_AdvCiv-SAS/Docs/Modding_Ressources/README_DLL_Compilation.md); `--compiler`, `--vcvars` and `--sdk` override their locations, and `CIV4_LIB_INSTALL_PATH` supplies the library root.
- VS2010's `vcvars32.bat` supplies environment setup; the compiler invoked is VC2003's `cl.exe`.
- Exercises Unicode paths, reading during append, literal percent text, overwrite/append behavior, partial and zero-progress writes, shared retry-buffer limits, recovery notices, handle reuse/release and shutdown before BUG's root handoff.
- Runs in temporary directories, including simulated fallback output; no game DLL, project intermediates or real user logs are changed. The single C++ fixture needs no separate header because it shares no declarations with another compiled source file.
  - The executable and compiler outputs live under a temporary Windows folder, normally `%TEMP%\sas-cpp-writer-...`, and are removed when the runner finishes; they are not stored in Downloads, committed or installed into Civ4.
  - For example, on this setup the executable can be `C:\Users\PC\AppData\Local\Temp\sas-cpp-writer-ab12cd34\writer.exe`; the generated folder suffix varies.
  - Each invocation rebuilds from the current writer source, so later implementation changes are tested. Test cases remain unchanged until deliberately updated; no previously compiled test executable is reused.

Run this test when changing diagnostic buffering, paths, file sharing, retry/loss handling or lifecycle checkpoints, and when investigating missing or corrupted BBAI/SASGameRecord output. Unrelated AI, UI or gameplay edits do not require it.

A successful run exits with code 0 and prints three `PASS C++ diagnostic writer` results, including a child process terminated without checkpoints or cleanup whose final submitted rows remain readable in both logs. Missing tools, compilation errors or failed assertions return a nonzero exit code.

The harness supplies minimal game scaffolding and simulates failures. Full DLL compilation, Python/EXE lifecycle integration, in-game live log access and autoplay performance still require separate validation.

## Why this test is local

Our current CI check/test workflows use Ubuntu; this harness needs Windows APIs and the configured legacy compiler/SDK. Automating it would require a Windows job with that toolchain provisioned, so no automatic workflow is added for this single specialized test. It can be automated later if that runner setup becomes worthwhile.

The light-source ZIP includes these test sources for external review; executing them still requires the tools listed above.
