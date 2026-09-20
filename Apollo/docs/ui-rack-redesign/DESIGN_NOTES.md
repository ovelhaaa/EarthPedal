# Apollo — Reforma visual / UI-UX (rack retrofuturista da era Apollo)

> **Natureza das imagens deste diretório:** todas as imagens em
> `Apollo/docs/ui-rack-redesign/` são **CONCEPT RENDER** geradas
> programaticamente por [`render_concepts.py`](render_concepts.py) e
> rasterizadas com Chrome headless. **Nenhuma é screenshot do plugin em
> execução.** O ambiente desta tarefa não conseguiu compilar a GUI JUCE 8 (ver
> §9). O SVG/PNG reproduz fielmente a geometria e os materiais implementados no
> código, mas continua sendo um desenho de conceito.

---

## 1. Objetivo e limites

Reforma visual profunda do editor do plugin **APOLLO**, transformando-o em um
equipamento físico de rack imaginário (inspiração 1968–1975: consoles de
missão, instrumentação científica/laboratorial, broadcast). A tarefa **não**
altera DSP, IDs APVTS, ranges, defaults, tipos, serialização, automação nem o
comportamento de Freeze, Overdrive, octave, bypass, mix, resampling ou
Dattorro.

Arquitetura funcional preservada: **REVERB**, **OCTAVE**, **PERFORMANCE**,
**OUTPUT**, mais o header de identidade/estado.

---

## 2. Duas direções visuais e direção escolhida

Os dois concepts usam **exatamente a mesma arquitetura funcional** e diferem
apenas no material do chassis/placa superior.

### CONCEPT A — DARK FLIGHT COMPUTER
Arquivos: `concept_a_dark_flight_computer.svg/.png`.
Chassis grafite-escuro, painel superior escuro, luzes laranja/vermelhas
localizadas. Sensação de computador de bordo / console de missão.
Vantagem: imersivo e contemporâneo ao vocabulário de ficção científica.
Desvantagem: contraste entre chassis e módulos é pequeno; as legendas
gravadas perdem força e a leitura em uso musical fica menos imediata.

### CONCEPT B — APOLLO LAB EQUIPMENT (hybrid)
Arquivos: `concept_b_lab_equipment.svg/.png`.
Chassis/placa superior em ivory/off-white levemente texturizado, com os
quatro módulos pretos de processamento claramente rebaixados nele.
Vantagem: forte hierarquia, serigrafia gravada legível, contraste
claro/escuro que evoca equipamento aeroespacial e de laboratório.
Desvantagem: mais “claro”; exige disciplina para não parecer vintage
decorativo.

### Direção escolhida: **híbrido = Concept B** — *congelado por enquanto*
Exatamente a preferência conceitual indicada: **chassis/painel estrutural
vintage (ivory) + módulos pretos de processamento**. É a direção implementada
em `PluginEditor.cpp` / `ApolloLookAndFeel.cpp` / `ApolloTheme.cpp`. O Concept A
permanece apenas como variação documentada, obtida trocando só o tema do chassis
(`head_theme(dark=True)`), o que comprova que a arquitetura é agnóstica de tema.

> **Status:** direção visual **fixada (Concept B)** para esta etapa. Qualquer
> retomada do Concept A deve ser tratada como experimento separado, mantendo a
> mesma arquitetura funcional e os 15 IDs APVTS.

---

## 3. Vocabulário de design e materiais

- Geometria modular: quatro módulos físicos distintos rebaixados no chassis.
- Serigrafia/gravação: título de seção em caixa alta com tracking aberto e
  subtítulo técnico alinhado à direita, mais placa de modelo.
- Parafusos nos cantos do chassis e de cada módulo; chanfros; filetes de
  separação; rebaixos (wells) para displays, seletores e lâmpadas.
- Luz vem do **topo/esquerda** em todos os elementos (gradientes e highlights
  consistentes).
- Contraste preto/off-white como base; cor usada com economia:
  - **laranja** = valor de parâmetro / status normal;
  - **vermelho** = bypass / warning / estado especial;
  - **cyan** = indicador secundário (atividade de LFO).
