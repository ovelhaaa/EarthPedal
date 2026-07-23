# Apollo — Especificação de UI/UX: Fase 1

## 1. Objetivo e limites da Fase 1

Esta especificação transforma o inventário de parâmetros atual do Apollo em uma
arquitetura de informação, nomenclatura visível e microcopy para a Fase 2. O
objetivo é que o músico entenda **o resultado sonoro**, a relação entre modo e
ação de performance e o estado global do efeito antes de qualquer decisão de
estética final.

### Dentro do escopo

- Organizar os 15 parâmetros APVTS existentes em `REVERB`, `OCTAVE`,
  `PERFORMANCE` e `OUTPUT`.
- Definir rótulos, formatação pretendida, tooltips e estados de interface.
- Preservar os IDs, os nomes de automação existentes, as faixas, os defaults,
  a serialização e o DSP.

### Fora do escopo

- LookAndFeel final, medidores, clipping, presets, redimensionamento
  responsivo, novos parâmetros e qualquer mudança de DSP/automação.
- Alterar o comportamento de bypass, Freeze, Overdrive, oitava ou o estado
  persistido.

> **Nota de descoberta:** `Apollo/docs/UI_UX_PHASE_0.md`, indicado como fonte
> desta fase, não está presente na árvore de trabalho na data desta
> especificação. Esta Fase 1 foi, portanto, reconciliada diretamente com o
> processador, editor, notas de portabilidade e README disponíveis. Quando o
> documento da Fase 0 for recuperado, esta nota e quaisquer divergências devem
> ser revisadas antes de congelar a Fase 2.

### Princípios de linguagem

1. Mostrar termos musicais e orientados ao resultado; IDs e termos de
   implementação ficam no host/automação.
2. Distinguir **seleção** (`Perform Action`, `Octave Mode`) de **ação enquanto
   sustentada** (`Perform / Gate`).
3. Usar percentuais para controles normalizados quando não houver unidade
   musical confirmada. `Pre-delay` é a exceção confirmada: o valor APVTS é
   enviado como segundos ao DSP, portanto pode ser exibido de `0–1000 ms`.
4. Atenuação visual informa contexto, mas nunca remove o componente da árvore
   de acessibilidade, da recuperação de preset ou da leitura/escrita por
   automação.

## 2. Arquitetura de informação proposta

| Grupo | Pergunta do músico | Conteúdo | Justificativa |
| --- | --- | --- | --- |
| `REVERB` | “Como o espaço responde?” | Size, Pre-delay, Decay, Tone, Mod Rate, Mod Depth, Input Diffusion | Reúne tempo, cor e movimento do plate antes de escolhas de oitava. |
| `OCTAVE` | “Que sinal entra no reverb?” | Octave Mode, dois shelves de cor da ramificação, roteamento dry/octave pendente | O modo ativa a ramificação; suas correções tonais devem permanecer junto dela. |
| `PERFORMANCE` | “O que acontece enquanto eu seguro?” | Perform Action e Perform / Gate | Separar a escolha da ação do disparo evita confundir configuração persistente com gesto momentâneo. |
| `OUTPUT` | “Quanto efeito sai e ele está ligado?” | Mix e Bypass | Fecha o fluxo sonoro com balanceamento Dry/Wet e estado global. |

`INPUT` não é um painel separado: `Input Diffusion` fica em `REVERB` porque
altera a difusão na entrada do algoritmo de reverb, não o ganho de entrada.

## 3. Wireframe textual da tela

