# PulsePoint OBS

Leva os plugins da [PulsePoint](https://pulsepoint.company) para o **OBS Studio** como filtros de
áudio. Cada plugin tem o seu módulo: no OBS ele aparece em **Filtros → +** com o nome do próprio
plugin (ex.: *Lift Vocal Processor*), e as propriedades do filtro têm um botão que abre a interface.

O módulo não contém o processamento de áudio: ele carrega o **VST3** do plugin, instalado pelo mesmo
instalador e licenciado à parte. Sem o VST3 (ou sem licença), o áudio passa intocado.

## Instalação

Pelo instalador do plugin, marcando **OBS Studio**. O VST3 é marcado junto.

| | Módulo | VST3 |
|---|---|---|
| Windows | `C:\ProgramData\obs-studio\plugins\pulsepoint-<plugin>\bin\64bit\` | `C:\Program Files\Common Files\VST3\` |
| macOS | `~/Library/Application Support/obs-studio/plugins/pulsepoint-<plugin>.plugin` | `/Library/Audio/Plug-Ins/VST3/` |

OBS Studio 30 ou mais novo, 64 bits.

### Onde o módulo procura o VST3

1. o arquivo escolhido em **Localizar…** nas propriedades do filtro (vale para aquele filtro e fica
   lembrado para os próximos);
2. o caminho gravado pelo instalador em `PulsePoint/OBS/<plugin>.txt`, dentro de `C:\ProgramData`
   (Windows) ou `/Library/Application Support` (macOS);
3. a pasta padrão de VST3 do sistema.

## Compilar

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target obs_lift     # ou qualquer obs_<plugin> de plugins.cmake
```

Precisa de CMake 3.22+ e de um compilador C++17 (Xcode no Mac, Visual Studio 2022 no Windows). O JUCE
8 é baixado no configure. Não precisa do SDK do OBS: os headers do libobs (OBS 30.0.2) estão em
`third_party/obs-studio/`, e as funções são resolvidas no processo do OBS quando ele carrega o módulo
(`src/ObsApi.cpp`).

## Licença

[AGPL-3.0](LICENSE). Usa o [libobs](https://github.com/obsproject/obs-studio) (GPL-2.0 ou posterior)
e o [JUCE](https://juce.com) (AGPL-3.0). Os plugins PulsePoint que este módulo carrega são produtos
separados, com licença própria.
