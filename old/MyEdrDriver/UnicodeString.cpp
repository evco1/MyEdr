#include "UnicodeString.h"

#include <wdm.h>

UnicodeString::UnicodeString(const USHORT maxCharCount)
{
    m_unicodeStringBuffer.allocate(maxCharCount * sizeof(wchar_t));
    m_unicodeString = { 0, static_cast<USHORT>(maxCharCount * sizeof(wchar_t)), m_unicodeStringBuffer.get() };
}

UnicodeString::UnicodeString(const wchar_t* string) :
    m_unicodeStringBuffer(nullptr)
{
    RtlInitUnicodeString(&m_unicodeString, string);
}

const UNICODE_STRING& UnicodeString::operator*() const
{
    return m_unicodeString;
}

UNICODE_STRING& UnicodeString::operator*()
{
    return m_unicodeString;
}

const UNICODE_STRING* UnicodeString::operator->() const
{
    return &m_unicodeString;
}

UNICODE_STRING* UnicodeString::operator->()
{
    return &m_unicodeString;
}

bool UnicodeString::isInitialized() const
{
    return m_unicodeStringBuffer.isAllocated();
}

const UNICODE_STRING* UnicodeString::get() const
{
    return &m_unicodeString;
}

UNICODE_STRING* UnicodeString::get()
{
    return &m_unicodeString;
}
