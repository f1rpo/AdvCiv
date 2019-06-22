#include "CvGameCoreDLL.h"
#include "CyCity.h"

// dlph.34/advc: Added in order to reduce the size of CyCityInterface1.cpp

void CyCityPythonInterface2(python::class_<CyCity>& x)
{
	OutputDebugString("Python Extension Module - CyCityPythonInterface2\n");

	x
		// advc.001n: (Moved here from CyCityInterface1.cpp as an example)
		.def("AI_neededFloatingDefenders", &CyCity::AI_neededFloatingDefenders, "int ()")
		;
}
