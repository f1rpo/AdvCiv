#include "CvGameCoreDLL.h"
#include "CvGameCoreDLLUndefNew.h"
#include <new>
#include <psapi.h>
/*  advc.mak: This fixes a linker error related to psapi. One can probably also
	amend this through Visual Studio settings. I copied the line from here:
	https://stackoverflow.com/questions/3151728/unresolved-external-symbols-in-compiling-32-bit-application-in-windows-64 */
#pragma comment(lib, "psapi.lib")

#ifdef MEMORY_TRACKING
void	ProfileTrackAlloc(void* ptr);

void	ProfileTrackDeAlloc(void* ptr);
/*  advc.003j (comment): This isn't currently called. Also not called: DllMain,
	EnableDetailedTrace, ProfileTrackAlloc and ProfileTrackDeAlloc. */
void DumpMemUsage(const char* fn, int line)
{
	PROCESS_MEMORY_COUNTERS pmc;

	if ( GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof(pmc)) )
	{
		char buffer[200];

		sprintf(buffer, "memory (%s,%d): %d Kbytes, peak %d\n", fn, line, (int)(pmc.WorkingSetSize/1024), (int)(pmc.PeakWorkingSetSize/1024));
		OutputDebugString(buffer);
	}
}

#define	PROFILE_TRACK_ALLOC(x)		ProfileTrackAlloc(x);
#define	PROFILE_TRACK_DEALLOC(x)	ProfileTrackDeAlloc(x);
#else
#define	PROFILE_TRACK_ALLOC(x)
#define	PROFILE_TRACK_DEALLOC(x)
#endif

#ifdef USE_MEMMANAGER // K-Mod. There is a similar #ifdef in the header file, so I assume it's meant to be here as well...
//
// operator global new and delete override for gamecore DLL
//
void *__cdecl operator new(size_t size)
{
	if (gDLL)
	{
		void* result = NULL;

		try
		{
			result = gDLL->newMem(size, __FILE__, __LINE__);
			/*  advc.make: Don't obscure uninitialized memory by setting _Val=0.
				Adopted from C2C (billw2015). */
			memset(result, 0xDA, size);
			PROFILE_TRACK_ALLOC(result);
		}
		catch(std::exception const&) // advc.003: Better to catch it by reference
		{
			OutputDebugString("Allocation failure\n");
		}

		return result;
	}

	//::MessageBoxA(NULL,"UNsafe alloc","CvGameCore",MB_OK); // disabled by K-Mod, for now.
	//OutputDebugString("Alloc [unsafe]"); // advc.make: Don't need this either, do we?
	return malloc(size);
}

void __cdecl operator delete (void *p)
{
	if (gDLL)
	{
		PROFILE_TRACK_DEALLOC(p);
		gDLL->delMem(p, __FILE__, __LINE__);
	}
	else free(p);
}

void* operator new[](size_t size)
{
	if (gDLL)
	{
		//OutputDebugString("Alloc [safe]");
		void* result = gDLL->newMemArray(size, __FILE__, __LINE__);
		memset(result, 0xDA, size); // advc.make
		return result;
	}

	OutputDebugString("Alloc [unsafe]");
	::MessageBoxA(NULL,"UNsafe alloc","CvGameCore",MB_OK);
	return malloc(size);
}

void operator delete[](void* pvMem)
{
	if (gDLL)
		gDLL->delMemArray(pvMem, __FILE__, __LINE__);
	else free(pvMem);
}

void *__cdecl operator new(size_t size, char* pcFile, int iLine)
{
	void* result = gDLL->newMem(size, pcFile, iLine);
	memset(result, 0xDA, size); // advc.make
	return result;
}

void *__cdecl operator new[](size_t size, char* pcFile, int iLine)
{
	void* result = gDLL->newMem(size, pcFile, iLine);
	memset(result, 0xDA, size); // advc.make
	return result;
}

void __cdecl operator delete(void* pvMem, char* pcFile, int iLine)
{
	gDLL->delMem(pvMem, pcFile, iLine);
}