```text
APOLLO
├── Header
│   ├── Identidade do plugin: APOLLO
│   ├── Estado global: ACTIVE | BYPASSED
│   └── Ajuda contextual futura: tooltip/foco do controle atual
├── REVERB
│   ├── Size [Small | Medium | Large]
│   ├── Pre-delay [0–1000 ms]
│   ├── Decay [%]
│   ├── Tone [High Cut ← centro → Low Cut]
│   ├── Mod Rate [%]
│   ├── Mod Depth [%]
│   └── Input Diffusion [On | Off]
├── OCTAVE
│   ├── Octave Mode [Off | Up | Down | Up + Down]
│   ├── Octave High Shelf [dB] — rótulo de produto provisório
│   ├── Octave Low Shelf [dB] — rótulo de produto provisório
│   └── Octave dry routing [PENDENTE de validação]
├── PERFORMANCE
│   ├── Perform Action [Freeze | Overdrive | Octave Perform]
│   └── Perform / Gate [pressionar e sustentar]
└── OUTPUT
    ├── Mix [Dry ← → Wet]
    └── Bypass [On | Off]
```

**Ordem e foco:** navegação por teclado segue a ordem acima; controles
atenuados continuam focáveis, anunciam seu estado e mantêm a ligação APVTS.
O Header apenas reflete estado e não cria um segundo parâmetro de bypass.

## 4. Dicionário de controles e microcopy

Os “nomes atuais de automação” abaixo são os nomes públicos já registrados no
APVTS. Eles não são uma solicitação de renomeação. “Padrão” é o valor existente
do parâmetro, não uma recomendação de produto.

| ID interno / nome atual de automação | Rótulo visível proposto | Grupo | Unidade/formatação pretendida | Tooltip curto | Padrão | Dependências e indisponibilidade |
| --- | --- | --- | --- | --- | --- | --- |
| `predelay` / `Pre-Delay` | **Pre-delay** | REVERB | `0–1000 ms`; conversão linear para segundos enviada a `setPreDelay`. O DSP reserva um segundo completo no sample rate ativo. | “Atrasa a entrada do reverb.” | `0 ms` (`0.0`) | Sempre disponível; em bypass continua legível/automatizável, sem efeito audível enquanto bypassado. |
| `mix` / `Mix` | **Mix** | OUTPUT | `Dry ← 0–100% → Wet`; manter a escala do parâmetro, sem prometer crossfade linear | “Equilibra sinal direto e reverb.” | centro (`0.5`) | Sempre disponível; bypass não o desabilita. |
| `decay` / `Decay` | **Decay** | REVERB | `%` (`0–100%`) até validação de escala musical | “Define quanto tempo o reverb sustenta.” | `87.7%` (`0.877`) | Durante Freeze ativo, o DSP força a sustentação; manter o valor visível como “valor retomado ao soltar”. |
| `moddepth` / `Mod Depth` | **Mod Depth** | REVERB | `%` (`0–100%`) | “Define a intensidade do movimento.” | `6.25%` (`0.0625`) | Sempre disponível. |
| `modspeed` / `Mod Speed` | **Mod Rate** | REVERB | `%` (`0–100%`); **não exibir Hz** até confirmar unidade musical do tank | “Define a velocidade do movimento.” | `4.66%` (`0.0466`) | Sempre disponível. Diverge de “Mod Speed” apenas para uma leitura musical mais direta. |
| `damp` / `Damp` | **Tone** | REVERB | controle bipolar contextual: `High Cut ← → Low Cut`; percentual opcional, sem Hz no mostrador principal | “Escurece à esquerda; afina os graves à direita.” | centro (`0.5`) | Sempre disponível. O ponto central é neutro na navegação, mas a equivalência auditiva final requer validação. |
| `eq1_gain` / `EQ1 Gain` | **Octave High Shelf** *(provisório)* | OCTAVE | `−24–+24 dB` | “Ajusta o shelf alto da ramificação de oitava.” | `−11 dB` | Atenuar quando Octave Mode = Off; não ocultar, desanexar ou impedir automação/preset/foco. Não chamar de nível de voz. |
| `eq2_gain` / `EQ2 Gain` | **Octave Low Shelf** *(provisório)* | OCTAVE | `−24–+24 dB` | “Ajusta o shelf baixo da ramificação de oitava.” | `+5 dB` | Mesma regra de `eq1_gain`. Não chamar de nível de voz. |
| `time_scale` / `Time Scale` | **Size** | REVERB | escolhas `Small`, `Medium`, `Large` | “Escolhe o tamanho do espaço.” | `Large` (índice `2`) | Sempre disponível. “Size” substitui o termo técnico “Time Scale” na UI, sem mudar automação. |
| `effect_mode` / `Effect Mode` | **Octave Mode** | OCTAVE | escolhas `Off`, `Up`, `Down`, `Up + Down` | “Escolhe as oitavas que alimentam o reverb.” | `Off` (índice `0`) | Quando Off, atenuar somente controles exclusivos da ramificação; `Perform / Gate` pode continuar disponível para Freeze/Overdrive. |
| `footswitch_mode` / `Momentary Mode` | **Perform Action** | PERFORMANCE | escolhas `Freeze`, `Overdrive`, `Octave Perform` | “Escolhe o que o botão Perform faz enquanto é sustentado.” | `Freeze` (índice `0`) | Sempre disponível. “Octave Perform” descreve a ação, não um novo modo de oitava. |
| `input_diffusion` / `Input Diffusion` | **Input Diffusion** | REVERB | `On` / `Off` | “Espalha o sinal antes do plate.” | `On` (`true`) | Sempre disponível. |
| `octave_dry_mix` / `Octave Dry Mix` | **PENDENTE — não congelar rótulo** | OCTAVE | booleano `On` / `Off` somente após aprovação semântica | “Roteamento dry da ramificação de oitava — validação pendente.” | `On` (`true`) | Atenuar quando Octave Mode = Off, preservando foco/automação/preset. Exceção atual no modo Down precisa ser auditada antes de definir copy. |
| `bypass` / `UI Bypass` | **Bypass** | OUTPUT | `On` / `Off`; Header: `BYPASSED` / `ACTIVE` | “Passa o sinal direto e ignora o efeito.” | `Off` (`false`) | Parâmetro de bypass interno, distinto do bypass do host. Não bloquear outros controles; eles devem continuar recuperáveis e automatizáveis. |
| `momentary_effect` / `Momentary Switch` | **Perform / Gate** | PERFORMANCE | botão booleano sustentado: `Hold` / `Active`; acessível também por teclado e automação | “Sustente para executar a ação selecionada.” | inativo (`false`) | Deve continuar automatizável. Seu efeito depende de Perform Action; em Octave Perform sem Octave Mode ativo, comunicar indisponibilidade sonora, sem remover o booleano. |

