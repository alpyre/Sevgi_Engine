/******************************************************************************
 * ConvertMap                                                                 *
 * Takes a JSON tilemap file created by Tiled, parses it and creates .map and *
 * .obj (Gameobject bank) files from the layers found within.                 *
 * The format of the .map file used by the engine is simply two UWORDS of map *
 * width and height values followed by an array of UWORDS for each tile-id.   *
 * It will create the proper Gameobject bank with custom members if the       *
 * gameobject.h file is passed.                                               *
 * Will create a drawer with the filename of the .js file and save the .map   *
 * and .obj files into it using the names of the layers given in Tiled.       *
 * If you don't have a blank tile at the beginning of the tileset you use in  *
 * Tiled, you have to use ADDZERO to create it as you convert the tileset for *
 * the engine. And if you've used ADDZERO in tileset conversion, you also     *
 * have to use ADDZERO here for the tilemap to be compatible with the tileset.*
 * TODO: Speed up header file analyze!                                        *
 * TODO: Write some error strings for checked error conditions!               *
 ******************************************************************************/

///defines
#define PROGRAMNAME     "ConvertMap"
#define VERSION         0
#define REVISION        9
#define VERSIONSTRING   "0.9"

//define command line syntax and number of options
#define RDARGS_TEMPLATE "F=File/A, P=Path/K, H=Header/K, Z=ADDZERO/S"
#define RDARGS_OPTIONS  4

enum {
  OPT_FILE,
  OPT_PATH,
  OPT_HEADER,
  OPT_ADDZERO
};

//#define or #undef GENERATEWBMAIN to enable workbench startup
#define GENERATEWBMAIN

#define UNKNOWN_TYPE 0xFFFFFFFF
///
///includes
//standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

//Amiga headers
#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/layers.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <datatypes/pictureclass.h>
#include <libraries/asl.h>
#include <libraries/commodities.h>
#include <libraries/gadtools.h>
#include <libraries/iffparse.h>
#include <libraries/locale.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <utility/hooks.h>

//Amiga protos
#include <clib/alib_protos.h>
#include <proto/asl.h>
#include <proto/commodities.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/locale.h>
#include <proto/rexxsyslib.h>
#include <proto/utility.h>
#include <proto/wb.h>

#include "utility.h"
///
///structs
/***********************************************
* Global configuration struct for this program *
************************************************/
struct Config
{
  struct RDArgs *RDArgs;

  //command line options
  #if RDARGS_OPTIONS
  LONG Options[RDARGS_OPTIONS];
  #endif

  //<YOUR GLOBAL DATA HERE>

};

enum TypeIDs {
  UBYTE_T,
  BYTE_T,
  UWORD_T,
  WORD_T,
  LONG_T,
  ULONG_T,
  FLOAT_T,
  DOUBLE_T,
  BOOL_T
};

struct Member {
  struct MinNode node;
  STRPTR name;
  ULONG type;
  ULONG size;
  union {
    UBYTE  ubyte_v;
    BYTE   byte_v;
    UWORD  uword_v;
    WORD   word_v;
    LONG   long_v;
    ULONG  ulong_v;
    FLOAT  float_v;
    DOUBLE double_v;
    BOOL   bool_v;
  }storage;
};

///
///globals
/***********************************************
* Version string for this program              *
************************************************/
#if defined(__SASC)
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " "  __AMIGADATE__ "\n\0";
#elif defined(_DCC)
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __COMMODORE_DATE__ ")\n\0";
#elif defined(__GNUC__)
__attribute__((section(".text"))) volatile static const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __DATE__ ")\n\0";
#else
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __DATE__ ")\n\0";
#endif

APTR g_MemoryPool = NULL;
struct MinList g_members;

STRPTR g_filename = NULL;
STRPTR g_output_path = NULL;

