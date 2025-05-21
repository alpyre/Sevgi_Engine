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
#include <dos/dostags.h>

//Amiga protos
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>

//MUI headers
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

//SDI headers
#include <SDI_compiler.h>
#include <SDI_hook.h>
#include <SDI_interrupt.h>
#include <SDI_lib.h>
#include <SDI_misc.h>
#include <SDI_stdarg.h>

#include "utility.h"
///
///globals
extern APTR g_MemoryPool;
extern BPTR g_System_Path_List;

struct {
  STRPTR MUI_pp_center;
}g_Strings = {"\33c"};
///
///prototypes
///

//Utility functions for AmigaDOS
///getProgDir()
STRPTR getProgDir()
{
  UBYTE buffer[MAX_STR_LENGTH];
  BPTR lock = GetProgramDir();
  CurrentDir(lock);
  if (NameFromLock(lock, buffer, MAX_STR_LENGTH)) {
    return makeString(buffer);
  }
  else return NULL;
}
///
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
        BPTR fh_dest = Open(fileDest, MODE_READWRITE);

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

          SetFileSize(fh_dest, 0, OFFSET_CURRENT);
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
///CopyDir(dirSrc, dirDest)
/******************************************************************************
 * Quietly copies a directory.                                                *
 ******************************************************************************/
BOOL CopyDir(STRPTR dirSrc, STRPTR dirDest)
{
  BOOL success = FALSE;

  if (dirSrc && dirDest && Exists(dirSrc)) {
    BPTR lock_dest = Lock(dirDest, SHARED_LOCK);
    if (!lock_dest) lock_dest = CreateDir(dirDest);
    if (lock_dest) {
      BPTR lock_src = Lock(dirSrc, SHARED_LOCK);
      if (lock_src) {
        struct FileInfoBlock* fib = AllocDosObject(DOS_FIB, NULL);
        if (fib) {
          if (Examine(lock_src, fib)) {
            if (fib->fib_DirEntryType > 0) {
              success = TRUE;
              while (ExNext(lock_src, fib)) {
                STRPTR file_src = makePath(dirSrc, fib->fib_FileName, NULL);
                STRPTR file_dest = makePath(dirDest, fib->fib_FileName, NULL);

                if (file_src && file_dest) {
                  if (fib->fib_DirEntryType > 0) {
                    success = CopyDir(file_src, file_dest);
                  }
                  else {
                    success = CopyFile(file_src, file_dest);
                  }
                }
                else success = FALSE;

                freeString(file_src);
                freeString(file_dest);
                if (!success) break;
              }
            }
            else {
              success = CopyFile(dirSrc, dirDest);
            }
          }
          FreeDosObject(DOS_FIB, fib);
        }
        UnLock(lock_src);
      }
      UnLock(lock_dest);
    }
  }

  return success;
}
///
///getSystemPathList()
BPTR getSystemPathList(struct WBStartup* wbs)
{
  struct Process* wbproc = wbs->sm_Message.mn_ReplyPort->mp_SigTask;
  struct CommandLineInterface* cli = (struct CommandLineInterface*) BADDR(wbproc->pr_CLI);

  return cli->cli_CommandDir;
}
///
///freePathList(path_list)
VOID freePathList(BPTR path_list)
{
  if (path_list) {
    struct PathListEntry* next = (struct PathListEntry*)BADDR(path_list);
    struct PathListEntry* current = NULL;

    while ((current = next)) {
      next = BADDR(current->ple_Next);
      UnLock(current->ple_Lock);
      FreeVec(current);
    }
  }
}
///
///copyPathList(path_list)
BPTR copyPathList(BPTR path_list)
{
  struct PathListEntry* orig    = BADDR(path_list);
  struct PathListEntry* head    = NULL;
  struct PathListEntry* current = NULL;
  struct PathListEntry* next    = NULL;

  while (orig) {
    if (next || (next = AllocVec(sizeof(struct PathListEntry), MEMF_PUBLIC | MEMF_CLEAR))) {
      if ((next->ple_Lock = DupLock(orig->ple_Lock))) {
        if (!head) head = next;
        if (current) current->ple_Next = MKBADDR(next);
        current = next;
        next = NULL;
      }
    }
    else {
      freePathList(MKBADDR(head));
      head = NULL;
      break;
    }

    orig = BADDR(orig->ple_Next);
  }
  if (next) FreeVec(next);

  return MKBADDR(head);
}
///
///execute(returnInfoPtr, command)
/******************************************************************************
 * A wrapper function which calls the API function System() providing a way   *
 * to access the return code and output string, also providing the subtask    *
 * with a CLI and the system's pathlist in the case of being ran from the     *
 * Workbench.                                                                 *
 ******************************************************************************/
