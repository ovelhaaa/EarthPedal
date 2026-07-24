# Apollo — UI/UX Fase 4: acessibilidade, foco e resize seguro

## Escopo implementado

A Fase 4 reforça acessibilidade básica, navegação por teclado, foco visual,
legibilidade e redimensionamento do editor sem alterar DSP, parâmetros APVTS,
ranges, defaults, serialização, attachments ou configuração de formatos.

## Acessibilidade e teclado

- Controles visíveis automatizáveis recebem foco por teclado quando o JUCE 8 do
  projeto permite: sliders, ComboBoxes e ToggleButtons.
- Foram adicionados `title` e `description` em controles principais como melhor
  esforço para leitores de tela e hosts que exponham a árvore de acessibilidade
  do JUCE.
- A ordem explícita de foco segue a arquitetura: REVERB, OCTAVE, PERFORMANCE e
  OUTPUT. O header permanece informativo e não cria um controle duplicado de
  bypass.
- Sliders preservam gestos nativos de teclado do JUCE; ComboBoxes preservam
  navegação por setas; toggles usam comportamento nativo de botão.
- `momentary_effect` continua sendo um booleano APVTS automatizável: mouse down,
  Space ou Return escrevem `true`; mouse up, liberação da tecla ou perda de foco
  escrevem `false`. Automação do host continua refletida pela attachment.

## Contraste e estados visuais

- Texto atenuado foi elevado para melhor contraste sobre o fundo escuro.
- Estados ativos e indisponíveis usam texto além de cor: bypass interno mostra
  `BYPASS — On/Off`, Octave Off explica que os controles permanecem
  automatizáveis e Performance mostra a ação ativa ou a ausência de modo de
  oitava.
- Foco de teclado usa contornos, não apenas mudança de cor. O fader vertical de
  Mix agora recebe anel de foco como os knobs, ComboBoxes e botões.
- Controles de OCTAVE atenuados em `effect_mode == Off` continuam legíveis,
  focáveis e anexados ao APVTS.

## Resize e HiDPI

- O editor agora é redimensionável com proporção fixa baseada no tamanho padrão
  `900 x 620`.
- Limites seguros: mínimo `760 x 540`, máximo `1400 x 980`.
- Constantes locais centralizam os tamanhos do editor para evitar números
  mágicos dispersos.
- O ambiente de CI/terminal usado nesta fase não fornece display/host gráfico;
  portanto a validação HiDPI real em 125%, 150% ou 200% e capturas Standalone
  ficam pendentes para QA manual. A validação realizada foi build, teste DSP e
  inspeção estática de layout/foco.

## Compatibilidade de formatos e host

O projeto declara `VST3`, `AU` e `Standalone` no CMake. Esta fase não altera
essa configuração. Também não altera os 15 IDs de automação, tipos, ranges,
skew, defaults, persistência, serialização, DSP, roteamento ou semântica de
bypass interno e `momentary_effect`.

Sem hosts reais disponíveis neste ambiente, a validação de host ficou limitada
a build/inspeção estática. A validação em DAWs deve confirmar Tab/Shift+Tab,
leitores de tela onde aplicável, automação atualizando UI sem flicker evidente,
bypass interno distinto do bypass de host e renderização em VST3/AU/Standalone.

## Matriz de QA recomendada

| Estado | Verificação visual/teclado | Observação de contrato |
| --- | --- | --- |
| Padrão | Mix e estado `Active — processing` legíveis; foco percorre REVERB → OCTAVE → PERFORMANCE → OUTPUT. | Nenhum parâmetro ou DSP alterado. |
| Octave Off | Shelves e roteamento atenuados com texto `OCTAVE OFF — controls remain automatable`. | Componentes seguem focáveis e automatizáveis. |
| Freeze ativo | Performance mostra `Freeze Active`; Decay mantém valor de retorno. | `momentary_effect` continua booleano momentâneo. |
| Overdrive ativo | Performance mostra `Drive Active`. | Sem parâmetro novo de drive. |
| Octave Perform + Octave Off | Performance mostra ausência de modo de oitava. | Gate não altera `effect_mode`. |
| Bypass interno | Header e botão mostram bypass interno, distinto do host. | Demais controles continuam editáveis/automatizáveis. |
| Resize mínimo/máximo | Sem sobreposição crítica; áreas clicáveis permanecem confortáveis. | Proporção fixa preserva arquitetura visual. |
