#ifndef __PASCAL_STRING_UTIL__
#define __PASCAL_STRING_UTIL__

void PascalAppend(Str255 str1, const Str255 str2);
void PascalCopy(const Str255 str1, Str255 str2);
bool PascalStringCompare(const Str255 str1, const Str255 str2);
void CtoPascal(const char* cs, Str255 str);
void PascalToC(const Str255 str, char* cs);
void FloatToPascal(long double value, Str255 str);

#endif
