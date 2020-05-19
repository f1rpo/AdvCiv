// advc.plotr: New file; see comment in header.

#include "CvGameCoreDLL.h"
#include "PlotRadiusIterator.h"
#include "CvUnit.h"

// Not in the header b/c CvUnit.h isn't included there
template<bool bIN_CIRCLE>
CvPlot* SquareIterator<bIN_CIRCLE>::getUnitPlot(CvUnit const& kUnit) const
{
	return kUnit.plot();
}

// Explicit instantiations (for linker)
template class SquareIterator<true>;
template class SquareIterator<false>;
