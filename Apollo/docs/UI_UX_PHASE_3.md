# Apollo — UI/UX Fase 3: feedback de controles

## 1. Objetivo e escopo implementado

Esta fase aperfeiçoa os controles já organizados na Fase 2 sem tocar no DSP,
nos parâmetros APVTS, nos seus IDs, ranges, defaults, skew, serialização ou
ordem. A entrega concentra-se em leitura de valor, reset previsível, gesto de
performance, estados de interação e microcopy curta.

## 2. Componentes e interações implementados

- Todos os sliders contínuos têm rótulo e valor persistente: `predelay`,
  `decay`, `damp`, `modspeed`, `moddepth`, `eq1_gain`, `eq2_gain` e `mix`
  permanecem nos mesmos attachments. `time_scale` é o seletor discreto
  **Size**, e `input_diffusion` é booleano, por isso ambos mostram a escolha
  atual em vez de um valor contínuo.
- Os controles contínuos configuram duplo clique para o default real obtido do
  parâmetro APVTS. Isso escreve pelo attachment existente, portanto não altera
  o default do parâmetro nem ignora automação/recall do host.
- **Octave Mode** mantém o ComboBox seguro ligado a `effect_mode`, com as
  escolhas visíveis **Off, Up, Down, Up + Down** (a primeira corresponde ao
  estado técnico `None`). **Perform Action** mantém **Freeze, Overdrive,
  Octave Perform** ligado a `footswitch_mode`; a escolha nunca é apresentada
  como uma ativação.
- O botão **Perform / Gate** continua no mesmo `ButtonAttachment` de
  `momentary_effect`. Mouse down/Space/Return escreve true; mouse up/liberação
  da tecla escreve false. Atualizações de automação continuam sendo refletidas
  pelo attachment, inclusive quando não há gesto local em curso.
- `octave_dry_mix` conserva o rótulo provisório **Octave Dry Routing (pending)**
  e uma leitura On/Off. Nenhuma alegação de polaridade sonora foi adicionada.

## 3. Formatação de valores e unidades

| Parâmetro | Leitura | Justificativa |
| --- | --- | --- |
| `time_scale` | Small / Medium / Large | Escolhas discretas do APVTS. |
| `predelay` | ms | O processador passa o valor em segundos a `setPreDelay`; a UI mostra valor × 1000. |
| `decay`, `damp`, `modspeed`, `moddepth`, `mix` | % | Escala neutra para valores normalizados cuja unidade musical/física não foi confirmada. Mod Rate não afirma Hz. |
| `eq1_gain`, `eq2_gain` | dB | São ganhos usados diretamente para criar os shelves do DSP. |
| `input_diffusion` | On / Off | Booleano. |

Mix inclui polos textuais **DRY** e **WET** além do percentual. O valor de
Tone/Damp permanece percentual; não afirma frequência ou neutralidade no
centro.

## 4. Comportamento do Perform/Gate

O gesto é momentâneo e preserva o booleano automatizável, em conformidade com
o consumo atual em `PluginProcessor.cpp`. O desenho apresenta **HOLD** durante
pressão, independentemente da cor. O texto de ação no botão torna explícita a
seleção atual. Caso o host automatize o parâmetro, seu estado é a fonte de
verdade da UI e dos textos de estado.

## 5. Estados visuais e feedback textual

- Hover recebe contorno âmbar; pressão altera preenchimento e adiciona
  **HOLD**; foco de teclado recebe anel/contorno de foco nos knobs, ComboBoxes
  e botões.
- O estado ativo usa texto, e não somente âmbar: **Freeze Active**, **Drive
  Active**, **Octave Perform Active** ou **No Octave Mode Selected**.
- Bypass interno usa **Bypassed — internal dry path** e o botão **BYPASS —
  On/Off**. Isto não representa o bypass do host.
- Com Octave Mode Off, shelves e roteamento ficam atenuados, mas continuam
  visíveis, focáveis, anexados, automatizáveis e recuperáveis. O texto
  **OCTAVE OFF — controls remain automatable** explicita essa condição.

## 6. Tooltips e microcopy aplicados

Foram adicionados tooltips para Size, Pre-delay, Decay, Tone, Mod Rate, Mod
Depth, Input Diffusion, Octave Mode, ambos os shelves de OCTAVE, o roteamento
dry/octave pendente, Perform Action, Perform/Gate, Mix e Bypass. A copy evita
unidades não confirmadas e descreve o resultado ou gesto, não a implementação.

## 7. Compatibilidade de automação preservada

Os 15 IDs APVTS permanecem literalmente inalterados: `predelay`, `mix`,
`decay`, `moddepth`, `modspeed`, `damp`, `eq1_gain`, `eq2_gain`,
`time_scale`, `effect_mode`, `footswitch_mode`, `input_diffusion`,
`octave_dry_mix`, `bypass` e `momentary_effect`. A implementação não modifica
o layout APVTS nem o processador; ela apenas configura componentes já ligados
por attachments. A atenuação de OCTAVE é exclusivamente visual e não remove
acesso por teclado, valor, attachment ou automação.

## 8. Limitações conhecidas e decisões adiadas

- A semântica e a polaridade auditiva de `octave_dry_mix` continuam pendentes,
  especialmente no modo Down.
- O nome dos shelves é provisório; Tone no centro e Mod Rate em Hz aguardam
  validação auditiva/DSP.
- Não há medidores, presets, redimensionamento responsivo, MIDI learn,
  animações complexas ou alteração de DSP nesta fase.
- A validação visual em Standalone depende de display e de um build JUCE
  completo disponíveis no ambiente.

## 9. Plano de validação executado

1. `git diff --check`.
2. Busca dos 15 IDs no processador e editor antes/depois da alteração.
3. Configuração/build CMake de Apollo e execução de `ApolloTest`, quando as
   dependências e o ambiente permitirem.
4. Inspeção estática do gesto gate, attachments, defaults de duplo clique,
   tooltips e textos de estado.
5. Captura de estados padrão, Octave Off, performance/bypass e valor contínuo
   quando houver execução gráfica disponível; capturas não são versionadas sem
   convenção do repositório.

## 10. Critérios de entrada para a Fase 4

A Fase 4 pode aprofundar a árvore de acessibilidade, anúncios a leitores de
tela, ordem de tabulação testada em hosts e a validação auditiva de
`octave_dry_mix`. Ela deve manter os mesmos 15 IDs e validar o comportamento
momentâneo com hosts/formatos alvo antes de expandir atalhos ou gestos.
