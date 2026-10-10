# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md).
#
# <!-- custom: Purpose: Scales CITYBILLBOARD_SCALE values in CIV4DetailManager.xml based on the active SAS_UI_FONT_BODY level.
# The multiplier percent for each font level (1..4) is read from SAS_BILLBOARD_SCALE_PERCENT_FONT_<N> defines.
# The XML file is rewritten on game load so the engine picks up the new values on the fly (the engine re-reads DetailManager without restart). (Claude code Opus 4.6) -->

import os
import re

from CvPythonExtensions import CyGlobalContext

import BugPath
import BugUtil
from SASFontUtils import getSASUIFontBody

gc = CyGlobalContext()

# Base AdvCiv CITYBILLBOARD_SCALE values (font-body 2 / 100%).
# Each tuple is (camera_distance, scale_value).
_BASE_BILLBOARD_KEYS = [(500, 1.72), (2500, 1.18), (3800, 1.06), (6000, 0.81), (8500, 0.75), (11500, 0.70), (13500, 0.67)]

_DETAIL_MANAGER_REL_PATH = os.path.join("Assets", "XML", "Misc", "CIV4DetailManager.xml")

def _getMultiplierPercent(iFontBody):
	# read SAS_BILLBOARD_SCALE_PERCENT_FONT_<N> for the given font level.
	if iFontBody < 1 or iFontBody > 4:
		return 100
	szDefine = "SAS_BILLBOARD_SCALE_PERCENT_FONT_%d" % iFontBody
	iPercent = gc.getDefineINT(szDefine)
	if iPercent < 10 or iPercent > 500:
		BugUtil.warn("SASBillboardScale: %s=%d out of range [10..500], using 100", szDefine, iPercent)
		return 100
	return iPercent

def _buildScaledKeys(iPercent):
	# return formatted CITYBILLBOARD_SCALE <Key> lines using the given percent multiplier.
	fMult = iPercent / 100.0
	lines = []
	for (iDist, fBase) in _BASE_BILLBOARD_KEYS:
		fScaled = fBase * fMult
		lines.append("\t\t<Key>%d,    %.2f</Key>" % (iDist, fScaled))
	return lines

def _rewriteDetailManagerXML(szPath, scaledKeyLines):
	# replace CITYBILLBOARD_SCALE key lines in CIV4DetailManager.xml; skip file write when unchanged.
	try:
		f = open(szPath, "r")
		content = f.read()
		f.close()
	except IOError:
		BugUtil.error("SASBillboardScale: cannot read %s", szPath)
		return False

	# <!-- custom: Restoring archived comments beside active XML values put comments between this Name and its Keys; the old immediate-Key match failed and disabled runtime billboard scaling.
	# Match and preserve intervening XML comments. (GPT-5.6-Sol) -->
	pattern = r"(<Name>CITYBILLBOARD_SCALE</Name>\s*\n(?:\s*<!--[\s\S]*?-->\s*\n)*)((?:\s*<Key>[^<]*</Key>\s*\n)+)"
	replacement = r"\1" + "\n".join(scaledKeyLines) + "\n"

	match = re.search(pattern, content)
	if match is None:
		BugUtil.error("SASBillboardScale: CITYBILLBOARD_SCALE block not found in %s", szPath)
		return False

	currentKeys = [line.strip() for line in match.group(2).splitlines() if line.strip()]
	targetKeys = [line.strip() for line in scaledKeyLines]
	# If already in the desired state, do nothing to avoid needless disk churn.
	if currentKeys == targetKeys:
		BugUtil.info("SASBillboardScale: CITYBILLBOARD_SCALE already up to date in %s", szPath)
		return True

	newContent, count = re.subn(pattern, replacement, content)
	if count == 0:
		BugUtil.error("SASBillboardScale: CITYBILLBOARD_SCALE block not found in %s", szPath)
		return False

	try:
		f = open(szPath, "w")
		f.write(newContent)
		f.close()
	except IOError:
		BugUtil.error("SASBillboardScale: cannot write %s", szPath)
		return False

	return True

def apply():
	# apply define-driven CITYBILLBOARD_SCALE keys for the active SAS_UI_FONT_BODY.
	iFontBody = getSASUIFontBody()
	iPercent = _getMultiplierPercent(iFontBody)

	szModDir = BugPath.getModDir()
	if not szModDir:
		BugUtil.error("SASBillboardScale: mod dir not found")
		return

	szPath = os.path.join(szModDir, _DETAIL_MANAGER_REL_PATH)
	if not os.path.isfile(szPath):
		BugUtil.error("SASBillboardScale: file not found: %s", szPath)
		return

	scaledKeyLines = _buildScaledKeys(iPercent)

	if _rewriteDetailManagerXML(szPath, scaledKeyLines):
		BugUtil.info("SASBillboardScale: applied %d%% scale (font body %d) to %s", iPercent, iFontBody, szPath)
