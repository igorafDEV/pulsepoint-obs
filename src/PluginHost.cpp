#include "Host.h"
#include "Log.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <mutex>
#include <vector>

namespace pp::host
{
namespace
{
// juce::String de um const char* lê Latin-1: todo texto com acento passa por aqui
juce::String text (const char* utf8) { return juce::String::fromUTF8 (utf8); }

// Onde o instalador grava o caminho do VST3 que ele instalou (uma linha, UTF-8).
juce::File installerPathFile()
{
#if JUCE_WINDOWS
    auto base = juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory);   // C:\ProgramData
#else
    auto base = juce::File ("/Library/Application Support");
#endif
    return base.getChildFile ("PulsePoint/OBS/" PP_OBS_SLUG ".txt");
}

// Onde fica o caminho que o próprio cliente escolheu em "Localizar", valendo para os filtros novos.
juce::File userPathFile()
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    base = base.getChildFile ("Application Support");
#endif
    return base.getChildFile ("PulsePoint/OBS/" PP_OBS_SLUG ".txt");
}

// Onde procurar o VST3, na ordem: o escolhido neste filtro, o escolhido pelo cliente, o que o
// instalador gravou e, por último, as pastas padrão de VST3.
juce::Array<juce::File> candidates (const juce::String& chosen)
{
    juce::Array<juce::File> files;
    auto add = [&files] (const juce::String& path)
    {
        auto p = path.trim();
        if (juce::File::isAbsolutePath (p))
            files.addIfNotAlreadyThere (juce::File (p));
    };

    add (chosen);
    add (userPathFile().loadFileAsString());
    add (installerPathFile().loadFileAsString());
#if JUCE_WINDOWS
    add (juce::SystemStats::getEnvironmentVariable ("CommonProgramFiles", "C:\\Program Files\\Common Files")
         + "\\VST3\\" PP_OBS_VST3);
#else
    add ("/Library/Audio/Plug-Ins/VST3/" PP_OBS_VST3);
    add (juce::File::getSpecialLocation (juce::File::userHomeDirectory)
             .getChildFile ("Library/Audio/Plug-Ins/VST3/" PP_OBS_VST3)
             .getFullPathName());
#endif
    return files;
}

class EditorWindow : public juce::DocumentWindow
{
public:
    explicit EditorWindow (juce::AudioProcessorEditor* editor)
        : DocumentWindow (PP_OBS_NAME, juce::Colours::black, closeButton | minimiseButton)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (editor, true);
        setResizable (editor->isResizable(), false);
        centreWithSize (getWidth(), getHeight());
    }

    // fechar só esconde: reabrir é instantâneo e o editor não perde o que estava mostrando
    void closeButtonPressed() override { setVisible (false); }
};

void onMessageThread (const Handle& host, std::function<void (PluginHost&)> fn)
{
    if (juce::MessageManager::existsAndIsCurrentThread())
    {
        fn (*host);
        return;
    }

    juce::MessageManager::callAsync ([weak = std::weak_ptr<PluginHost> (host), fn = std::move (fn)]
    {
        if (auto h = weak.lock())
            fn (*h);
    });
}

std::mutex pendingMutex;
std::vector<Handle> pending;

void releasePending()
{
    std::vector<Handle> doomed;
    {
        const std::lock_guard<std::mutex> lock (pendingMutex);
        doomed.swap (pending);
    }
    // os destrutores rodam aqui, na thread de mensagens
}
} // namespace

class PluginHost : public std::enable_shared_from_this<PluginHost>
{
public:
    PluginHost (juce::String state, juce::String path, double rate, int channels)
        : sampleRate (rate > 0 ? rate : 48000.0),
          numChannels (juce::jlimit (1, 8, channels)),
          pendingState (std::move (state)),
          chosenPath (std::move (path))
    {
    }

    ~PluginHost()
    {
        chooser.reset();
        window.reset();
        if (instance != nullptr)
            instance->releaseResources();
    }

    void load()
    {
        JUCE_ASSERT_MESSAGE_THREAD
        problem = {};

        for (auto& file : candidates (chosenPath))
        {
            if (! file.exists())
                continue;

            juce::OwnedArray<juce::PluginDescription> types;
            vst3.findAllTypesForFile (types, file.getFullPathName());

            if (types.isEmpty())
            {
                problem = text ("O arquivo ") + file.getFullPathName() + text (" não abriu como VST3.");
                continue;
            }

            juce::String error;
            if (auto created = vst3.createInstanceFromDescription (*types[0], sampleRate, maxBlock, error))
            {
                pp::obs::log (pp::obs::logInfo, "carregado de %s", file.getFullPathName().toRawUTF8());
                install (std::move (created));
                return;
            }

            problem = text ("O ") + file.getFileName() + text (" não abriu: ") + error;
        }

        if (problem.isEmpty())
            problem = text ("Não encontrei o " PP_OBS_VST3 ". Reinstale o " PP_OBS_NAME
                            " marcando o VST3, ou mostre onde ele está.");

        pp::obs::log (pp::obs::logWarning, "%s", problem.toRawUTF8());
    }

