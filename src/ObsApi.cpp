#include "ObsApi.h"

#include <cstdarg>
#include <cstdio>

#ifdef _WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
#else
 #include <dlfcn.h>
#endif

namespace pp::obs
{
namespace
{
void* findSymbol (const char* name)
{
#ifdef _WIN32
    static HMODULE libobs = GetModuleHandleW (L"obs.dll");
    return libobs != nullptr ? reinterpret_cast<void*> (GetProcAddress (libobs, name)) : nullptr;
#else
    return dlsym (RTLD_DEFAULT, name);
#endif
}

template <typename Fn>
bool resolve (Fn& fn, const char* name)
{
    fn = reinterpret_cast<Fn> (findSymbol (name));
    return fn != nullptr;
}

// blogva quer um va_list; o jeito portátil de montar um é passar por uma função variádica
void forward (int level, const char* format, ...)
{
    va_list args;
    va_start (args, format);
    api().blogva (level, format, args);
    va_end (args);
}
} // namespace

bool Api::load()
{
    resolve (blogva, "blogva");
    // text_set_info_type é opcional: sem ela o aviso só não sai em amarelo
    resolve (property_text_set_info_type, "obs_property_text_set_info_type");

    return resolve (register_source_s, "obs_register_source_s")
        && resolve (data_get_string, "obs_data_get_string")
        && resolve (data_set_string, "obs_data_set_string")
        && resolve (get_audio_info, "obs_get_audio_info")
        && resolve (properties_create, "obs_properties_create")
        && resolve (properties_add_button2, "obs_properties_add_button2")
        && resolve (properties_add_text, "obs_properties_add_text");
}

void* moduleHandle()
{
#ifdef _WIN32
    HMODULE self = nullptr;
    GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                        reinterpret_cast<LPCWSTR> (&moduleHandle), &self);
    return self;
#else
    return nullptr;
#endif
}

Api& api()
{
    static Api instance;
    return instance;
}

void log (int level, const char* format, ...)
{
    char line[1024];
    va_list args;
    va_start (args, format);
    std::vsnprintf (line, sizeof (line), format, args);
    va_end (args);

    if (api().blogva != nullptr)
        forward (level, "[%s] %s", PP_OBS_NAME, line);
}
} // namespace pp::obs
