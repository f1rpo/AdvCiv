#!/usr/bin/env python
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)
#
# Build check: compile Civ4 runtime Python with real CPython 2.4 syntax rules.

import optparse
import os
import py_compile
import re
import shutil
import sys
import tempfile


SCAN_RELATIVE_DIRS = (
	"Assets/Python",
	"PrivateMaps",
)

IGNORED_DIR_NAMES = (
	".git",
	"__pycache__",
)


class CompileFailure:
	def __init__(self, relative_path, line_number, message):
		self.relative_path = relative_path
		self.line_number = line_number
		self.message = message

	def format(self):
		if self.line_number:
			return "%s: line %s: %s" % (self.relative_path, self.line_number, self.message)
		return "%s: %s" % (self.relative_path, self.message)


def write_line(text):
	sys.stdout.write(text)
	sys.stdout.write("\n")


def normalize_slashes(path):
	return path.replace(os.sep, "/")


def relative_to_repo(repo_root, path):
	absolute_root = os.path.abspath(repo_root)
	absolute_path = os.path.abspath(path)
	prefix = absolute_root + os.sep
	if absolute_path.startswith(prefix):
		return normalize_slashes(absolute_path[len(prefix):])
	return normalize_slashes(path)


def is_ignored_dir(dir_name):
	for ignored_name in IGNORED_DIR_NAMES:
		if dir_name == ignored_name:
			return 1
	return 0


def iter_python_files(repo_root):
	paths = []
	for relative_dir in SCAN_RELATIVE_DIRS:
		root = os.path.join(repo_root, relative_dir)
		if not os.path.isdir(root):
			continue
		for dir_path, dir_names, file_names in os.walk(root):
			kept_dir_names = []
			for dir_name in dir_names:
				if not is_ignored_dir(dir_name):
					kept_dir_names.append(dir_name)
			dir_names[:] = kept_dir_names

			for file_name in file_names:
				if file_name.endswith(".py"):
					paths.append(os.path.join(dir_path, file_name))
	paths.sort()
	return paths


def extract_line_number(message):
	match = re.search(r"line ([0-9]+)", message)
	if match:
		return match.group(1)
	return None


def compile_file(repo_root, path, bytecode_dir):
	relative_path = relative_to_repo(repo_root, path)
	cfile = os.path.join(bytecode_dir, relative_path.replace("/", "__").replace("\\", "__") + "c")

	try:
		# <!-- custom: Use py_compile on the actual file, but send bytecode to a temp dir so the Docker check catches real CPython 2.4 parser/bytecode errors without writing .pyc files into the mounted repository. (GPT-5.5) -->
		py_compile.compile(path, cfile, relative_path, True)
	except py_compile.PyCompileError:
		exc = sys.exc_info()[1]
		message = str(exc)
		return CompileFailure(relative_path, extract_line_number(message), message)
	except Exception:
		exc = sys.exc_info()[1]
		return CompileFailure(relative_path, None, "compile failed: %s" % exc)
	return None


def check_python24_compile(repo_root):
	failures = []
	paths = iter_python_files(repo_root)
	bytecode_dir = tempfile.mkdtemp(prefix="advciv-sas-python24-")
	try:
		for path in paths:
			failure = compile_file(repo_root, path, bytecode_dir)
			if failure is not None:
				failures.append(failure)
	finally:
		shutil.rmtree(bytecode_dir, 1)
	return paths, failures


def main():
	parser = optparse.OptionParser(description="Compile-check Civ4 runtime Python files with CPython 2.4.")
	parser.add_option("--repo-root", dest="repo_root", default=os.getcwd(), help="repository root; defaults to current directory")
	options, _args = parser.parse_args()

	repo_root = os.path.abspath(options.repo_root)
	paths, failures = check_python24_compile(repo_root)

	if failures:
		write_line("FAIL Python 2.4 compile compatibility")
		for failure in failures:
			write_line("  - " + failure.format())
		return 1

	write_line("PASS Python 2.4 compile compatibility: checked %s Python files" % len(paths))
	return 0


if __name__ == "__main__":
	sys.exit(main())
