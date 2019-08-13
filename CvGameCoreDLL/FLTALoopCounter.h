#pragma once

#ifndef CIV4_FLTA_LOOP_COUNTER_H
#define CIV4_FLTA_LOOP_COUNTER_H

/*  advc.003s: Helper macros for iterating over FFreeListTrashArray.
	Don't want to add these to FFreeListTrashArray.h though b/c that header
	gets included everywhere and I don't want the macros in the global namespace. */
#define CONCATVARNAME_IMPL(prefix, lineNum) prefix##lineNum
// This second layer of indirection is necessary
#define CONCATVARNAME(prefix, lineNum) CONCATVARNAME_IMPL(prefix, lineNum)
// ('iLoopCounter_##__LINE__' won't work)
#define LOOPCOUNTERNAME CONCATVARNAME(iLoopCounter_, __LINE__)

#endif