BOOL execute(struct ReturnInfo* ri, STRPTR command)
{
  BOOL ret_val = FALSE;
  LONG result = 0;
  ULONG NP_Path_Tag = TAG_IGNORE;
  BPTR  NP_Path_Val = NULL;

  if (g_System_Path_List) {
    NP_Path_Tag = NP_Path;
    NP_Path_Val = copyPathList(g_System_Path_List);
  }

  if (ri && command) {
    BPTR fh = Open(TEMP_RETURN_STR, MODE_NEWFILE);
    if (fh) {
      result = SystemTags(command, NP_Cli, TRUE, NP_Path_Tag, NP_Path_Val, SYS_Output, fh, TAG_END);
      if (result < 0) {
        freePathList(NP_Path_Val);
        ri->code = 20;
        ri->string[0] = NULL;
      }
      else {
        LONG len;

        Seek(fh, 0, OFFSET_BEGINNING);
        len = Read(fh, ri->string, MAX_STR_LENGTH);
        if (len > 0) ri->string[len - 1] = NULL; //removes the last new line
        else ri->string[0] = NULL;
        ri->code = result;
        ret_val = TRUE;
      }

      Close(fh);
      DeleteFile(TEMP_RETURN_STR);
    }
  }

  return ret_val;
}
///
///getFileSize(filename)
/******************************************************************************
 * A negative return value indicates a failure at examining the file.         *
 ******************************************************************************/
