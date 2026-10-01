# Apollo UI Final — identidade e contrato dos controles

Milestone: **2001 Retrofuturist Identity + Control Contract Freeze**.

## A. Resumo visual

Chassis ivory homogêneo, SPACE / OUTPUT / OCTAVE / PERFORM sobre uma única
superfície graphite, divisórias finas e alinhamentos regulares. Saíram metal
escovado, parafusos, placas empilhadas, knobs estriados, caps cromados e bevels
profundos. Knobs baixos com relevo discreto, ponteiro ivory e escala fina.
Decay maior, modulação secundária e fader vertical preservado.

Header reduzido a APOLLO, STEREO SPACE PROCESSOR e bypass retangular.
Âmbar comunica seleção/gate; vermelho aparece somente em BYPASSED.
Não há logos, HAL, props ou elementos específicos de cinema.

## B. Arquivos alterados

| Arquivo | Motivo |
|---|---|
| Source/ApolloTheme.h | Paleta final; retirar componentes/materiais decorativos; regiões integradas e seletores com reset. |
| Source/ApolloTheme.cpp | Superfícies precisas; tipografia e títulos sem placas ou lamps redundantes. |
| Source/ApolloLookAndFeel.h | Nome/descrição do knob industrial final. |
| Source/ApolloLookAndFeel.cpp | Knobs, fader, bancos geométricos, Feed explícito, Perform âmbar, bypass e foco discreto; valores sem molduras. |
| Source/PluginEditor.h | Gate com ownership local e liberação segura; seleção explícita de Feed; TooltipWindow; módulos e labels. |
| Source/PluginEditor.cpp | Layout, 15 attachments, readout bipolar, Mod Rate como multiplicador; copy/acessibilidade e estado efetivo de oitava. |
| Source/test_ui.cpp | Contrato, attachments bidirecionais, recall, teclas/reset, gate, resize e renders do editor real. |
| Source/test_dsp.cpp | Entrada --ui; correção de createWriterFor/stream ownership para JUCE 8.0.0, sem mudança nos sinais testados. |
| CMakeLists.txt | Solicitar teclado ao host; registrar testes/fontes; flags de teste sem navegador/curl; permitir desativar cópia para pasta do sistema (default ON preservado). |
| ../shared/EarthDSPCore.cpp | Em applyDamp, restaurar filtro oposto ao neutro histórico; nenhuma outra mudança sonora. |
| ../shared/tests/damp_history_test.cpp | Seis transições × snap/smoothing × quatro sample rates. |
| ../shared/tests/CMakeLists.txt | Registrar regressão independente de JUCE no CTest. |
| tools/ui_final/render_svg.py | Gerador SVG reproduzível, só biblioteca padrão Python. |
| docs/ui-final/apollo-default.svg | Demonstração default. |
| docs/ui-final/apollo-octave-active.svg | Demonstração Up+Down ativo. |
| docs/ui-final/apollo-perform-active.svg | Demonstração Freeze com gate ON. |
| docs/ui-final/apollo-perform-drive.svg | Demonstração Drive com gate ON. |
| docs/ui-final/apollo-perform-octave.svg | Demonstração Octave com gate ON. |
| docs/ui-final/apollo-bypass.svg | Demonstração bypass. |
| docs/ui-final/apollo-tone-high-cut.svg | Demonstração Tone HI. |
| docs/ui-final/apollo-tone-low-cut.svg | Demonstração Tone LO. |
| docs/ui-final/apollo-tone-comparison.svg | Comparação High Cut / Flat / Low Cut. |
| docs/ui-final/apollo-feed-oct.svg | Demonstração Feed OCT. |
| docs/ui-final/README.md | Este relatório e auditoria. |
| docs/ui-final/VALIDATION.md | Comandos/resultados e limitações de verificação. |

## C. Auditoria: UI → APVTS → Processor → DSP

Todos os IDs usam **ParameterID version 1**, na ordem original.
SA = SliderAttachment; CA = ComboBoxAttachment; BA = ButtonAttachment.
Os floats mantêm também o **intervalo histórico 0.01** do construtor JUCE.
A coluna Default lista os valores declarados originais; JUCE apresenta decay
como 0.88, moddepth como 0.06 e modspeed como 0.05 após normalização/snap.
Por isso o readout default de Mod Rate é 1.05x. Essa quantização já existia
e foi preservada, incluindo double-click reset.