struct StorageType {
  STRPTR type;
  ULONG  id;
  ULONG  size;
}g_storage_type[] = {{"BYTE", BYTE_T, 1},
                     {"UBYTE", UBYTE_T, 1},
                     {"WORD", WORD_T, 2},
                     {"UWORD", UWORD_T, 2},
                     {"LONG", LONG_T, 4},
                     {"ULONG", ULONG_T, 4},
                     {"FLOAT", FLOAT_T, 4},
                     {"DOUBLE", DOUBLE_T, 8},
                     {"BOOL", BOOL_T, 2},
                     {"char", BYTE_T, 1},
                     {"unsigned char", UBYTE_T, 1},
                     {"short", WORD_T, 2},
                     {"unsigned short", UWORD_T, 2},
                     {"long", LONG_T, 4},
                     {"unsigned long", ULONG_T, 4},
                     {"double", DOUBLE_T, 8},
                     {"bool", BOOL_T, 2},
                     {"int", LONG_T, 4},
                     {"unsigned int", ULONG_T, 4},
                     {"int8_t", BYTE_T, 1},
                     {"uint8_t", UBYTE_T, 1},
                     {"int16_t", WORD_T, 2},
                     {"uint16_t", UWORD_T, 2},
                     {"int32_t", LONG_T, 4},
                     {"uint32_t", ULONG_T, 4},
                     {"float", FLOAT_T, 4}
                   };

struct DefaultMembers {
  STRPTR name;
  ULONG type;
  ULONG size;
}g_default_members[] = {{"x",                LONG_T,  4},
                        {"y",                LONG_T,  4},
                        {"width",            UWORD_T, 2},
                        {"height",           UWORD_T, 2},
                        {"type",             UBYTE_T, 1},
                        {"state",            UBYTE_T, 1},
                        {"me_mask",          UBYTE_T, 1},
                        {"hit_mask",         UBYTE_T, 1},
                        {"collide_func",     UBYTE_T, 1},
                        {"collideTile_func", UBYTE_T, 1},
                        {"anim_type",        UBYTE_T, 1},
                        {"anim_state",       UBYTE_T, 1},
                        {"anim_frame",       UBYTE_T, 1},
                        {"anim_func",        UBYTE_T, 1},
                        {"image",            UWORD_T, 2},
                        {"bank",             UBYTE_T, 1},
                        {"priority",         BYTE_T,  1}
                      };
///
///prototypes
/***********************************************
* Function forward declarations                *
************************************************/
int            main   (int argc, char **argv);
int            wbmain (struct WBStartup *wbs);
struct Config *Init   (void);
int            Main   (struct Config *config);
void           CleanUp(struct Config *config);

BOOL evaluateOptions(struct Config* config);
STRPTR getStringByID(UBYTE* layer, STRPTR id);
BOOL getValueByID(UBYTE* layer, STRPTR id, LONG* value);
UWORD getValue(UBYTE** buffer);
BOOL locateObjStart(UBYTE** buffer);
UBYTE* locateObjEnd(UBYTE* buffer);
VOID resetMembers();
VOID getMemberValues(UBYTE* object);
ULONG getObjectCount(UBYTE* object);
VOID writeMemberValues(BPTR fh);
///
///init
/***********************************************
* Program initialization                       *
* - Allocates the config struct to store the   *
*   global configuration data.                 *
* - Do your other initial allocations here.    *
************************************************/
struct Config *Init()
{
  struct Config *config;

  #if defined(__amigaos4__)
  g_MemoryPool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_ANY | MEMF_CLEAR,
                                                  ASOPOOL_Puddle, 2048,
                                                  ASOPOOL_Threshold, 2048,
                                                  ASOPOOL_Name, "Sevgi_Editor pool",
                                                  ASOPOOL_LockMem, FALSE,
                                                  TAG_DONE);
  #else
  g_MemoryPool = CreatePool(MEMF_ANY | MEMF_CLEAR, 2048, 2048);
  #endif

  config = (struct Config*)AllocPooled(g_MemoryPool, sizeof(struct Config));

  if (config)
  {
    NewList((struct List*)&g_members);
  }

  return(config);
}
///
///entry
/***********************************************
 * Ground level entry point                    *
 * - Branches regarding Shell/WB call.         *
 ***********************************************/