LONG getFileSize(STRPTR filename)
{
  LONG size = -1;
  struct FileInfoBlock* fib = AllocDosObject(DOS_FIB, NULL);
  if (fib) {
    BPTR lock = Lock(filename, SHARED_LOCK);
    if (lock) {
      if (Examine(lock, fib)) {
        size = fib->fib_Size;
      }
      UnLock(lock);
    }

    FreeDosObject(DOS_FIB, fib);
  }

  return size;
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

STRPTR makeString3(STRPTR str1, STRPTR str2, STRPTR str3) {
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

  while (Read(fh, &buf[i], 1) > 0) {
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
///toLower(string)
VOID toLower(STRPTR str)
{
  if (str) {
    STRPTR c = str;

    while (*c) {
      if (*c >= 'A' && *c <= 'Z') *c += 32;
      c++;
    }
  }
}
///
///toUpper(string)
VOID toUpper(STRPTR str)
{
  if (str) {
    STRPTR c = str;

    while (*c) {
      if (*c >= 'a' && *c <= 'z') *c -= 32;
      c++;
    }
  }
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
    ULONG path_len = (ULONG)cursor - (ULONG)pathname;
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
 * Returns a the directory part of a pathname in a newly allocated string.    *
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
///searchString(source, str, &pos)
/******************************************************************************
 * A KMP algorithm that searches a string in another source string.           *
 * Returns true if str exists in source string. Puts the position of the find *
 * into pos if non-NULL.                                                      *
 ******************************************************************************/
BOOL searchString(STRPTR source, STRPTR str, ULONG* pos)
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

    i = 0;
    ch = source[i++];
    while (ch) {
      if (str[j] == ch) {
        j++;
      }

      if (j == M) {
        found = TRUE;
        if (pos) *pos = i - M;
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
///locateStrInFile(fileHandle, str)
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
    if (ch == '}') break;
  }

  return retval;
}
///
///readCStyleDataString(fileHandle, dest_string, skip_spaces)
/******************************************************************************
 * Reads a string of characters from the current location of a file, stopping *
 * at comma, } or " characters. This function is to read c style strings or   *
 * variable names from c sytle arays. Returns TRUE if there is more elements. *
 * The function expects file position to be at the first character of the     *
 * string to be read. If the file position is on an opening double quotes it  *
 * will skip it and read the enclosed string without skipping spaces.         *
 * Use the readString() macro for a more convenient use of this function.     *
 ******************************************************************************/
BOOL readCStyleDataString(BPTR fh, STRPTR* dest_string)
{
  UBYTE buffer[256];
  UBYTE* curs = buffer;
  LONG more_elements = -1; // -1: UNKNOWN, 0: not, 1: yes
  BOOL skip_spaces = TRUE;
  BOOL retval = FALSE;

  while (Read(fh, curs, 1) > 0) {
    if (curs == buffer && *curs == 34 /* double quote */ && skip_spaces) {
      skip_spaces = FALSE;
      continue;
    }
    if (skip_spaces && (*curs == ' ' || *curs == 9 /*tab*/ || *curs == 10 /*newline*/ || *curs == 13 /*CR*/)) {
      continue;
    }
    if (*curs == 34 /* double quote */) {
      *curs = NULL;
      break;
    }
    if (*curs == '}') {
      *curs = NULL;
      more_elements = 0;
      break;
    }
    if (*curs == ',') {
      *curs = NULL;
      more_elements = 1;
      retval = TRUE;
      break;
    }
    curs++;
  }

  if (curs == buffer) *dest_string = NULL;
  else *dest_string = makeString(buffer);

  if (more_elements == -1) {
    UBYTE ch;
    while (Read(fh, &ch, 1) > 0) {
      if (ch == ',') {
        retval = TRUE;
        break;
      }
      if (ch == '}') break;
    }
  }

  return retval;
}
///

//Utility functions for MUI
///MUI_NewButton(text)
/******************************************************************************
 * Creates a standard text button object.                                     *
 ******************************************************************************/
Object *MUI_NewButton(STRPTR text, UBYTE hi_char, STRPTR short_help)
{
  Object *obj = MUI_NewObject(MUIC_Text,
    MUIA_InputMode, MUIV_InputMode_RelVerify,
    MUIA_Frame, MUIV_Frame_Button,
    MUIA_Background, MUII_ButtonBack,
    MUIA_Font, MUIV_Font_Button,
    MUIA_Text_PreParse, g_Strings.MUI_pp_center,
    MUIA_Text_Contents, (ULONG)text,
    MUIA_ShortHelp, (ULONG)short_help,
    hi_char ? MUIA_ControlChar : TAG_IGNORE, (ULONG)hi_char,
    hi_char ? MUIA_Text_HiChar : TAG_IGNORE, (ULONG)hi_char,
  TAG_END);

  return obj;
}
///
///MUI_NewCheckMark(objPtr, state, label, key, help)
/******************************************************************************
 * Creates a standard checkbox GUI element.                                   *
 ******************************************************************************/
Object* MUI_NewCheckMark(Object** obj, BOOL state, STRPTR label, UBYTE key, STRPTR help)
{
  Object* object;

  if (label) {
    object = MUI_NewObject(MUIC_Group,
      MUIA_Group_Horiz, TRUE,
      MUIA_Group_Child, (*obj = MUI_NewObject(MUIC_Image,
        MUIA_HorizWeight, 1000,
        MUIA_Frame, MUIV_Frame_ImageButton,
        MUIA_InputMode, MUIV_InputMode_Toggle,
        MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE,
        MUIA_Selected, state,
        MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE,
        key ? MUIA_ControlChar : TAG_IGNORE, key,
      TAG_END)),
      MUIA_Group_Child, MUI_NewObject(MUIC_Text,
        MUIA_Text_Contents, label,
        MUIA_Text_HiChar, key,
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, MUIA_HorizWeight, 1, TAG_END),
      MUIA_ShortHelp, help,
    TAG_END);
  }
  else {
    object = MUI_NewObject(MUIC_Group,
      MUIA_Group_Horiz, TRUE,
      MUIA_Group_Child, (*obj = MUI_NewObject(MUIC_Image,
        MUIA_HorizWeight, 1000,
        MUIA_Frame, MUIV_Frame_ImageButton,
        MUIA_InputMode, MUIV_InputMode_Toggle,
        MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE,
        MUIA_Selected, state,
        MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE,
      TAG_END)),
      MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, MUIA_HorizWeight, 1, TAG_END),
      MUIA_ShortHelp, help,
    TAG_END);
  }

  return object;
}
///
///MUI_GetChild(object, order)
/******************************************************************************
 * Returns the n'th child of a MUI group object. (n is the order parameter)   *
 ******************************************************************************/
Object* MUI_GetChild(Object* obj, ULONG order)
{
  Object *child = NULL;
  struct Node *object_state;
  struct List* child_list = NULL;

  GetAttr(MUIA_Group_ChildList, obj, (ULONG*)&child_list);

  if (child_list) {
    ULONG i;

    object_state = child_list->lh_Head;
    for (i = 0; i < order; i++) {
      child = NextObject(&object_state);
    }
  }

  return child;
}
///
///MUI_HorizontalTitle(title)
Object* MUI_HorizontalTitle(STRPTR title)
{
  return MUI_NewObject(MUIC_Rectangle,
    MUIA_Rectangle_BarTitle, title,
    MUIA_Rectangle_HBar, TRUE,
    MUIA_FixHeight, 4,
  TAG_END);
}
///
