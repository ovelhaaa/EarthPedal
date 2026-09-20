# Apollo — Observações de DSP (auditoria apenas, sem alterações)

Estas observações foram levantadas ao ler `Source/PluginProcessor.cpp` e os
headers em `Source/DSP/**` durante a reforma visual. **Nenhuma delas foi
corrigida nesta tarefa.** O comportamento sonoro, os IDs APVTS, os ranges, os
defaults e a serialização permanecem exatamente como estavam. Cada item é uma
sugestão de auditoria futura, com severidade provável entre parênteses.

## 1. `octave_dry_mix` — semântica ambígua (P2, produto)

Em `PluginProcessor.cpp:365-367`:

```cpp
float dryLevel = 0.5f;
if (!octave_dry_mix || effect_mode == 2)
    mix += dryLevel * buff[j];
```

O nome `octave_dry_mix` sugere “misturar o dry no ramo de oitava quando ligado”,
mas o código soma o dry quando o booleano é **falso** (`!octave_dry_mix`) **ou**
quando o modo é `Down`. É uma dupla negação com exceção por modo, difícil de
descrever para o usuário. Por isso a UI mantém o rótulo neutro `DRY ROUTING`.

- **Auditoria sugerida:** teste auditivo Up / Down / Up+Down com o booleano
  ligado/desligado; tabela entrada×saída; só então aprovar a copy final.
- **Não alterar ID, polaridade ou valor** antes da auditoria.

## 2. Compensação de latência do ramo de oitava (P2)

`prepareToPlay` calcula (`PluginProcessor.cpp:154-165`):

```cpp
float latencySamples = 89.0f * (float)(sampleRate / 48000.0);
latencySamples += 8.0f;
setLatencySamples((int) latencySamples);
```

É uma estimativa fixa (64 de FIFO + ~22 de FIR + ~1.5 de Lagrange + 8 do IIR
anti-alias). Os filtros IIR e o resampling de Lagrange têm atraso de grupo
dependente da frequência, então a compensação do dry nunca é exata em todas as
frequências nem em todos os sample rates.

- **Auditoria sugerida:** medir atraso de grupo real (impulso) em 44,1/48/96 kHz
  e avaliar cancelamento dry/wet; documentar tolerância.

## 3. Unders/overs de FIFO no ramo de oitava (P2)

No mesmo bloco, se `slideDownValid < required48k`, o código faz
`octaveOutTemp.clear()` (`PluginProcessor.cpp:391-407`), ou seja, o ramo de
oitava fica mudo naquele bloco. O pré-preenchimento (`slideDownValid = 64`)
mitiga, mas blocos muito grandes ou taxas não múltiplas podem causar “dropouts”.

- **Auditoria sugerida:** stress com blocos 1..8192 e sample rates variados;
  verificar se o clear silencioso é aceitável ou se precisa de fallback.

## 4. Troca abrupta de `input_diffusion` (P3)

`reverb.enableInputDiffusion(input_diffusion)` é chamado a cada bloco
(`PluginProcessor.cpp:255`) sem suavização. Alternar o toggle pode gerar um
clique/degradação de difusão instantânea.

- **Auditoria sugerida:** crossfade curto ou mudança em boundary de buffer.

## 5. Coeficientes de shelf recalculados sem smoothing (P3)

`eq1`/`eq2` recriam coeficientes ao detectar mudança (`PluginProcessor.cpp:246-253`).
Não há interpolação de coeficientes; automação rápida de `eq1_gain`/`eq2_gain`
pode gerar zipper noise.

- **Auditoria sugerida:** `SmoothedValue` no ganho e recomputo em rampa.

## 6. `setTimeScale` a cada bloco (P3)

`reverb.setTimeScale(setTimeScale)` é chamado sempre, mesmo sem mudança
(`PluginProcessor.cpp:211`). Dependendo do Dattorro isso pode reposicionar
delays por bloco.

- **Auditoria sugerida:** chamar apenas quando o índice de `time_scale` mudar e,
  se necessário, fazer crossfade.

## 7. Freeze força decay sem crossfade (P3)

