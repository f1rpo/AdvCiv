# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Manual-only Windows check of the actual diagnostic writer with the local Visual C++ Toolkit 2003 (VC2003) compiler.
# Build a temporary executable instead of the game DLL; source extraction avoids maintaining a second buffering implementation that could pass while the real code regresses.
# The VS2010 vcvars32.bat supplies environment setup only; compilation explicitly invokes the VC2003 cl.exe selected below. (GPT-6.1-Sol) -->
import argparse
import os
# <!-- custom: Parse the actual emitted runtime-status snapshots with a standard JSON parser rather than duplicating the C++ formatter in assertions. (GPT-6.1-Sol) -->
import json
# <!-- custom: Read the compiled recorder revision for output comparisons so legitimate future increments do not require changing test expectations too. (GPT-6.1-Sol) -->
import re
from pathlib import Path
import subprocess
import tempfile


# <!-- custom: The runner now lives under tests/diagnostics; resolve the mod root two parent directories above this test folder. (GPT-6.1-Sol) -->
ROOT = Path(__file__).resolve().parents[2]


def main():
    program_files = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"))
    parser = argparse.ArgumentParser(description="Exercise the current C++ diagnostic writer without building/installing a DLL (Windows, local VC2003 SDK required).")
    parser.add_argument("--compiler", type=Path, default=program_files / "Civ4SDK/Microsoft Visual C++ Toolkit 2003/bin/cl.exe")
    parser.add_argument("--vcvars", type=Path, default=program_files / "Microsoft Visual Studio 10.0/VC/bin/vcvars32.bat")
    parser.add_argument("--sdk", type=Path, default=program_files / "Civ4SDK/WindowsSDK")
    parser.add_argument("--export-runtime-examples", type=Path, help="Export illustrative runtime JSON/JSONL fixtures to this directory after the checks pass; these are not game measurements.")
    args = parser.parse_args()
    for path in (args.compiler, args.vcvars, args.sdk):
        if not path.exists():
            parser.error("required local compiler/SDK path missing: %s" % path)

    # <!-- custom: Pass cmd one quoted command line; subprocess argv-list escaping inserted backslashes before the batch-path quotes and the setup failed. (GPT-6.1-Sol) -->
    setup = subprocess.run('cmd.exe /d /s /c "call "%s" >nul && set"' % args.vcvars, capture_output=True, text=True, check=True)
    environment = os.environ.copy()
    for line in setup.stdout.splitlines():
        key, separator, value = line.partition("=")
        if separator and key:
            environment[key.upper()] = value
    toolkit = args.compiler.parent.parent
    environment["PATH"] = str(args.compiler.parent) + os.pathsep + environment.get("PATH", "")
    libraries = Path(os.environ.get("CIV4_LIB_INSTALL_PATH", str(program_files / "Civ4Libs")))
    boost = libraries / "Boost-1.32.0/include"
    if not (boost / "boost/static_assert.hpp").is_file():
        parser.error("Civ4 Boost include path missing: %s" % boost)
    environment["INCLUDE"] = os.pathsep.join((str(toolkit / "include"), str(args.sdk / "include"), str(boost), environment.get("INCLUDE", "")))
    environment["LIB"] = os.pathsep.join((str(toolkit / "lib"), str(args.sdk / "lib"), environment.get("LIB", "")))

    source = (ROOT / "CvGameCoreDLL/CvGameCoreUtils.cpp").read_text(encoding="utf-8")
    start = source.index("struct SASDiagnosticLogBuffer\n")
    end = source.index("// <!-- custom: The Makefile stamps", start)
    with tempfile.TemporaryDirectory(prefix="sas-cpp-writer-") as directory:
        temporary = Path(directory)
        # <!-- custom: The first extracted fixture omitted the header default argument and VC2003 rejected no-argument calls with C2660; include the actual public declaration so the fixture follows the current interface too. (GPT-6.1-Sol) -->
        header = (ROOT / "CvGameCoreDLL/CvGameCoreUtils.h").read_text(encoding="utf-8")
        # <!-- custom: The writer now depends on the public producer-family enum; extracting only the struct caused VC2003 C2146/C2501 failures, so include the real header enum before its declarations. (GPT-6.1-Sol) -->
        family_start = header.index("enum SASDiagnosticLogFamily\n")
        family_end = header.index("};", family_start) + 2
        event_start = header.index("enum SASRuntimeStatusReason\n")
        event_end = header.index("};", event_start) + 2
        enums = (ROOT / "CvGameCoreDLL/CvEnums.h").read_text(encoding="utf-8")
        autoplay_enum = enums[enums.index("#define SAS_AUTOPLAY_END_CAUSE_ROWS"):enums.index("enum TradeableItems")]
        declarations = [autoplay_enum, header[event_start:event_end], header[family_start:family_end]] + [line for line in header.splitlines() if line.startswith(("bool flushSASDiagnosticLogs(", "CvString getSASDiagnosticWriterHealthFields(", "void setSASRuntimeStatusRootDirectory(", "void updateSASRuntimeStatusAutoPlay("))]
        (temporary / "writer_under_test.inc").write_text("\n".join(declarations) + "\n" + source[start:end], encoding="utf-8")
        # <!-- custom: Status gained gameplay dependencies inside the extracted writer and VC2003 rejected the old fixture.
        # Keep production services separate, then test both with explicit game scaffolding and real Win32 publication. (GPT-6.1-Sol) -->
        status = (ROOT / "CvGameCoreDLL/SASRuntimeStatus.cpp").read_text(encoding="utf-8")
        expected_revision = int(re.search(r"\bSAS_RUNTIME_REVISION\s*=\s*(\d+)", status).group(1))
        status_start = status.index("// <!-- custom: Finite process/game/session states")
        clock_start = source.index("// <!-- custom: SYSTEMTIME is the common")
        clock_end = source.index("// <!-- custom: SAS diagnostic wall-duration", clock_start)
        identity_start = source.index("// <!-- custom: The existing processUtc value")
        identity_end = source.index("// <!-- custom: BBAI and SASGameRecord need", identity_start)
        # <!-- custom: Execute the actual CvGame hook condition without compiling the whole game; this catches duplicate countdown publications without maintaining a second predicate. (GPT-6.1-Sol) -->
        game = (ROOT / "CvGameCoreDLL/CvGame.cpp").read_text(encoding="utf-8")
        gate_line = next(line.strip() for line in game.splitlines() if line.strip().startswith("if (isSASRuntimeStatusEnabled() && iOldAIAutoPlay"))
        gate = "\nstatic bool shouldPublishAutoplayChange(int iOldAIAutoPlay, int m_iAIAutoPlay, bool bChangePlayerStatus) { return " + gate_line[4:-1] + "; }\n"
        # <!-- custom: Exercise the real DLL-identity formatter with a controlled module-context fixture; production keeps its existing Win32 resolver, while the fixture counts lookups to catch per-turn rehashing and disabled work. (GPT-6.1-Sol) -->
        context_start = source.index("struct SASBinaryDiagnosticContext\n")
        context_end = source.index("// <!-- custom: FILETIME uses", context_start)
        fingerprint_start = source.index("static CvString getSASBinaryFingerprint(")
        fingerprint_end = source.index("// <!-- custom: DLL and EXE diagnostics", fingerprint_start)
        dll_start = source.index("CvString getSASDllRuntimeIdentityJson(")
        dll_end = source.index("CvString getSASExeContextFields()", dll_start)
        # <!-- custom: Keep executable fixture code in the C++ harness; Python only extracts the production binary context/formatter and supplies its resolver declaration in the temporary include. (GPT-6.1-Sol) -->
        binary_fixture = source[context_start:context_end] + "static SASBinaryDiagnosticContext const& getSASDllDiagnosticContext();\n" + source[fingerprint_start:fingerprint_end] + source[dll_start:dll_end]
        (temporary / "runtime_binary_context_under_test.inc").write_text(binary_fixture, encoding="utf-8")
        cause_start = source.index("char const* getSASAutoPlayEndCause(")
        cause_end = source.index("// <!-- custom: Keep one iterable source of truth", cause_start)
        (temporary / "runtime_status_under_test.inc").write_text(source[cause_start:cause_end] + source[clock_start:clock_end] + source[identity_start:identity_end] + status[status_start:] + gate, encoding="utf-8")
        executable = temporary / "writer.exe"
        subprocess.run([
            str(args.compiler), "/nologo", "/EHsc", "/MT", "/W3", "/D_USRDLL",
            "/I" + str(ROOT / "CvGameCoreDLL"), "/I" + str(temporary),
            str(ROOT / "tests/diagnostics/sas_diagnostic_writer_harness.cpp"),
            "/Fo" + str(temporary / "writer.obj"), "/Fe" + str(executable),
        ], cwd=temporary, env=environment, check=True)
        environment["SAS_DIAGNOSTIC_TEST_ROOT"] = str(temporary)
        subprocess.run([str(executable)], cwd=temporary, env=environment, check=True)
        subprocess.run([str(executable), "--terminal-fallback"], cwd=temporary, env=environment, check=True)
        subprocess.run([str(executable), "--runtime-status-disabled"], cwd=temporary, env=environment, check=True)
        subprocess.run([str(executable), "--runtime-status"], cwd=temporary, env=environment, check=True)
        subprocess.run([str(executable), "--runtime-status-output-failure"], cwd=temporary, env=environment, check=True)
        recovered = json.loads((temporary / "runtime_failed_output.json").read_text(encoding="utf-8"))
        assert recovered["previousActivityDurationMilliseconds"] == 5000 and recovered["historyWriteFailures"] == 1
        assert recovered["historyRowsWritten"] == 2 and recovered["activitySerial"] == 3 and recovered["statusWriteFailures"] == 1
        recovered_status = json.loads((temporary / "runtime_failed_status.json").read_text(encoding="utf-8"))
        assert recovered_status["statusWriteFailures"] == 5 and recovered_status["historyWriteFailures"] == 2
        assert recovered_status["historyRowsWritten"] == 2 and recovered_status["activitySerial"] == 4
        snapshots = {path.stem: json.loads(path.read_text(encoding="utf-8")) for path in (temporary / "runtime_snapshots").glob("*.json")}
        expected = {"no_game": ("NO_GAME", "NONE", 0), "starting": ("SESSION_STARTING", "NEW_GAME", 1), "ready": ("PLAYING", "NEW_GAME", 1), "autoplay": ("PLAYING", "NEW_GAME", 1), "turn": ("PLAYING", "NEW_GAME", 1), "interrupted": ("PLAYING", "NEW_GAME", 1), "menu_request": ("PLAYING", "NEW_GAME", 1), "menu_reset": ("NO_GAME", "NONE", 1), "exit_request": ("NO_GAME", "NONE", 2), "load_starting": ("SESSION_STARTING", "LOAD_GAME", 2), "loaded": ("PLAYING", "LOAD_GAME", 2), "ended": ("GAME_ENDED", "LOAD_GAME", 2), "reset": ("NO_GAME", "NONE", 2), "exit": ("NO_GAME", "NONE", 2)}
        reasons = {"no_game": "ROOT_READY", "starting": "NEW_GAME_STARTED", "ready": "NEW_GAME_NATIVE_READY", "autoplay": "AUTOPLAY_STARTED", "turn": "TURN_COMPLETED", "interrupted": "AUTOPLAY_ENDED", "menu_request": "RETURN_TO_MENU_REQUESTED", "menu_reset": "GAME_RESET", "exit_request": "EXIT_REQUESTED", "load_starting": "LOAD_GAME_STARTED", "loaded": "LOAD_GAME_NATIVE_READY", "ended": "GAME_ENDED", "reset": "GAME_RESET", "exit": "PROCESS_EXIT"}
        for name, values in expected.items():
            row = snapshots[name]
            if row["updateReason"] != reasons[name]:
                raise RuntimeError("wrong serialized event name: " + name)
            if (row["gameState"], row["sessionKind"], row["sessionSerial"]) != values:
                raise RuntimeError("wrong runtime lifecycle snapshot: " + name)
            if row["processId"] <= 0 or row["processStartUtc"] == "unknown_time":
                raise RuntimeError("missing process identity: " + name)
        # <!-- custom: Check the actual nested whitespace as well as parsed equality; valid JSON alone previously let a compact identity remain inside the indented status file. (GPT-6.1-Sol) -->
        for path in (temporary / "runtime_snapshots").glob("*.json"):
            text = path.read_text(encoding="utf-8")
            assert '\n  "dllIdentity": {\n    "buildConfiguration": "Debug-opt",' in text
            assert '\n    "peTimestampRaw": 42\n  },\n  "statusSerial": ' in text
        for row in snapshots.values():
            assert row["revision"] == expected_revision and "schema" not in row
            assert row["dllIdentity"] == {"buildConfiguration": "Debug-opt", "moduleFound": 1, "fileReadable": 1, "fileSizeBytes": 123456, "fingerprint": "FNV1A64:123456789ABCDEF0", "lastWriteUtc": "20261018T000000.000Z", "peTimestampRaw": 42}
        assert snapshots["starting"]["gameTurn"] == -1 and snapshots["load_starting"]["loadedSaveCheckpointTurn"] == -1
        assert snapshots["autoplay"]["autoplayActive"] == 1 and snapshots["turn"]["lastCompletedTurn"] == 4
        assert snapshots["loaded"]["loadedSaveCheckpointTurn"] == 7 and snapshots["loaded"]["loadCount"] == 1
        assert snapshots["exit"]["processState"] == "EXITED_CLEANLY"
        assert snapshots["ready"]["previousActivity"] == "CREATING_GAME" and snapshots["ready"]["previousActivityDurationMilliseconds"] == 15800
        assert snapshots["autoplay"]["previousActivityDurationMilliseconds"] == 3200
        assert snapshots["turn"]["activityElapsedMilliseconds"] == 5900
        assert snapshots["interrupted"]["previousActivity"] == "AUTOPLAY" and snapshots["interrupted"]["previousActivityDurationMilliseconds"] == 16000
        assert snapshots["menu_reset"]["previousActivity"] == "RETURNING_TO_MENU" and snapshots["menu_reset"]["previousActivityDurationMilliseconds"] == 3800
        assert snapshots["loaded"]["previousActivity"] == "LOADING_SAVE" and snapshots["loaded"]["previousActivityDurationMilliseconds"] == 3200
        timeline = [json.loads(line) for line in (temporary / "runtime_snapshots/timeline.jsonl").read_text(encoding="utf-8").splitlines()]
        assert len(timeline) == snapshots["exit"]["historyRowsWritten"] and snapshots["exit"]["historyWriteFailures"] == 0
        assert [row["stateSerial"] for row in timeline] == list(range(1, len(timeline) + 1))
        assert timeline[0]["state"] == "NO_GAME" and timeline[-1]["state"] == "SHUTTING_DOWN"
        stop = next(row for row in timeline if row["reason"] == "AUTOPLAY_ENDED")
        assert stop["autoplayEndCause"] == "USER_INTERRUPTED" and stop["previousRequestedAutoplayTurns"] == 5
        assert all(row["revision"] == expected_revision and "schema" not in row and row["dllIdentity"] == snapshots["exit"]["dllIdentity"] for row in timeline)
        loaded = next(row for row in timeline if row["reason"] == "LOAD_GAME_NATIVE_READY")
        assert loaded["loadedSaveCheckpointTurn"] == 7 and loaded["loadedSaveCheckpointUtc"] == "20261018T000000.000Z"
        # <!-- custom: Export a matched snapshot/history prefix only on explicit request; use a portable companion basename and keep fixture data clearly separate from empirical Civ4 results. (GPT-6.1-Sol) -->
        if args.export_runtime_examples is not None:
            destination = args.export_runtime_examples.resolve()
            destination.mkdir(parents=True, exist_ok=True)
            example = dict(snapshots["loaded"])
            example["historyFilename"] = "SASRuntimeHistory_example.jsonl"
            rows = [row for row in timeline if row["stateSerial"] <= example["activitySerial"]]
            assert len(rows) == example["historyRowsWritten"]
            (destination / "SASRuntimeStatus_example.json").write_text(json.dumps(example, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
            (destination / example["historyFilename"]).write_text("".join(json.dumps(row, ensure_ascii=False, separators=(",", ":")) + "\n" for row in rows), encoding="utf-8", newline="\n")
            print("Exported illustrative fixture examples (controlled DLL/game/elapsed-clock data; not Civ4 benchmark measurements): " + str(destination))
        print("PASS C++ runtime status: JSON/lifecycle/DLL identity, timed history/interruption, root migration, partial append rollback and disabled no-I/O")
        abrupt = subprocess.run([str(executable), "--abrupt-tail"], cwd=temporary, env=environment)
        if abrupt.returncode != 73:
            raise RuntimeError("abrupt writer fixture did not terminate at the expected point")
        for filename, expected in (("BBAI_crash.log", b"BBAI last evidence 75%\r\nfinal BBAI tail\r\n"), ("SASGameRecord_crash.log", b"SGR last evidence 75%\r\nfinal SGR tail\r\n")):
            if (temporary / "abrupt/Logs" / filename).read_bytes() != expected:
                raise RuntimeError("last completed row lost after abrupt termination: " + filename)
        print("PASS C++ diagnostic writer: both final rows survive abrupt process termination without cleanup")
    print("No game DLL or project intermediates were changed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
