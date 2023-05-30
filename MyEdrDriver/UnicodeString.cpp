#include "UnicodeString.h"
#include "Result.h"

UnicodeString::UnicodeString() :
	m_rawUnicodeString{ new wchar_t{ UNICODE_NULL } },
	m_unicodeString{ 0, 0, nullptr }
{
	RETURN_ON_CONDITION(nullptr == m_rawUnicodeString, );
	RtlInitUnicodeString(&m_unicodeString, m_rawUnicodeString.get());
}

int UnicodeString::operator<=>(const UnicodeString& other) const
{
	ULONG hash, otherHash;
	RETURN_ON_BAD_STATUS(RtlHashUnicodeString(&m_unicodeString, TRUE, HASH_STRING_ALGORITHM_DEFAULT, &hash), -1);
	RETURN_ON_BAD_STATUS(RtlHashUnicodeString(&other.m_unicodeString, TRUE, HASH_STRING_ALGORITHM_DEFAULT, &otherHash), 1);
	
	if (hash < otherHash) {
		return -1;
	}

	if (hash > otherHash) {
		return 1;
	}

	return 0;
}

NTSTATUS UnicodeString::copyFrom(const UnicodeString& other)
{
	RETURN_ON_CONDITION(this == &other, STATUS_SUCCESS);
	return copyFrom(other.m_unicodeString);
}

NTSTATUS UnicodeString::copyFrom(const UNICODE_STRING& unicodeString)
{
	if (m_unicodeString.MaximumLength < (unicodeString.Length + sizeof(wchar_t)))
	{
		m_rawUnicodeString = new wchar_t[(unicodeString.Length / sizeof(wchar_t)) + 1];
		RETURN_ON_CONDITION(nullptr == m_rawUnicodeString, STATUS_INSUFFICIENT_RESOURCES);
	}

	m_unicodeString = {
		unicodeString.Length,
		static_cast<USHORT>(unicodeString.Length + sizeof(wchar_t)),
		m_rawUnicodeString.get()
	};
	RtlCopyUnicodeString(&m_unicodeString, &unicodeString);
	return STATUS_SUCCESS;
}

NTSTATUS UnicodeString::copyFrom(const wchar_t* rawUnicodeString)
{
	RETURN_ON_CONDITION(nullptr == rawUnicodeString, STATUS_INVALID_PARAMETER);
	UNICODE_STRING unicodeString;
	RtlInitUnicodeString(&unicodeString, rawUnicodeString);
	return copyFrom(unicodeString);
}

size_t UnicodeString::length() const
{
	return m_unicodeString.Length / sizeof(wchar_t);
}

const UNICODE_STRING& UnicodeString::get() const
{
	return m_unicodeString;
}

UNICODE_STRING& UnicodeString::get()
{
	return m_unicodeString;
}

const wchar_t* UnicodeString::getRaw() const
{
	return m_rawUnicodeString.get();
}

wchar_t* UnicodeString::getRaw()
{
	return m_rawUnicodeString.get();
}