void __cdecl operator delete[](void* pvMem, char* pcFile, int iLine)
{
	gDLL->delMem(pvMem, pcFile, iLine);
}


void* reallocMem(void* a, unsigned int uiBytes, const char* pcFile, int iLine)
{
	return gDLL->reallocMem(a, uiBytes, pcFile, iLine);
}

unsigned int memSize(void* a)
{
	return gDLL->memSize(a);
}
#endif // K-Mod

BOOL APIENTRY DllMain(HANDLE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved)
{
	switch( ul_reason_for_call ) {
	case DLL_PROCESS_ATTACH:
		{
		// The DLL is being loaded into the virtual address space of the current process as a result of the process starting up
		OutputDebugString("DLL_PROCESS_ATTACH\n");

		// set timer precision
		MMRESULT iTimeSet = timeBeginPeriod(1);		// set timeGetTime and sleep resolution to 1 ms, otherwise it's 10-16ms
		FAssertMsg(iTimeSet==TIMERR_NOERROR, "failed setting timer resolution to 1 ms");
		}
		break;
	case DLL_THREAD_ATTACH:
		// OutputDebugString("DLL_THREAD_ATTACH\n");
		break;
	case DLL_THREAD_DETACH:
		// OutputDebugString("DLL_THREAD_DETACH\n");
		break;
	case DLL_PROCESS_DETACH:

		OutputDebugString("DLL_PROCESS_DETACH\n");
		timeEndPeriod(1);
		break;
	}

	return TRUE;	// success
}

#ifdef USE_INTERNAL_PROFILER

//	Uncomment the following line to provide (very) detailed tracing to
//	the debug stream (view with DbgView or a debugger)
#define DETAILED_TRACE

#define MAX_SAMPLES 900
static ProfileSample* _currentSample = NULL;
static int numSamples = 0;
static ProfileSample* sampleList[MAX_SAMPLES];
static ProfileSample* sampleStack[MAX_SAMPLES];
static int depth = -1;

#ifdef DETAILED_TRACE
static ProfileSample* lastExit = NULL;
static int exitCount = 0;
static bool detailedTraceEnabled = false;

static void GenerateTabString(char* buffer,int n)
{
	while(n-- > 0)
	{
		*buffer++ = '\t';
	}

	*buffer = '\0';
}
#endif

void EnableDetailedTrace(bool enable)
{
#ifdef DETAILED_TRACE
	detailedTraceEnabled = enable;
#endif
}

void IFPBeginSample(ProfileSample* sample)
{
	if ( sample->Id == -1 )
	{
		if ( numSamples == MAX_SAMPLES )
		{
			dumpProfileStack();
			::MessageBox(NULL,"Profile sample limit exceeded","CvGameCore",MB_OK);
			return;
		}
		sample->Id = numSamples;
		sample->OpenProfiles = 0;
		sample->ProfileInstances = 0;
		sample->Accumulator.QuadPart = 0;
		sample->ChildrenSampleTime.QuadPart = 0;
		sampleList[numSamples++] = sample;
	}

	if ( ++depth == MAX_SAMPLES )
	{
		::MessageBox(NULL,"Sample stack overflow","CvGameCore",MB_OK);
	}
	else
	{
		sampleStack[depth] = sample;
	}

	sample->ProfileInstances++;
	sample->OpenProfiles++;

	if ( sample->OpenProfiles == 1 )
	{
		if ( _currentSample == NULL )
		{
			sample->Parent = -1;
		}
		else
		{
			sample->Parent = _currentSample->Id;
		}

		QueryPerformanceCounter(&sample->StartTime);
	}

	_currentSample = sample;

#ifdef DETAILED_TRACE
	if ( detailedTraceEnabled && lastExit != sample )
	{
		char buffer[300];

		if ( exitCount != 0 )
		{
			GenerateTabString(buffer, depth);
			sprintf(buffer+depth, "[%d]\n", exitCount);
			OutputDebugString(buffer);

			exitCount = 0;
		}

		GenerateTabString(buffer, depth);
		sprintf(buffer+depth, "-->%s\n", sample->Name);

		OutputDebugString(buffer);
	}
#endif
}

