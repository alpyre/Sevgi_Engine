///defines
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
#include <workbench/icon.h>

//Amiga protos
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include "utility.h"
#include "tooltypes.h"
///
///globals
extern APTR g_MemoryPool;
///

///getKeyStr(tooltype_str)
STRPTR getKeyStr(STRPTR tooltype_str)
{
  STRPTR key_str = NULL;
  STRPTR pos = tooltype_str;
  STRPTR str_start;
  UBYTE end_char = 0;

  while (*pos == ' ') pos++;
  str_start = pos;

  while (TRUE) {
    if (*pos == 0 || *pos == ' ' || *pos == ':' || *pos == '=') {
      end_char = *pos;
      *pos = NULL;
      key_str = makeString(str_start);
      toLower(key_str);
      *pos = end_char;
      break;
    }
    pos++;
  }

  if (end_char == ' ') {
    while (*pos == ' ') pos++;
    if (!(*pos == 0 || *pos == ':' || *pos == '=')) {
      freeString(key_str);
      key_str = NULL;
    }
  }

  return key_str;
}
///
///getValueStr(tooltype_string)
STRPTR getValueStr(STRPTR tooltype_str)
{
  STRPTR value_str = NULL;
  STRPTR pos = tooltype_str;
  BOOL separator_not_met = TRUE;

  while (*pos) {
    if (*pos == ' ') {
      pos++;
      continue;
    }
    if (separator_not_met && (*pos == '=' || *pos == ':')) {
      separator_not_met = FALSE;
      pos++;
      continue;
    }
    break;
  }

  if (*pos == '"') {
    STRPTR str_start = ++pos;

    while (*pos && *pos != '"') pos++;

    if (*pos == '"') {
      *pos = NULL;
      value_str = makeString(str_start);
      *pos = '"';
    }
    else value_str = makeString(str_start - 1);
  }
  else if (*pos == 39 /* single quote */) {
    STRPTR str_start = ++pos;

    while (*pos && *pos != 39) pos++;

    if (*pos == 39) {
      *pos = NULL;
      value_str = makeString(str_start);
      *pos = 39;
    }
    else value_str = makeString(str_start - 1);
  }
  else {
    value_str = makeString(pos);
  }

  return value_str;
}
///
///matchKeys(key1, key2)
BOOL matchKeys(STRPTR key1, STRPTR key2)
{
  BOOL match = FALSE;

  if (key1 && *key1) {
    if (!strcmp(key1, key2)) match = TRUE;
    else if (*key1 == '(' && !strcmp(key1 + 1, key2)) match = TRUE;
  }

  return match;
}
///
///makeToolType(ttpref)
STRPTR makeToolType(struct ToolTypePref* ttpref)
{
  STRPTR tooltype = NULL;
  UBYTE buffer[MAX_STR_LENGTH];

  switch (ttpref->type) {
    case TTP_KEYWORD:
      if (ttpref->data.is_true) tooltype = makeString(ttpref->key);
      else tooltype = makeString3("(", ttpref->key, ")");
    break;
    case TTP_BOOL:
      sprintf(buffer, "%s = %s", ttpref->key, ttpref->data.is_true ? "TRUE" : "FALSE");
      tooltype = makeString(buffer);
    break;
    case TTP_STRING:
      sprintf(buffer, "%s = %s", ttpref->key, ttpref->data.string);
      tooltype = makeString(buffer);
    break;
    case TTP_VALUE:
      sprintf(buffer, "%s = %ld", ttpref->key, ttpref->data.value);
      tooltype = makeString(buffer);
    break;
  }

  return tooltype;
}
///

