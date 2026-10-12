#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

from pathlib import Path
import argparse
import io
import sys
import xml.sax

# <!-- custom: Resolve repository/shared-helper paths from this subject subfolder after the workflow-tool reorganization. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))

from xml_defines import get_default_repo_root


TEXT_XML_RELATIVE_PATH = Path("Assets/XML/Text")


class XmlNode:
	def __init__(self, tag: str, line: int, parent: "XmlNode | None" = None) -> None:
		self.tag = tag
		self.line = line
		self.parent = parent
		self.children: list[XmlNode] = []
		self.text_parts: list[str] = []

	@property
	def text(self) -> str:
		return "".join(self.text_parts).strip()


class LineNumberXmlHandler(xml.sax.handler.ContentHandler):
	def __init__(self) -> None:
		super().__init__()
		self.locator = None
		self.root: XmlNode | None = None
		self.stack: list[XmlNode] = []

	def setDocumentLocator(self, locator) -> None:
		self.locator = locator

	def startElement(self, name, attrs) -> None:
		line = self.locator.getLineNumber() if self.locator is not None else 0
		parent = self.stack[-1] if self.stack else None
		node = XmlNode(name, line, parent)
		if parent is not None:
			parent.children.append(node)
		else:
			self.root = node
		self.stack.append(node)

	def characters(self, content) -> None:
		if self.stack:
			self.stack[-1].text_parts.append(content)

	def endElement(self, name) -> None:
		self.stack.pop()


def read_xml_text(path: Path) -> str:
	data = path.read_bytes()

	for encoding in ("utf-8-sig", "cp1252", "latin1"):
		try:
			return data.decode(encoding)
		except UnicodeDecodeError:
			continue

	return data.decode("latin1", errors="replace")


def parse_xml_with_line_numbers(path: Path) -> XmlNode:
	handler = LineNumberXmlHandler()
	parser = xml.sax.make_parser()
	parser.setFeature(xml.sax.handler.feature_namespaces, False)
	parser.setContentHandler(handler)

	parser.parse(io.StringIO(read_xml_text(path)))

	if handler.root is None:
		raise ValueError("XML file has no root element")

	return handler.root


def local_name(tag: str) -> str:
	return tag.rsplit("}", 1)[-1]


def direct_child_by_local_name(node: XmlNode, child_name: str) -> XmlNode | None:
	for child in node.children:
		if local_name(child.tag) == child_name:
			return child
	return None


def walk_nodes(root: XmlNode):
	yield root
	for child in root.children:
		for node in walk_nodes(child):
			yield node


def collect_text_tags(relative_path: Path, root: XmlNode) -> list[tuple[str, str, int]]:
	entries: list[tuple[str, str, int]] = []

	for node in walk_nodes(root):
		if local_name(node.tag) != "TEXT":
			continue

		tag_node = direct_child_by_local_name(node, "Tag")
		if tag_node is None or not tag_node.text:
			continue

		entries.append((tag_node.text, str(relative_path), tag_node.line))

	return entries


def check_xml_text_duplicate_tags(repo_root: Path) -> tuple[list[str], list[str], int]:
	errors: list[str] = []
	entries_by_tag: dict[str, list[tuple[str, int]]] = {}
	total_tags = 0

	text_dir = repo_root / TEXT_XML_RELATIVE_PATH
	if not text_dir.exists():
		return [f"{TEXT_XML_RELATIVE_PATH}: missing text XML folder"], [], 0

	for path in sorted(text_dir.rglob("*.xml")):
		if path.name.endswith("Schema.xml"):
			continue

		relative_path = path.relative_to(repo_root)

		try:
			root = parse_xml_with_line_numbers(path)
		except xml.sax.SAXParseException as exc:
			errors.append(f"{relative_path}: XML parse error at line {exc.getLineNumber()}, column {exc.getColumnNumber()}: {exc.getMessage()}")
			continue
		except Exception as exc:
			errors.append(f"{relative_path}: XML parse error: {exc}")
			continue

		for tag_value, entry_path, line in collect_text_tags(relative_path, root):
			total_tags += 1
			entries_by_tag.setdefault(tag_value, []).append((entry_path, line))

	failures: list[str] = []
	for tag_value, locations in sorted(entries_by_tag.items()):
		if len(locations) <= 1:
			continue

		location_text = "; ".join(f"{entry_path}: line {line}" for entry_path, line in locations)
		failures.append(f"duplicate <TEXT>/<Tag> {tag_value} ({len(locations)} entries): {location_text}")

	return errors, failures, total_tags


def print_section(title: str, lines: list[str]) -> None:
	print(title)
	if not lines:
		print("  - none")
		return

	for line in lines:
		print(f"  - {line}")


def main() -> int:
	parser = argparse.ArgumentParser(description="Check that GameText XML TXT_KEY tags are not defined more than once.")
	parser.add_argument("--repo-root", type=Path, default=get_default_repo_root(), help="repository root; defaults to the root containing .github/")
	args = parser.parse_args()

	errors, failures, total_tags = check_xml_text_duplicate_tags(args.repo_root)

	if errors:
		print_section("FAIL XML text duplicate tag setup/parse errors", errors)
		return 1

	if failures:
		print_section("FAIL XML text duplicate tags", failures)
		return 1

	print(f"PASS XML text duplicate tags: {total_tags} text tag(s) checked")
	return 0


if __name__ == "__main__":
	sys.exit(main())