- Sem LED ring completo em todos os knobs; o valor é lido pelo ponteiro físico +
  escala impressa. Sem neon onipresente, sem glassmorphism, sem cards
  arredondados, sem estética gamer/cyberpunk.

### Tipografia
Fontes empacotadas do JUCE (nada externo): sans do sistema para títulos,
rótulos e valores (valores usam a fonte monoespaçada padrão para leitura
técnica). Títulos: caixa alta, bold, kerning aumentado. Rótulos: compactos.
Valores: monoespaçados em displays rebaixados laranja.

---

## 4. Paleta centralizada

Toda a cor está em `Source/ApolloTheme.h` (`namespace ApolloTheme`), sem
números mágicos duplicados: chassis (light/mid/dark/edge/shadow), grafite
(light/graphite/dark/deep/edge), placa preta de display, textos do painel e do
chassis, highlight/shadow de gravação, laranja (+deep/glow), vermelho (+deep),
cyan, lâmpada apagada, metal (screw/cap/highlight).

Também centralizados: as dimensões do design (900 × 620), as chaves de
propriedade dos controles (`apolloStyle`, `apolloDim`, `apolloDisplay`,
`apolloCaption`, `apolloVertical`, `apolloBipolar`) e os helpers vetoriais de
material (`drawRaisedPlate`, `drawInsetWell`, `drawBrushedMetal`, `drawScrew`,
`drawEngravedText`, `drawLamp`).

---

## 5. Arquitetura de componentes JUCE

| Arquivo | Responsabilidade |
| --- | --- |
| `Source/ApolloTheme.h/.cpp` | Paleta, fontes, geometria do design, helpers vetoriais e componentes reutilizáveis. |
| `Source/ApolloLookAndFeel.h/.cpp` | Um único LookAndFeel que pinta knobs, fader, botões, seletores, labels e popup. Geometria separada em helpers (`drawKnobScale`, `drawVintageKnob`, `drawRocker`, `drawMomentary`, `drawBypass`, `drawFocusHalo`). |
| `Source/PluginEditor.h/.cpp` | Composição do rack: panneis, controles, attachments, layout escalável e microcopy de estado. |

Componentes reutilizáveis criados:

- `ApolloRackPanel` — placa de módulo com chanfro, parafusos, título/subtítulo
  gravados e lâmpada de status. **Decorativo** (não intercepta mouse, não entra
  no foco).
- `ApolloAnnunciator` — janela de vidro escuro rebaixada com lâmpada interna e
  legenda (ex.: `FREEZE ACTIVE`). **Decorativo**.
- `ApolloStatusLamp` — lâmpada piloto isolada. **Decorativo**.
- `ApolloSelector : juce::ComboBox` — continua sendo um `ComboBox` real (mesmo
  `ComboBoxAttachment`), mas desenha todas as posições como um seletor mecânico
  segmentado e seleciona a posição clicada. A zona final reserva o popup nativo
  para teclado/acessibilidade.

O `drawRotarySlider` foi decomposto; não concentra centenas de linhas — a
escala impressa e o corpo do knob são funções dedicadas.

---

## 6. Layout, geometria e resize

Design base **900 × 620**. Todo o layout é autorado em coordenadas do design e
escalado uniformemente em `resized()`:

- `s = min(largura/900, altura/620)`; conteúdo centralizado; aspect ratio fixo
  já garantido pelo `Constrainer` (`900×620` … `1400×980`).
- `ApolloLookAndFeel::setUiScale(s)` e `applyFontScale(s)` mantêm traços,
  fontes, ticks e labels proporcionais, de forma que a UI continua legível e
  funcional de 100% ao maximo (~155%).

Blocos (coordenadas de design):

| Módulo | Bounds | Papel |
| --- | --- | --- |
| REVERB | 14,108 · 580×300 | Maior módulo; “coração” da máquina. |
| OUTPUT | 604,108 · 282×360 | Coluna estreita; **fader centralizado** (DRY/WET), sem rótulo MIX. |
| OCTAVE | 14,418 · 580×198 | Seção auxiliar; modo em push buttons e shelves alinhados na mesma faixa. |
| PERFORMANCE | 604,478 · 282×138 | Coluna estreita e baixa; 3 push buttons + lâmpada de painel. |

