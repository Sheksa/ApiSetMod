// ApiSet.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <Winternl.h>
#include "ntdll.h"

#define DLLNAME L"lemon.dll"
#define DLLLEN  9

// Thanks Blackbone for the structs
typedef struct _API_SET_VALUE_ENTRY
{
	ULONG Flags;
	ULONG NameOffset;
	ULONG NameLength;
	ULONG ValueOffset;
	ULONG ValueLength;
} API_SET_VALUE_ENTRY, *PAPI_SET_VALUE_ENTRY;

typedef struct _API_SET_VALUE_ARRAY
{
	ULONG Flags;
	ULONG NameOffset;
	ULONG Unk;
	ULONG NameLength;
	ULONG DataOffset;
	ULONG Count;
} API_SET_VALUE_ARRAY, *PAPI_SET_VALUE_ARRAY;

typedef struct _API_SET_NAMESPACE_ENTRY
{
	ULONG Limit;
	ULONG Size;
} API_SET_NAMESPACE_ENTRY, *PAPI_SET_NAMESPACE_ENTRY;

typedef struct _API_SET_NAMESPACE_ARRAY
{
	ULONG Version;
	ULONG Size;
	ULONG Flags;
	ULONG Count;
	ULONG Start;
	ULONG End;
	ULONG Unk[2];
} API_SET_NAMESPACE_ARRAY, *PAPI_SET_NAMESPACE_ARRAY;

void __memcpy(void * dest, void * src, size_t n)
{
	char *tmpsrc = (char *)src;
	char *tmpdest = (char *)dest;

	for (int i = 0; i<n; i++)
		tmpdest[i] = tmpsrc[i];
}

int __wcscmp(const wchar_t *s1, const wchar_t *s2)
{
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return 0;
	return (*(const int *)s1 - *(const int *)--s2);
}

int _tmain(int argc, _TCHAR* argv[])
{

	PEB * peb = NtCurrentTeb()->ProcessEnvironmentBlock;
	PVOID ProcessHeap = peb->Reserved4[1];	// ProcessHeap
	PAPI_SET_NAMESPACE_ARRAY pApiSetMap = (PAPI_SET_NAMESPACE_ARRAY)peb->Reserved9[0]; // ApiSetMap

	int realApiSetMapSize = pApiSetMap->Size;

	PAPI_SET_NAMESPACE_ARRAY pFakeApiSetMap = (PAPI_SET_NAMESPACE_ARRAY)RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, realApiSetMapSize + (DLLLEN * 2));

	__memcpy((char *)pFakeApiSetMap, (char *)pApiSetMap, realApiSetMapSize);
	__memcpy(((PUCHAR)pFakeApiSetMap + realApiSetMapSize), DLLNAME, (DLLLEN * 2));

	DbgPrint("\n#Peb Address:%x\nApiSetMap:%x\nFakeApiSetMap:%x\n", peb, peb->Reserved9[0], pFakeApiSetMap);
	for (size_t i = 0; i < pFakeApiSetMap->Count; i++)
	{
		wchar_t apiNameBuf[255] = { 0 };
		wchar_t apiHostNameBuf[255] = { 0 };
		size_t oldValueLen = 0;

		PAPI_SET_NAMESPACE_ENTRY pDescriptor = (PAPI_SET_NAMESPACE_ENTRY)((PUCHAR)pFakeApiSetMap + pFakeApiSetMap->End + i * sizeof(API_SET_NAMESPACE_ENTRY));
		PAPI_SET_VALUE_ARRAY pHostArray = (PAPI_SET_VALUE_ARRAY)((PUCHAR)pFakeApiSetMap + pFakeApiSetMap->Start + sizeof(API_SET_VALUE_ARRAY) * pDescriptor->Size);

		__memcpy((char *)apiNameBuf, (char *)pFakeApiSetMap + pHostArray->NameOffset, pHostArray->NameLength);

		PAPI_SET_VALUE_ENTRY pHost = (PAPI_SET_VALUE_ENTRY)((PUCHAR)pFakeApiSetMap + pHostArray->DataOffset);
		__memcpy((char *)apiHostNameBuf, (char *)pFakeApiSetMap + pHost->ValueOffset, pHost->ValueLength);
		if (__wcscmp(apiNameBuf, L"api-ms-win-core-file-l2-1") == 0)
		{
			oldValueLen = pHost->ValueLength;
			pHost->ValueLength = DLLLEN * 2;
			pHost->ValueOffset = realApiSetMapSize;
		}
		unsigned char* p = (unsigned char *)apiHostNameBuf;
		while (oldValueLen--)
			*p++ = (unsigned char)'\0';

		__memcpy((char *)apiHostNameBuf, (char *)pFakeApiSetMap + pHost->ValueOffset, pHost->ValueLength);
		if (pHostArray->Count == 1)
			DbgPrint("APISETMOD: [%d] %ws -> {%ws}\n", i, apiNameBuf, apiHostNameBuf);
		else
		{
			wchar_t baseApiHostNameBuf[255] = { 0 };
			__memcpy(baseApiHostNameBuf, (PUCHAR)pFakeApiSetMap + pHost[1].ValueOffset, pHost[1].ValueLength);
			DbgPrint("APISETMOD: [%d] %ws --> {%ws} --> {%ws}\n", i, apiNameBuf, apiHostNameBuf, baseApiHostNameBuf);
		}

	}
	peb->Reserved9[0] = pFakeApiSetMap;
	return 0;
}

