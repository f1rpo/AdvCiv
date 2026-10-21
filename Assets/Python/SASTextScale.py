# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md).

from CvPythonExtensions import *
import re
# <!-- custom: Use the leaf logging module directly for structured font-size diagnostics; importing CvUtil here would create CvUtil -> SASTextScale -> CvUtil. (ChatGPT-5.6-Sol) -->
import SASPythonLogging

gc = CyGlobalContext()

# <!-- custom: Keep SAS text-size defines/tags with the text-scaling helpers that consume them.
# They formerly lived in SASFontUtils and briefly in the broader UI-resource cache; neither icon/glyph nor art-resource code should be imported merely to format text. (ChatGPT-5.6-Sol) -->
_SAS_UI_FONT_SIZE_CACHE = {}

def _getSASUIFontSize(szDefineName):
	if szDefineName in _SAS_UI_FONT_SIZE_CACHE:
		return _SAS_UI_FONT_SIZE_CACHE[szDefineName]
	iSize = gc.getDefineINT(szDefineName)
	if iSize < 1 or iSize > 4:
		_SAS_UI_FONT_SIZE_CACHE[szDefineName] = None
		raise RuntimeError("SASTextScale: %s=%d out of [1,4] - check GlobalDefines_advciv_sas.xml" % (szDefineName, iSize))
	_SAS_UI_FONT_SIZE_CACHE[szDefineName] = iSize
	# <!-- custom: Font-size resolution is deterministic per define/run, so expose the define and resolved size as searchable fields instead of ad-hoc prose. (ChatGPT-5.6-Sol) -->
	SASPythonLogging.writeRecord("UI_FONT_SIZE_RESOLVED", "define=%s size=%d" % (szDefineName, iSize))
	return iSize


def getSASUIFontTiny():
	return _getSASUIFontSize("SAS_UI_FONT_TINY")


def getSASUIFontBody():
	return _getSASUIFontSize("SAS_UI_FONT_BODY")


def getSASUIFontLabel():
	return _getSASUIFontSize("SAS_UI_FONT_LABEL")


def getSASUIFontTitle():
	return _getSASUIFontSize("SAS_UI_FONT_TITLE")


def getSASUIFontHover():
	return _getSASUIFontSize("SAS_UI_FONT_HOVER")


class _DynamicFontTag:
	def __init__(self, szDefineName, bBold=False):
		self.szDefineName = szDefineName
		self._isBold = bBold
		self._cachedTag = None
		if not bBold:
			self.bold = _DynamicFontTag(szDefineName, True)

	def _build(self):
		if self._cachedTag is not None:
			return self._cachedTag
		iSize = _getSASUIFontSize(self.szDefineName)
		if iSize is None:
			self._cachedTag = u""
		elif self._isBold:
			self._cachedTag = u"<font=%db>" % iSize
		else:
			self._cachedTag = u"<font=%d>" % iSize
		return self._cachedTag

	def __unicode__(self):
		return self._build()

	def __str__(self):
		return self._build().encode("ascii")

	def __add__(self, szOther):
		return self._build() + szOther

	def __radd__(self, szOther):
		return szOther + self._build()


SAS_FONT_TAG_CLOSE = u"</font>"

sasFontTagTiny = _DynamicFontTag("SAS_UI_FONT_TINY")
sasFontTagBody = _DynamicFontTag("SAS_UI_FONT_BODY")
sasFontTagLabel = _DynamicFontTag("SAS_UI_FONT_LABEL")
sasFontTagTitle = _DynamicFontTag("SAS_UI_FONT_TITLE")
sasFontTagHover = _DynamicFontTag("SAS_UI_FONT_HOVER")

SAS_FONT_OPEN_RE = re.compile(u"<font=[^>]*>", re.IGNORECASE)
SAS_FONT_CLOSE_RE = re.compile(u"</font>", re.IGNORECASE)

def applyFontTag(szText, szTag):
	if szText is None:
		return szText
	if szText == "":
		return szText
	try:
		szUnicode = unicode(szText)
	except:
		return szText
	# <!-- custom: usage rule: always try bodyText/labelText first (simple path).
	# If text already contains <font=...>, leave it unchanged here to avoid double-wrapping.
	# Escalate to normalize* only after reproducing a real failure with simple wrapping in that exact caller/data path.
	# Note: this guidance was established while working on Sevopedia paths.
	# Advisors/other UI also use this helper, but they were not necessarily migrated under the same strict "escalate to normalize* only after failure" rule.
	# Current empirical examples: ConceptInfo/NewConceptInfo and Sevopedia Leader Civilopedia text need normalize; hints, Unit/History civilopedia text, and Traits (Effects/Background) often work with simple bodyText/labelText. (GPT-5.3-Codex) -->
	# Extra example: Sevopedia Unit history panel ("Background") for concept-based entries (e.g. Great People units) needed normalizeLabelText; simple wrapping did not apply SAS scaling there.
	# Same issue/pattern applies to Sevopedia Vote history when vote Civilopedia reuses concept pages (UN/AP). (GPT-5.3-Codex) -->
	if szUnicode.find(u"<font=") != -1:
		return szUnicode
	return szTag + szUnicode + SAS_FONT_TAG_CLOSE

def stripFontTags(szText):
	if szText is None:
		return szText
	if szText == "":
		return szText
	try:
		szUnicode = unicode(szText)
	except:
		return szText
	szUnicode = SAS_FONT_OPEN_RE.sub(u"", szUnicode)
	szUnicode = SAS_FONT_CLOSE_RE.sub(u"", szUnicode)
	return szUnicode