O painel OUTPUT e o PERFORMANCE foram **estreitados** (282); o PERFORMANCE ficou
mais **baixo** (138), deixando o OUTPUT mais **alto** (360) para dar curso ao
fader de mix.

Header (placa superior ivory): wordmark **APOLLO** gravado, subtítulo
`STEREO SPACE PROCESSOR` / `PLATE + OCTAVE SYSTEM`, placa de modelo
`MOD. APOLLO-RA`, e o **botão BYPASS interno** no lugar do antigo indicador
`SYSTEM / ACTIVE`.

---

## 7. Controles

### Knobs (estilo Marconi / instrumento vintage)
Corpo em baquelite preto com saia estriada (flutes), domo escuro, aro cromado e
cap central cromado, ponteiro mecânico ivory e **escala impressa ao redor**
(ticks maiores a cada 25%, menores entre eles). Sem anel LED. Foco de teclado =
halo laranja temporário. Double-click reset preservado (valor default real do
parâmetro APVTS).

- **Diâmetro uniforme por painel:** todos os knobs do REVERB têm 84 (Pre-delay,
  Decay, Tone, Mod Rate, Mod Depth) e ambos os shelves do OCTAVE têm 84. Não há
  mais knob “destacado” por tamanho.
- **Pre-delay**: valor em ms.
- **Decay**: um dos controles principais, mas com o mesmo diâmetro dos demais.
- **Tone**: representa o bipolar existente — end labels `HI CUT` ← centro →
  `LO CUT`, com tick central destacado em laranja. **Não inventa frequências.**
- **Mod Rate / Mod Depth**: bloco secundário de modulação, mesmo diâmetro. O
  indicador `MOD LFO` fica **ao lado desse bloco**, e não na coluna dos botões.
- Valores em displays rebaixados, monoespaçados laranja.

### Lâmpadas de painel com tamanho padrão
A lâmpada de status ao lado do título de cada módulo tem **tamanho fixo de 16
unidades de design** em todas as seções (antes variava com a altura do painel),
garantindo alinhamento visual entre os módulos da fileira superior e inferior.

### Size, Octave Mode e Perform Action — push buttons interlocked (estilo Boss DC-2)
`time_scale`, `effect_mode` e `footswitch_mode` continuam `juce::ComboBox` +
`ComboBoxAttachment`; o `ApolloSelector` apenas desenha um banco de **botões
retangulares interlocked**: a posição selecionada fica abaixada (recessed) e as
demais levantadas, como as teclas do Boss DC-2. Clicar em qualquer tecla a
seleciona; teclado (setas) e automação continuam funcionando.

- **Size**: `SMALL / MEDIUM / LARGE`, em pilha **vertical** com a mesma largura
  do botão de Input Diffusion (um em cima do outro).
- **Octave Mode**: `OFF / UP / DOWN / UP+DOWN`, horizontal.
- **Perform Action**: `FREEZE / OVERDRIVE / OCTAVE`, horizontal.

O indicador textual `OCTAVE OFF` foi removido; o estado do módulo é dado pela
lâmpada do painel.

### Input Diffusion
Chave tipo rocker física (`ON/OFF`), ainda `ToggleButton` + `ButtonAttachment`.

### Octave High/Low Shelf
Mesmos `eq1_gain` / `eq2_gain` e attachments. Os dois knobs passaram a ficar
**alinhados verticalmente com o banco de modo e com o dry routing**, na mesma
faixa, em vez de deslocados para baixo; ambos com o mesmo diâmetro (84).

### Performance
- `footswitch_mode` = os 3 push buttons interlocked acima.
- `momentary_effect` **não tem mais botão na UI**: é disparado por
  **MIDI / automação do host**. O `ButtonAttachment` é mantido (componente
  oculto) para preservar o contrato do parâmetro; a UI apenas reflete o estado.
- A **lâmpada do painel** (à direita de `OPERATIONAL CONTROL`) indica o estado:
  apagada = pronto; laranja = ação ativa; vermelho = Octave Perform sem modo.
- Não há mais o botão `PERFORM` grande nem a faixa de texto de estado; há apenas
  a nota discreta `TRIGGER: MIDI / AUTOMATION` abaixo dos botões.

