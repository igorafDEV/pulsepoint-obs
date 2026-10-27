# Um módulo do OBS por plugin PulsePoint.
#   NAME  o nome que aparece no OBS, igual ao PRODUCT_NAME do plugin
#   VST3  o bundle que o instalador do plugin põe na pasta padrão de VST3 (<NAME>.vst3)
#
# O slug (primeiro argumento) é o mesmo do plugin e entra no id do filtro gravado nas cenas
# do OBS: não muda depois de publicado.

pp_obs_host(air       NAME "Air Adaptive Exciter"         VST3 "Air Adaptive Exciter.vst3")
pp_obs_host(echo      NAME "Echo Creative Delay"          VST3 "Echo Creative Delay.vst3")
pp_obs_host(final     NAME "Final Master"                 VST3 "Final Master.vst3")
pp_obs_host(focus     NAME "Focus Parametric EQ"          VST3 "Focus Parametric EQ.vst3")
pp_obs_host(grip      NAME "Grip Dynamic Compressor"      VST3 "Grip Dynamic Compressor.vst3")
pp_obs_host(lift      NAME "Lift Vocal Processor"         VST3 "Lift Vocal Processor.vst3")
pp_obs_host(pulsekey  NAME "PulseKey Pitch Detector"      VST3 "PulseKey Pitch Detector.vst3")
pp_obs_host(pulsetune NAME "PulseTune Pitch Correction"   VST3 "PulseTune Pitch Correction.vst3")
pp_obs_host(push      NAME "Push Advanced Clipper"        VST3 "Push Advanced Clipper.vst3")
pp_obs_host(shield    NAME "SHIELD Noise Gate"            VST3 "SHIELD Noise Gate.vst3")
pp_obs_host(silk      NAME "SILK Intelligent De-Esser"    VST3 "SILK Intelligent De-Esser.vst3")
pp_obs_host(space     NAME "Space Creative Reverb"        VST3 "Space Creative Reverb.vst3")
pp_obs_host(voxa      NAME "VOXA Vocal Correct"           VST3 "VOXA Vocal Correct.vst3")
pp_obs_host(wide      NAME "Wide Stereo Imager"           VST3 "Wide Stereo Imager.vst3")
