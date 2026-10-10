# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md).

from SASFontUtils import sasFontTagBody, SAS_FONT_TAG_CLOSE, sasFontTagLabel, sasFontTagTitle, sasFontTagTiny
import re

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
	# <!-- custom: table headers use inline <img> text instead of separate hoverable/clickable image widgets because separate header icons do not easily sort with the table column.
	# Centralizing this keeps advisor icon headers consistent. Long_Comments_py.txt #17. (GPT-5.5) -->
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
