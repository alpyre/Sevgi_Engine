///defines
#define READ_BUFFER_SIZE 256
///
///includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

//Amiga headers
#include <exec/exec.h>
#include <dos/dos.h>

//Amiga protos
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "utility.h"
///
///globals
extern APTR g_MemoryPool;
///
///prototypes
///

//Utility functions for AmigaDOS
///Exists(fileName)
/******************************************************************************
 * Checks if a pathname is existent on disk.                                  *
 ******************************************************************************/
BOOL Exists(STRPTR filename)
{
  BPTR lock;

  if (filename)
  {
    if((lock = Lock(filename, SHARED_LOCK)))
    {
      UnLock(lock);
      return TRUE;
    }
    if(IoErr() == ERROR_OBJECT_IN_USE)
      return TRUE;
  }

return FALSE;
}
///
///CopyFile(fileSrc, fileDest)
/******************************************************************************
 * Quietly copies a file.                                                     *
 ******************************************************************************/
BOOL CopyFile(STRPTR fileSrc, STRPTR fileDest)
{
  #define COPY_BUFFER_SIZE 1024
  UBYTE buffer[COPY_BUFFER_SIZE];
  struct FileInfoBlock* fib = AllocDosObject(DOS_FIB, NULL);
  BOOL success = TRUE;

  if (fib) {
    BPTR fh_src;
    BPTR lock = Lock(fileSrc, ACCESS_READ);

    if (lock) {
      ULONG steps;
      ULONG last_step_size;

      Examine(lock, fib);
      UnLock(lock);

      steps = fib->fib_Size / COPY_BUFFER_SIZE;
      last_step_size = fib->fib_Size % COPY_BUFFER_SIZE;

      fh_src = Open(fileSrc, MODE_OLDFILE);
      if (fh_src) {
        BPTR fh_dest = Open(fileDest, MODE_NEWFILE);

        if (fh_dest) {
          ULONG i;
          ULONG size;

          for (i = 0; i < steps; i++) {
            size = Read(fh_src, buffer, COPY_BUFFER_SIZE);
            if (size > 0) {
              size = Write(fh_dest, buffer, COPY_BUFFER_SIZE);
              if (size <= 0) {
                success = FALSE;
                break;
              }
            }
            else {
              success = FALSE;
              break;
            }
          }

          if (success) {
            size = Read(fh_src, buffer, last_step_size);
            if (size > 0) {
              size = Write(fh_dest, buffer, last_step_size);
              if (size <= 0) success = FALSE;
            }
            else success = FALSE;
          }

          Close(fh_dest);

          if (!success) DeleteFile(fileDest);
        }
        else success = FALSE;

        Close(fh_src);
      }
      else success = FALSE;
    }
    else success = FALSE;

    FreeDosObject(DOS_FIB, fib);
  }
  else success = FALSE;

  return success;
}
///

//Utility functions for strings
///aToi(str, resultPtr)
/***********************************************************************
 * Takes a number in string form.                                      *
 * Converts the value into a LONG int                                  *
 * and puts the result into the LONG result (the given result pointer) *
 * The input text MUST contain an integer number.                      *
 ***********************************************************************/
BOOL aToi(STRPTR text, LONG *result)
{
  STRPTR curs = text;
  LONG value = 0;
  LONG multiplier = 1;
  WORD len = 0;

  // Find the start position of the number in string
  while (*curs)
  {
    if (*curs > 47 && *curs < 58) break;
    curs++;
  }

  //Find the number end (or decimal part of the number end)
  while (TRUE)
  {
    if (!*curs || *curs == '.' || (*curs!=',' && (*curs < 48 || *curs > 57))) break;
    curs++;
    len++;
  }
  //Step on the last digit
  if (len) curs--;
  else return FALSE;  // empty string!

  for (; len > 0; len--)
  {
    if (len == 1 && *curs == '-')          // first letter can be a minus sign.
      { value *= -1; break; }
    if (*curs == 44) { curs--; continue; } // if you meet a "," just skip it
    value += ((*curs - 48) * multiplier);
    multiplier *= 10;
    curs--;
  }

  *result = value;
  return TRUE;
}
///
///aTof(str)
/***********************************************************************
 * Takes a floating point number in string form.                       *
 * Converts the value into a DOUBLE and returns it.                    *
 * The input string should contain a number either in point decimal or *
 * exponential notation. Otherwise 0.0 will be returned.               *
 ***********************************************************************/
