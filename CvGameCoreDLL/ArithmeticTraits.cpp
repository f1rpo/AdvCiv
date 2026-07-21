#include "CvGameCoreDLL.h"
#include "ArithmeticTraits.h"

// advc.enum: See comments in header file

/*	NB: The _MIN literals in float.h represent the positive values closest to 0;
	not what we want here. */
float const arithm_traits<float>::min = -FLT_MAX;
float const arithm_traits<float>::max = FLT_MAX;
double const arithm_traits<double>::min = -DBL_MAX;
double const arithm_traits<double>::max = DBL_MAX;
