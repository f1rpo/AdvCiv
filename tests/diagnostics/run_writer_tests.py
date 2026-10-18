# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Manual-only Windows check of the actual diagnostic writer with the local Visual C++ Toolkit 2003 (VC2003) compiler.
# Build a temporary executable instead of the game DLL; source extraction avoids maintaining a second buffering implementation that could pass while the real code regresses.
# The VS2010 vcvars32.bat supplies environment setup only; compilation explicitly invokes the VC2003 cl.exe selected below. (GPT-6.1-Sol) -->
import argparse
import os
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
        declarations = [header[family_start:family_end]] + [line for line in header.splitlines() if line.startswith("bool flushSASDiagnosticLogs(") or line.startswith("CvString getSASDiagnosticWriterHealthFields(")]
        (temporary / "writer_under_test.inc").write_text("\n".join(declarations) + "\n" + source[start:end], encoding="utf-8")
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
