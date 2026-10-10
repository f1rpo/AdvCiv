#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
import argparse
import csv
import datetime
import io
import os
import re
import xml.etree.ElementTree as ET

DEFAULT_OUTPUT_DIR = os.path.join("LLM_Helpers", "outputs")
DEFAULT_EXAMPLE_DIR = os.path.join("LLM_Helpers", "examples")
DEFAULT_LEFT_LABEL = "File 1"
DEFAULT_RIGHT_LABEL = "File 2"

NUMERIC_RE = re.compile(r"^[+-]?\d+(?:\.\d+)?$")

def strip_ns(tag):
    if "}" in tag:
        return tag.rsplit("}", 1)[1]
    return tag

def text_value(node):
    text = (node.text or "").strip()
    if text:
        return text
    return ""

def is_leaf(node):
    return len(list(node)) == 0

def compact_child_signature(node):
    children = list(node)
    if not children:
        return text_value(node)
    parts = []
    for child in children:
        if is_leaf(child):
            parts.append("%s=%s" % (strip_ns(child.tag), text_value(child)))
        else:
            parts.append("%s={%s}" % (strip_ns(child.tag), compact_collection_value(child)))
    return ",".join(parts)

def compact_collection_value(node):
    items = [compact_child_signature(child) for child in list(node)]
    counts = {}
    for item in items:
        counts[item] = counts.get(item, 0) + 1
    parts = []
    for item in sorted(counts.keys()):
        if counts[item] == 1:
            parts.append(item)
        else:
            parts.append("%s x%d" % (item, counts[item]))
    return "; ".join(parts)

def is_repeated_collection(node):
    children = list(node)
    if not children:
        return False
    first = strip_ns(children[0].tag)
    for child in children:
        if strip_ns(child.tag) != first:
            return False
    return True

def child_path(parent_path, child, sibling_counts, sibling_seen):
    tag = strip_ns(child.tag)
    if sibling_counts.get(tag, 0) <= 1:
        return "%s/%s" % (parent_path, tag)
    sibling_seen[tag] = sibling_seen.get(tag, 0) + 1
    return "%s/%s[%d]" % (parent_path, tag, sibling_seen[tag])

def flatten_node(node, parent_path, out):
    children = list(node)
    if not children:
        out[parent_path] = text_value(node)
        return
    if parent_path and is_repeated_collection(node):
        out[parent_path] = compact_collection_value(node)
        return

    sibling_counts = {}
    for child in children:
        tag = strip_ns(child.tag)
        sibling_counts[tag] = sibling_counts.get(tag, 0) + 1

    sibling_seen = {}
    for child in children:
        flatten_node(child, child_path(parent_path, child, sibling_counts, sibling_seen), out)

def parse_handicaps(path):
    tree = ET.parse(path)
    root = tree.getroot()
    result = []
    for node in root.iter():
        if strip_ns(node.tag) != "HandicapInfo":
            continue
        h_type = None
        for child in list(node):
            if strip_ns(child.tag) == "Type":
                h_type = text_value(child)
                break
        if not h_type:
            raise RuntimeError("HandicapInfo without Type in %s" % path)
        flat = {}
        flatten_node(node, "", flat)
        result.append((h_type, flat))
    return result

def numeric_delta(base, compare):
    if not (NUMERIC_RE.match(base or "") and NUMERIC_RE.match(compare or "")):
        return ""
    if "." in base or "." in compare:
        delta = float(compare) - float(base)
        if delta == 0:
            return "0"
        return "%+f" % delta
    delta = int(compare) - int(base)
    if delta == 0:
        return "0"
    return "%+d" % delta

def numeric_delta_percent(base, compare):
    if not (NUMERIC_RE.match(base or "") and NUMERIC_RE.match(compare or "")):
        return ""
    base_value = float(base)
    if base_value == 0:
        return ""
    delta_pct = (float(compare) - base_value) * 100.0 / base_value
    if delta_pct == 0:
        return "0%"
    return "%+.1f%%" % delta_pct

def md_cell(value):
    return str(value).replace("\\", "\\\\").replace("|", "\\|").replace("\r", " ").replace("\n", " ")

