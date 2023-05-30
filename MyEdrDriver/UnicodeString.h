#pragma once

#include "AutoDeletedPointer.h"

#include <wdm.h>

class UnicodeString
{
public:
	UnicodeString();
	UnicodeString(const UnicodeString& unicodeString) = delete;
	UnicodeString(UnicodeString&& unicodeString) = default;

	UnicodeString& operator=(const UnicodeString& other) = delete;
	UnicodeString& operator=(UnicodeString&& other) = default;

	int operator<=>(const UnicodeString& other) const;

	NTSTATUS copyFrom(const UnicodeString& unicodeString);
	NTSTATUS copyFrom(const UNICODE_STRING& unicodeString);
	NTSTATUS copyFrom(const wchar_t* rawUnicodeString);

	size_t length() const;

	const UNICODE_STRING& get() const;
	UNICODE_STRING& get();

	const wchar_t* getRaw() const;
	wchar_t* getRaw();

private:
	AutoDeletedPointer<wchar_t> m_rawUnicodeString;
	UNICODE_STRING m_unicodeString;
};
