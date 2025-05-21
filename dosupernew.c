#if defined(__amigaos4__)
#include <intuition/intuition.h>
#include <intuition/iobsolete.h>  // <-- Required for DoSuperMethod()
#include <proto/intuition.h>
#else
#include <clib/alib_protos.h>     // <-- Required for DoSuperMethod()
#endif
#include <proto/exec.h>

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#if !defined(__MORPHOS__)
  Object* VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, ...)
  {
    Object *rc;
    VA_LIST args;

    VA_START(args, obj);
    #if defined(__AROS__)
    rc = (Object *) DoSuperNewTagList(cl, obj, NULL, (struct TagItem *) VA_ARG(args, IPTR));
    #else
    rc = (Object *) DoSuperMethod(cl, obj, OM_NEW, VA_ARG(args, ULONG), NULL);
    #endif
    VA_END(args);

    return rc;
  }
#endif // !__MORPHOS__
