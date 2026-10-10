#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

import argparse
import io
import os
import re
import sys
import zipfile


RNG_ROW_PREFIX = "GAME_RECORD_RNG_CHECKPOINT "
STATE_ROW_PREFIX = "GAME_RECORD_STATE_CHECKPOINT "
SOURCE_ROW_PREFIX = "GAME_RECORD_SOURCE_CONTEXT "
GAME_SETTINGS_ROW_PREFIX = "GAME_RECORD_GAME_SETTINGS "
FIELD_RE = re.compile(r"([A-Za-z][A-Za-z0-9]*)=([^ ]+)")
STREAMS = ("map", "sync")
DEFAULT_EXAMPLE_OUTPUT = os.path.join(os.path.dirname(__file__), "examples", "sasgamerecord_rng_compared.txt")
COUNTERS = (
    "Calls",
    "NullMessageCalls",
    "ExternalCalls",
    "DeterministicRangeCalls",
    "SeedSets",
)
COMPARE_SUFFIXES = (
    "SessionStartState",
    "IntervalStartState",
    "State",
    "IntervalCalls",
    "SessionCalls",
    "IntervalNullMessageCalls",
    "SessionNullMessageCalls",
    "IntervalExternalCalls",
    "SessionExternalCalls",
    "IntervalDeterministicRangeCalls",
    "SessionDeterministicRangeCalls",
    "IntervalSeedSets",
    "SessionSeedSets",
    "IntervalStreamFingerprint",
    "SessionStreamFingerprint",
    "IntervalCallFingerprint",
    "SessionCallFingerprint",
)

STATE_COUNT_FIELDS = (
    "everAliveTeamCount",
    "everAlivePlayerCount",
    "cityCount",
    "unitCount",
    "groupCount",
    "plotCount",
    "dealCount",
)
STATE_HASH_COMPONENTS = ("game", "teams", "players", "cities", "units", "groups", "plots", "deals")
STATE_COMPONENTS = STATE_HASH_COMPONENTS + ("combined",)
FINGERPRINT_RE = re.compile(r"^FNV1A64:[0-9A-Fa-f]{16}$")


def read_record(path):
    if not zipfile.is_zipfile(path):
        with io.open(path, "r", encoding="utf-8", errors="replace") as handle:
            return handle.read(), os.path.basename(path)

    with zipfile.ZipFile(path, "r") as archive:
        members = [name for name in archive.namelist() if not name.endswith("/") and name.lower().endswith(".log")]
        if len(members) != 1:
            raise ValueError("%s contains %d .log members; provide a ZIP containing exactly one record" % (path, len(members)))
        data = archive.read(members[0]).decode("utf-8", errors="replace")
        return data, "%s:%s" % (os.path.basename(path), members[0])


def parse_rows(text, prefix):
    rows = []
    for line_number, raw in enumerate(text.splitlines(), 1):
        if not raw.startswith(prefix):
            continue
        fields = dict(FIELD_RE.findall(raw[len(prefix):]))
        fields["_line"] = line_number
        rows.append(fields)
    return rows


def parse_source_context(text):
    rows = parse_rows(text, SOURCE_ROW_PREFIX)
    return rows[0] if rows else {}


# <!-- custom: Reuse the existing settings row to warn when reloads intentionally reseed synchronized RNG; no duplicate runtime field is needed. (GPT-5.6-Sol) -->
def parse_game_settings(text):
    rows = parse_rows(text, GAME_SETTINGS_ROW_PREFIX)
    return rows[0] if rows else {}


def has_game_option(settings, option):
    return option in settings.get("options", "").split(",")


def integer(row, key):
    try:
        return int(row[key])
    except (KeyError, ValueError):
        raise ValueError("line %s has invalid or missing %s" % (row.get("_line", "?"), key))


