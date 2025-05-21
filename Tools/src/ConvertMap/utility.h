#ifndef UTILITY_H
#define UTILITY_H

#define DOS_RESERVED ":/"
#define DOS_UNRECOMMENDED ";*?#<>~|$`'\"%"
#define DOS_BRACKETS "()[]"
#define MAX_STR_LENGTH 256
#define MAX_CMD_LENGTH 512

#if !defined (__MORPHOS__)
  #define MAX(a, b) (a > b ? a : b)
  #define MIN(a, b) (a > b ? b : a)
#endif

#define NO_MORE_DEFINITIONS 0
#define MORE_DEFINITIONS   -1
#define MORE_VARIABLES     -2
#define SYNTAX_ERROR       -3

#define MODE_VARIABLE       0
#define MODE_TYPE           1

BOOL Exists(STRPTR filename);
BOOL CopyFile(STRPTR fileSrc, STRPTR fileDest);

BOOL aToi(STRPTR text, LONG *result);
DOUBLE aTof(UBYTE* str);
STRPTR makeString(STRPTR str);
STRPTR makeString2(STRPTR str1, STRPTR str2);
STRPTR makeString3(STRPTR str1, STRPTR str2, STRPTR str3);
VOID writeString(BPTR fh, STRPTR str);
STRPTR readStringFromFile(BPTR fh);
STRPTR setString(STRPTR dest, STRPTR src);
STRPTR setString2(STRPTR dest, STRPTR src1, STRPTR src2);
VOID freeString(STRPTR str);

STRPTR makePath(STRPTR dir, STRPTR file, STRPTR extension);
STRPTR stripExtension(STRPTR pathname);
VOID replaceChars(STRPTR str, STRPTR restricted, UBYTE ch);
STRPTR pathPart(STRPTR pathname);
STRPTR searchString(STRPTR source, STRPTR str);
BOOL locateStrInFile(BPTR fh, STRPTR str);
BOOL locateArrayStart(BPTR fh);
BOOL locateArrayEnd(BPTR fh);
LONG readDefinition(BPTR fh, STRPTR* dest_string, ULONG mode);

#endif /* UTILITY_H */