### Microcopy de estados de performance e globais

| Condição observável | Texto de estado primário | Texto auxiliar / anúncio acessível |
| --- | --- | --- |
| `Perform Action = Freeze` e `Perform / Gate = Active` | **FREEZE ACTIVE** | “Decay está sustentado; solte Perform para retomar o valor de Decay.” |
| `Perform Action = Overdrive` e `Perform / Gate = Active` | **OVERDRIVE ACTIVE** | “Overdrive de performance está ativo enquanto Perform estiver sustentado.” |
| `Perform Action = Octave Perform`, `Perform / Gate = Active` e Octave Mode diferente de Off | **OCTAVE PERFORM ACTIVE** | “A entrada da oitava está ativa enquanto Perform estiver sustentado.” |
| `Perform Action = Octave Perform`, `Perform / Gate = Active` e Octave Mode = Off | **OCTAVE PERFORM: OCTAVE OFF** | “Selecione um Octave Mode para ouvir esta ação.” |
| `bypass = true` | **BYPASSED** | “Sinal direto ativo; os valores do efeito permanecem prontos para retorno.” |
| Octave Mode = Off | **OCTAVE OFF** | “Controles de cor e roteamento da oitava estão inativos para o áudio, mas permanecem disponíveis para preset e automação.” |
| Controle visualmente atenuado | **INACTIVE IN CURRENT MODE** | “O valor permanece editável por teclado e automação; altere o modo para ouvi-lo.” |

