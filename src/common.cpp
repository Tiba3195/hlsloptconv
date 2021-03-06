

#include "common.hpp"

#include <time.h>
#if _WIN32
#  include <windows.h>
#endif
#if __APPLE__
#  include <sys/time.h>
#endif


using namespace HOC;


void* HOC_MALLOC_EH(size_t sz)
{
	void* p = HOC_MALLOC(sz);
	if (!p)
		abort();
	return p;
}


OutStream& OutStream::operator << (short v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%hd", v));
	return *this;
}

OutStream& OutStream::operator << (unsigned short v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%hu", v));
	return *this;
}

OutStream& OutStream::operator << (int v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%d", v));
	return *this;
}

OutStream& OutStream::operator << (unsigned int v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%u", v));
	return *this;
}

OutStream& OutStream::operator << (long v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%ld", v));
	return *this;
}

OutStream& OutStream::operator << (unsigned long v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%lu", v));
	return *this;
}

OutStream& OutStream::operator << (long long v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%lld", v));
	return *this;
}

OutStream& OutStream::operator << (unsigned long long v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%llu", v));
	return *this;
}

OutStream& OutStream::operator << (float v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%.6g", v));
	return *this;
}

OutStream& OutStream::operator << (double v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%.18g", v));
	return *this;
}

OutStream& OutStream::operator << (bool v)
{
	if (v)
		Write(STRLIT_SIZE("true"));
	else
		Write(STRLIT_SIZE("false"));
	return *this;
}

OutStream& OutStream::operator << (const void* v)
{
	char bfr[32];
	Write(bfr, sprintf(bfr, "%p", v));
	return *this;
}

OutStream& OutStream::operator << (const String& v)
{
	Write(v.c_str(), v.size());
	return *this;
}

OutStream& OutStream::operator << (const Twine& v)
{
	if (v._cstr)
		*this << v._cstr;
	if (v._str)
		*this << *v._str;
	if (v._lft)
		*this << *v._lft;
	if (v._rgt)
		*this << *v._rgt;
	return *this;
}

void StringStream::Write(const char* str, size_t size)
{
	strbuf.append(str, size);
}

void StringStream::Write(const char* str)
{
	strbuf.append(str);
}

void FILEStream::Write(const char* str, size_t size)
{
	fwrite(str, size, 1, file);
}

void CallbackStream::Write(const char* str, size_t size)
{
	if (textOut && textOut->func)
		textOut->func(str, size, textOut->userData);
}


double HOC::GetTime()
{
#if __linux
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + 0.000000001 * (double) ts.tv_nsec;
#elif __APPLE__
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double) tv.tv_sec + 0.000001 * (double) tv.tv_usec;
#elif defined(_WIN32)
	LARGE_INTEGER cnt, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&cnt);
	return double(cnt.QuadPart) / double(freq.QuadPart);
#endif
}


Diagnostic::Diagnostic(OutStream* eos, const char* src)
{
	errorOutputStream = eos;
	sourceFiles.push_back(src);
}

uint32_t Diagnostic::GetSourceID(const String& src)
{
	for (size_t i = 0; i < sourceFiles.size(); ++i)
		if (sourceFiles[i] == src)
			return uint32_t(i);
	sourceFiles.push_back(src);
	return uint32_t(sourceFiles.size() - 1);
}

void Diagnostic::PrintMessage(const char* type, const Twine& msg, const Location& loc)
{
	if (!errorOutputStream)
		return;
	*errorOutputStream << sourceFiles[loc.source < sourceFiles.size() ? loc.source : 0] << ":";
	if (loc != Location::BAD())
	{
		*errorOutputStream << loc.line << ":" << loc.off << ":";
	}
	*errorOutputStream << " " << type << ": " << msg << "\n";
}

void Diagnostic::EmitError(const Twine& msg, const Location& loc)
{
	hasErrors = true;
	PrintError(msg, loc);
}


static bool IsValidVecSwizzleWriteMask(uint8_t mask, int ncomp)
{
	uint16_t compsModified = 0;
	for (int i = 0; i < ncomp; ++i)
	{
		uint16_t flag = 1 << ((mask >> (i * 2)) & 0x3);
		if (compsModified & flag)
			return false;
		compsModified |= flag;
	}
	return true;
}

static bool IsValidMtxSwizzleWriteMask(uint16_t mask, int ncomp)
{
	uint16_t compsModified = 0;
	for (int i = 0; i < ncomp; ++i)
	{
		uint16_t flag = 1 << ((mask >> (i * 4)) & 0xf);
		if (compsModified & flag)
			return false;
		compsModified |= flag;
	}
	return true;
}

bool HOC::IsValidSwizzleWriteMask(uint32_t mask, bool matrix, int ncomp)
{
	return matrix ? IsValidMtxSwizzleWriteMask(mask, ncomp) : IsValidVecSwizzleWriteMask(mask, ncomp);
}

