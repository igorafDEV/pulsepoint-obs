// Module.cpp — o módulo do OBS: registra um filtro de áudio com o nome do plugin.
//
// No OBS ele aparece em Filtros > "+" com o nome do produto (ex.: "Lift Vocal Processor"), e as
// propriedades do filtro têm um botão que abre a interface do plugin. O áudio do filtro passa
// pelo VST3 instalado pelo mesmo instalador (PluginHost.cpp).
#include "Host.h"
#include "ObsApi.h"

#include <media-io/audio-io.h>

using pp::obs::api;

OBS_DECLARE_MODULE()

MODULE_EXPORT const char* obs_module_name (void) { return PP_OBS_NAME; }
MODULE_EXPORT const char* obs_module_description (void) { return PP_OBS_NAME " - PulsePoint"; }

namespace
{
struct Filter
{
    pp::host::Handle host;
};

pp::host::Handle& hostOf (void* data) { return static_cast<Filter*> (data)->host; }

const char* getName (void*) { return PP_OBS_NAME; }

void* create (obs_data_t* settings, obs_source_t*)
{
    obs_audio_info info {};
    if (! api().get_audio_info (&info))
    {
        info.samples_per_sec = 48000;
        info.speakers = SPEAKERS_STEREO;
    }

    auto* filter = new Filter;
    filter->host = pp::host::create (api().data_get_string (settings, "state"),
                                     api().data_get_string (settings, "vst3_path"),
                                     (double) info.samples_per_sec,
                                     (int) get_audio_channels (info.speakers));
    return filter;
}

void destroy (void* data)
{
    auto* filter = static_cast<Filter*> (data);
    pp::host::release (std::move (filter->host));
    delete filter;
}

obs_audio_data* filterAudio (void* data, obs_audio_data* audio)
{
    // o OBS entrega float planar: data[canal] aponta para os frames daquele canal
    pp::host::process (*hostOf (data), reinterpret_cast<float**> (audio->data), (int) audio->frames);
    return audio;
}

void save (void* data, obs_data_t* settings)
{
    auto& host = *hostOf (data);
    api().data_set_string (settings, "state", pp::host::state (host).c_str());
    api().data_set_string (settings, "vst3_path", pp::host::chosenPath (host).c_str());
}

bool openClicked (obs_properties_t*, obs_property_t*, void* data)
{
    pp::host::openEditor (hostOf (data));
    return false;
}

bool locateClicked (obs_properties_t*, obs_property_t*, void* data)
{
    pp::host::choosePluginFile (hostOf (data));
    return false;
}

obs_properties_t* getProperties (void* data)
{
    auto* props = api().properties_create();
    auto& host = *hostOf (data);

    if (pp::host::isLoaded (host))
    {
        api().properties_add_button2 (props, "open", "Abrir " PP_OBS_NAME, openClicked, data);
        return props;
    }

    auto* warning = api().properties_add_text (props, "problem", pp::host::problem (host).c_str(), OBS_TEXT_INFO);
    if (api().property_text_set_info_type != nullptr)
        api().property_text_set_info_type (warning, OBS_TEXT_INFO_WARNING);
    api().properties_add_button2 (props, "locate", "Localizar " PP_OBS_VST3 "...", locateClicked, data);
    return props;
}
} // namespace

bool obs_module_load (void)
{
    if (! api().load())
        return false;

    pp::host::initialise (pp::obs::moduleHandle());

    obs_source_info info {};
    info.id = "pulsepoint_" PP_OBS_SLUG "_filter";   // fica gravado nas cenas: nunca mudar
    info.type = OBS_SOURCE_TYPE_FILTER;
    info.output_flags = OBS_SOURCE_AUDIO;
    info.get_name = getName;
    info.create = create;
    info.destroy = destroy;
    info.filter_audio = filterAudio;
    info.save = save;
    info.get_properties = getProperties;
    api().register_source_s (&info, sizeof (info));

    pp::obs::log (pp::obs::logInfo, "pronto (host %s)", PP_OBS_HOST_VERSION);
    return true;
}

void obs_module_unload (void)
{
    pp::host::shutdown();
}
