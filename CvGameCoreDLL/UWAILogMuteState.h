// AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
// (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

#pragma once

#ifndef UWAI_LOG_MUTE_STATE_H
#define UWAI_LOG_MUTE_STATE_H

// <!-- custom: Shared nested mute depth for UWAI diagnostics.
// Unlike ordinary BBAI paths, UWAI recursively evaluates hypothetical alternatives inside one top-level decision; selected internal passes can therefore calculate normally while remaining diagnostically silent.
// Keeping those speculative alternatives silent prevents them from being mistaken for the selected scenario in the shared BBAI stream.
// This state is not a logger, formatter or cache, and it must not affect scoring, cache policy, RNG or gameplay.
// Output still goes directly through BBAI. See KI#505.3. (ChatGPT-5.6-Sol) -->
class UWAILogMuteState
{
public:
	explicit UWAILogMuteState(bool bMute = false) : m_iMuteDepth(bMute ? 1 : 0) {}
	bool isMuted() const { return (m_iMuteDepth > 0); }
	void pushMute() { m_iMuteDepth++; }
	void popMute()
	{
		FAssert(m_iMuteDepth > 0);
		if (m_iMuteDepth > 0)
			m_iMuteDepth--;
	}

private:
	int m_iMuteDepth;
};

#endif