int main(int argc, char **argv)
{
  int rc = 20;

  //argc != 0 identifies call from shell
  if (argc)
  {
    struct Config *config = Init();

    if (config)
    {
      #if RDARGS_OPTIONS
        // parse command line arguments
        if ((config->RDArgs = ReadArgs(RDARGS_TEMPLATE, config->Options, NULL)))
          rc = Main(config);
        else
          PrintFault(IoErr(), PROGRAMNAME);
      #else
        rc = Main(config);
      #endif

      CleanUp(config);
    }
  }
  else
    rc = wbmain((struct WBStartup *)argv);

  return(rc);
}

/***********************************************
 * Workbench main                              *
 * - This executable was called from Workbench *
 ***********************************************/
int wbmain(struct WBStartup *wbs)
{
  int rc = 20;

  #ifdef GENERATEWBMAIN
    struct Config *config = Init();

    if (config)
    {
      //<SET Config->Options[] HERE>

      rc = Main(config);

      CleanUp(config);
    }
  #endif

  return(rc);
}
///
///main
/***********************************************
 * Developer level main                        *
 * - Code your program here.                   *
 ***********************************************/
int Main(struct Config *config)
{
  int rc = 0;
  ULONG num_tilemaps = 0;
  ULONG num_objectbanks = 0;

  if (evaluateOptions(config)) {
    BPTR fh = Open((STRPTR)config->Options[OPT_FILE], MODE_OLDFILE);

    if (fh) {
      if (locateStrInFile(fh, "\"layers\":[")) {
        BPTR lock = NULL;

        if (Exists(g_output_path)) lock = Lock(g_output_path, EXCLUSIVE_LOCK);
        else lock = CreateDir(g_output_path);

        if (lock) {
          while (locateArrayStart(fh)) {
            LONG fh_layer_start = Seek(fh, 0, OFFSET_CURRENT);
            LONG fh_layer_end;
            LONG layer_size;
            UBYTE* layer_contents;

            locateArrayEnd(fh);
            fh_layer_end = Seek(fh, 0, OFFSET_CURRENT);
            layer_size = fh_layer_end - fh_layer_start;
            layer_contents = AllocPooled(g_MemoryPool, layer_size + 1);
            if (layer_contents) {
              Seek(fh, fh_layer_start, OFFSET_BEGINNING);
              Read(fh, layer_contents, layer_size);

              if (searchString(layer_contents, "\"type\":\"tilelayer\"")) {
                STRPTR name = getStringByID(layer_contents, "name");
                if (name) {
                  STRPTR file_name = NULL;
                  LONG width_l = 0;
                  LONG height_l = 0;

                  replaceChars(name, DOS_RESERVED DOS_UNRECOMMENDED, '_');

                  file_name = makePath(g_output_path, name, ".map");
                  freeString(name);
                  getValueByID(layer_contents, "width", &width_l);
                  getValueByID(layer_contents, "height", &height_l);

                  if (file_name && width_l && height_l) {
                    UWORD width = width_l;
                    UWORD height = height_l;
                    BPTR fh_s = Open(file_name, MODE_READWRITE);
                    freeString(file_name);

                    if (fh_s) {
                      UBYTE* buffer = searchString(layer_contents, "\"data\":[");
                      ULONG i;
                      ULONG i_max;

                      if (buffer) {
                        Write(fh_s, &width, 2);
                        Write(fh_s, &height, 2);

                        for (i = 0, i_max = width * height; i < i_max; i++) {
                          UWORD value = getValue(&buffer);
                          // Tiled saves the first tile (tile 0) with id 1 and an empty tile with id 0
                          // Below line converts it to engine requirements
                          if (value && !config->Options[OPT_ADDZERO]) value--;
                          Write(fh_s, &value, 2);
                        }
                      }

                      num_tilemaps++;
                      SetFileSize(fh_s, 0, OFFSET_CURRENT);
                      Close(fh_s);
                    }
                  }
                }

              }
              else if (searchString(layer_contents, "\"type\":\"objectgroup\"")) {
                //this is an objects layer
                STRPTR name = getStringByID(layer_contents, "name");
                if (name) {
                  STRPTR file_name = NULL;

                  replaceChars(name, DOS_RESERVED DOS_UNRECOMMENDED, '_');
                  file_name = makePath(g_output_path, name, ".obj");
                  freeString(name);
                  if (file_name) {
                    UBYTE* object = searchString(layer_contents, "\"objects\":[");

                    if (object) {
                      ULONG num_gameobjects = getObjectCount(object);

                      if (num_gameobjects) {
                        BPTR fh_s = Open(file_name, MODE_READWRITE);
                        freeString(file_name);

                        if (fh_s) {
                          Write(fh_s, "GAMEOBJ", 8);
                          Write(fh_s, &num_gameobjects, sizeof(num_gameobjects));

                          while (locateObjStart(&object)) {
                            UBYTE* object_end = locateObjEnd(object);
                            *object_end = NULL;

                            resetMembers();
                            getMemberValues(object);

                            {
                              UBYTE* gameobject_start = searchString(object, "\"propertytype\":\"Gameobject\"");
                              if (gameobject_start) {
                                if (locateObjStart(&gameobject_start))
                                {
                                  UBYTE* gameobject_end = locateObjEnd(gameobject_start);
                                  *gameobject_end = NULL;

                                  getMemberValues(gameobject_start);

                                  *gameobject_end = '}';
                                }
                              }
                            }

                            writeMemberValues(fh_s);

                            *object_end = '}';
                            object = object_end + 1;
                          }

                          num_objectbanks++;
                          SetFileSize(fh_s, 0, OFFSET_CURRENT);
                          Close(fh_s);
                        }
                      }
                    }
                  }
                }
              }

              FreePooled(g_MemoryPool, layer_contents, layer_size + 1);
            }
          }

          UnLock(lock);
        }
      }

      Close(fh);
    }
  }
  else rc = 20;

  printf("Tilemap file: %s\nConverted layers:\n  Tilemaps: %lu\n  Objects: %lu\nOutput directory: %s/\n",
          FilePart((STRPTR)config->Options[OPT_FILE]), num_tilemaps, num_objectbanks, g_output_path);

  return(rc);
}
///
///cleanup
/***********************************************
 * Clean up before exit                        *
 * - Free allocated resources here.            *
 ***********************************************/