def dict_by_type(data):
    return dict((h_type, fields) for h_type, fields in data)

def infer_left_type_for_right_index(right_idx, left_data, right_data):
    # <!-- custom: Prefer exact <Type> matches, but keep target/right XML order readable when the target adds a new lowest or highest handicap.
    # New target-only entries are compared to the nearest shared boundary dynamically, e.g. a new Rookie before Settler compares to the base file's lowest shared Settler without hardcoding Rookie/Settler names. (ChatGPT-5.5) -->
    left_by_type = dict_by_type(left_data)
    right_type = right_data[right_idx][0]
    if right_type in left_by_type:
        return right_type, "exact <Type> match"

    shared = []
    for idx, (candidate_type, _fields) in enumerate(right_data):
        if candidate_type in left_by_type:
            left_idx = [i for i, (left_type, _left_fields) in enumerate(left_data) if left_type == candidate_type][0]
            shared.append((idx, left_idx, candidate_type))
    if not shared:
        if right_idx < len(left_data):
            return left_data[right_idx][0], "fallback same XML index; no shared <Type> anchors"
        return "", "no comparable entry"

    before = [item for item in shared if item[0] < right_idx]
    after = [item for item in shared if item[0] > right_idx]
    if not before:
        return after[0][2], "closest inferred boundary; target-only entry before first shared <Type>"
    if not after:
        return before[-1][2], "closest inferred boundary; target-only entry after last shared <Type>"

    prev_item = before[-1]
    next_item = after[0]
    prev_distance = right_idx - prev_item[0]
    next_distance = next_item[0] - right_idx
    if prev_distance <= next_distance:
        return prev_item[2], "closest inferred neighbor; target-only entry between shared <Type> anchors"
    return next_item[2], "closest inferred neighbor; target-only entry between shared <Type> anchors"

def collect_entry_pairs(left_data, right_data):
    rows = []
    left_by_type = dict_by_type(left_data)
    right_types = set(h_type for h_type, _fields in right_data)
    paired_left_types = set()
    for idx, (right_type, right_fields) in enumerate(right_data):
        left_type, match_note = infer_left_type_for_right_index(idx, left_data, right_data)
        left_fields = left_by_type.get(left_type, {})
        if left_type:
            paired_left_types.add(left_type)
        rows.append(("#%02d" % idx, left_type, left_fields.get("/Description", ""), right_type, right_fields.get("/Description", ""), match_note))
    for left_idx, (left_type, left_fields) in enumerate(left_data):
        if left_type not in right_types and left_type not in paired_left_types:
            rows.append(("left-only#%02d" % left_idx, left_type, left_fields.get("/Description", ""), "", "", "left-only entry; no target/right equivalent"))
    return rows

def fields_by_type(data):
    return dict((h_type, fields) for h_type, fields in data)

def collect_diffs(entry_pairs, left_data, right_data, include_unchanged):
    rows = []
    left_fields_by_type = fields_by_type(left_data)
    right_fields_by_type = fields_by_type(right_data)
    for handicap, left_type, _left_description, right_type, _right_description, _match_note in entry_pairs:
        left_fields = left_fields_by_type.get(left_type, {})
        right_fields = right_fields_by_type.get(right_type, {})
        for field in sorted(set(left_fields.keys()) | set(right_fields.keys())):
            left = left_fields.get(field, "")
            right = right_fields.get(field, "")
            if include_unchanged or left != right:
                rows.append((handicap, field.lstrip("/"), left, right, numeric_delta(left, right), numeric_delta_percent(left, right)))
    return rows

def collect_diff_stats(entry_pairs, left_data, right_data):
    stats = {}
    total_changed = 0
    total_comparable = 0
    left_fields_by_type = fields_by_type(left_data)
    right_fields_by_type = fields_by_type(right_data)
    for handicap, left_type, _left_description, right_type, _right_description, _match_note in entry_pairs:
        left_fields = left_fields_by_type.get(left_type, {})
        right_fields = right_fields_by_type.get(right_type, {})
        field_count = 0
        changed_count = 0
        for field in set(left_fields.keys()) | set(right_fields.keys()):
            field_count += 1
            if left_fields.get(field, "") != right_fields.get(field, ""):
                changed_count += 1
        stats[handicap] = (changed_count, field_count)
        total_changed += changed_count
        total_comparable += field_count
    return stats, total_changed, total_comparable