def advance_lcg(state, calls):
    multiplier = 1103515245
    increment = 12345
    accumulated_multiplier = 1
    accumulated_increment = 0
    while calls > 0:
        if calls & 1:
            accumulated_multiplier = (accumulated_multiplier * multiplier) & 0xFFFFFFFF
            accumulated_increment = (accumulated_increment * multiplier + increment) & 0xFFFFFFFF
        increment = (increment * (multiplier + 1)) & 0xFFFFFFFF
        multiplier = (multiplier * multiplier) & 0xFFFFFFFF
        calls >>= 1
    return (accumulated_multiplier * state + accumulated_increment) & 0xFFFFFFFF


def validate_record(rows, label):
    errors = []
    previous = {}
    for index, row in enumerate(rows):
        for stream in STREAMS:
            prefix = stream
            start_state = integer(row, prefix + "IntervalStartState")
            end_state = integer(row, prefix + "State")
            interval_calls = integer(row, prefix + "IntervalCalls")
            interval_seed_sets = integer(row, prefix + "IntervalSeedSets")
            session_start_state = integer(row, prefix + "SessionStartState")

            if stream not in previous:
                expected_start = session_start_state
                previous[stream] = {"state": session_start_state, "sessionStartState": session_start_state}
                for counter in COUNTERS:
                    previous[stream][counter] = 0
            else:
                expected_start = previous[stream]["state"]

            if session_start_state != previous[stream]["sessionStartState"]:
                errors.append("%s row %d %s session start %u changed from %u" % (label, index + 1, stream, session_start_state, previous[stream]["sessionStartState"]))
            if start_state != expected_start:
                errors.append("%s row %d %s interval start %u != prior state %u" % (label, index + 1, stream, start_state, expected_start))
            if interval_seed_sets == 0:
                reconstructed = advance_lcg(start_state, interval_calls)
                if reconstructed != end_state:
                    errors.append("%s row %d %s LCG reconstruction %u != state %u" % (label, index + 1, stream, reconstructed, end_state))

            for counter in COUNTERS:
                interval_value = integer(row, prefix + "Interval" + counter)
                session_value = integer(row, prefix + "Session" + counter)
                expected_session = previous[stream][counter] + interval_value
                if session_value != expected_session:
                    errors.append("%s row %d %s session%s %d != prior + interval %d" % (label, index + 1, stream, counter, session_value, expected_session))
                previous[stream][counter] = session_value

            for subset in ("NullMessageCalls", "ExternalCalls", "DeterministicRangeCalls"):
                if integer(row, prefix + "Interval" + subset) > interval_calls:
                    errors.append("%s row %d %s interval%s exceeds intervalCalls" % (label, index + 1, stream, subset))
            previous[stream]["state"] = end_state
    return errors


def fingerprint64_value(text):
    if not FINGERPRINT_RE.match(text or ""):
        raise ValueError("invalid FNV1A64 fingerprint %r" % text)
    return int(text.split(":", 1)[1], 16)


def fnv1a64_uint32(value, hash_value):
    prime = 0x100000001B3
    for shift in (0, 8, 16, 24):
        hash_value ^= (value >> shift) & 0xFF
        hash_value = (hash_value * prime) & 0xFFFFFFFFFFFFFFFF
    return hash_value


def expected_state_combined_fingerprint(row):
    # <!-- custom: Mirror SASGameRecordLog.cpp exactly: fixed component order, each 64-bit fingerprint as low/high 32-bit words, then the visible object counts. (ChatGPT-5.6-Sol) -->
    hash_value = 0xCBF29CE484222325
    for component in STATE_HASH_COMPONENTS:
        value = fingerprint64_value(row.get(component + "Fingerprint", ""))
        hash_value = fnv1a64_uint32(value & 0xFFFFFFFF, hash_value)
        hash_value = fnv1a64_uint32((value >> 32) & 0xFFFFFFFF, hash_value)
    for key in STATE_COUNT_FIELDS:
        hash_value = fnv1a64_uint32(integer(row, key) & 0xFFFFFFFF, hash_value)
    return "FNV1A64:%016X" % hash_value