DOUBLE aTof(UBYTE* str)
{
  DOUBLE a = 0.0;
  LONG e = 0;
  LONG ch;

  while ((ch = *str++) && (ch >= '0' && ch <= '9')) {
    a = a * 10.0 + (ch - '0');
  }

  if (ch == '.') {
    while ((ch = *str++) && (ch >= '0' && ch <= '9')) {
      a = a * 10.0 + (ch - '0');
      e = e - 1;
    }
  }

  if (ch == 'e' || ch == 'E') {
    LONG sign = 1;
    LONG i = 0;

    ch = *str++;
    if (ch == '+')
      ch = *str++;
    else if (ch == '-') {
      ch = *str++;
      sign = -1;
    }

    while ((ch >= '0' && ch <= '9')) {
      i = i * 10 + (ch - '0');
      ch = *str++;
    }

    e += i * sign;
  }

  while (e > 0) {
    a *= 10.0;
    e--;
  }

  while (e < 0) {
    a *= 0.1;
    e++;
  }

  return a;
}
///
///makeString{num}(...)
/******************************************************************************
 * Creates a string allocated in program's memoryPool from all the strings    *
 * passed.                                                                    *
 ******************************************************************************/
STRPTR makeString(STRPTR str)
{
  STRPTR result = AllocPooled(g_MemoryPool, strlen(str) + 1);
  if (result) strcpy(result, str);

  return result;
}

STRPTR makeString2(STRPTR str1, STRPTR str2)
{
  ULONG len = strlen(str1) + strlen(str2) + 1;
  STRPTR result = AllocPooled(g_MemoryPool, len);

  if (result) {
    strcpy(result, str1);
    strcat(result, str2);
  }

  return result;
}

STRPTR makeString3(STRPTR str1, STRPTR str2, STRPTR str3)
{
  ULONG len = strlen(str1) + strlen(str2) + strlen(str2) + 1;
  STRPTR result = AllocPooled(g_MemoryPool, len);

  if (result) {
    strcpy(result, str1);
    strcat(result, str2);
    strcat(result, str3);
  }

  return result;
}

///
///writeString(filehandle, string)
/******************************************************************************
 * Writes a string to the the given file handle as a NULL terminated array of *
 * characters.                                                                *
 ******************************************************************************/
VOID writeString(BPTR fh, STRPTR str)
{
  Write(fh, str, strlen(str) + 1);
}
///
///readStringFromFile(filehandle)
/******************************************************************************
 * Reads a NULL terminated array of characters from the the current position  *
 * of the given file handle and returns the read string as a STRPTR allocated *
 * in programs memory pool.                                                   *
 ******************************************************************************/
STRPTR readStringFromFile(BPTR fh)
{
  STRPTR str = NULL;
  UBYTE buf[READ_BUFFER_SIZE];
  ULONG i = 0;

  while (TRUE) {
    Read(fh, &buf[i], 1);
    if (!buf[i]) break;
    i++;
    if (i >= READ_BUFFER_SIZE) {
      return NULL;
    }
  }

  str = AllocPooled(g_MemoryPool, i + 1);
  if (str) {
    if (i) strncpy(str, buf, i);
    str[i] = NULL;
  }

  return str;
}
///
///setString{num}(dest, ..., const_src)
/******************************************************************************
 * Replaces the contents of a STRPTR (which must be allocated on programs     *
 * memory pool) with another string. Previous contents will be deallocated.   *
 * Use as: strptr = setString(strptr, "new_contents");                        *
 ******************************************************************************/
STRPTR setString(STRPTR dest, STRPTR src)
{
  freeString(dest);
  return makeString(src);
}

STRPTR setString2(STRPTR dest, STRPTR src1, STRPTR src2)
{
  STRPTR result = makeString2(src1, src2);
  freeString(src1);
  if (dest != src1) freeString(dest);
  return result;
}
///
///freeString(string)
/******************************************************************************
 * Frees a string allocated on program's memory pool with the functions above.*                                                            *
 ******************************************************************************/
VOID freeString(STRPTR str)
{
  if (str) FreePooled(g_MemoryPool, str, strlen(str) + 1);
}
///
///makePath(directory, filename, extension)
/******************************************************************************
 * Adds a file name to a directory name in a newly allocated string from      *
 * program's memoryPool.                                                      *
 ******************************************************************************/