### Output
- `mix` = fader vertical físico **centralizado** no painel, com slot rebaixado,
  escala impressa 0–100, `WET` em cima e `DRY` embaixo. **Sem rótulo MIX e sem
  valor numérico** — só o slider e a escala.
- O cap do fader é **mais estreito na horizontal e mais alto na vertical**,
  aproximando-se de um fader de mixer real.
- O **Bypass saiu do OUTPUT** e passou para o header.

### Bypass (header)
`bypass` = grande pushbutton retangular iluminado no lugar do antigo indicador
`SYSTEM/ACTIVE`. Ativo: luz laranja discreta + `ACTIVE`. Bypassado: vermelho
evidente + `BYPASSED`. Mantém o parâmetro `bypass` interno (≠ bypass do host).

---

## 8. Estados e microcopy

- **Octave Mode = Off**: em vez de `opacity=0.42` em tudo, a solução é
  skeuomórfica: a lâmpada `OCTAVE` apaga, o backlight do módulo escurece, os
  rótulos secundários ficam mais escuros e os knobs/rocker são redesenhados com
  contraste reduzido. Os controles **permanecem fisicamente presentes,
  focáveis, editáveis, automatizáveis e recuperáveis por preset**.
- **Bypass interno**: header `BYPASSED` (lâmpada vermelha), módulos escurecem,
  controles continuam legíveis/editáveis/automatizáveis.
- **Freeze/Overdrive/Octave Perform**: indicado pela lâmpada do painel
  PERFORMANCE (laranja quando ativo; vermelho quando Perform = Octave e Octave
  Mode = Off). Não há mais faixa de texto.
- **`octave_dry_mix`**: mantém rótulo tecnicamente neutro **`DRY ROUTING`** e
  documenta que a copy final depende de auditoria de semântica (ver
  `DSP_NOTES.md`). Nenhuma polaridade nova foi prometida.

O header agora traz o **botão BYPASS** no lugar do indicador `SYSTEM/ACTIVE`;
`ACTIVE`/`BYPASSED` continuam refletindo a semântica real do bypass interno. O
antigo “? Contextual help” foi removido da pintura; a ajuda contextual real
continua sendo oferecida por tooltips/descrições nos controles.

---

## 9. Validação executada

### 9.1 Compilação (bloqueada pelo ambiente)
Toolchains disponíveis: MinGW-w64 **GCC 14.2**, **clang 19.1** (target MinGW),
CMake e Ninja. **Não há MSVC nem Windows SDK** instalados.

O JUCE 8.0.0 inclui Direct2D incondicionalmente no Windows
(`juce_graphics/juce_graphics.cpp` inclui `native/juce_DirectX_windows.h` e os
`juce_Direct2D*_windows`), exigindo tipos do Windows SDK moderno
(`ID2D1SpriteBatch`, `ID2D1DeviceContext3/4`, `DWRITE_GLYPH_IMAGE_FORMATS`)
ausentes nos headers do MinGW. Resultado do `cmake` real:

```
CMake Error at .../juceaide/CMakeLists.txt:143 (message):
  Failed to build juceaide
.../juce_DirectX_windows.h -> juce_CompilerWarnings.h:183: error ...
```

Conclusão honesta: **Standalone e VST3 não puderam ser compilados neste
ambiente**. A validação de compilação foi substituída por um *syntax check*
com `clang++ -std=c++20 -fsyntax-only` incluindo os headers reais do JUCE 8.0.0,
que passou para os três TUs alterados:

```
ApolloTheme.cpp         exit=0
ApolloLookAndFeel.cpp   exit=0
PluginEditor.cpp        exit=0
```

Isso valida tipos, assinaturas de override, uso de API JUCE e consistência do
layout, mas **não substitui** um build/link reais nem validação runtime.

### 9.2 Matriz de estados (concept renders)
Cada estado foi gerado e conferido visualmente:

