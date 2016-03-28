#include <uspios.h>
#include <uspi/string.h>
#include "../errlog.h"

void LogWrite(const char *source, unsigned int severity, const char *message, ...)
{
    va_list args;
    TString string;

    va_start(args, message);

    String(&string);
    StringFormatV(&string, message, args);
    StringFormat(&string, "[%s] %s", source, string.m_pBuffer);

    va_end(args);

    if(severity == LOG_ERROR || severity == LOG_WARNING)
    {
        ErrorDisplayMessage(string.m_pBuffer, 1);
    }
    else if(severity == 0xff)
        ErrorDisplayMessage(string.m_pBuffer, 0);

    _String(&string);
}

