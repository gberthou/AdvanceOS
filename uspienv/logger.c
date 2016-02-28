#include <uspios.h>
#include "../errlog.h"

void LogWrite(const char *source, unsigned int severity, const char *message, ...)
{
    (void)source;
    if(severity == LOG_ERROR || severity == LOG_WARNING)
    {
        ErrorDisplayMessage(message);
    }
}

