# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md).

# <!-- custom: sys supplies the existing engine-owned output streams; the DLL clock supplies fresh UTC because both embedded Python wall-clock APIs stayed fixed in a measured 15-second trial.
# Keep this module independent of BUG/CvUtil/UI imports so it can wrap streams before their startup output. (GPT-6.1-Sol) -->
import sys
from CvPythonExtensions import getSASDiagnosticUtcTimestamp

_timestampEnabled = True

def setTimestampEnabled(enabled):
	# <!-- custom: Reuse BUG's timestamp preference for the shared stream layer instead of querying options on every write. (GPT-6.1-Sol) -->
	global _timestampEnabled
	_timestampEnabled = enabled

class NativeSink:
	# <!-- custom: Preserve the inherited engine's separate byte/unicode destinations while adapting its callables to the stream wrapper; no new files or output destinations are introduced. (GPT-6.1-Sol) -->
	def __init__(self, narrow, wide):
		self._narrow = narrow
		self._wide = wide

	def write(self, text):
		if isinstance(text, unicode):
			self._wide(text)
		else:
			self._narrow(text)

class TimestampedStream:
	# <!-- custom: A shared stdout/stderr boundary covers direct prints and traceback continuation rows that individual BUG call-site changes missed.
	# Python print may split one row across several writes: retain only line-start state and submit each write immediately, rather than buffering the last crash evidence until a newline.
	# This observes Python writes after installation; early EXE output and native writes bypassing these streams remain outside its coverage. (GPT-6.1-Sol) -->
	_sasTimestamped = True

	def __init__(self, stream):
		self._stream = stream
		self._lineStart = True
		self.softspace = getattr(stream, "softspace", 0)

	def write(self, text):
		if not text:
			return
		parts = text.split("\n")
		output = []
		for index, part in enumerate(parts):
			if part:
				if self._lineStart and _timestampEnabled:
					stamp = getSASDiagnosticUtcTimestamp()
					output.append(stamp[:4] + "-" + stamp[4:6] + "-" + stamp[6:11] + ":" + stamp[11:13] + ":" + stamp[13:] + " ")
				output.append(part)
				self._lineStart = False
			if index < len(parts) - 1:
				output.append("\n")
				self._lineStart = True
		# <!-- custom: Forward one combined write to avoid creating extra engine-log rows for the prefix itself; retain blank rows and the input's byte/unicode type. (GPT-6.1-Sol) -->
		separator = ""
		if isinstance(text, unicode):
			separator = u""
		return self._stream.write(separator.join(output))

	def writeSASRaw(self, text):
		# <!-- custom: Generated Python cache modules must remain copyable, so their serializer explicitly bypasses prefixes without disabling timestamps globally. (GPT-6.1-Sol) -->
		if text:
			self._lineStart = text.endswith("\n")
		return self._stream.write(text)

	def flush(self):
		# <!-- custom: The inherited native sinks have no flush method; forward it when the original stream does, without adding a line buffer here. (GPT-6.1-Sol) -->
		flush = getattr(self._stream, "flush", None)
		if flush is not None:
			return flush()

	def __getattr__(self, name):
		return getattr(self._stream, name)

def install():
	# <!-- custom: Preserve existing destinations and tolerate repeated imports/installation; CvUtil's native redirect classes also carry the marker so later EXE replacement does not lose timestamping. (GPT-6.1-Sol) -->
	if not getattr(sys.stdout, "_sasTimestamped", False):
		sys.stdout = TimestampedStream(sys.stdout)
	if not getattr(sys.stderr, "_sasTimestamped", False):
		sys.stderr = TimestampedStream(sys.stderr)

def writeRaw(text):
	# <!-- custom: Keep the explicit raw-cache path usable even before stream installation, including external serializer callers. (GPT-6.1-Sol) -->
	writer = getattr(sys.stdout, "writeSASRaw", sys.stdout.write)
	return writer(text)

# <!-- custom: Standardize machine-searchable Python diagnostics at the shared stream boundary without changing timestamp enablement or destinations.
# szRecord is the stable grep key; szFields is an optional already-formatted key=value payload and may be byte or Unicode text.
# Submit the prefix, payload and terminating newline together; TimestampedStream remains solely responsible for UTC.
# Preserve byte payloads instead of interpolating them into a Unicode template: Python 2.4 implicitly decodes those bytes as ASCII and raises on accented filenames/text.
# Match the prefix type to the payload so the existing native byte/Unicode destinations remain in control of text encoding. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
def writeRecord(szRecord, szFields=""):
	szPrefix = "record=%s" % szRecord
	if isinstance(szFields, unicode):
		szPrefix = unicode(szPrefix)
	elif isinstance(szPrefix, unicode):
		szPrefix = szPrefix.encode("utf-8")
	if szFields:
		sys.stdout.write(szPrefix + " " + szFields + "\n")
	else:
		sys.stdout.write(szPrefix + "\n")

# <!-- custom: Install as this dedicated stream module finishes importing, before CvUtil's next dependency can emit output.
# This keeps callers' imports together without postponing the early interception; install() is idempotent for reused streams. (GPT-6.1-Sol) -->
install()