STRPTR makePath(STRPTR dir, STRPTR file, STRPTR extension)
{
  STRPTR result = NULL;
  ULONG len = 0;

  if (file) {
    if (dir) {
      UWORD dir_len = strlen(dir);
      UBYTE dir_tail = dir[dir_len - 1];
      UBYTE extra = (dir_tail == '/' || dir_tail == ':') ? 1 : 2;

      if (extension) {
        len = dir_len + strlen(file) + strlen(extension) + extra;
        result = AllocPooled(g_MemoryPool, len);
        if (result) {
          strcpy(result, dir);
          AddPart(result, file, len);
          strcat(result, extension);
        }
      }
      else {
        len = dir_len + strlen(file) + extra;
        result = AllocPooled(g_MemoryPool, len);
        if (result) {
          strcpy(result, dir);
          AddPart(result, file, len);
        }
      }
    }
    else {
      if (extension) {
        len = strlen(file) + strlen(extension) + 1;
        result = AllocPooled(g_MemoryPool, len);
        if (result) {
          strcpy(result, file);
          strcat(result, extension);
        }
      }
      else {
        len = strlen(file) + 1;
        result = AllocPooled(g_MemoryPool, len);
        if (result) {
          strcpy(result, file);
        }
      }
    }
  }
  return result;
}
///
///stripExtension(filename)
/******************************************************************************
 * Returns a new string with all the extension characters removed from the    *
 * given pathname. Will return NULL for a filename like ".ext".               *
 ******************************************************************************/
STRPTR stripExtension(STRPTR pathname)
{
  STRPTR result = NULL;

  if (pathname) {
    UBYTE *cursor = FilePart(pathname);
    ULONG path_len = cursor - pathname;
    ULONG len = 0;

    while (*cursor) {
      if (*cursor == '.') break;
      len++;
      cursor++;
    }

    if (len) {
      len += path_len;
      result = AllocPooled(g_MemoryPool, len + 1);
      strncpy(result, pathname, len);
      result[len] = 0;
    }
  }

  return result;
}
///
///replaceChars(string, restricted, char)
/******************************************************************************
 * Replaces all occurances of a list of characters (passed as a string in the *
 * second argument) in a string with the char given in the third argument.    *
 * Can be used to make filename strings compatible with AmigaDOS replacing    *
 * restricted characters with an allowed one.                                 *
 ******************************************************************************/
VOID replaceChars(STRPTR str, STRPTR restricted, UBYTE ch)
{
  UBYTE* c1 = str;
  UBYTE* c2 = restricted;

  if (str) {
    while (*c1) {
      for (c2 = restricted; *c2; c2++) {
        if (*c1 == *c2) *c1 = ch;
      }
      c1++;
    }
  }
}
///
///pathPart(pathname)
/******************************************************************************
 * Returns the directory part of a pathname in a newly allocated string.      *
 ******************************************************************************/
STRPTR pathPart(STRPTR pathname)
{
  STRPTR result = NULL;

  if (pathname) {
    ULONG path_len = PathPart(pathname) - pathname;

    result = AllocPooled(g_MemoryPool, path_len + 1);
    if (result) {
      strncpy(result, pathname, path_len);
      result[path_len] = 0;
    }
  }

  return result;
}
///
///searchString(source, str)
/******************************************************************************
 * A KMP algorithm that searches a string in another source string.           *
 * Returns the position of the first character after the found string.        *
 * Return 0 if the string is not found.                                       *
 ******************************************************************************/
STRPTR searchString(STRPTR source, STRPTR str)
{
  STRPTR found = NULL;
  ULONG M = strlen(str);
  UBYTE* lps = AllocPooled(g_MemoryPool, M);

  if (lps) {
    LONG i;
    ULONG j = 0;
    UBYTE len = 0; // length of the previous longest prefix suffix
    UBYTE ch;

    i = 1;
    while (i < M) {
      if (str[i] == str[len]) {
        len++;
        lps[i] = len;
        i++;
      }
      else {
        if (len != 0) {
          len = lps[len - 1];
        }
        else {
          lps[i] = 0;
          i++;
        }
      }
    }

    i = 0;
    ch = source[i++];
    while (ch) {
      if (str[j] == ch) {
        j++;
      }

      if (j == M) {
        found = source + i;
        break;
      }
      else {
        ch = source[i++];
        if (ch && str[j] != ch) {
          if (j != 0)
            j = lps[j - 1];
          else
            ch = source[i++];
        }
      }
    }

    FreePooled(g_MemoryPool, lps, M);
  }

  return found;
}
///
///locateStrInFile(filehandle, str)
/******************************************************************************
 * A KMP algorithm that locates a string in a file without loading the file.  *
 * It returns TRUE if the string is found and the file seek position will be  *
 * at the byte just after the first instance of the found string. It begins   *
 * the search from the current seek position of the opened file.              *
 ******************************************************************************/
