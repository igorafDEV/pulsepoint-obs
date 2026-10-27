// Log.h — escreve no log do OBS (Ajuda > Arquivos de log), prefixado com o nome do plugin.
// Sem headers do OBS: pode ser incluído junto com o JUCE.
#pragma once

namespace pp::obs
{
// os mesmos valores do LOG_* de util/base.h do libobs
constexpr int logError = 100;
constexpr int logWarning = 200;
constexpr int logInfo = 300;

void log (int level, const char* format, ...);
} // namespace pp::obs