def percentage_text(changed_count, total_count):
    if total_count == 0:
        return "0%"
    return "%.1f%%" % (changed_count * 100.0 / total_count)

def default_output_path(run_time):
    return os.path.join(DEFAULT_OUTPUT_DIR, "handicap_compare_%s.md" % run_time.strftime("%Y%m%dT%H%M%SZ"))

def example_output_path():
    return os.path.join(DEFAULT_EXAMPLE_DIR, "handicap_infos_compared.md")

def build_tsv_text(left_label, right_label, entry_pairs, rows, include_unchanged, matrix_rows=None):
    handicaps = [entry[0] for entry in entry_pairs]
    fields = sorted(set(row[1] for row in rows if include_unchanged or row[2] != row[3]))
    field_set = set(fields)
    if matrix_rows is None:
        matrix_rows = rows
    cell_data = {}
    for handicap, field, left, right, delta, delta_pct in matrix_rows:
        if field in field_set:
            cell_data[(field, handicap)] = (left, right, delta, delta_pct)
    out = io.StringIO()
    writer = csv.writer(out, dialect="excel-tab")
    header = ["Field"]
    for handicap in handicaps:
        header.extend(["%s %s" % (handicap, left_label), "%s %s" % (handicap, right_label), "%s Delta" % handicap, "%s Delta %%" % handicap])
    writer.writerow(header)
    for field in fields:
        row = [field]
        for handicap in handicaps:
            left, right, delta, delta_pct = cell_data.get((field, handicap), ("", "", "", ""))
            row.extend([left, right, delta, delta_pct])
        writer.writerow(row)
    return out.getvalue()