`current_freezeDecay.setTargetValue(1.0f)` durante Freeze
(`PluginProcessor.cpp:415-419`). O smoother é de 5 ms, mas a transição para
decay=1 pode produzir um salto audível de cauda.

- **Auditoria sugerida:** envelope dedicado ao entrar/sair de Freeze.

## 8. Overdrive: fórmula de ganho e mono (P3)

`overdriveLeft/Right` recebem a saída **mono** do reverb em L/R idênticos
(`PluginProcessor.cpp:468`), e o ganho de compensação
`1 - (OD^2 * 2.8 - 0.1296)` (`:476-477`) depende de `OD` estar numa faixa
segura; em valores fora de `[0.4, 0.6]` pode haver atenuação/boost inesperado.

- **Auditoria sugerida:** varrer `setOD` e medir headroom; documentar curva.

## 9. Soma para mono na entrada (P2)

`monoIn = (inputL + inputR) * 0.5f` (`PluginProcessor.cpp:458`) alimenta o
reverb, mas o dry de saída usa `inputL/inputR` originais. Em material estéreo
muito decorrelacionado isso pode reduzir a sensação de largura do wet e criar
diferença de imagem dry/wet.

- **Auditoria sugerida:** avaliar processamento estéreo do Dattorro (ele já tem
  L/R) em vez de alimentar L=R.

## 10. `getTailLengthSeconds()` retorna 0.0 (P2)

`PluginProcessor.cpp:62`. Com cauda de reverb longa (decay até 1.0 + Freeze),
reportar 0.0 pode fazer o host truncar render offline / congelar antes da cauda
terminar.

- **Auditoria sugerida:** retornar estimativa compatível com decay/Freeze.

## 11. Diagnóstico `std::cout` no processamento (P3)

`printed1c` imprime no primeiro bloco com octave ativo
(`PluginProcessor.cpp:309-313`) e o destrutor imprime métricas. Não é tempo
real-safe e polui o console do host.

- **Auditoria sugerida:** remover/substituir por logging condicionado a build
  de debug.

## 12. `energy_in_above_24k` / `prev_x_in/out` (P3)

São estado de diagnóstico com diferença de primeira ordem usada como
pseudo-filtro passa-alta; não afetam o áudio, mas são membros públicos
(`PluginProcessor.h:40`) e acumulam indefinidamente.

- **Auditoria sugerida:** isolar/bloquear em builds de release.

## 13. Dependência de sample rate do ramo de oitava (P2)

O `OctaveGenerator` é instanciado com `48000/resample_factor = 8000 Hz`
(`PluginProcessor.cpp:78`) e os shelves usam 8 kHz (`:82-83`), assumindo o
caminho resampleado para 48 kHz. Em hosts a 96 kHz o FIR de anti-alias e o
Lagrange mudam de comportamento; a branch `@ 8 kHz` continua fixa.

- **Auditoria sugerida:** validar a resposta dos shelves e do OctaveGenerator
  nas três taxas; confirmar que a filtragem anti-alias cobre 24 kHz.

## 14. FIFOs com shift O(n) (P3, performance)

`slideUp`/`slideDown` fazem loops de deslocamento por amostra
(`PluginProcessor.cpp:333-337`, `:400-403`). Com buffers de 4096 e blocos
grandes, o custo é quadrático no bloco.

- **Auditoria sugerida:** usar buffer circular / `juce::dsp::DelayLine`.

---

### Resumo de prioridade

| # | Tema | Severidade |
| --- | --- | --- |
| 1 | `octave_dry_mix` semântica/copy | P2 (produto) |
| 9 | Soma mono na entrada do reverb | P2 |
| 10 | `getTailLengthSeconds = 0` | P2 |
| 13 | Ramo de oitava depende de 48 kHz | P2 |
| 2 | Latência de compensação aproximada | P2 |
| 3 | Underrun silencioso da FIFO | P2 |
| 4,5,6,7,8,11,12,14 | Suavização/logging/perf | P3 |

Nenhum item acima foi implementado nesta entrega. A reforma visual toca
exclusivamente UI (editor, LookAndFeel, tema) e a lista de fontes do
`CMakeLists.txt`.
