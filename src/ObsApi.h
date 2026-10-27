// ObsApi.h — as poucas funções do libobs que o host usa, resolvidas em tempo de execução.
//
// O módulo não linka contra o libobs: quando o OBS carrega o módulo, o libobs já está no
// processo, e as funções são procuradas nele (dlsym no Mac, GetProcAddress no obs.dll no
// Windows). Assim o build não precisa do SDK do OBS, só dos headers em third_party/.
#pragma once

#include <obs-module.h>

#include "Log.h"

namespace pp::obs
{
struct Api
{
    void (*register_source_s) (const struct obs_source_info*, size_t) = nullptr;
    const char* (*data_get_string) (obs_data_t*, const char*) = nullptr;
    void (*data_set_string) (obs_data_t*, const char*, const char*) = nullptr;
    bool (*get_audio_info) (struct obs_audio_info*) = nullptr;
    obs_properties_t* (*properties_create)() = nullptr;
    obs_property_t* (*properties_add_button2) (obs_properties_t*, const char*, const char*,
                                               obs_property_clicked_t, void*) = nullptr;
    obs_property_t* (*properties_add_text) (obs_properties_t*, const char*, const char*,
                                            enum obs_text_type) = nullptr;
    void (*property_text_set_info_type) (obs_property_t*, enum obs_text_info_type) = nullptr;
    void (*blogva) (int, const char*, va_list) = nullptr;

    // false se faltar alguma função obrigatória (OBS antigo demais)
    bool load();
};

Api& api();

// o HMODULE deste .dll no Windows (o JUCE precisa dele para criar janelas); nullptr no Mac
void* moduleHandle();
} // namespace pp::obs