void IFPEndSample(ProfileSample* sample)
{
	if ( _currentSample != sample )
	{
		MessageBox(NULL,"Sample closure not matched","CvGameCore",MB_OK);
	}

	if ( depth < 0 )
	{
		MessageBox(NULL,"Too many end-samples","CvGameCore",MB_OK);
	}
	else if ( depth == 0 )
	{
		_currentSample = NULL;
		depth = -1;
	}
	else
	{
		_currentSample = sampleStack[--depth];
	}

	if ( sample->OpenProfiles-- == 1 )
	{
		LARGE_INTEGER now;
		LONGLONG ellapsed;

		QueryPerformanceCounter(&now);

		ellapsed = (now.QuadPart - sample->StartTime.QuadPart);
		sample->Accumulator.QuadPart += ellapsed;

		if ( _currentSample != NULL )
		{
			_currentSample->ChildrenSampleTime.QuadPart += ellapsed;
		}
	}

#ifdef DETAILED_TRACE
	if ( detailedTraceEnabled && lastExit != sample )
	{
		char buffer[300];

		GenerateTabString(buffer, depth+1);
		strcpy(buffer+depth+1, "...\n");

		OutputDebugString(buffer);
		exitCount = 1;
	}
	else
	{
		exitCount++;
	}

	lastExit = sample;
#endif
}

void IFPBegin(void)
{
	for(int i = 0; i < numSamples; i++ )
	{
		sampleList[i]->Accumulator.QuadPart = 0;
		sampleList[i]->ChildrenSampleTime.QuadPart = 0;
		sampleList[i]->ProfileInstances = sampleList[i]->OpenProfiles;
	}
}

void IFPEnd(void)
{
	//	Log the timings
	char buffer[300];
	LARGE_INTEGER freq;

	QueryPerformanceFrequency(&freq);

	gDLL->logMsg("IFP_log.txt","Fn\tTime (mS)\tAvg time\t#calls\tChild time\tParent\n");

	for(int i = 0; i < numSamples; i++ )
	{
		if ( sampleList[i]->ProfileInstances != 0 )
		{
			sprintf(buffer,
					"%s\t%d\t%d\t%u\t%d\t%s\n",
					sampleList[i]->Name,
					(int)((1000*sampleList[i]->Accumulator.QuadPart)/freq.QuadPart),
					(int)((1000*sampleList[i]->Accumulator.QuadPart)/(freq.QuadPart*sampleList[i]->ProfileInstances)),
					sampleList[i]->ProfileInstances,
					(int)((1000*sampleList[i]->ChildrenSampleTime.QuadPart)/freq.QuadPart),
					sampleList[i]->Parent == -1 ? "" : sampleList[sampleList[i]->Parent]->Name);
			gDLL->logMsg("IFP_log.txt",buffer);
		}
	}
}

static bool isInLongLivedSection = false;
static ProfileSample rootSample__("Root");

//
// dump the current (profile) call stack to debug output
//
void dumpProfileStack(void)
{
	int i = 0;
	int dumpDepth = depth;
	char buffer[200];

	OutputDebugString("Profile stack:\n");

	while(dumpDepth >= 0)
	{
		char* ptr = buffer;

		i++;
		for(int j = 0; j < i; j++)
		{
			*ptr++ = '\t';
		}
		strcpy(ptr,sampleStack[dumpDepth--]->Name);
		strcat(ptr, "\n");
		OutputDebugString(buffer);
	}
}

static int pythonDepth = 0;

bool IFPPythonCall(const char* callerFn, const char* moduleName, const char* fxnName, void* fxnArg)
{
	PROFILE("IFPPythonCall1");

	//OutputDebugString(CvString::format("Python call to %s::%s [%d]\n", moduleName, fxnName, pythonDepth++).c_str());

	bool result = gDLL->getPythonIFace()->callFunction(moduleName, fxnName, fxnArg);

	//OutputDebugString("...complete\n");
	pythonDepth--;

	return result;
}