Não usar “On” para a ação `Perform / Gate` quando o usuário precisa sustentar
o gesto: “Active” evita sugerir um latch. A implementação da Fase 2 deve
refletir o estado booleano existente, sem convertê-lo em parâmetro não
automatizável.

## 5. Matriz de estados dependentes

| Estado / causa | REVERB | OCTAVE | PERFORMANCE | OUTPUT / Header | Regra de acesso |
| --- | --- | --- | --- | --- | --- |
| Normal (`bypass` Off, Octave Mode diferente de Off) | Todos ativos | Todos ativos; `octave_dry_mix` ainda com rótulo pendente | ação selecionada, Gate pronto | `ACTIVE` | Todas as attachments APVTS ativas. |
| Octave Mode = Off | Sem mudança | Shelves e roteamento atenuados; mostrar `OCTAVE OFF` | Freeze e Overdrive funcionam; Octave Perform informa que falta modo | `ACTIVE` | Atenuação é somente visual/contextual; foco, preset e automação continuam. |
| Freeze Gate ativo | Mostrar Decay como valor a retomar | Sem mudança | `FREEZE ACTIVE`; Gate sustentado | `ACTIVE` | Não substituir o valor salvo de `decay`; refletir que o DSP o sobrepõe temporariamente. |
| Overdrive Gate ativo | Sem mudança | Sem mudança | `OVERDRIVE ACTIVE`; Gate sustentado | `ACTIVE` | Não criar novo controle ou estado persistente. |
| Octave Perform Gate ativo + modo de oitava ativo | Sem mudança | Sem mudança | `OCTAVE PERFORM ACTIVE`; Gate sustentado | `ACTIVE` | A mensagem descreve o roteamento percebido, não altera modo/preset. |
| Octave Perform Gate ativo + Octave Mode Off | Sem mudança | Permanecer atenuado | `OCTAVE PERFORM: OCTAVE OFF` | `ACTIVE` | O botão continua acessível e automatizável; não há resultado de oitava até escolher modo. |
| Bypass interno On | Valores visíveis, sem resultado audível | Valores visíveis, sem resultado audível | Gate/ação continuam legíveis | `BYPASSED` | Nunca desabilitar attachments, teclado, automação nem recuperação de preset. |

## 6. Decisões pendentes e validações necessárias

| Tema | Evidência atual | Validação obrigatória antes de congelar a Fase 2 | Estado |
| --- | --- | --- | --- |
| `octave_dry_mix` | O código soma dry quando o booleano é falso **ou** quando o modo é Down; portanto o nome atual não descreve inequivocamente o resultado percebido. | Fazer teste auditivo com Up, Down e Up + Down, booleano ligado/desligado, e confirmar com produto se o objetivo é “dry no ramo de oitava”, “octave only”, ou outra semântica. Registrar uma tabela de entrada/saída. | **Bloqueado / rótulo pendente.** Alternativas a validar: “Octave Only”, “Include Dry in Octave Path” e “Octave Dry Routing”. |
| Shelves `eq1_gain` / `eq2_gain` | São filtros high-shelf e low-shelf, não níveis independentes de vozes. | Revisão de produto/auditiva para aprovar “Octave High Shelf” e “Octave Low Shelf”, ou nomear por resultado musical sem ocultar que são filtros. | **Provisório.** |
| Tone no centro | O DSP muda entre caminhos de high cut e low cut em `0.5`; a equivalência perceptiva no centro não foi medida. | Sweep auditivo e revisão de texto para confirmar “centro/neutro”, sem exibir frequências como especificação de UX. | **Pendente.** |
| Mod Rate em Hz | O processador mapeia o parâmetro a um valor passado ao tank, mas a unidade musical final não está documentada. | Confirmar contrato da LFO/tank e, se necessário, medir a taxa resultante por sample rate. | **Pendente; usar %.** |
| Pre-delay | O processador passa `predelay` diretamente como segundos a `setPreDelay`; o método multiplica por sample rate e redimensiona o buffer para acomodar 1 s mais uma amostra de guarda. | Confirmar no teste de UI que a formatação adotada é `valor × 1000`; o teste DSP deve cobrir a capacidade de 1 s nos sample rates suportados. | **Confirmado para proposta ms; regressão DSP coberta em 44,1, 48 e 96 kHz.** |
| Fase 0 ausente | Arquivo solicitado não está no checkout. | Recuperar o documento e reconciliar inventário, decisões e terminologia com esta especificação. | **Pendente de fonte.** |