void CleanUp(struct Config *config)
{
  if (config)
  {
    //Everything is pooled so we can ditch this part
    /*
    struct Member* member, *next_member;

    for ((struct Member*)g_members.mlh_Head; (next_member = member->node.mln_Succ); member = next_member) {
      //Remove((struct Node*)member);
      freeMember(member);
    }
    */

    // free command line arguments
    #if RDARGS_OPTIONS
      if (config->RDArgs)
        FreeArgs(config->RDArgs);
    #endif

    FreePooled(g_MemoryPool, config, sizeof(struct Config));

    if (g_MemoryPool) {
      #if defined(__amigaos4__)
      FreeSysObject(ASOT_MEMPOOL, g_MemoryPool);
      #else
      DeletePool(g_MemoryPool);
      #endif
    }
  }
}
///

///newMember(name, type, size)
/******************************************************************************
 * Allocates and initializes a new struct Member item in memory pool.         *
 ******************************************************************************/
struct Member* newMember(STRPTR name, ULONG type, ULONG size)
{
  struct Member* member = (struct Member*)AllocPooled(g_MemoryPool, sizeof(struct Member));

  if (member) {
    member->name = name;
    member->type = type;
    member->size = size;
  }

  return member;
}
///
///freeMember(member)
/******************************************************************************
 * Frees the memory allocated by newMember().                                 *
 ******************************************************************************/
VOID freeMember(struct Member* member)
{
  if (member) {
    freeString(member->name);
    FreePooled(g_MemoryPool, member, sizeof(struct Member));
  }
}
///
///findType(str)
/******************************************************************************
 * Searches the storage type string extracted from struct GameObjectData in   *
 * the known storage types array g_storage_type[].                            *
 * Will return the enum for the type, will return UNKNOWN_TYPE if the type is *
 * illegal.                                                                   *
 ******************************************************************************/
