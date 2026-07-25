# Apollo — UI/UX Fase 5: validação final

## 1. Objetivo e limites da Fase 5

Esta fase valida a UI implementada sem redesenhar o produto nem alterar DSP,
roteamento, APVTS, serialização ou formatos. A execução cobriu build Release,
DSP automatizado e Standalone Linux em Xvfb. Não havia DAW, `pluginval`, macOS,
Windows, AU validator, leitor de tela, hardware de áudio ou display HiDPI real.
A Fase 0 solicitada está ausente; as Fases 1–4 registram a mesma lacuna. O DSP e
o layout APVTS foram tratados como fontes técnicas de verdade.

Ficaram fora do escopo otimização DSP, presets, medidores, novos parâmetros e
qualquer interpretação nova de `octave_dry_mix`.

## 2. Ambiente de teste

| Item | Ambiente efetivamente usado |
| --- | --- |
| SO / arquitetura | Ubuntu 24.04.4 LTS, kernel 6.12.13, x86_64 |
| Compilador / build | GCC/G++ 13.3.0; CMake 3.28.3; Ninja 1.11.1; Release/C++20 |
| Dependências | JUCE 8.0.0 via FetchContent; ALSA, X11, GTK3 headers, Mesa/OpenGL |
| Formatos | Standalone e VST3 compilados; AU não é gerado em Linux |
| Hosts | Apenas wrapper Standalone JUCE; nenhuma DAW disponível |
| Ferramentas | `ApolloTest`, Xvfb, xdotool e ImageMagick; sem `pluginval`/`auval` |
| Display | Xvfb 1920×1200, captura lógica 100%; tentativa com `JUCE_SCALE_FACTOR=1.5` não mudou a escala |
| Áudio | Impulso estéreo + seno de 440 Hz gerados pelo teste; 44,1/48/96 kHz. Sem dispositivo físico; Standalone informou input silenciado |

## 3. Inventário de builds e artefatos

| Alvo/comando | Resultado | Artefato/observação |
| --- | --- | --- |
| `cmake -S Apollo -B Apollo/build-phase5 -G Ninja -DCMAKE_BUILD_TYPE=Release` | Falha inicial ambiental | Faltava `X11/extensions/Xrandr.h`; dependências Linux foram instaladas e a configuração repetida. |
| `cmake -S Apollo -B Apollo/build-phase5 -G Ninja -DCMAKE_BUILD_TYPE=Release` | PASS após dependências | Build tree `Apollo/build-phase5`. |
| `cmake --build Apollo/build-phase5 --target Apollo_All -j2` | PASS | `Apollo_artefacts/Release/Standalone/Apollo` e `Apollo_artefacts/Release/VST3/Apollo.vst3`; warnings preexistentes de DSP/JUCE permanecem. |
| `cmake --build Apollo/build-phase5 --target ApolloTest -j2` | Falha ambiental/configuração Linux inicial | `ApolloTest` herdava `juce_gui_extra` sem desativar web browser e não encontrou `gtk/gtk.h` no include path. |
| configuração com `-DCMAKE_CXX_FLAGS='-DJUCE_WEB_BROWSER=0 -DJUCE_USE_CURL=0'` + build `ApolloTest` | PASS | `ApolloTest_artefacts/Release/ApolloTest`. Workaround local, não versionado. |
| `./build-phase5/ApolloTest_artefacts/Release/ApolloTest` (cwd `Apollo`) | PASS | `test_out_44100.wav`, `test_out_48000.wav`, `test_out_96000.wav`. |

Não há alvo AU no gerador Linux. O VST3 foi compilado e instalado em
`/root/.vst3/Apollo.vst3`, mas não foi instanciado em host nem validado por
`pluginval`; portanto não recebe PASS de runtime.

## 4. Matriz de regressão funcional

A auditoria estática confirmou exatamente 15 `ParameterID`, versão 1, na ordem
abaixo. Nenhum contrato foi editado nesta fase. “Attachment” significa que a
ligação bidirecional JUCE foi confirmada no código; não equivale a automação em
DAW. Persistência foi confirmada por inspeção de `copyState`/XML/`replaceState`,
não por recuperação de sessão em host.

