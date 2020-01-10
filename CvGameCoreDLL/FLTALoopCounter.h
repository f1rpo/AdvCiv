#pragma once

#ifndef CIV4_FLTA_LOOP_COUNTER_H
#define CIV4_FLTA_LOOP_COUNTER_H

// advc.003s: Helper macros (primarily) for iterating over FFreeListTrashArray
#define CONCATVARNAME_IMPL(prefix, lineNum) prefix##lineNum
// This second layer of indirection is necessary
#define CONCATVARNAME(prefix, lineNum) CONCATVARNAME_IMPL(prefix, lineNum)
// ('iLoopCounter_##__LINE__' won't work)
#define LOOPCOUNTERNAME CONCATVARNAME(iLoopCounter_, __LINE__)

#endif