def write_markdown(path, left_path, right_path, left_label, right_label, entry_pairs, rows, include_unchanged, run_time, tsv_text, diff_stats, total_changed, total_comparable):
    out_dir = os.path.dirname(path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    left_full_path = os.path.abspath(left_path)
    right_full_path = os.path.abspath(right_path)
    output_full_path = os.path.abspath(path)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Handicap Info comparison: %s vs %s\n\n" % (left_label, right_label))
        f.write("Generated by `LLM_Helpers/compare_handicap_infos.py`.\n\n")
        f.write("- Run time: `%s UTC`\n" % run_time.strftime("%Y-%m-%d %H:%M:%S"))
        f.write("- Output path: `%s`\n" % output_full_path.replace("\\", "/"))
        f.write("- %s path: `%s`\n" % (left_label, left_full_path.replace("\\", "/")))
        f.write("- %s path: `%s`\n" % (right_label, right_full_path.replace("\\", "/")))
        f.write("- Rows shown: %s\n" % ("all comparable fields" if include_unchanged else "changed fields only"))
        f.write("- Match behavior: target/right XML order; exact `<Type>` matches when available; target-only entries compare to the nearest shared boundary dynamically; left-only entries are listed at the end.\n")
        f.write("- Missing fields on either side are shown as blank cells\n")
        f.write("- The spreadsheet matrix fills unchanged cells for any shown field, so each row can show the full handicap curve\n")
        f.write("- Delta %% is computed as (%s - %s) / %s; blank when either value is non-numeric or %s is 0\n" % (right_label, left_label, left_label, left_label))
        f.write("- Changed fields: %d/%d (%s)\n\n" % (total_changed, total_comparable, percentage_text(total_changed, total_comparable)))
        f.write("## Compared Handicap Entries\n\n")
        f.write("| Row | %s Type | %s Description | %s Type | %s Description | Match note |\n" % (left_label, left_label, right_label, right_label))
        f.write("| --- | --- | --- | --- | --- | --- |\n")
        for index, left_type, left_description, right_type, right_description, match_note in entry_pairs:
            f.write("| `%s` | `%s` | `%s` | `%s` | `%s` | %s |\n" % (md_cell(index), md_cell(left_type), md_cell(left_description), md_cell(right_type), md_cell(right_description), md_cell(match_note)))
        f.write("\n")
        f.write("## Changes by Handicap\n\n")
        for handicap in sorted(diff_stats.keys()):
            changed_count, total_count = diff_stats[handicap]
            if changed_count:
                f.write("- `%s`: %d/%d changed fields (%s)\n" % (handicap, changed_count, total_count, percentage_text(changed_count, total_count)))
        f.write("\n")
        f.write("## Changed Fields\n\n")
        f.write("| Handicap | Field | %s | %s | Delta | Delta %% |\n" % (left_label, right_label))
        f.write("| --- | --- | ---: | ---: | ---: | ---: |\n")
        for handicap, field, left, right, delta, delta_pct in rows:
            if not include_unchanged and left == right:
                continue
            f.write("| `%s` | `%s` | `%s` | `%s` | `%s` | `%s` |\n" % (md_cell(handicap), md_cell(field), md_cell(left), md_cell(right), md_cell(delta), md_cell(delta_pct)))
        f.write("\n")
        f.write("## Spreadsheet Matrix TSV\n\n")
        f.write("Copy this tab-separated block into Excel, LibreOffice, or another spreadsheet tool. It uses one row per field and grouped %s/%s/delta columns for each compared row.\n\n" % (left_label, right_label))
        f.write("```tsv\n")
        f.write(tsv_text)
        if not tsv_text.endswith("\n"):
            f.write("\n")
        f.write("```\n")

def write_tsv(path, tsv_text):
    out_dir = os.path.dirname(path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(tsv_text)

def main():
    parser = argparse.ArgumentParser(description="Compare two CIV4HandicapInfo.xml files for LLM review.")
    parser.add_argument("left", help="Left CIV4HandicapInfo.xml path")
    parser.add_argument("right", help="Right CIV4HandicapInfo.xml path")
    # <!-- custom: Keep generic File 1/File 2 defaults for arbitrary XML comparisons, while allowing clearer labels for published examples. Credit: GPT-5.5-Thinking review. (GPT-5.5) -->
    parser.add_argument("--left-label", "--file1-label", dest="left_label", default=DEFAULT_LEFT_LABEL, help="Display label for the left/file 1 XML")
    parser.add_argument("--right-label", "--file2-label", dest="right_label", default=DEFAULT_RIGHT_LABEL, help="Display label for the right/file 2 XML")
    parser.add_argument("--output", help="Markdown report output path")
    parser.add_argument("--example-output", action="store_true", help="Write to the stable example Markdown path instead of a timestamped default path")
    parser.add_argument("--tsv-output", help="Optional separate spreadsheet-friendly TSV matrix output path")
    parser.add_argument("--include-unchanged", action="store_true", help="Include unchanged fields too")
    args = parser.parse_args()

    run_time = datetime.datetime.now(datetime.timezone.utc)
    output_path = args.output or (example_output_path() if args.example_output else default_output_path(run_time))
    left_data = parse_handicaps(args.left)
    right_data = parse_handicaps(args.right)
    entry_pairs = collect_entry_pairs(left_data, right_data)
    rows = collect_diffs(entry_pairs, left_data, right_data, args.include_unchanged)
    diff_stats, total_changed, total_comparable = collect_diff_stats(entry_pairs, left_data, right_data)
    matrix_rows = rows if args.include_unchanged else collect_diffs(entry_pairs, left_data, right_data, True)
    tsv_text = build_tsv_text(args.left_label, args.right_label, entry_pairs, rows, args.include_unchanged, matrix_rows)
    write_markdown(output_path, args.left, args.right, args.left_label, args.right_label, entry_pairs, rows, args.include_unchanged, run_time, tsv_text, diff_stats, total_changed, total_comparable)
    print("Wrote %d changed field rows to %s" % (len(rows), output_path))
    if args.tsv_output:
        write_tsv(args.tsv_output, tsv_text)
        print("Wrote spreadsheet matrix to %s" % args.tsv_output)

if __name__ == "__main__":
    main()