| ID | Tipo / range / default auditado | UI→APVTS | APVTS→UI | Estado salvo | Default / extremos | Resultado |
| --- | --- | --- | --- | --- | --- | --- |
| `predelay` | float 0..1; 0 | Attachment | Attachment | Estático | 0/1 e reset configurados | PASS WITH LIMITATIONS |
| `mix` | float 0..1; 0.5 | Attachment | Attachment | Estático | 0/0.5/1 | PASS WITH LIMITATIONS |
| `decay` | float 0..1; 0.877 | Attachment | Attachment | Estático | 0/0.877/1 | PASS WITH LIMITATIONS |
| `moddepth` | float 0..1; 0.0625 | Attachment | Attachment | Estático | 0/0.0625/1 | PASS WITH LIMITATIONS |
| `modspeed` | float 0..1; 0.0466 | Attachment | Attachment | Estático | 0/0.0466/1 | PASS WITH LIMITATIONS |
| `damp` | float 0..1; 0.5 | Attachment | Attachment | Estático | 0/0.5/1 | PASS WITH LIMITATIONS |
| `eq1_gain` | float −24..24; −11 | Attachment | Attachment | Estático | −24/−11/24 | PASS WITH LIMITATIONS |
| `eq2_gain` | float −24..24; 5 | Attachment | Attachment | Estático | −24/5/24 | PASS WITH LIMITATIONS |
| `time_scale` | choice Small/Medium/Large; Large | Attachment | Attachment | Estático | três escolhas | PASS WITH LIMITATIONS |
| `effect_mode` | choice Off/Up/Down/Up + Down; Off | Attachment | Attachment + timer | Estático | quatro escolhas capturadas | PASS WITH LIMITATIONS |
| `footswitch_mode` | choice Freeze/Overdrive/Octave Perform; Freeze | Attachment | Attachment + timer | Estático | três escolhas exercitadas visualmente | PASS WITH LIMITATIONS |
| `input_diffusion` | bool; true | Attachment | Attachment + timer | Estático | false/true | PASS WITH LIMITATIONS |
| `octave_dry_mix` | bool; true | Attachment | Attachment + timer | Estático | false/true | PASS WITH LIMITATIONS |
| `bypass` | bool; false | Attachment | Attachment + timer | Estático | mouse/foco visual; sem DAW | PASS WITH LIMITATIONS |
| `momentary_effect` | bool; false | Attachment momentâneo | Attachment + timer | Estático | mouse down/up exercitado | PASS WITH LIMITATIONS |

Undo/Redo, escrita ociosa, flicker sob automação rápida e recuperação de sessão
não foram executados por ausência de host. O timer apenas lê valores e atualiza
apresentação; não escreve parâmetros quando o usuário está ocioso.

## 5. Matriz de estados visuais

Inspeção no Standalone/Xvfb, 900×620 de conteúdo (janela 908×684):

| Estado | Evidência | Resultado observado |
| --- | --- | --- |
| Active / padrão | `default_100.png` | Hierarquia e valores legíveis; estado interno explícito. PASS. |
| Octave None/Off | `octave_off_100.png` | Atenuação + texto, attachments/foco preservados. PASS. |
| Octave Up | `octave_up_100.png` | Estado ativo, valores preservados. PASS. |
| Octave Down | `octave_down_100.png` | Estado ativo; sem promessa nova sobre exceção dry. PASS. |
| Octave Both | `octave_both_100.png` | Estado ativo e controles enfatizados. PASS. |
| Freeze Active | `freeze_active_100.png` | Texto de ação ativa. PASS. |
| Drive Active | `drive_active_100.png` | Texto de ação ativa. PASS. |
| Octave Perform Active | `octave_perform_active_100.png` | Texto ativo com modo de oitava aplicável. PASS. |
| Effect + None | Inspeção de condição/copy no editor | `No Octave Mode Selected`; não foi capturado separadamente. PASS WITH LIMITATIONS. |
| Bypassed | `bypassed_100.png` | Header e botão dizem bypass interno; controles permanecem. PASS. |