///getTooltypes(prog_path, ttprefs[])
BOOL getTooltypes(STRPTR prog_path, struct ToolTypePref* ttprefs)
{
  BOOL retval = FALSE;
  struct DiskObject* prog_icon = GetDiskObject(prog_path);

  if (prog_icon) {
    STRPTR *tooltypes = prog_icon->do_ToolTypes;
    BOOL* read;
    ULONG num_ttprefs;
    LONG i = 0;

    i = 0;
    while (ttprefs[i++].type);
    num_ttprefs = i;

    read = (BOOL*)AllocPooled(g_MemoryPool, sizeof(BOOL) * num_ttprefs);
    if (read) {
      i = 0;
      while (tooltypes[i]) {
        LONG l = 0;
        STRPTR key = getKeyStr(tooltypes[i]);

        if (key) {
          while (ttprefs[l].type) {
            if (!strcmp(key, ttprefs[l].key)) {
              STRPTR value_str = getValueStr(&tooltypes[i][strlen(key)]);

              switch (ttprefs[l].type) {
                case TTP_KEYWORD:
                case TTP_BOOL:
                toLower(value_str);
                if (!strncmp(value_str, "false", 5) || *value_str == '0')
                ttprefs[l].data.is_true = FALSE;
                else
                ttprefs[l].data.is_true = TRUE;
                freeString(value_str);
                break;
                case TTP_STRING:
                ttprefs[l].data.string = value_str;
                read[l] = TRUE;
                break;
                case TTP_VALUE:
                aToi(value_str, &ttprefs[l].data.value);
                freeString(value_str);
                break;
              }

              break;
            }
            l++;
          }
          freeString(key);
        }
        i++;
      }

      //Create memory allocated copies of the default strings for the unfound
      //keys so that a call to freeToolTypePrefStrings() does not crash the
      //system if default strings were allocated on the program stack!
      i = 0;
      while (ttprefs[i].type) {
        if (ttprefs[i].type == TTP_STRING && ttprefs[i].data.string && !read[i]) {
          ttprefs[i].data.string = makeString(ttprefs[i].data.string);
        }
        i++;
      }

      retval = TRUE;
      FreePooled(g_MemoryPool, read, sizeof(BOOL) * num_ttprefs);
    }

    FreeDiskObject(prog_icon);
  }

  return retval;
}
///
///setTooltypes(prog_path, ttprefs[])
VOID setTooltypes(STRPTR prog_path, struct ToolTypePref* ttprefs)
{
  struct DiskObject* prog_icon = GetDiskObject(prog_path);

  if (prog_icon) {
    STRPTR *tooltypes = prog_icon->do_ToolTypes;
    STRPTR *new_tooltypes = NULL;
    ULONG i = 0;
    ULONG num_tooltypes = 0;
    ULONG num_ttprefs = 0;

    while(tooltypes[i++]);
    num_tooltypes = i;
    i = 0;
    while (ttprefs[i++].type);
    num_ttprefs = i;

    new_tooltypes = (STRPTR*)AllocPooled(g_MemoryPool, sizeof(STRPTR) * num_tooltypes + num_ttprefs);
    if (new_tooltypes) {
      BOOL* saved = (BOOL*)AllocPooled(g_MemoryPool, sizeof(BOOL) * num_ttprefs);
      if (saved) {
        ULONG j = 0;
        i = 0;

        while (tooltypes[i]) {
          STRPTR key = getKeyStr(tooltypes[i]);
          ULONG l = 0;
          BOOL found = FALSE;
          while (ttprefs[l].type) {
            if (matchKeys(key, ttprefs[l].key)) {
              new_tooltypes[j++] = makeToolType(&ttprefs[l]);
              found = TRUE;
              saved[l] = TRUE;
              break;
            }
            l++;
          }

          if (!found) {
            new_tooltypes[j++] = makeString(tooltypes[i]);
          }

          freeString(key);
          i++;
        }

        i = 0;
        while (ttprefs[i].type) {
          if (!saved[i]) {
            new_tooltypes[j++] = makeToolType(&ttprefs[i]);
          }
          i++;
        }

        prog_icon->do_ToolTypes = new_tooltypes;
        PutDiskObject(prog_path, prog_icon);
        prog_icon->do_ToolTypes = tooltypes;

        FreePooled(g_MemoryPool, saved, sizeof(BOOL) * num_ttprefs);
      }

      //free new_tooltypes
      i = 0;
      while (new_tooltypes[i]) {
        freeString(new_tooltypes[i]);
        i++;
      }
      FreePooled(g_MemoryPool, new_tooltypes, sizeof(STRPTR) * num_tooltypes + num_ttprefs);
    }

    FreeDiskObject(prog_icon);
  }
}
///
///freeToolTypePrefStrings(ttprefs[])
/******************************************************************************
 * WARNING: Calling this assumes all the strings on the ToolTypePref array    *
 * were allocated on the g_MemoryPool or NULL. A succesful getTooltypes()     *
 * guarantees this. So beware not to call this if getTooltypes() returns      *
 * FALSE. BTW, we can safely ignore calling this if we do the getTooltypes()  *
 * once at program start, and setTooltypes() at program quit.                 *
 ******************************************************************************/
VOID freeToolTypePrefStrings(struct ToolTypePref* ttprefs)
{
  ULONG i = 0;

  while (ttprefs[i].type) {
    if (ttprefs[i].type == TTP_STRING) {
      freeString(ttprefs[i].data.string); ttprefs[i].data.string = NULL;
    }
    i++;
  }
}
///
