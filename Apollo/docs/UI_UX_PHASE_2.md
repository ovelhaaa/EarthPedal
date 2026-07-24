# Apollo — UI/UX Fase 2: wireframe funcional

## Objetivo e escopo implementado

A Fase 2 substitui a organização geométrica anterior por um wireframe JUCE com
quatro blocos semânticos: **REVERB**, **OCTAVE**, **PERFORMANCE** e **OUTPUT**.
O foco desta entrega é hierarquia, leitura de estado, navegação por teclado e
preservação integral dos contratos do `AudioProcessorValueTreeState` (APVTS).
Não há alteração de DSP, parâmetros, serialização ou automação.

## Arquivos alterados

- `Source/PluginEditor.h` e `Source/PluginEditor.cpp`: estrutura dos painéis,
  attachments existentes, textos de estado, formatação de valores e layout.
- `Source/ApolloLookAndFeel.cpp`: anel de foco para ComboBoxes e botões.

## Mapeamento da arquitetura visual

| Área | Controles e apresentação |
| --- | --- |
| Header | `APOLLO`, estado `ACTIVE — processing` ou `BYPASSED — internal dry path` e affordance discreta de ajuda contextual futura. |
| REVERB | Size (`time_scale`), Pre-delay (`predelay`), Decay, Tone, Mod Rate, Mod Depth e Input Diffusion. |
| OCTAVE | Octave Mode (`effect_mode`), Octave High Shelf (`eq1_gain`), Octave Low Shelf (`eq2_gain`) e **Octave Dry Routing (pending)** (`octave_dry_mix`). |
| PERFORMANCE | Perform Action (`footswitch_mode`) e **PERFORM / GATE** (`momentary_effect`). |
| OUTPUT | Mix (`mix`) destacado em fader vertical com polos **DRY**/**WET**, e Bypass (`bypass`) distinto. |

Os valores são mostrados continuamente: Pre-delay em ms, shelves em dB, e
controles normalizados em percentuais. Mod Rate permanece em %, nunca Hz. Tone
permanece percentual porque a equivalência física/musical ainda não foi
validada.

## Estados dependentes implementados

- `effect_mode == None` mostra **OCTAVE OFF — controls remain automatable** e
  atenua somente os shelves e o roteamento de octave. Os componentes continuam
  visíveis, focáveis, com attachments ativos e recuperáveis por automação e
  presets; a atenuação é exclusivamente visual.
- Gate ativo mostra **FREEZE ACTIVE**, **OVERDRIVE ACTIVE** ou **OCTAVE PERFORM
  ACTIVE**. Com Octave Perform e modo Off, mostra **OCTAVE PERFORM: OCTAVE
  OFF**, sem alterar a lógica do processador.
- `bypass == true` mostra **BYPASSED — internal dry path**. Esta mensagem trata
  somente do bypass interno; não tenta inferir o bypass oferecido pelo host.
- O LookAndFeel fornece foco visível para botão e seletor; knobs mantêm o foco
  padrão JUCE. Estados são apresentados por texto além da cor âmbar.

## Compatibilidade de automação preservada

Nenhum parâmetro foi renomeado, recriado ou reordenado. Os 15 IDs continuam
ligados por attachments aos controles apresentados abaixo:

| ID | Controle da Fase 2 |
| --- | --- |
| `predelay` | Pre-delay |
| `mix` | Mix |
| `decay` | Decay |
| `moddepth` | Mod Depth |
| `modspeed` | Mod Rate |
| `damp` | Tone |
| `eq1_gain` | Octave High Shelf |
| `eq2_gain` | Octave Low Shelf |
| `time_scale` | Size |
| `effect_mode` | Octave Mode |
| `footswitch_mode` | Perform Action |
| `input_diffusion` | Input Diffusion |
| `octave_dry_mix` | Octave Dry Routing (pending) |
| `bypass` | Bypass |
| `momentary_effect` | Perform / Gate |

## Decisões deliberadamente adiadas e limitações conhecidas

- `octave_dry_mix` conserva a nomenclatura explicitamente pendente. A UI não
  atribui uma polaridade ou promessa sonora nova antes da auditoria Up/Down/Both.
- Os nomes de shelves são provisórios, mas não descrevem os filtros como níveis
  de voz. Tone no centro ainda não é rotulado como neutro.
- Não foram implementados medidores, presets, gráficos bitmap, identidade
  visual final, animações, nem resize responsivo completo. O layout tem tamanho
  fixo de wireframe de 900 x 620 para manter todos os parâmetros visíveis.
- A captura gráfica depende de um host/Standalone executável e servidor de
  exibição no ambiente de validação.

## Plano de validação da Fase 2

1. Compilar `ApolloTest` e as variantes plugin/Standalone quando dependências
   JUCE e ambiente permitirem.
2. Confirmar os 15 IDs no processador e nos attachments do editor.
3. Abrir Standalone/host e capturar o estado padrão e, no mínimo, Octave Off,
   Perform ativo ou Bypass interno. Capturas são QA transitório, não arquivos
   versionados, salvo convenção futura do repositório.
4. Exercitar automação de todos os parâmetros, especialmente os controles
   atenuados, e restaurar um estado salvo com Octave Off.
5. Verificar foco por teclado, reset padrão oferecido naturalmente pelos
   componentes JUCE e ausência de alterações de DSP.

## Critérios de entrada para a Fase 3

A Fase 3 só deve iniciar após validação visual em host, auditoria auditiva de
`octave_dry_mix`, aprovação dos nomes dos shelves/Tone e decisão de produto
sobre presets, medição e estratégia de resize. Essas decisões não devem mudar
os IDs, ranges, defaults, ordem ou persistência do APVTS.
