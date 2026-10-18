# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: Share numbered-entry parsing and explanation presence between recorder histories; their different revision/provenance policies remain in their own checkers.
# Regex locates Markdown fields/headings; fenced examples and HTML comments cannot substitute for a real history entry or explanatory text. (GPT-6.1-Sol) -->
import re


def revision_entries(text):
    visible = []
    fence = None
    for line in text.splitlines():
        marker = re.match(r"^ {0,3}(`{3,}|~{3,})", line)
        if fence is not None:
            if re.fullmatch(r" {0,3}" + re.escape(fence[0]) + "{" + str(len(fence)) + r",}[ \t]*", line):
                fence = None
            continue
        if marker:
            fence = marker.group(1)
            continue
        visible.append(line)
    text = re.sub(r"<!--.*?-->", "", "\n".join(visible), flags=re.DOTALL)
    headings = list(re.finditer(r"^#{1,3}[ \t]+[^\n]+$", text, re.MULTILINE))
    result = []
    for index, heading in enumerate(headings):
        number = re.match(r"### Revision (\d+)\b", heading.group())
        if number:
            end = headings[index + 1].start() if index + 1 < len(headings) else len(text)
            result.append((int(number.group(1)), text[heading.end():end]))
    return result


# <!-- custom: A matching revision heading with only Date/Git commit fields previously passed.
# Require actual Change text, allowing continued paragraphs/sub-bullets without minimum length or subjective writing rules; an optional revision selects the entry belonging to one committed bump. (GPT-6.1-Sol) -->
def change_errors(text, revision=None):
    entries = revision_entries(text)
    if revision is not None:
        entries = [(number, body) for number, body in entries if number == revision]
        if not entries:
            return [f"revision {revision}: missing corresponding history entry"]
    errors = []
    for number, body in entries:
        change = re.search(r"^- \*\*Change:\*\*[ \t]*(.*?)(?=^- \*\*[^*\n]+:\*\*|\Z)", body, re.MULTILINE | re.DOTALL)
        if change is None or not change.group(1).strip(" \t\r\n-*+_`"):
            errors.append(f"revision {number}: missing nonempty Change explanation")
    return errors