bool IFPPythonCall(const char* callerFn, const char* moduleName, const char* fxnName, void* fxnArg, long* result)
{
	PROFILE("IFPPythonCall2");

	//OutputDebugString(CvString::format("Python call to %s::%s [%d]\n", moduleName, fxnName, pythonDepth++).c_str());

	bool bResult = gDLL->getPythonIFace()->callFunction(moduleName, fxnName, fxnArg, result);

	//OutputDebugString("...complete\n");
	pythonDepth--;

	return bResult;
}


bool IFPPythonCall(const char* callerFn, const char* moduleName, const char* fxnName, void* fxnArg, CvString* result)
{
	PROFILE("IFPPythonCall3");

	//OutputDebugString(CvString::format("Python call to %s::%s [%d]\n", moduleName, fxnName, pythonDepth++).c_str());

	bool bResult = gDLL->getPythonIFace()->callFunction(moduleName, fxnName, fxnArg, result);

	//OutputDebugString("...complete\n");
	pythonDepth--;

	return bResult;
}


bool IFPPythonCall(const char* callerFn, const char* moduleName, const char* fxnName, void* fxnArg, CvWString* result)
{
	PROFILE("IFPPythonCall4");

	//OutputDebugString(CvString::format("Python call to %s::%s [%d]\n", moduleName, fxnName, pythonDepth++).c_str());

	bool bResult = gDLL->getPythonIFace()->callFunction(moduleName, fxnName, fxnArg, result);

	//OutputDebugString("...complete\n");
	pythonDepth--;

	return bResult;
}


bool IFPPythonCall(const char* callerFn, const char* moduleName, const char* fxnName, void* fxnArg, std::vector<byte>* pList)
{
	PROFILE("IFPPythonCall5");

	//OutputDebugString(CvString::format("Python call to %s::%s [%d]\n", moduleName, fxnName, pythonDepth++).c_str());

	bool result = gDLL->getPythonIFace()->callFunction(moduleName, fxnName, fxnArg, pList);

	//OutputDebugString("...complete\n");
	pythonDepth--;

	return result;
}


bool IFPPythonCall(const char* callerFn, const char* moduleName, const char* fxnName, void* fxnArg, std::vector<int> *pIntList)
{
	PROFILE("IFPPythonCall6");

	//OutputDebugString(CvString::format("Python call to %s::%s [%d]\n", moduleName, fxnName, pythonDepth++).c_str());

	bool result = gDLL->getPythonIFace()->callFunction(moduleName, fxnName, fxnArg, pIntList);

	//OutputDebugString("...complete\n");
	pythonDepth--;

	return result;
}


bool IFPPythonCall(const char* callerFn, const char* moduleName, const char* fxnName, void* fxnArg, int* pIntList, int* iListSize)
{
	PROFILE("IFPPythonCall7");

	//OutputDebugString(CvString::format("Python call to %s::%s [%d]\n", moduleName, fxnName, pythonDepth++).c_str());

	bool result = gDLL->getPythonIFace()->callFunction(moduleName, fxnName, fxnArg, pIntList, iListSize);

	//OutputDebugString("...complete\n");
	pythonDepth--;

	return result;
}


bool IFPPythonCall(const char* callerFn, const char* moduleName, const char* fxnName, void* fxnArg, std::vector<float> *pFloatList)
{
	PROFILE("IFPPythonCall8");

	//OutputDebugString(CvString::format("Python call to %s::%s [%d]\n", moduleName, fxnName, pythonDepth++).c_str());

	bool result = gDLL->getPythonIFace()->callFunction(moduleName, fxnName, fxnArg, pFloatList);

	//OutputDebugString("...complete\n");
	pythonDepth--;

	return result;
}



#endif

//
// enable dll profiler if necessary, clear history
//
void startProfilingDLL(bool longLived)
{
#ifdef USE_INTERNAL_PROFILER
	if ( longLived )
	{
		isInLongLivedSection = true;
	}
	else if (GC.isDLLProfilerEnabled() && !isInLongLivedSection)
	{
		IFPBegin();
		IFPBeginSample(&rootSample__);
	}
#else
	if (GC.isDLLProfilerEnabled())
	{
		gDLL->ProfilerBegin();
	}
#endif
}

