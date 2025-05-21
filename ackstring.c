/******************************************************************************
 * AckString                                                                  *
 * Automatically acknowledges string contents when the gadget goes inactive!  *
 ******************************************************************************/
///Includes
#include <proto/exec.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "ackstring.h"
///
///Structs
struct cl_Data
{
  //<SUBCLASS VARIABLES HERE>
};

struct cl_Msg
{
  ULONG MethodID;
  //<SUBCLASS METHOD MESSAGE PAYLOAD HERE>
};
///

//<DEFINE SUBCLASS METHODS HERE>

///Dispatcher
SDISPATCHER(cl_Dispatcher)
{
  struct cl_Data *data;
  if (! (msg->MethodID == OM_NEW)) data = INST_DATA(cl, obj);

  switch(msg->MethodID)
  {
    case MUIM_GoInactive:
      DoMethod(obj, MUIM_Set, MUIA_String_Acknowledge, TRUE);

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_AckString(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_String, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