def validate_state_record(state_rows, rng_rows, label):
    errors = []
    if not state_rows:
        return errors
    if len(state_rows) != len(rng_rows):
        errors.append("%s has %d state checkpoints but %d RNG checkpoints" % (label, len(state_rows), len(rng_rows)))
    count = min(len(state_rows), len(rng_rows))
    for index in range(count):
        state_row = state_rows[index]
        rng_row = rng_rows[index]
        if row_identity(state_row) != row_identity(rng_row):
            errors.append("%s state row %d identity %s != RNG identity %s" % (label, index + 1, row_identity(state_row), row_identity(rng_row)))
        if state_row.get("coverage") != "CORE":
            errors.append("%s state row %d has unexpected coverage=%s" % (label, index + 1, state_row.get("coverage", "<missing>")))
        for key in STATE_COUNT_FIELDS:
            value = integer(state_row, key)
            if value < 0:
                errors.append("%s state row %d %s is negative" % (label, index + 1, key))
        for component in STATE_COMPONENTS:
            key = component + "Fingerprint"
            value = state_row.get(key, "")
            if not FINGERPRINT_RE.match(value):
                errors.append("%s state row %d has invalid or missing %s" % (label, index + 1, key))
        compute_ms = integer(state_row, "computeMilliseconds")
        if compute_ms < 0:
            errors.append("%s state row %d computeMilliseconds is negative" % (label, index + 1))
        try:
            expected_combined = expected_state_combined_fingerprint(state_row)
            if state_row.get("combinedFingerprint", "").upper() != expected_combined:
                errors.append("%s state row %d combinedFingerprint does not match component hashes/counts" % (label, index + 1))
        except ValueError as error:
            errors.append("%s state row %d cannot validate combinedFingerprint: %s" % (label, index + 1, error))
    return errors


def row_identity(row):
    return "turn=%s reason=%s" % (row.get("turn", "?"), row.get("reason", "?"))


def compare_rows(rows_a, rows_b):
    count = min(len(rows_a), len(rows_b))
    for index in range(count):
        row_a = rows_a[index]
        row_b = rows_b[index]
        identity_a = row_identity(row_a)
        identity_b = row_identity(row_b)
        if identity_a != identity_b:
            return index, [("checkpoint", identity_a, identity_b)]
        differences = []
        if row_a.get("rngFingerprintSchema") != row_b.get("rngFingerprintSchema"):
            differences.append(("rngFingerprintSchema", row_a.get("rngFingerprintSchema", "<missing>"), row_b.get("rngFingerprintSchema", "<missing>")))
        for stream in STREAMS:
            for suffix in COMPARE_SUFFIXES:
                key = stream + suffix
                if row_a.get(key) != row_b.get(key):
                    differences.append((key, row_a.get(key, "<missing>"), row_b.get(key, "<missing>")))
        if differences:
            return index, differences
    if len(rows_a) != len(rows_b):
        return count, [("checkpointCount", str(len(rows_a)), str(len(rows_b)))]
    return None, []


def compare_state_rows(rows_a, rows_b):
    if not rows_a or not rows_b:
        return None, [], False
    count = min(len(rows_a), len(rows_b))
    for index in range(count):
        row_a = rows_a[index]
        row_b = rows_b[index]
        identity_a = row_identity(row_a)
        identity_b = row_identity(row_b)
        if identity_a != identity_b:
            return index, [("checkpoint", identity_a, identity_b)], True
        differences = []
        if row_a.get("coverage") != row_b.get("coverage"):
            differences.append(("coverage", row_a.get("coverage", "<missing>"), row_b.get("coverage", "<missing>")))
        for key in STATE_COUNT_FIELDS:
            if row_a.get(key) != row_b.get(key):
                differences.append((key, row_a.get(key, "<missing>"), row_b.get(key, "<missing>")))
        for component in STATE_COMPONENTS:
            key = component + "Fingerprint"
            if row_a.get(key) != row_b.get(key):
                differences.append((key, row_a.get(key, "<missing>"), row_b.get(key, "<missing>")))
        if differences:
            return index, differences, True
    if len(rows_a) != len(rows_b):
        return count, [("checkpointCount", str(len(rows_a)), str(len(rows_b)))], True
    return None, [], True