| Estado | Arquivo | Critério conferido |
| --- | --- | --- |
| Default | `01_default.png` | Hierarquia, push banks interlocked, escalas, header com BYPASS. |
| Octave ativo (Up+Down) | `02_octave_active.png` | Módulo aceso, tecla `UP+DOWN` pressionada, shelves alinhados. |
| Freeze ativo | `03_freeze_active.png` | Lâmpada do PERFORMANCE laranja, tecla `FREEZE` pressionada. |
| Overdrive ativo | `04_overdrive_active.png` | Lâmpada laranja, tecla `OVERDRIVE` pressionada. |
| Octave Perform ativo | `05_octave_perform_active.png` | Tecla `OCTAVE` pressionada + módulo OCTAVE aceso. |
| Bypassed | `06_bypassed.png` | Botão do header vermelho/`BYPASSED`; REVERB e OCTAVE escurecidos. |
| Foco de teclado | `07_keyboard_focus.png` | Halo laranja inequívoco no knob Decay. |
| Tamanho máximo | `08_max_size.png` | Layout 1400×980 sem corte/sobreposição. |

### 9.3 Semântica preservada
Confirmado estaticamente que os **15 IDs** existem no processador e todos estão
ligados no editor (7 via `SliderAttachment`, 1 para `mix`, 3 via
`ComboBoxAttachment`, 4 via `ButtonAttachment`). A ordem de foco segue a
arquitetura; como `momentary_effect` deixou de ter botão visível (disparo por
MIDI/automação), ele não é mais um ponto de foco — o parâmetro continua ligado
por `ButtonAttachment` em componente oculto e permanece automatizável.

### 9.4 Não testado (limitações)
- Build/link reais, HiDPI 125/150/200% reais, hosts DAW, VST3/AU runtime.
- Disparo do `momentary_effect` por MIDI/automação em host.
- Automação em host e leitura por leitor de tela.
- Capturas reais do Standalone (ver §11).

---

## 10. Arquivos alterados/adicionados

**Adicionados**
- `Source/ApolloTheme.h`, `Source/ApolloTheme.cpp`
- `docs/ui-rack-redesign/render_concepts.py`
- `docs/ui-rack-redesign/*.svg` (10), `*.png` (10),
  `APOLLO_RACK_CONTACT_SHEET.png`
- `docs/ui-rack-redesign/DESIGN_NOTES.md` (este), `DSP_NOTES.md`

**Alterados**
- `Source/ApolloLookAndFeel.h`, `Source/ApolloLookAndFeel.cpp` (reescritos)
- `Source/PluginEditor.h`, `Source/PluginEditor.cpp` (reescritos)
- `CMakeLists.txt` (adiciona `Source/ApolloTheme.cpp` aos alvos `Apollo` e
  `ApolloTest`)

**Não alterados**
- `Source/PluginProcessor.*` e todo o `Source/DSP/**`.

---

## 11. Como substituir por screenshots reais

Quando houver MSVC + Windows SDK (ou Linux com X11/GTK) disponível:

1. `cmake -S Apollo -B Apollo/build -G Ninja -DCMAKE_BUILD_TYPE=Release`.
2. `cmake --build Apollo/build --target Apollo_All`.
3. Abrir o Standalone, ajustar os estados da §9.2 e capturar em 100%, 125%,
   150% e 200%.
4. Substituir/complementar os `*.png` deste diretório, removendo a marca de
   CONCEPT RENDER apenas dos arquivos efetivamente capturados.

---

## 12. Confirmações finais

- **IDs APVTS, ranges, defaults, tipos, serialização e automação: inalterados.**
- **DSP, roteamento, Freeze, Overdrive, octave, bypass, mix, resampling e
  Dattorro: inalterados.**
- `footswitch_mode`, `time_scale` e `effect_mode` continuam `ComboBox` +
  `ComboBoxAttachment`, agora desenhados como push buttons interlocked (DC-2).
- `momentary_effect` continua booleano sem latch. Por decisão de produto ele
  **não tem mais controle visível**: é disparado por MIDI/automação, e o
  `ButtonAttachment` é mantido em componente oculto para preservar o contrato.
- `bypass` continua sendo o parâmetro interno e agora vive no header (≠ bypass do
  host).
- Double-click reset e controle por teclado preservados.
- Estados visuais dependem de texto + lâmpada, não apenas de cor.
- Componentes decorativos não interceptam mouse nem entram na ordem de foco.