def tinyText(szText):
	return applyFontTag(szText, sasFontTagTiny)

def bodyText(szText):
	return applyFontTag(szText, sasFontTagBody)

def labelText(szText):
	return applyFontTag(szText, sasFontTagLabel)

def imageText(szButton, iSize, szText=None):
	# <!-- custom: Central inline-image formatter for text/table labels.
	# Callers still own presentation choices such as size, surrounding indentation/gaps, and selected-state resource choice.
	# This originally targeted sortable advisor headers (Long_Comments_py.txt #17) and now also removes repeated equivalent <img> formatting elsewhere. (GPT-5.5 + ChatGPT-5.6-Sol) -->
	szImage = u"<img=%s size=%d></img>" % (unicode(szButton), iSize)
	if szText is None or szText == "":
		return szImage
	return szImage + u" " + unicode(szText)

def labelImageText(szButton, iSize, szText=None):
	return labelText(imageText(szButton, iSize, szText))

def titleText(szText):
	return applyFontTag(szText, sasFontTagTitle)

def normalizeBodyText(szText):
	# <!-- custom: fallback rule: normalize* is not default.
	# Use only for proven problematic sources where simple bodyText/labelText fails because embedded <font=...> blocks SAS upscaling. (GPT-5.3-Codex) -->
	return applyFontTag(stripFontTags(szText), sasFontTagBody)

def normalizeLabelText(szText):
	return applyFontTag(stripFontTags(szText), sasFontTagLabel)

def setTableTextScaled(screen, szTable, iCol, iRow, szText, szIcon, eWidgetType, iData1, iData2, eJustify, szTag):
	screen.setTableText(szTable, iCol, iRow, applyFontTag(szText, szTag), szIcon, eWidgetType, iData1, iData2, eJustify)

def setTableIntScaled(screen, szTable, iCol, iRow, szText, szIcon, eWidgetType, iData1, iData2, eJustify, szTag):
	screen.setTableInt(szTable, iCol, iRow, applyFontTag(szText, szTag), szIcon, eWidgetType, iData1, iData2, eJustify)

def setTableDateScaled(screen, szTable, iCol, iRow, szText, szIcon, eWidgetType, iData1, iData2, eJustify, szTag):
	screen.setTableDate(szTable, iCol, iRow, applyFontTag(szText, szTag), szIcon, eWidgetType, iData1, iData2, eJustify)

def setTableColumnHeaderScaled(screen, szTable, iCol, szText, iWidth, szTag):
	screen.setTableColumnHeader(szTable, iCol, applyFontTag(szText, szTag), iWidth)

def appendListBoxStringScaled(screen, szWidgetName, szText, eWidgetType, iData1, iData2, eJustify, szTag):
	screen.appendListBoxString(szWidgetName, applyFontTag(szText, szTag), eWidgetType, iData1, iData2, eJustify)

def appendListBoxStringNoUpdateScaled(screen, szWidgetName, szText, eWidgetType, iData1, iData2, eJustify, szTag):
	screen.appendListBoxStringNoUpdate(szWidgetName, applyFontTag(szText, szTag), eWidgetType, iData1, iData2, eJustify)

def setTableTextLabel(screen, szTable, iCol, iRow, szText, szIcon, eWidgetType, iData1, iData2, eJustify):
	setTableTextScaled(screen, szTable, iCol, iRow, szText, szIcon, eWidgetType, iData1, iData2, eJustify, sasFontTagLabel)

def setTableIntLabel(screen, szTable, iCol, iRow, szText, szIcon, eWidgetType, iData1, iData2, eJustify):
	# <!-- custom: use this for sortable numeric table cells, even when display strings include signs or color tags (e.g. +8, -2, raw signed years like -4000).
	# Do not use it for formatted date text such as 4000 BC / 800 AD; those are intentionally textual.
	# Civ4 table sorting treats setTableInt cells numerically, while setTableText sorts lexically. (GPT-5.5) -->
	setTableIntScaled(screen, szTable, iCol, iRow, szText, szIcon, eWidgetType, iData1, iData2, eJustify, sasFontTagLabel)

def setTableDateLabel(screen, szTable, iCol, iRow, szText, szIcon, eWidgetType, iData1, iData2, eJustify):
	# <!-- custom: preserve Civ4's date-typed table sorting while applying the shared label font.
	# Generic setTableText made formatted BC/AD dates sort lexically. See KI#229. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	setTableDateScaled(screen, szTable, iCol, iRow, szText, szIcon, eWidgetType, iData1, iData2, eJustify, sasFontTagLabel)

def setTableColumnHeaderLabel(screen, szTable, iCol, szText, iWidth):
	setTableColumnHeaderScaled(screen, szTable, iCol, szText, iWidth, sasFontTagLabel)

def appendListBoxStringLabel(screen, szWidgetName, szText, eWidgetType, iData1, iData2, eJustify):
	appendListBoxStringScaled(screen, szWidgetName, szText, eWidgetType, iData1, iData2, eJustify, sasFontTagLabel)

def appendListBoxStringNoUpdateLabel(screen, szWidgetName, szText, eWidgetType, iData1, iData2, eJustify):
	appendListBoxStringNoUpdateScaled(screen, szWidgetName, szText, eWidgetType, iData1, iData2, eJustify, sasFontTagLabel)