ULONG findType(STRPTR str) {
  ULONG i;
  ULONG i_max = sizeof(g_storage_type) / sizeof(struct StorageType);
  ULONG retval = UNKNOWN_TYPE;

  if (str) {
    for (i = 0; i < i_max; i++) {
      if (!strcmp(str, g_storage_type[i].type)) {
        retval = i;
        break;
      }
    }
  }

  return retval;
}
///
///getMembersFromHeaderFile(filename)
/******************************************************************************
 * Locates and parses the struct GameObjectData defined in the gameobject.h   *
 * file passed and populates the g_members list analyzing it.                 *
 * Will return FALSE if the file is invalid, the struct has illegal members   *
 * or some syntax error.                                                      *
 ******************************************************************************/
BOOL getMembersFromHeaderFile(STRPTR filename)
{
  STRPTR type_str = NULL;
  STRPTR var_str = NULL;
  BPTR fh = Open(filename, MODE_OLDFILE);
  if (fh) {
    if (locateStrInFile(fh, "struct GameObjectData")) {
      if (locateArrayStart(fh)) {
        LONG read_result;
        ULONG type_index;

        do {
          read_result = readDefinition(fh, &type_str, MODE_TYPE);
          if (read_result == SYNTAX_ERROR) goto syntax_error;
          if (read_result == NO_MORE_DEFINITIONS) break;

          type_index = findType(type_str);
          if (type_index == UNKNOWN_TYPE) goto syntax_error;
          freeString(type_str); type_str = NULL;

          do {
            struct Member* member;

            read_result = readDefinition(fh, &var_str, MODE_VARIABLE);
            if (read_result == SYNTAX_ERROR) goto syntax_error;

            if (read_result > 0) {
              ULONG i = 0;
              STRPTR arr_str = var_str;

              while (TRUE) {
                UBYTE buf[16];

                sprintf(buf, "_%lu", ++i);
                member = newMember(arr_str, g_storage_type[type_index].id, g_storage_type[type_index].size);
                if (member) AddTail((struct List*)&g_members, (struct Node*)member);
                else goto memory_error;

                if (i < read_result) arr_str = makeString2(var_str, buf);
                else break;
              }
            }
            else
            {
              member = newMember(var_str, g_storage_type[type_index].id, g_storage_type[type_index].size);
              if (member) AddTail((struct List*)&g_members, (struct Node*)member);
              else goto memory_error;
            }
          } while(read_result == MORE_VARIABLES);

          if (read_result == SYNTAX_ERROR) goto syntax_error;
        } while(read_result == MORE_DEFINITIONS);
      }
    }

    Close(fh);
  }
  else return FALSE;

  return TRUE;

syntax_error:
  Close(fh);
  freeString(type_str);
  freeString(var_str);
  puts("Header file has a syntax error!");
  return FALSE;

memory_error:
  Close(fh);
  freeString(var_str);
  puts("Memory error!");
  return FALSE;
}
///
///evaluateOptions(config)
/******************************************************************************
 * Checks for the validity of the arguments passed by ReadArgs().             *
 * Does the necesseary initializations to the program regarding the values.   *
 ******************************************************************************/
BOOL evaluateOptions(struct Config* config)
{
  BOOL retval = TRUE;

  if (Exists((STRPTR)config->Options[OPT_FILE])) {
    g_filename = stripExtension(FilePart((STRPTR)config->Options[OPT_FILE]));
    if (g_filename) {
      if (config->Options[OPT_PATH]) {
        g_output_path = makePath((STRPTR)config->Options[OPT_PATH], g_filename, NULL);
      }
      else {
        STRPTR file_path = pathPart((STRPTR)config->Options[OPT_FILE]);
        g_output_path = makePath(file_path, g_filename, NULL);
        freeString(file_path);
      }

      if (g_output_path) {
        if (config->Options[OPT_HEADER]) {
          if (getMembersFromHeaderFile((STRPTR)config->Options[OPT_HEADER])) {

          }
          else retval = FALSE;
        }
        else {
          //create the default members list
          ULONG i;
          ULONG i_max = sizeof(g_default_members) / sizeof(struct DefaultMembers);

          for (i = 0; i < i_max; i++) {
            struct Member* member = newMember(makeString(g_default_members[i].name), g_default_members[i].type, g_default_members[i].size);
            if (member) {
              AddTail((struct List*)&g_members, (struct Node*)member);
            }
          }
        }
      }
      else {
        retval = FALSE;
        puts("Invalid output path!");
      }
    }
    else {
      puts("Invalid source file!");
      retval = FALSE;
    }
  }
  else {
    retval = FALSE;
    puts("Invalid source file!");
  }

  return retval;
}
///
///getStringByID(layer, id)
/******************************************************************************
 * Searches the given JSON id in the memory pointer passed in 'layer'.        *
 * Returns the value of the id in a newly allocated string.                   *
 * String values enclosed in double quotes will be stripped of them.          *
 ******************************************************************************/