# <!-- custom: Closing Civ4 can append one final checkpoint after an otherwise identical run.
# Report that lifecycle-only asymmetry separately instead of calling it gameplay-state divergence. (GPT-5.6-Sol) -->
def unmatched_session_finalize(rows_a, rows_b, index, differences):
    if len(differences) != 1 or differences[0][0] != "checkpointCount" or abs(len(rows_a) - len(rows_b)) != 1:
        return None
    longer_label, longer_rows = ("A", rows_a) if len(rows_a) > len(rows_b) else ("B", rows_b)
    if index != min(len(rows_a), len(rows_b)) or longer_rows[index].get("reason") != "SESSION_FINALIZE":
        return None
    return longer_label


def state_compute_summary(rows):
    if not rows:
        return None
    values = sorted(integer(row, "computeMilliseconds") for row in rows)
    total = sum(values)
    middle = len(values) // 2
    if len(values) % 2:
        median = values[middle]
    else:
        median = (values[middle - 1] + values[middle]) / 2.0
    p95_index = max(0, int((len(values) - 1) * 0.95))
    return {"count": len(values), "total": total, "median": median, "p95": values[p95_index], "max": values[-1]}


def state_difference_summary(differences):
    names = [item[0] for item in differences]
    components = [name[:-len("Fingerprint")] for name in names if name.endswith("Fingerprint") and name != "combinedFingerprint"]
    counts = [name for name in names if name in STATE_COUNT_FIELDS]
    details = []
    if components:
        details.append("components=" + ",".join(components))
    if counts:
        details.append("counts=" + ",".join(counts))
    if not details and "combinedFingerprint" in names:
        details.append("combined fingerprint only")
    return "; ".join(details) if details else "checkpoint metadata differs"


def explain(differences):
    names = {item[0] for item in differences}
    if "checkpoint" in names or "checkpointCount" in names:
        return "The lifecycle/checkpoint sequence differs before field-level RNG comparison can continue."
    if any(name.endswith("IntervalCalls") or name.endswith("SessionCalls") for name in names):
        return "At least one run consumed a different number of authoritative RNG advances in this interval."
    if any(name.endswith("State") or "StreamFingerprint" in name for name in names):
        return "Authoritative random-stream consumption diverged even if the number of calls happened to match."
    if any("CallFingerprint" in name or "NullMessageCalls" in name or "ExternalCalls" in name for name in names):
        return "Random values may still match, but call provenance, labels/data, NULL-message use, or EXE-wrapper origin differs."
    return "The checkpoint metadata differs."