//
// dump profile stats on-screen
//
void stopProfilingDLL(bool longLived)
{
#ifdef USE_INTERNAL_PROFILER
	if ( longLived )
	{
		isInLongLivedSection = false;
	}
	else if (GC.isDLLProfilerEnabled() && !isInLongLivedSection)
	{
		IFPEndSample(&rootSample__);
		IFPEnd();
	}
#else
	if (GC.isDLLProfilerEnabled())
	{
		gDLL->ProfilerEnd();
	}
#endif
}

#ifdef MEMORY_TRACKING
CMemoryTrack*	CMemoryTrack::trackStack[MAX_TRACK_DEPTH];
int CMemoryTrack::m_trackStackDepth = 0;

CMemoryTrack::CMemoryTrack(const char* name, bool valid)
{
	m_highWater = 0;
	m_name = name;
	m_valid = valid;
	// <advc.003> Safer to initialize this
	for(int i = 0; i < MAX_TRACKED_ALLOCS; i++)
		m_track[i] = NULL; // </advc.003>
	if ( m_trackStackDepth < MAX_TRACK_DEPTH )
	{
		trackStack[m_trackStackDepth++] = this;
	}
}

CMemoryTrack::~CMemoryTrack()
{
	if ( m_valid )
	{
		for(int i = 0; i < m_highWater; i++)
		{
			if ( m_track[i] != NULL )
			{
				char buffer[200];

				sprintf(buffer, "Apparent memory leak detected in %s\n", m_name);
				OutputDebugString(buffer);
			}
		}
	}

	if ( trackStack[m_trackStackDepth-1] == this )
	{
		m_trackStackDepth--;
	}
}

void CMemoryTrack::NoteAlloc(void* ptr)
{
	if ( m_valid )
	{
		for(int i = 0; i < m_highWater; i++)
		{
			if ( m_track[i] == NULL )
			{
				break;
			}
		}

		if ( i == m_highWater )
		{
			if ( m_highWater < MAX_TRACKED_ALLOCS )
			{
				m_highWater++;
			}
			else
			{
				m_valid = false;
				return;
			}
		}

		m_track[i] = ptr;
	}
}

void CMemoryTrack::NoteDeAlloc(void* ptr)
{
	if ( m_valid )
	{
		for(int i = 0; i < m_highWater; i++)
		{
			if ( m_track[i] == ptr )
			{
				m_track[i] = NULL;
				break;
			}
		}
	}
}

CMemoryTrack* CMemoryTrack::GetCurrent(void)
{
	if ( 0 < m_trackStackDepth && m_trackStackDepth < MAX_TRACK_DEPTH )
	{
		return trackStack[m_trackStackDepth-1];
	}
	else
	{
		return NULL;
	}
}

CMemoryTrace::CMemoryTrace(const char* name)
{
	PROCESS_MEMORY_COUNTERS pmc;

	GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof(pmc));
	m_name = name;
	m_start = pmc.WorkingSetSize;
}

CMemoryTrace::~CMemoryTrace()
{
	PROCESS_MEMORY_COUNTERS pmc;

	if ( GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof(pmc)) )
	{
		char buffer[200];

		sprintf(buffer, "function %s added %dK bytes, total now %dK\n", m_name, (int)(pmc.WorkingSetSize - m_start)/1024, (int)pmc.WorkingSetSize/1024);
		OutputDebugString(buffer);
	}
}

void ProfileTrackAlloc(void* ptr)
{
	CMemoryTrack* current = CMemoryTrack::GetCurrent();

	if ( current != NULL )
	{
		current->NoteAlloc(ptr);
	}
}

void ProfileTrackDeAlloc(void* ptr)
{
	CMemoryTrack* current = CMemoryTrack::GetCurrent();

	if ( current != NULL )
	{
		current->NoteDeAlloc(ptr);
	}
}

#endif