O processor lê cada ID via getRawParameterValue(...)->load() em processBlock.
Em bypass o core continua processando, mas a saída recebe dry por crossfade.

| ID APVTS | Tipo / range ou choices originais, na ordem | Default | Controle / attachment | Processor → EarthParameters | Efeito real / condição no EarthDSPCore |
|---|---|---:|---|---|---|
| predelay | Float 0…1 s | 0 | PRE-DELAY, ms / attachPredelay (SA) | predelay → preDelaySeconds | preDelay_.next() → setPreDelay; caminho de reverb. |
| mix | Float 0…1 | 0.5 | Fader DRY/WET / faderMixAttachment (SA) | mix → mix | mix_.next() → mixGains; curva histórica dry/wet e wet headroom 0.4. |
| decay | Float 0…1 | 0.877465 | DECAY maior / attachDecay (SA) | decay → decay | decay_.next() → setDecay; Freeze + performanceActive força alvo 1 durante gate. |
| moddepth | Float 0…1 | 0.0625 | MOD DEPTH, % / attachModDepth (SA) | moddepth → modulationDepth | modDepth_.next() × 8 → setTankModDepth; excursão dos LFOs do tank. |
| modspeed | Float 0…1 | 0.0466667 | MOD RATE, 0.30…15.30x / attachModSpeed (SA) | modspeed → modulationSpeed | 0.3 + modSpeed_.next() × 15 → setTankModSpeed; multiplica quatro frequências-base (0.10 / 0.15 / 0.12 / 0.18 Hz); audível com depth > 0. |
| damp | Float 0…1 | 0.5 | TONE: HI %, FLAT, LO % / attachDamp (SA) | damp → damp | damp_.next() → applyDamp; metade esquerda High Cut pitch 3…10 com Low Cut neutro 0; direita Low Cut pitch 0…9 com High Cut neutro 10; centro ambos neutros históricos. |
| eq1_gain | Float −24…+24 dB | −11 | PRESENCE, dB / attachEq1 (SA) | eq1_gain → octaveHighShelfDb | updateShelves → makeHighShelf(48000, 140 Hz, 0.707, gain); filtra oitava antes da soma dry; audível quando octaveMode efetivo != Off. |
| eq2_gain | Float −24…+24 dB | +5 | BODY, dB / attachEq2 (SA) | eq2_gain → octaveLowShelfDb | updateShelves → makeLowShelf(48000, 160 Hz, 0.707, gain); mesma condição de Presence. |
| time_scale | Choice Small / Medium / Large (0 / 1 / 2) | 2: Large | SIZE / attachTimeScale (CA) | round(time_scale) → mapReverbSize → reverbSize | applyStaticParameters → setTimeScale(1 / 2 / 4). |
| effect_mode | Choice None / Up Octave / Down Octave / Both Octaves (0…3) | 0: None | OFF/UP/DOWN/UP+DOWN / attachEffectMode (CA) | round(effect_mode) → mapOctaveMode → octaveMode; footswitch==2 && !momentary força Off | Off: mono direto ao reverb; Up: up1×2; Down: down1×2 + down2×2; Both: todos. Em Perform Octave só atua com gate ON. |
| footswitch_mode | Choice Freeze / Overdrive / Effect (0 / 1 / 2) | 0: Freeze | FREEZE/DRIVE/OCTAVE / attachFootswitchMode (CA) | round(footswitch_mode) → mapPerformanceMode → performanceMode; também condiciona octaveMode | Freeze força decay=1 quando ativo; Overdrive aplica drive/compensação históricos ao wet com release; Octave condiciona seleção no adapter. Selecionar ação não abre gate. |
| input_diffusion | Bool false / true | true | DIFFUSION OFF/ON / attachInputDiffusion (BA) | input_diffusion → inputDiffusion | applyStaticParameters → enableInputDiffusion antes do tank. |
| octave_dry_mix | Bool false / true | true | REVERB FEED OCT / OCT + DRY / attachOctaveDryMix (BA) | octave_dry_mix → includeDryInOctavePath; true positivo preservado | processOctave48Sample adiciona 0.5 × dry depois dos shelves, antes de reverb; apenas com oitava efetivamente ativa. |
| bypass | Bool false / true | false | ACTIVE / BYPASSED / attachBypass (BA) | bypass → bypass | bypass_.next() → crossfade para input L/R; janela histórica de 10 ms. |
| momentary_effect | Bool false / true | false | PERFORM / attachMomentaryEffect (BA) | momentary_effect → performanceActive; condiciona octaveMode em Effect | Gate das três ações; mesmo booleano para automação; release preserva smoothing do DSP. |