def emit_report(lines, output_path):
    report = "\n".join(lines) + "\n"
    sys.stdout.write(report)
    if output_path:
        output_dir = os.path.dirname(os.path.abspath(output_path))
        if output_dir and not os.path.isdir(output_dir):
            os.makedirs(output_dir)
        with io.open(output_path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(report)
        print("Wrote: %s" % output_path)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Validate and compare authoritative RNG plus semantic-state checkpoints in two SASGameRecord .log or single-log .zip files.")
    parser.add_argument("record_a")
    parser.add_argument("record_b")
    output_group = parser.add_mutually_exclusive_group()
    output_group.add_argument("--output", help="also write the report to this path")
    output_group.add_argument("--example-output", action="store_true", help="also refresh LLM_Helpers/examples/sasgamerecord_rng_compared.txt")
    args = parser.parse_args(argv)
    output_path = DEFAULT_EXAMPLE_OUTPUT if args.example_output else args.output

    try:
        text_a, label_a = read_record(args.record_a)
        text_b, label_b = read_record(args.record_b)
        rows_a = parse_rows(text_a, RNG_ROW_PREFIX)
        rows_b = parse_rows(text_b, RNG_ROW_PREFIX)
        state_rows_a = parse_rows(text_a, STATE_ROW_PREFIX)
        state_rows_b = parse_rows(text_b, STATE_ROW_PREFIX)
        source_a = parse_source_context(text_a)
        source_b = parse_source_context(text_b)
        settings_a = parse_game_settings(text_a)
        settings_b = parse_game_settings(text_b)
        if not rows_a or not rows_b:
            raise ValueError("both records must contain GAME_RECORD_RNG_CHECKPOINT rows")

        errors_a = validate_record(rows_a, "A") + validate_state_record(state_rows_a, rows_a, "A")
        errors_b = validate_record(rows_b, "B") + validate_state_record(state_rows_b, rows_b, "B")
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        print("error: %s" % error, file=sys.stderr)
        return 2

    lines = []
    revision_a = source_a.get("recordRevision")
    revision_b = source_b.get("recordRevision")
    revision_display_a = revision_a if revision_a is not None else "<unknown>"
    revision_display_b = revision_b if revision_b is not None else "<unknown>"
    revision_comparable = revision_a is not None and revision_b is not None and revision_a == revision_b
    lines.append("A: %s (%d RNG checkpoints, %d state checkpoints, revision %s, %s)" % (label_a, len(rows_a), len(state_rows_a), revision_display_a, "valid" if not errors_a else "%d validation error(s)" % len(errors_a)))
    lines.append("B: %s (%d RNG checkpoints, %d state checkpoints, revision %s, %s)" % (label_b, len(rows_b), len(state_rows_b), revision_display_b, "valid" if not errors_b else "%d validation error(s)" % len(errors_b)))
    for label, settings in (("A", settings_a), ("B", settings_b)):
        if has_game_option(settings, "GAMEOPTION_NEW_RANDOM_SEED"):
            lines.append("REPRODUCIBILITY NOTE: %s enables GAMEOPTION_NEW_RANDOM_SEED; loading that save intentionally reseeds the synchronized RNG, so repeated reload runs are not expected to match." % label)
    for error in errors_a + errors_b:
        lines.append("VALIDATION: %s" % error)
    if errors_a or errors_b:
        emit_report(lines, output_path)
        return 2

    rng_index, rng_differences = compare_rows(rows_a, rows_b)
    rng_finalize_only = unmatched_session_finalize(rows_a, rows_b, rng_index, rng_differences) if rng_index is not None else None
    if rng_index is None:
        lines.append("RNG checkpoints are identical.")
    elif rng_finalize_only:
        lines.append("RNG checkpoints are identical through all %d comparable checkpoints; %s alone has a trailing SESSION_FINALIZE checkpoint." % (rng_index, rng_finalize_only))
    else:
        lines.append("First RNG divergence at checkpoint %d:" % (rng_index + 1))
        if rng_index < len(rows_a):
            lines.append("  A %s" % row_identity(rows_a[rng_index]))
        if rng_index < len(rows_b):
            lines.append("  B %s" % row_identity(rows_b[rng_index]))
        for name, value_a, value_b in rng_differences:
            lines.append("  %s: %s vs %s" % (name, value_a, value_b))
        lines.append("Interpretation: %s" % explain(rng_differences))

    state_index, state_differences, state_comparable = compare_state_rows(state_rows_a, state_rows_b)
    state_finalize_only = unmatched_session_finalize(state_rows_a, state_rows_b, state_index, state_differences) if state_index is not None else None
    if not state_comparable:
        lines.append("State checkpoint comparison unavailable: both records need GAME_RECORD_STATE_CHECKPOINT rows.")
    else:
        if revision_a is None or revision_b is None:
            lines.append("STATE NOTE: recordRevision is unavailable for %s; semantic state hashes are recipe-specific, so recipe compatibility is unknown." % ("both records" if revision_a is None and revision_b is None else ("A" if revision_a is None else "B")))
        elif revision_a != revision_b:
            lines.append("STATE NOTE: recordRevision differs (%s vs %s); state hashes are recipe-specific, so a hash difference may reflect recorder changes as well as gameplay state." % (revision_a, revision_b))
        summary_a = state_compute_summary(state_rows_a)
        summary_b = state_compute_summary(state_rows_b)
        if summary_a and summary_b:
            lines.append("State fingerprint cost A: total=%d ms median=%s ms p95=%d ms max=%d ms" % (summary_a["total"], summary_a["median"], summary_a["p95"], summary_a["max"]))
            lines.append("State fingerprint cost B: total=%d ms median=%s ms p95=%d ms max=%d ms" % (summary_b["total"], summary_b["median"], summary_b["p95"], summary_b["max"]))
        if state_index is None:
            lines.append("State checkpoints are identical.")
        elif state_finalize_only:
            lines.append("State checkpoints are identical through all %d comparable checkpoints; %s alone has a trailing SESSION_FINALIZE checkpoint." % (state_index, state_finalize_only))
        else:
            lines.append("First state divergence at checkpoint %d:" % (state_index + 1))
            if state_index < len(state_rows_a):
                lines.append("  A %s" % row_identity(state_rows_a[state_index]))
            if state_index < len(state_rows_b):
                lines.append("  B %s" % row_identity(state_rows_b[state_index]))
            for name, value_a, value_b in state_differences:
                lines.append("  %s: %s vs %s" % (name, value_a, value_b))
            lines.append("Interpretation: semantic CORE state diverged (%s)." % state_difference_summary(state_differences))
            if rng_index is None and revision_comparable:
                lines.append("The authoritative RNG checkpoints still match, so this is evidence of deterministic/non-RNG state divergence within the recorded CORE coverage.")

        # <!-- custom: Interpret relative divergence only when both records expose the same explicit official revision, which identifies the exact state-hash recipe.
        # Missing revision metadata must not accidentally compare equal as <unknown> == <unknown>. (ChatGPT-5.6-Sol) -->
        if not revision_comparable:
            if revision_a is None or revision_b is None:
                lines.append("Combined RNG/state ordering interpretation is intentionally suppressed because explicit matching recordRevision metadata is unavailable.")
            else:
                lines.append("Combined RNG/state ordering interpretation is intentionally suppressed across different recordRevision values.")
        elif (rng_index is None or rng_finalize_only) and (state_index is None or state_finalize_only):
            lines.append("Combined interpretation: authoritative RNG and semantic CORE state match at every comparable checkpoint.")
        elif rng_index is None or rng_finalize_only:
            lines.append("Combined interpretation: semantic CORE state diverges while authoritative RNG still matches; deterministic or non-authoritative-RNG state diverged first within checkpoint granularity.")
        elif state_index is None or state_finalize_only:
            lines.append("Combined interpretation: authoritative RNG diverges, but semantic CORE state remains equal through every comparable state checkpoint.")
        elif state_index < rng_index:
            lines.append("Combined interpretation: semantic CORE state diverges before authoritative RNG, pointing first to deterministic or non-authoritative-RNG execution rather than the tracked RNG streams.")
        elif rng_index < state_index:
            lines.append("Combined interpretation: authoritative RNG diverges before semantic CORE state; different random-stream consumption appears first and CORE gameplay state separates only at a later checkpoint.")
        else:
            lines.append("Combined interpretation: authoritative RNG and semantic CORE state first differ at the same checkpoint; both changed within that interval, so checkpoint data alone cannot order which divergence occurred first.")

    emit_report(lines, output_path)
    return 1 if ((rng_index is not None and not rng_finalize_only) or (state_comparable and state_index is not None and not state_finalize_only)) else 0


if __name__ == "__main__":
    sys.exit(main())
