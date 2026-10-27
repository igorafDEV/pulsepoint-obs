// Host.h — a ponte entre o filtro do OBS (Module.cpp) e o plugin carregado (PluginHost.cpp).
//
// Sem JUCE e sem headers do OBS aqui: os dois mundos só se encontram por esta interface.
//
// Threads:
//   - mensagens (a thread da interface do OBS, onde o JUCE foi iniciado): carregar o VST3, abrir
//     a janela, escolher o arquivo. As funções abaixo que mexem nisso pulam para ela sozinhas.
//   - áudio do OBS: só process(). Sem alocação nem espera: se a instância está sendo trocada
//     naquele instante, o bloco passa intocado.
#pragma once

#include <memory>
#include <string>

namespace pp::host
{
class PluginHost;
using Handle = std::shared_ptr<PluginHost>;

// cria o host e pede o carregamento do VST3; state e path vêm da cena salva do OBS
Handle create (const char* state, const char* chosenPath, double sampleRate, int numChannels);

// destrói na thread de mensagens; pode ser chamado de qualquer thread
void release (Handle host);

// thread de áudio: processa no lugar os canais planos do OBS
void process (PluginHost& host, float** channels, int numFrames) noexcept;

// o que o OBS grava na cena
std::string state (PluginHost& host);
std::string chosenPath (PluginHost& host);

bool isLoaded (PluginHost& host);
std::string problem (PluginHost& host);   // por que não carregou, para as propriedades

void openEditor (const Handle& host);
void choosePluginFile (const Handle& host);

// no load/unload do módulo, na thread da interface do OBS
void initialise (void* moduleHandle);
void shutdown();
} // namespace pp::host
