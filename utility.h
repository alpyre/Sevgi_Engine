#ifndef UTILITY_H
#define UTILITY_H

#define DOS_RESERVED ":/"
#define DOS_UNRECOMMENDED ";*?#<>~|$`'\"%"
#define DOS_BRACKETS "()[]"
#define NON_ALPHANUMERIC "!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~"
#define ALPHANUMERIC "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define MAX_STR_LENGTH 256
#define MAX_CMD_LENGTH 512
#define TEMP_RETURN_STR  "T:svgedtr_return_str"

#if !defined (__MORPHOS__)
  #define MAX(a, b) ((a) > (b) ? (a) : (b))
  #define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))

#define readString(f, d) readCStyleDataString(f, d)

struct ReturnInfo {
  LONG  code;
  UBYTE string[MAX_STR_LENGTH];
};

struct PathListEntry {
 BPTR ple_Next;
 BPTR ple_Lock;
};

STRPTR getProgDir();
BPTR getSystemPathList();
BOOL Exists(STRPTR filename);
BOOL CopyFile(STRPTR fileSrc, STRPTR fileDest);
#define CopyDir(dirSrc, dirDest) CopyFile(dirSrc, dirDest)
BOOL execute(struct ReturnInfo* ri, STRPTR command);
LONG runCommand(STRPTR command, STRPTR dir, STRPTR output);
LONG getFileSize(STRPTR filename);

BOOL aToi(STRPTR text, LONG *result);
STRPTR makeString(STRPTR str);
STRPTR makeString2(STRPTR str1, STRPTR str2);
STRPTR makeString3(STRPTR str1, STRPTR str2, STRPTR str3);
VOID writeString(BPTR fh, STRPTR str);
STRPTR readStringFromFile(BPTR fh);
STRPTR setString(STRPTR dest, STRPTR src);
STRPTR setString2(STRPTR dest, STRPTR src1, STRPTR src2);
VOID toLower(STRPTR str);
VOID toUpper(STRPTR str);
VOID freeString(STRPTR str);

STRPTR makePath(STRPTR dir, STRPTR file, STRPTR extension);
STRPTR stripExtension(STRPTR pathname);
VOID replaceChars(STRPTR str, STRPTR restricted, UBYTE ch);
STRPTR pathPart(STRPTR pathname);
BOOL searchString(STRPTR source, STRPTR str, ULONG* pos);
BOOL locateStrInFile(BPTR fh, STRPTR str);
BOOL locateArrayStart(BPTR fh);
BOOL readCStyleDataString(BPTR fh, STRPTR* dest_string);
BOOL getString(BPTR fh, STRPTR* dest_string);

Object* MUI_NewButton(STRPTR text, UBYTE hi_char, STRPTR short_help);
Object* MUI_NewCheckMark(Object** obj, BOOL state, STRPTR label, UBYTE key, STRPTR help);
Object* MUI_GetChild(Object* obj, ULONG order);
Object* MUI_HorizontalTitle(STRPTR title);

#endif /* UTILITY_H */
