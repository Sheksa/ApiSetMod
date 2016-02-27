// ApiSetPrint: Prints the API Contracts for Win10.
//

#include "stdafx.h"
#include <Windows.h>
#include <Winternl.h>

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


int _tmain(int argc, _TCHAR* argv[])
{
	PEB * peb = NtCurrentTeb()->ProcessEnvironmentBlock;
	
	PAPI_SET_NAMESPACE_ARRAY pApiSetMap = (PAPI_SET_NAMESPACE_ARRAY) peb->Reserved9[0]; // ApiSetMap

	for (size_t i = 0; i < pApiSetMap->Count; i++)
	{
		wchar_t apiNameBuf[255] = { 0 };
		wchar_t apiHostNameBuf[255] = { 0 };
		size_t oldValueLen = 0;

		PAPI_SET_NAMESPACE_ENTRY pDescriptor = (PAPI_SET_NAMESPACE_ENTRY)((PUCHAR)pApiSetMap + pApiSetMap->End + i * sizeof(API_SET_NAMESPACE_ENTRY));
		PAPI_SET_VALUE_ARRAY pHostArray = (PAPI_SET_VALUE_ARRAY)((PUCHAR)pApiSetMap + pApiSetMap->Start + sizeof(API_SET_VALUE_ARRAY) * pDescriptor->Size);

		memcpy(apiNameBuf, (PUCHAR)pApiSetMap + pHostArray->NameOffset, pHostArray->NameLength);

		PAPI_SET_VALUE_ENTRY pHost = (PAPI_SET_VALUE_ENTRY)((PUCHAR)pApiSetMap + pHostArray->DataOffset);
		memcpy(apiHostNameBuf, (PUCHAR)pApiSetMap + pHost->ValueOffset, pHost->ValueLength);

		if (pHostArray->Count == 1)
			wprintf(L"[%d] %s -> %s\n", i, apiNameBuf, apiHostNameBuf);
		else
		{
			wchar_t baseApiHostNameBuf[255] = { 0 };
			memcpy(baseApiHostNameBuf, (PUCHAR)pApiSetMap + pHost[1].ValueOffset, pHost[1].ValueLength);
			wprintf(L"[%d] %s --> %s --> %s\n", i, apiNameBuf, apiHostNameBuf, baseApiHostNameBuf);
		}
		
	}

	getchar();
	return 0;
}