## 7. Critérios de aceite para a Fase 2

1. A UI implementa exatamente os quatro grupos `REVERB`, `OCTAVE`,
   `PERFORMANCE` e `OUTPUT`, mais Header, na hierarquia desta especificação.
2. Os 15 controles desta tabela têm attachment ao mesmo ID APVTS existente;
   nenhuma faixa, default, nome de automação, persistência ou DSP é alterado.
3. Rótulos visíveis correspondem à coluna “Rótulo visível proposto”; itens
   provisórios ou pendentes permanecem explicitamente marcados até aprovação.
4. `Size`, `Octave Mode` e `Perform Action` são apresentados como seletores;
   `Perform / Gate` é apresentado como ação sustentada e continua ligado ao
   booleano automatizável `momentary_effect`.
5. A interface apresenta todas as mensagens de estado da tabela de microcopy
   para Freeze, Overdrive, Octave Perform, Octave Off e Bypass.
6. Com Octave Mode Off, os controles de oitava podem ser atenuados ou
   recolhidos visualmente, porém continuam no foco por teclado, legíveis por
   acessibilidade, recuperáveis por preset e atualizáveis por automação.
7. Com bypass interno ativo, nenhum controle deixa de estar ligado ao APVTS;
   Header mostra `BYPASSED` e a microcopy esclarece que os valores estão
   preservados.
8. Formatação física não confirmada usa percentuais; somente Pre-delay pode
   exibir ms conforme a conversão documentada. Mod Rate não exibe Hz.
9. Nenhum rótulo final para `octave_dry_mix` é publicado sem a validação
   auditiva/DSP registrada na seção 6.
10. A implementação não inclui LookAndFeel completo, medidores, clipping,
    presets, resize responsivo, DSP novo ou parâmetros novos.

## 8. Riscos de compatibilidade de automação

| Risco | Mitigação obrigatória |
| --- | --- |
| Renomear ID, alterar versão de `ParameterID`, recriar parâmetro ou trocar tipo/range/default quebra sessões e lanes existentes. | Manter literalmente os 15 IDs, versão `1`, tipos, escolhas, ordem semântica e defaults atuais; alterar apenas texto desenhado pela UI. |
| Confundir texto de UI com nome do parâmetro automatizado pode levar a alteração de `AudioParameter` para coincidir com o rótulo. | Tratar “nome atual de automação” como contrato legado; `Size`, `Octave Mode` e `Perform Action` são somente apresentação. |
| Converter `momentary_effect` em botão local sem attachment elimina automação e recuperação de estado. | Manter `ButtonAttachment`/equivalente para `momentary_effect`; gesto de pressionar/soltar apenas escreve o mesmo booleano. |
| Esconder controles dependentes pode impedir host, teclado ou preset de restaurar valores. | Usar atenuação/recolhimento visual não destrutivo, preservar componente/semântica acessível e attachment. |
| Confundir bypass interno com bypass do host gera estado incorreto ou duplicado. | Exibir `Bypass` apenas como o parâmetro interno `bypass`; manter a distinção explícita em tooltip e documentação. |
| Fixar uma interpretação de `octave_dry_mix` antes da auditoria pode inverter expectativas de presets existentes. | Não mudar ID, valor, polaridade, comportamento ou rótulo final até concluir a validação em modos Up, Down e Up + Down. |
