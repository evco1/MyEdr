#pragma once

#include "AutoDeletedPointer.h"

#include <wdm.h>

class UnicodeString final
{
public:
	UnicodeString() = delete;
	UnicodeString(const UnicodeString&) = delete;
	UnicodeString(UnicodeString&&) = default;
	UnicodeString(USHORT maxCharCount);
	UnicodeString(const wchar_t* string);

	~UnicodeString() = default;

	UnicodeString& operator=(const UnicodeString&) = delete;
	UnicodeString& operator=(UnicodeString&&) = default;

	const UNICODE_STRING& operator*() const;
	UNICODE_STRING& operator*();

	const UNICODE_STRING* operator->() const;
	UNICODE_STRING* operator->();

	bool isInitialized() const;

	const UNICODE_STRING* get() const;
	UNICODE_STRING* get();

private:
	AutoDeletedPointer<wchar_t> m_unicodeStringBuffer;
	UNICODE_STRING m_unicodeString;
};