BOOL locateStrInFile(BPTR fh, STRPTR str)
{
  BOOL found = FALSE;
  ULONG M = strlen(str);
  UBYTE* lps = AllocPooled(g_MemoryPool, M);

  if (lps) {
    LONG i;
    ULONG j = 0;
    UBYTE len = 0; // length of the previous longest prefix suffix
    UBYTE ch;

    i = 1;
    while (i < M) {
      if (str[i] == str[len]) {
        len++;
        lps[i] = len;
        i++;
      }
      else {
        if (len != 0) {
          len = lps[len - 1];
        }
        else {
          lps[i] = 0;
          i++;
        }
      }
    }

    i = Read(fh, &ch, 1);
    while (i > 0) {
      if (str[j] == ch) {
        j++;
      }

      if (j == M) {
        found = TRUE;
        break;
      }
      else {
        i = Read(fh, &ch, 1);
        if (i > 0 && str[j] != ch) {
          if (j != 0)
          j = lps[j - 1];
          else
          i = Read(fh, &ch, 1);
        }
      }
    }

    FreePooled(g_MemoryPool, lps, M);
  }

  return found;
}
///
///locateArrayStart(filehandle)
/******************************************************************************
 * Reads ahead a file until it finds a '{' character and returns TRUE.        *
 * If it meets a '}' before finding a '{' or end of file returns FALSE        *
 ******************************************************************************/
BOOL locateArrayStart(BPTR fh)
{
  UBYTE ch;
  BOOL retval = FALSE;

  while (Read(fh, &ch, 1) > 0) {
    if (ch == '{') {
      retval = TRUE;
      break;
    }
    if (ch == '}' || ch == ']') break;
  }

  return retval;
}
///
///locateArrayEnd(filehandle)
/******************************************************************************
 * Reads ahead a file until it finds a '}' character and returns TRUE.        *
 * If it meets a '{' before finding a '}' it skips that nested section, and   *
 * keep searching.                                                            *
 * Returns FALSE if it cannot find the matching '}' before the end of file.   *
 ******************************************************************************/
BOOL locateArrayEnd(BPTR fh)
{
  UBYTE ch;
  BOOL retval = FALSE;
  ULONG nested = 0;

  while (Read(fh, &ch, 1) > 0) {
    if (ch == '}') {
      if (nested) {
        nested--;
        continue;
      }
      else {
        retval = TRUE;
        break;
      }
    }
    if (ch == '{') nested++;
  }

  return retval;
}
///
///readDefinition(filehandle, dest_string, mode)
/******************************************************************************
 * Reads a string of characters from the current location of a file, stopping *
 * at comma, ; or } characters. This function is to read c style strings or   *
 * variable names from c style arays. Returns MORE_VARIABLES if there are     *
 * more elements. The function expects file position to be at the first       *
 * character of the string to be read, but if there is whitespace it will     *
 * skip it. If the file position is on an opening double quotes it will read  *
 * the enclosed string without skipping spaces.                               *
 ******************************************************************************/
LONG readDefinition(BPTR fh, STRPTR* dest_string, ULONG mode)
{
  UBYTE buffer[128];
  UBYTE* curs = buffer;
  LONG retval = MORE_DEFINITIONS;
  BOOL preceding_space = TRUE;

  *dest_string = NULL;

keep_reading:
  while (Read(fh, curs, 1) > 0) {

    if (*curs == ' ' || *curs == 9 /*tab*/ || *curs == 10 /*newline*/ || *curs == 13 /*CR*/) {
      if (preceding_space) continue; // preceding whitespace
      else if (mode == MODE_TYPE) break;
      else continue; // succeding whitespace
    }
    if (*curs == '/') {
      Read(fh, curs, 1);
      if (*curs == '*') {
        do {
          while (Read(fh, curs, 1) > 0 && *curs != '*');
        } while(Read(fh, curs, 1) > 0 && *curs != '/');
      }
      else {
        while (Read(fh, curs, 1) > 0 && *curs != 10 /*newline*/);
      }
      if (mode == MODE_TYPE && curs != buffer) break;
      continue;
    }

    if (*curs == '[') {
      if (mode == MODE_TYPE) {
        retval = SYNTAX_ERROR;
        break;
      }
      else {
        if (preceding_space) {
          retval = SYNTAX_ERROR;
          break;
        }
        else {
          STRPTR array_size_str;
          LONG array_size;
          readDefinition(fh, &array_size_str, MODE_VARIABLE);
          if (array_size_str) {
            if (aToi(array_size_str, &array_size) && array_size > 0) {
              retval = array_size;
            }
            else retval = SYNTAX_ERROR;

            freeString(array_size_str);
          }
          else retval = SYNTAX_ERROR;

          continue;
        }
      }
    }
    if (*curs == ']') {
      break;
    }

    if (*curs == '}') {
      retval = NO_MORE_DEFINITIONS;
      break;
    }
    if (*curs == ';') {
      break;
    }
    if (*curs == ',') {
      retval = MORE_VARIABLES;
      break;
    }

    curs++;
    preceding_space = FALSE;
  }

  *curs = NULL;

  if (!strcmp(buffer, "unsigned")) {
    *curs = ' ';
    curs++;
    preceding_space = TRUE;
    goto keep_reading;
  }

  if (curs != buffer) *dest_string = makeString(buffer);

  return retval;
}
///