STRPTR getStringByID(UBYTE* layer, STRPTR id)
{
  STRPTR string = NULL;
  STRPTR id_phrase = makeString3("\"", id, "\":");

  if (id_phrase) {
    ULONG len = 0;
    BOOL in_quotes;
    UBYTE* start = searchString(layer, id_phrase);
    UBYTE* curs = start;
    UBYTE end_char = NULL;

    if (*curs == '"') {
      in_quotes = TRUE;
      start++;
      curs++;
    }

    while (*curs) {
      if (*curs == '"' || *curs == 10 || *curs == ',') {
        end_char = *curs;
        break;
      }

      curs++;
      len++;
    }

    if (len) {
      if (*curs) {
        *curs = NULL;
        string = makeString(start);
        *curs = end_char;
      }
      else {
        string = makeString(start);
      }
    }

    freeString(id_phrase);
  }

  return string;
}
///
///getValueByID(layer, id, &value)
/******************************************************************************
 * Searches the given JSON id in the memory pointer passed in 'layer'.        *
 * Returns TRUE if the id is present in the layer.                            *
 * The value will be set to the LONG variable passed as reference in arg 3.   *
 ******************************************************************************/
BOOL getValueByID(UBYTE* layer, STRPTR id, LONG* value)
{
  STRPTR value_str = getStringByID(layer, id);
  if (value_str) {
    if (!strcmp(value_str, "true")) *value = 1;
    else aToi(value_str, value);

    freeString(value_str);

    return TRUE;
  }

  return FALSE;
}
///
///getValue(&buffer)
/******************************************************************************
 * This function is used to get tile ids from the data array in a tilemap     *
 * layer. buffer (passed as reference) should point to the start of the first *
 * id in memory. It will return the value and set the buffer to point to to   *
 * the next value in the array.                                               *
 ******************************************************************************/
UWORD getValue(UBYTE** buffer)
{
  ULONG value = 0;
  UBYTE* curs = *buffer;
  UBYTE end_char = NULL;

  while (*curs) {
    if (*curs == ',' || *curs == ']') {
      end_char = *curs;
      break;
    }
    curs++;
  }

  *curs = NULL;
  aToi(*buffer, &value);
  *curs = end_char;

  *buffer = curs + 1;

  return (UWORD)value;
}
///
///locateObjStart(&buffer)
/******************************************************************************
 * Sets the buffer pointer (passed as reference) to the next character after  *
 * the first encountered '{'.                                                 *
 * Returns FALSE if it meets a ']' before finding a '{'.                      *
 ******************************************************************************/
BOOL locateObjStart(UBYTE** buffer)
{
  BOOL retval = FALSE;
  UBYTE* curs = *buffer;

  while (*curs) {
    if (*curs == ']') {
      break;
    }
    if (*curs == '{') {
      retval = TRUE;
      curs++;
      break;
    }
    curs++;
  }

  *buffer = curs;

  return retval;
}
///
///locateObjEnd(buffer)
/******************************************************************************
 * Returns the address in the memory where this JSON object section ends.     *
 * Searches the '}' that ends current object pointed by buffer. If it finds   *
 * nested objects, it skips them.                                             *
 ******************************************************************************/
UBYTE* locateObjEnd(UBYTE* buffer)
{
  UBYTE* curs = buffer;
  ULONG nested = 0;

  while (*curs) {
    if (*curs == '{') nested++;
    if (*curs == '}') {
      if (nested) nested--;
      else break;
    }
    curs++;
  }

  return curs;
}
///
///resetMembers()
/******************************************************************************
 * Sets all struct member values enlisted in g_members list to zeroes.        *
 ******************************************************************************/
