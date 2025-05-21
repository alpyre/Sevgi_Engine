#ifndef DOSUPERNEW_H
#define DOSUPERNEW_H

#if !defined(__MORPHOS__)
Object* VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, ...);
#endif

#endif //DOSUPERNEW_H
