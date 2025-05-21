#ifndef TOOLTYPES_H
#define TOOLTYPES_H

//ToolTypePref types
#define TTP_END     0 //Terminator for struct ToolTypePref array
#define TTP_KEYWORD 1 //Key will be saved as {key} or ({key}) depending on data.is_true
#define TTP_BOOL    2 //Key will be saved as {key} = TRUE or FALSE depending on data.is_true
#define TTP_STRING  3 //Key will be saved as {key} = {data.string}
#define TTP_VALUE   4 //Key will be saved as {key} = {data.value}

struct ToolTypePref {
  ULONG type;
  STRPTR key;
  union {
    BOOL is_true;
    STRPTR string;
    LONG value;
  }data;
};

BOOL getTooltypes(STRPTR prog_path, struct ToolTypePref* tt_prefs);
VOID setTooltypes(STRPTR prog_path, struct ToolTypePref* tt_prefs);
VOID freeToolTypePrefStrings(struct ToolTypePref* ttprefs);

#endif /* TOOLTYPES_H */