VOID resetMembers()
{
  struct Member* member;

  for (member = (struct Member*)g_members.mlh_Head; member->node.mln_Succ; member = (struct Member*)member->node.mln_Succ) {
    member->storage.ulong_v = 0UL;
  }
}
///
///getMemberValues(object)
/******************************************************************************
 * Searches all the id strings in g_members, gets their values from the JSON  *
 * object and stores them in their storages in g_members regarding their      *
 * storage types.                                                             *
 ******************************************************************************/
VOID getMemberValues(UBYTE* object)
{
  struct Member* member;
  LONG value = 0;

  for (member = (struct Member*)g_members.mlh_Head; member->node.mln_Succ; member = (struct Member*)member->node.mln_Succ) {
    if (getValueByID(object, member->name, &value)) {
      switch (member->type) {
        case UBYTE_T:
          member->storage.ubyte_v = (UBYTE)value;
        break;
        case BYTE_T:
          member->storage.byte_v = (BYTE)value;
        break;
        case UWORD_T:
          member->storage.uword_v = (UWORD)value;
        break;
        case WORD_T:
          member->storage.word_v = (WORD)value;
        break;
        case LONG_T:
          member->storage.long_v = (LONG)value;
        break;
        case ULONG_T:
          member->storage.ulong_v = (ULONG)value;
        break;
        case FLOAT_T:
        {
          STRPTR double_str = getStringByID(object, member->name);
          DOUBLE value = 0;
          if (double_str) {
            value = aTof(double_str);
            freeString(double_str);
          }

          member->storage.float_v = (FLOAT)value;
        }
        break;
        case DOUBLE_T:
        {
          STRPTR double_str = getStringByID(object, member->name);
          DOUBLE value = 0;
          if (double_str) {
            value = aTof(double_str);
            freeString(double_str);
          }

          member->storage.double_v = (DOUBLE)value;
        }
        break;
        case BOOL_T:
          member->storage.bool_v = (BOOL)value;
        break;
      }
    }
  }
}
///
///getObjectCount(object)
/******************************************************************************
 * Counts and returns the number of of objects within an JSON object array.   *
 ******************************************************************************/
ULONG getObjectCount(UBYTE* object)
{
  UBYTE* buffer = object;
  ULONG count = 0;

  while (locateObjStart(&buffer)) {
    count++;
    buffer = locateObjEnd(buffer) + 1;
  }

  return count;
}
///
///writeMemberValues(filehandle)
/******************************************************************************
 * Writes all the values in g_members, regarding their storage types and      *
 * paddings into the given file handle to the gameobject bank file.           *
 ******************************************************************************/
VOID writeMemberValues(BPTR fh)
{
  ULONG pos = 0;
  struct Member* member = NULL;

  for (member = (struct Member*)g_members.mlh_Head; member->node.mln_Succ; member = (struct Member*)member->node.mln_Succ) {
    if ((member->size > 1) && (pos % 2)) {
      UBYTE pad = 0;
      Write(fh, &pad, 1);
      pos++;
    }
    Write(fh, &member->storage, member->size);

    /*
    switch (member->type) {
      case UBYTE_T:
        printf("%s: %lu\n", member->name, (ULONG)member->storage.ubyte_v);
      break;
      case BYTE_T:
        printf("%s: %ld\n", member->name, (LONG)member->storage.byte_v);
      break;
      case UWORD_T:
        printf("%s: %lu\n", member->name, (ULONG)member->storage.uword_v);
      break;
      case WORD_T:
        printf("%s: %ld\n", member->name, (LONG)member->storage.word_v);
      break;
      case LONG_T:
        printf("%s: %ld\n", member->name, member->storage.long_v);
      break;
      case ULONG_T:
        printf("%s: %lu\n", member->name, member->storage.ulong_v);
      break;
      case FLOAT_T:
      break;
      case DOUBLE_T:
      break;
      case BOOL_T:
      break;
    }
    */

    pos += member->size;
  }
}
///