Nenhum parâmetro foi removido/novo e nenhum controle está sem attachment.
Shelves/Feed continuam editáveis quando inativos, com indicação atenuada.
A indicação considera effect_mode, footswitch_mode e momentary_effect.
Tooltips/descriptions de Presence e Body informam shelves, frequências e dB.

## D. Controles removidos/rebatizados

- REVERB / SPACE GENERATOR → SPACE; PERFORMANCE / OPERATIONAL CONTROL → PERFORM.
- Oct High Shelf → PRESENCE; Oct Low Shelf → BODY.
- Dry Routing e “validação pendente” → REVERB FEED: OCT / OCT + DRY.
- OVERDRIVE → DRIVE na face; choice Overdrive do host intacta.
- Removidos SIGNAL GENERATOR, DRY / WET redundante no título, MOD. APOLLO-RA,
  PLATE + OCTAVE SYSTEM e duplicação de STEREO SPACE PROCESSOR.
- Removidos MOD LFO, lamps redundantes, parafusos, metal escovado e molduras.
- Removida TRIGGER: MIDI / AUTOMATION e toda promessa de MIDI dos tooltips.
- Tone: centro FLAT, lados HI/LO com intensidade relativa ao centro; escala
  central e HIGH CUT / FLAT / LOW CUT explícitos.
- Mod Rate: multiplicador real de velocidade. Hz único seria incorreto porque
  o tank usa quatro LFOs com frequências-base distintas.

## E. Correções funcionais

1. MIDI não anunciado nem implementado; acceptsMidi=false, mensagens ignoradas
   e NEEDS_MIDI_INPUT=FALSE permanecem.
2. Tone define ambos os cutoffs a cada atualização, preservando cada curva.
   Neutro são as posições históricas 13.75 Hz / 14080 Hz, sem novo bypass de filtros.
3. Perform visível e âmbar conforme parâmetro ON, com mouse/Space/Enter.
   Libera no mouseUp, key-up, focusLost, hide, disable e destruição do editor,
   antes de destruir attachment. Sem gesto local, foco não cancela ON da automação.
4. Estado OCTAVE considera gate em Perform Octave, Off e bypass.
5. Foco âmbar fino e ordem dos 15 controles; teclado solicitado ao host;
   setas e double-click reset nos sliders/selectors preservados.

## F. Compatibilidade

PluginProcessor.cpp e EarthParameters.h não foram alterados.
IDs, nomes do host, version hints, ordem, tipos, ranges, defaults e choices do
APVTS são idênticos ao início da milestone. XML Parameters permanece via
copyState/replaceState. Polaridade de octave_dry_mix e crossfade intactos.
A única mudança de áudio é a correção autorizada de histórico do damp.

## G. Testes

Ver VALIDATION.md para comandos e resultados finais.
A regressão foi executada antes do fix e reproduziu o bug.
Depois do fix, golden e damp_history passaram; 48 casos de Tone.
As referências golden existentes não foram regeneradas.
Os 10 SVGs têm XML válido, IDs únicos, gradients resolvidos e viewBoxes escaláveis.

## H. Evidência visual

SVGs são demonstrações vetoriais, não screenshots. Regenerar:

    python Apollo/tools/ui_final/render_svg.py

ApolloTest --ui produz PNGs via createComponentSnapshot do editor JUCE real.
Inclui estados, tamanho mínimo/máximo e rasterização a 100%, 150% e 200%.
Isso não equivale a alterar a escala global do Windows nem a testar em DAW.