## 6. Validação de acessibilidade

A ordem explícita é Size, Pre-delay, Decay, Tone, Mod Rate, Mod Depth, Input
Diffusion, Octave Mode, shelves, Dry Routing, Perform Action, Perform, Mix e
Bypass. Sliders, choices e botões têm foco; componentes decorativos não entram
na ordem. `title`, `description`, tooltips, valores e texto de estado foram
auditados. Tab foi exercitado e o anel foi capturado em
`keyboard_focus_100.png`; a navegação completa por Shift+Tab/setas/Enter/Space
não foi certificada manualmente em toda a árvore.

Estados não dependem apenas de cor: Off, Active, Bypassed e ações aparecem em
texto. Controles atenuados continuam focáveis. Não havia leitor de tela/AT-SPI
inspector configurado, logo anúncio real é NOT TESTED. Contraste foi apenas
inspecionado visualmente, sem medição WCAG automatizada.

## 7. Validação de resize e HiDPI

O mínimo/padrão 900×620 foi executado e não ocultou controles após a correção.
O código restringe 900×620 a 1400×980 com proporção fixa. Resize máximo e
contínuo não foram concluídos: o gerenciador Xvfb/xdotool não conseguiu impor o
tamanho ao wrapper com segurança. A tentativa `JUCE_SCALE_FACTOR=1.5` continuou
em 908×684; não constitui validação 150%. Escalas 125%, 150%, 175% e 200% são
NOT TESTED e permanecem obrigatórias em display/host reais.

## 8. Validação musical

| Fonte/configuração | Ação | Esperado | Observado | Severidade/evidência |
| --- | --- | --- | --- | --- |
| Impulso estéreo + seno 440 Hz/0,5, 44,1 kHz | Up octave + Overdrive, mix/decay intermediários | saída finita e WAV | processo terminou e gerou WAV | Nenhuma; `test_out_44100.wav` |
| Mesma fonte, 48 kHz | idem | processamento no rate nativo | terminou e gerou WAV | Nenhuma; `test_out_48000.wav` |
| Mesma fonte, 96 kHz | idem | resampling estável | terminou e gerou WAV | Nenhuma; `test_out_96000.wav` |
| Capacidade de pre-delay | 1 s em 44,1/48/96 kHz | buffer suficiente | assertions passaram | Nenhuma; saída de `ApolloTest` |

Guitarra, acordes densos, baixo, material sustentado/estéreo real, silêncio,
0 dBFS, graves/agudos amplos, Freeze, todos os modos de oitava, bypass durante
cauda e avaliação de clicks não foram ouvidos: não havia interface/dispositivo
nem host. Os WAVs são evidência programática, não validação auditiva. CPU,
memória, múltiplas instâncias e stress de automação também são NOT TESTED.

## 9. Matriz de hosts e formatos

| SO / host / formato | Instanciação/editor | Resize | Automação/recall | Bypass/Perform/close | Resultado |
| --- | --- | --- | --- | --- | --- |
| Ubuntu 24.04 x86_64 / JUCE Standalone / Standalone | PASS / PASS | mínimo PASS; máximo parcial | sem host; NOT TESTED | UI exercitada; fechamento SIGTERM sem crash observado | PASS WITH LIMITATIONS |
| Ubuntu 24.04 x86_64 / DAW / VST3 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED (somente build PASS) |
| macOS / host AU / AU | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| macOS / host VST3 / VST3 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |
| Windows / host VST3 / VST3 | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED | NOT TESTED |

## 10. Defeitos encontrados

### AP5-001 — P2 — texto mojibake e truncamento no tamanho mínimo

- **Reprodução:** abrir Standalone 900×620 no Linux/Xvfb.
- **Esperado:** estados e ações integralmente legíveis.
- **Observado:** em dash em strings narrow apareceu como `â—`; Input Diffusion,
  Dry Routing e Perform ultrapassavam ou truncavam seus bounds.
