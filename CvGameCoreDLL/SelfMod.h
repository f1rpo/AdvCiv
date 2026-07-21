// advc.092b: Let's put all self-modifying code in a single place
#pragma once
#ifndef SELF_MOD_H
#define SELF_MOD_H

// Runtime modifications to the native code of Civ4BeyondSword.exe (v3.19)
class Civ4BeyondSwordPatches
{
public:
	void patchPlotIndicatorSize(); // (exposed to Python via CyGameCoreUtils) 
	bool isPlotIndicatorSizePatched() const { return m_bPlotIndicatorSizePatched; }
	// <advc.092c>
	void setHelpTextAreaSize(float fWidth); // (exposed to Python via CyGame)
	float getHelpTextAreaWidth() const { return m_fHelpTextAreaWidth; }
	bool isHelpTextAreaWidthPatched() const { return (getHelpTextAreaWidth() != 0); }
	// </advc.092c>
	Civ4BeyondSwordPatches()
	:	m_bPlotIndicatorSizePatched(false), m_fHelpTextAreaWidth(0)
	{}

private:
	bool m_bPlotIndicatorSizePatched;
	float m_fHelpTextAreaWidth; // advc.092c
	void showErrorMsgToPlayer(CvWString szMsg);
};

namespace smc
{
	extern Civ4BeyondSwordPatches BtS_EXE;
}

#endif
