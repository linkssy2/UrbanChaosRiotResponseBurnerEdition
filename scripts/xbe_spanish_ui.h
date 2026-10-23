#pragma once
#include <cstring>
#include "xbe_language.h"

namespace trial_language {
// Exact labels only: unknown text and user-provided names are never rewritten.
inline const char* Ui(const char* text){
    if(!text||Active()!=4)return text;
    static const struct {const char *english,*spanish;} labels[]={
#include "xbe_spanish_ui_data.inc"
    };
    for(const auto& row:labels)if(std::strcmp(text,row.english)==0)return row.spanish;
    return text;
}
}