- **Causa provável:** literal UTF-8 passado pelo caminho narrow e copy longa em
  bounds fixos (Perform excedia o painel).
- **Correção:** separadores ASCII, copy de botão curta e bounds do Perform
  calculados pela largura disponível; sem mudança de attachment/DSP.
- **Regressão:** rebuild Standalone/VST3 PASS; novas capturas 100% confirmam
  ausência do mojibake e acesso aos controles essenciais.

### AP5-002 — P2 adiado — cobertura HiDPI/resize máximo incompleta

Limitação ambiental descrita na seção 7. Repetir em displays 100–200% e hosts
alvo antes de uma liberação comercial ampla.

Nenhum P0/P1 foi observado. Isso não prova ausência de P0/P1 em hosts não
executados.

## 11. Correções implementadas

O único arquivo de produção alterado foi `Source/PluginEditor.cpp`: copy visual
compatível com o caminho de texto do JUCE/Linux e layout mínimo do Perform. O
relatório `docs/UI_UX_PHASE_5.md` também foi criado para documentar a validação.
Risco baixo: nenhuma mudança toca processor, APVTS, parâmetros, attachments,
serialização ou formatos. Rebuild dos artefatos e inspeção das capturas foram
repetidos.

## 12. Limitações e decisões adiadas

- **Produto:** semântica/polaridade de `octave_dry_mix`, nomes provisórios dos
  shelves e centro de Tone seguem pendentes; nenhum comportamento foi mudado.
- **Ambiente QA:** sem DAW, pluginval, AU/macOS, Windows, áudio físico, leitor de
  tela e HiDPI real; VST3 teve apenas build.
- **Preexistentes:** warnings de strict aliasing/array bounds em `FastSqrt.h`,
  conversões DSP, comparação float e warning otimizado em `buffer.clear()`;
  não alterados para preservar DSP. `ApolloTest` requer flags locais para
  desabilitar o browser no Linux.
- **Decisão de produto:** recuperar/reconciliar `UI_UX_PHASE_0.md`; aprovar copy
  definitiva de dry routing/shelves.
- **Futuro não bloqueador desta correção:** matriz manual em DAWs alvo, stress,
  medição de contraste e escuta musical controlada.

## 13. Critérios de prontidão para entrega

| Critério | Estado |
| --- | --- |
| Build Standalone/VST3 Linux | PASS, com warnings preexistentes |
| Instanciação | Standalone PASS; VST3/AU em host NOT TESTED |
| Áudio | DSP automatizado PASS; escuta musical NOT TESTED |
| Automação | Contratos/attachments PASS estático; host NOT TESTED |
| Recuperação de sessão | Serialização PASS estático; host NOT TESTED |
| Estados visuais | Estados principais Standalone PASS; Effect+None sem captura dedicada |
| Teclado | foco/Tab parcial PASS; fluxo completo NOT TESTED |
| Acessibilidade | metadados/texto PASS estático; leitor de tela NOT TESTED |
| Resize | mínimo PASS; máximo/rápido NOT TESTED |
| HiDPI | NOT TESTED além de tentativa não efetiva |
| Hosts | Standalone com limitações; DAWs NOT TESTED |
| Formatos | Standalone runtime; VST3 build; AU NOT TESTED |
| Documentação | PASS; Fase 0 ausente explicitamente registrada |

Não há P0/P1 aberto conhecido, mas automação/recall em host, HiDPI e validação
musical real ainda reduzem a confiança e devem integrar o gate de release nos
sistemas-alvo.

## 14. Resultado final

`READY WITH DOCUMENTED LIMITATIONS`

A UI corrigida compila, abre e representa os estados principais no Standalone;
os 15 contratos APVTS continuam intactos e o teste DSP passa em três sample
rates. A classificação não alega compatibilidade VST3/AU em DAW, automação de
host, sessão, HiDPI ou escuta musical que não foram executadas. Evidências
visuais transitórias ficam em `/tmp/apollo-phase5-screenshots/` e não são
versionadas.