    void process (float** channels, int numFrames) noexcept
    {
        const juce::SpinLock::ScopedTryLockType lock (audioLock);
        if (! lock.isLocked() || instance == nullptr)
            return;

        // o plugin roda em estéreo: num OBS mono o canal único entra nos dois lados; com mais de
        // dois canais (5.1), só a frente esquerda/direita passa pelo plugin
        const int processed = juce::jmin (numChannels, 2);

        for (int done = 0; done < numFrames;)
        {
            const int n = juce::jmin (maxBlock, numFrames - done);

            for (int c = 0; c < buffer.getNumChannels(); ++c)
            {
                if (c < 2)
                    buffer.copyFrom (c, 0, channels[juce::jmin (c, numChannels - 1)] + done, n);
                else
                    buffer.clear (c, 0, n);
            }

            juce::AudioBuffer<float> block (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), n);
            instance->processBlock (block, midi);
            midi.clear();

            for (int c = 0; c < processed; ++c)
                juce::FloatVectorOperations::copy (channels[c] + done, buffer.getReadPointer (c), n);

            done += n;
        }
    }

    juce::String getState()
    {
        if (instance == nullptr)
            return pendingState;   // ainda não carregou: devolve o que veio da cena, sem perder nada

        juce::MemoryBlock data;
        instance->getStateInformation (data);
        return data.toBase64Encoding();
    }

    void openEditor()
    {
        JUCE_ASSERT_MESSAGE_THREAD
        if (instance == nullptr)
            return;

        if (window == nullptr)
        {
            auto* editor = instance->createEditorIfNeeded();
            if (editor == nullptr)
                return;
            window = std::make_unique<EditorWindow> (editor);
        }

        window->setVisible (true);
        window->toFront (true);
    }

    void choosePluginFile()
    {
        JUCE_ASSERT_MESSAGE_THREAD
#if JUCE_WINDOWS
        const juce::File start (juce::SystemStats::getEnvironmentVariable ("CommonProgramFiles", "C:\\Program Files\\Common Files") + "\\VST3");
#else
        const juce::File start ("/Library/Audio/Plug-Ins/VST3");
#endif
        chooser = std::make_unique<juce::FileChooser> (text ("Onde está o " PP_OBS_VST3 "?"), start, "*.vst3");

        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [weak = weak_from_this()] (const juce::FileChooser& fc)
        {
            auto self = weak.lock();
            const auto file = fc.getResult();
            if (self == nullptr || file == juce::File())
                return;

            self->chosenPath = file.getFullPathName();

            // vale também para os próximos filtros, sem perguntar de novo
            const auto remembered = userPathFile();
            remembered.getParentDirectory().createDirectory();
            remembered.replaceWithText (self->chosenPath, false, false, nullptr);

            self->load();
            self->openEditor();
        });
    }

    bool isLoaded() const { return loaded.load(); }

    juce::String chosenPath;
    juce::String problem;

private:
    void install (std::unique_ptr<juce::AudioPluginInstance> created)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        window.reset();   // o editor é da instância antiga

        juce::AudioProcessor::BusesLayout stereo;
        stereo.inputBuses.add (juce::AudioChannelSet::stereo());
        stereo.outputBuses.add (juce::AudioChannelSet::stereo());
        created->disableNonMainBuses();
        created->setBusesLayout (stereo);   // se recusar, fica no layout padrão dele

        created->setRateAndBufferSizeDetails (sampleRate, maxBlock);
        created->setNonRealtime (false);
        created->prepareToPlay (sampleRate, maxBlock);

        if (pendingState.isNotEmpty())
        {
            juce::MemoryBlock data;
            if (data.fromBase64Encoding (pendingState))
                created->setStateInformation (data.getData(), (int) data.getSize());
        }

        const int channels = juce::jmax (2, created->getTotalNumInputChannels(), created->getTotalNumOutputChannels());
        juce::AudioBuffer<float> newBuffer (channels, maxBlock);
        midi.ensureSize (256);

        {
            const juce::SpinLock::ScopedLockType lock (audioLock);
            std::swap (instance, created);
            std::swap (buffer, newBuffer);
        }
        loaded = true;

        if (created != nullptr)
            created->releaseResources();
    }

    static constexpr int maxBlock = 2048;

    const double sampleRate;
    const int numChannels;
    juce::String pendingState;

    juce::VST3PluginFormat vst3;
    std::unique_ptr<juce::AudioPluginInstance> instance;
    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;
    juce::SpinLock audioLock;
    std::atomic<bool> loaded { false };

    std::unique_ptr<EditorWindow> window;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE (PluginHost)
};

Handle create (const char* state, const char* path, double sampleRate, int numChannels)
{
    auto host = std::make_shared<PluginHost> (text (state != nullptr ? state : ""),
                                              text (path != nullptr ? path : ""),
                                              sampleRate, numChannels);
    onMessageThread (host, [] (PluginHost& h) { h.load(); });
    return host;
}

void release (Handle host)
{
    if (host == nullptr)
        return;

    if (juce::MessageManager::existsAndIsCurrentThread())
        return;   // o último shared_ptr morre ao sair daqui, nesta thread

    {
        const std::lock_guard<std::mutex> lock (pendingMutex);
        pending.push_back (std::move (host));
    }
    juce::MessageManager::callAsync ([] { releasePending(); });
}

void process (PluginHost& host, float** channels, int numFrames) noexcept { host.process (channels, numFrames); }

// o OBS salva a cena na thread da interface, que é a de mensagens
std::string state (PluginHost& host) { return host.getState().toStdString(); }
std::string chosenPath (PluginHost& host) { return host.chosenPath.toStdString(); }

bool isLoaded (PluginHost& host) { return host.isLoaded(); }
std::string problem (PluginHost& host) { return host.problem.toStdString(); }

void openEditor (const Handle& host) { onMessageThread (host, [] (PluginHost& h) { h.openEditor(); }); }
void choosePluginFile (const Handle& host) { onMessageThread (host, [] (PluginHost& h) { h.choosePluginFile(); }); }

void initialise (void* moduleHandle)
{
#if JUCE_WINDOWS
    juce::Process::setCurrentModuleInstanceHandle (moduleHandle);
#else
    juce::ignoreUnused (moduleHandle);
#endif
    juce::initialiseJuce_GUI();
}

void shutdown()
{
    releasePending();
    juce::shutdownJuce_GUI();
}
} // namespace pp::host
