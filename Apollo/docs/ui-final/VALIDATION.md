# Verificação final — Apollo UI Final

Data: 01/10/2026. Host Windows, MinGW-w64 GCC 14.2 / Clang 19.1.
JUCE permanece fixado em 8.0.0 no CMake.

## Resultado

| Verificação | Resultado |
|---|---|
| Apollo Standalone Release | Build/link concluídos; Apollo.exe abriu e expôs janela “Apollo”. |
| Apollo VST3 Release | Build/link concluídos; helper carregou módulo e gerou moduleinfo.json. |
| Apollo AU | Não disponível em Windows; não compilado/testado em macOS. |
| golden (core existente) | PASS. Referências não regeneradas. |
| damp_history | PASS: 48 casos, seis transições × snap/smoothing × quatro rates. |
| apollo_dsp | PASS: sinais de teste em 44.1/48/96 kHz, sem NaN/Inf. |
| apollo_ui_contract | PASS: 15 contratos, attachments nos dois sentidos, recall atual e XML legacy explícito. |
| Teclado e reset | PASS no teste do editor: setas de sliders/selectors e double-click defaults. |
| Perform | PASS: mouseDown/up, Space/Enter e release, perda de foco, hide, disable, fechamento e ownership da automação. |
| Feed | PASS: seleção OCT=false / OCT+DRY=true, polaridade original. |
| Estado de oitava | PASS: indicação dos shelves acompanha gate no modo Perform Octave. |
| Resize | PASS: controles contidos nos limites, 900×620 e 1400×964. |
| Rasterização HiDPI | 23 PNGs reais do editor; default em 900×620, 1350×930 e 1800×1240. Revisão visual sem clipping. |
| SVG | 10 documentos XML válidos, IDs únicos e paint servers resolvidos; revisão no browser. |
| Contrato de fonte | PluginProcessor.cpp idêntico ao HEAD original; 15 IDs/version hints; nenhuma promessa de MIDI no editor. |
| git diff --check | PASS. |

Os primeiros testes de damp executados **antes** da alteração reproduziram a
dependência do histórico em High Cut → Flat e nas trocas entre metades.
Após restaurar os cutoffs neutros históricos, todos os casos passam.

O primeiro teste de contrato detectou a quantização histórica do JUCE:
o construtor Float com min/max usa interval=0.01. O teste passou a verificar
essa propriedade e o default normalizado, sem mudar parâmetros de produção.
Decay declarado 0.877465 → 0.88; depth 0.0625 → 0.06; speed 0.0466667 → 0.05.
Os SVGs e readouts default refletem esses valores efetivos.

## Comandos executados

Core independente de JUCE:

    cmake -S shared -B Apollo/build_core_final -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    cmake --build Apollo/build_core_final -j 4
    ctest --test-dir Apollo/build_core_final --output-on-failure

Resultado: **2/2 PASS** (golden e damp_history).

Verificação de sintaxe com headers JUCE reais (antes do build completo):

    clang++ -std=c++20 -fsyntax-only -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -DJUCE_USE_DIRECT2D=0 -I Apollo/build_local/_deps/juce-src/modules -I Apollo/Source -I shared Apollo/Source/PluginEditor.cpp Apollo/Source/ApolloLookAndFeel.cpp Apollo/Source/ApolloTheme.cpp

Resultado: PASS.

Build padrão inicialmente tentado:

    cmake -S Apollo -B Apollo/build_final -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_JUCE=C:/progs/vst/EarthPedal/Apollo/build_local/_deps/juce-src -DJUCE_COPY_PLUGIN_AFTER_BUILD=OFF

Resultado: falha dentro de juceaide/Direct2D do JUCE 8.0.0 com o SDK MinGW
instalado, antes das fontes do Apollo.

Build final com toolchain local complementar:

    cmake -S Apollo -B Apollo/build_final_clang -G Ninja -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_JUCE=C:/progs/vst/EarthPedal/Apollo/build_local/_deps/juce-src -DJUCE_COPY_PLUGIN_AFTER_BUILD=OFF
    cmake --build Apollo/build_final_clang --target Apollo_Standalone Apollo_VST3 ApolloTest -j 4
    ctest --test-dir Apollo/build_final_clang --output-on-failure

Esses comandos usam o cache da configuração isolada descrita abaixo.
Resultado final: **Standalone + VST3 + ApolloTest compilados; 2/2 PASS** no CTest
(aproximadamente 1 s DSP + 10 s UI). Logs locais em build_final_headers.

Ferramenta SVG:

    python Apollo/tools/ui_final/render_svg.py
    git diff --check

## Ajustes locais de toolchain usados para conseguir executar o JUCE 8

Nenhum arquivo do JUCE ou da toolchain global foi modificado.
Os arquivos complementares ficaram somente no diretório ignorado
Apollo/build_final_headers.

- Headers oficiais mingw-w64 de d2d1/dwrite e versões derivadas foram baixados
  para um include overlay a partir de
  https://github.com/mingw-w64/mingw-w64/tree/master/mingw-w64-headers/include.
- Selecionado Clang 19.1 via CC/CXX.
- NTDDI_VERSION=0x0A000003 para expor interfaces gráficas exigidas pelo JUCE 8.
- graphics_compat.h define UNICODE/_UNICODE antes de incluir headers Windows,
  operadores constexpr de DWRITE_GLYPH_IMAGE_FORMATS ausentes no MinGW,
  e um getter de UUID para ComSmartPtr<IDXGISurface> (o JUCE 8 usa __uuidof
  do smart pointer nessa chamada; o macro MinGW não segue a conversão).
- Flags: -I overlay, -include graphics_compat.h, -femulated-tls e -pthread.
- Linker: -fuse-ld=lld; bibliotecas winpthread, d2d1, d3d11, dxgi, dwrite,
  dcomp e dxguid, além das bibliotecas padrão Windows.
- O mesmo overlay foi usado ao compilar juceaide no cache tools.
- Há warnings de compatibilidade dos headers, sem erros no build final.
- A opção de cópia pós-build agora respeita JUCE_COPY_PLUGIN_AFTER_BUILD=OFF.
  O default ON do projeto foi mantido. Nenhum plugin foi instalado no sistema.

Flags reais, caminhos e comandos completos permanecem nos CMakeCache.txt e
logs dos diretórios de build. Esta configuração local de validação não troca
a versão do JUCE nem altera o contrato/distribuição do plugin.

## Evidência visual e limites

renders/ contém **23 imagens do editor real**, geradas por
createComponentSnapshot no ApolloTest --ui:

- default-100 / default-150 / default-200
- minimum / maximum
- size-0 / size-1 / size-2
- diffusion-off / diffusion-on
- octave-0 / octave-1 / octave-2 / octave-3
- feed-oct / feed-oct-dry
- perform-0 / perform-1 / perform-2
- tone-20 / tone-50 / tone-80
- bypass

Foram revisados layout default, escalas, extremos de resize e estados visuais.
O Standalone abriu, mas Windows.Graphics.Capture falhou por timeout após
seleção e recuperação da janela. Por isso estes PNGs são renders do editor
real, **não capturas nativas da janela Standalone**.

Não foram validados escala global do Windows, interação manual em uma DAW,
automação enviada por um host externo, leitor de tela ou AU/macOS.
A automação/recall/teclado/gate foram exercitados programaticamente com
APVTS e controles JUCE reais. O helper VST3 carregou o binário, mas isso
não substitui uma sessão de áudio em DAW.

Os SVGs são demonstrações vetoriais e estão separados dos PNGs reais.
