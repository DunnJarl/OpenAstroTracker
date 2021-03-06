#include <Arduino.h>
#if DEBUG_LEVEL > 0
#include <stdarg.h>
#endif

#include "Utility.hpp"

#if DEBUG_LEVEL > 0
unsigned long RealTime::_pausedTime = 0;
unsigned long RealTime::_startTime = micros();
unsigned long RealTime::_suspendStart = 0;
int RealTime::_suspended = 0;
#endif


// Adjust the given number by the given adjustment, wrap around the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
int adjustWrap(int current, int adjustBy, int minVal, int maxVal)
{
  current += adjustBy;
  if (current > maxVal)
    current -= (maxVal + 1 - minVal);
  if (current < minVal)
    current += (maxVal + 1 - minVal);
  return current;
}

// Adjust the given number by the given adjustment, clamping to the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
int adjustClamp(int current, int adjustBy, int minVal, int maxVal)
{
  current += adjustBy;
  if (current > maxVal)
    current = maxVal;
  if (current < minVal)
    current = minVal;
  return current;
}

// Clamp the given number to the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
long clamp(long current, long minVal, long maxVal)
{
  if (current > maxVal)
    current = maxVal;
  if (current < minVal)
    current = minVal;
  return current;
}

// Clamp the given number to the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
int clamp(int current, int minVal, int maxVal)
{
  if (current > maxVal)
    current = maxVal;
  if (current < minVal)
    current = minVal;
  return current;
}

// Clamp the given number to the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
float clamp(float current, float minVal, float maxVal)
{
  if (current > maxVal)
    current = maxVal;
  if (current < minVal)
    current = minVal;
  return current;
}

#if DEBUG_LEVEL > 0

String formatArg(const char *input, va_list args)
{
  const char *nibble = "0123456789ABCDEF";
  char achBuffer[255];
  char *p = achBuffer;

  for (const char *i = input; *i != 0; i++)
  {
    if (*i != '%')
    {
      *p++ = *i;
      continue;
    }
    i++;
    switch (*i)
    {
    case '%':
    {
      *p++ = '%';
    }
    break;

    case 'c':
    {
      char *ch = va_arg(args, char *);
      *p++ = *ch;
    }
    break;

    case 's':
    {
      char *s = va_arg(args, char *);
      strcpy(p, s);
      p += strlen(s);
    }
    break;

    case 'd':
    {
      String s = String((int)va_arg(args, int));
      strcpy(p, s.c_str());
      p += s.length();
    }
    break;

    case 'x':
    {
      int n = (int)va_arg(args, int);
      int shift = 12;
      unsigned int mask = 0xF000;
      *p++ = '0';
      *p++ = 'x';
      while (shift >= 0)
      {
        int d = (n & mask) >> shift;
        *p++ = *(nibble + d);
        mask = mask >> 4;
        shift -= 4;
      }

      *p = 0;
    }
    break;

    case 'l':
    {
      String s = String((long)va_arg(args, long));
      strcpy(p, s.c_str());
      p += s.length();
    }
    break;

    case 'f':
    {
      float num = (float)va_arg(args, double);
      String s = String(num, 4);
      strcpy(p, s.c_str());
      p += s.length();
    }
    break;

    default:
    {
      *p++ = *i;
    }
    break;
    }
  }

  *p = '\0';
  return String(achBuffer);
}

String format(const char *input, ...)
{
  va_list argp;
  va_start(argp, input);
  String ret = formatArg(input, argp);
  va_end(argp);
  return ret;
}

// void log(const char* input) {
//   Serial.println(input);
//   Serial.flush();
// }

// void log(String input) {
//   Serial.println(input);
//   Serial.flush();
// }

void logv(int levelFlags, const char *input, ...)
{
  if ((levelFlags & DEBUG_LEVEL) != 0)
  {
    va_list argp;
    va_start(argp, input);
    Serial.println(formatArg(input, argp));
    Serial.flush();
    va_end(argp);
  }
}

#endif
