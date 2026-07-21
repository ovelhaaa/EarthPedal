# Apollo — Fase 0: descoberta e critérios de sucesso

**Status:** concluída em 2026-07-21
**Escopo:** registrar o comportamento implementado, os riscos de linguagem e as condições de aceite antes de alterar a interface. Esta fase não modifica o DSP, os IDs de automação nem o layout do plugin.

## 1. Inventário verificado

| Área | ID de automação (estável) | Nome atual no host | Tipo, faixa e padrão | Comportamento confirmado | Risco de UX / decisão para a Fase 1 |
| --- | --- | --- | --- | --- | --- |
| Reverb | `predelay` | Pre-Delay | contínuo, `0.0–1.0`, `0.0` | É enviado diretamente ao pré-delay do reverb. A documentação do pedal informa aproximadamente até 700 ms. | Mostrar uma unidade musical (ms) somente depois de validar a curva exata de `Dattorro::setPreDelay`. |
| Saída | `mix` | Mix | contínuo, `0.0–1.0`, `0.5` | Controla o crossfade de energia constante entre o sinal direto atrasado e o reverb. | Exibir `Dry` / `Wet` e percentual; não tratar o valor como um crossfade linear simples na copy. |
| Reverb | `decay` | Decay | contínuo, `0.0–1.0`, `0.877` | Controla a realimentação; com Freeze ativo, o alvo passa a `1.0`. | Manter o valor de automação; indicar visualmente que Freeze o está sobrepondo. |
| Reverb | `moddepth` | Mod Depth | contínuo, `0.0–1.0`, `0.0625` | É escalado por `8.0` antes de configurar a profundidade do tanque. | Não publicar porcentagem física sem uma escala aprovada. |
| Reverb | `modspeed` | Mod Speed | contínuo, `0.0–1.0`, `0.0466` | É mapeado para `0.3 + valor × 15.0` antes de configurar a velocidade do tanque. | Pode ser formatado como aproximadamente `0.3–15.3 Hz` após confirmar que a unidade do DSP é Hz. |
| Reverb | `damp` | Damp | contínuo, `0.0–1.0`, `0.5` | Abaixo do centro ajusta high-cut; acima, low-cut. | Usar copy bidirecional, por exemplo `Tone: High cut ← → Low cut`, em vez de sugerir um único filtro. |
| Octave | `eq1_gain` | EQ1 Gain | contínuo, `-24–24 dB`, `-11 dB` | Ajusta um high shelf de 140 Hz na ramificação de oitava. | O rótulo visual atual `Octave High` é mais musical; confirmar se representa ganho, não nível de oitava. |
| Octave | `eq2_gain` | EQ2 Gain | contínuo, `-24–24 dB`, `5 dB` | Ajusta um low shelf de 160 Hz na ramificação de oitava. | O rótulo visual atual `Octave Low` é mais musical; confirmar se representa ganho, não nível de oitava. |
| Reverb | `time_scale` | Time Scale | escolha: Small / Medium / Large; `Large` | Mapeia para escalas de tempo `1.0`, `2.0` e `4.0`. | Renomear apenas o rótulo visível para `Size`; preservar ID e nome de automação nesta etapa. |
| Octave | `effect_mode` | Effect Mode | escolha: None / Up / Down / Both; `None` | Habilita a ramificação de oitava e seleciona as vozes. | Rótulo visível proposto: `Octave Mode`; aplicar estados dependentes sem ocultar a automação. |
| Performance | `footswitch_mode` | Momentary Mode | escolha: Freeze / Overdrive / Effect; `Freeze` | Define a ação usada por `momentary_effect`. | Rótulo visível proposto: `Perform Action`; deve estar próximo do controle que a dispara. |
| Reverb | `input_diffusion` | Input Diffusion | booleano; ligado | Ativa/desativa a difusão de entrada do reverb. | Manter como toggle de timbre com tooltip que explique o resultado, não a implementação. |
| Roteamento Octave | `octave_dry_mix` | Octave Dry Mix | booleano; ligado | Quando desligado, adiciona sinal seco à ramificação de oitava; no modo Down, o seco também é adicionado mesmo quando está ligado. | **Bloqueador de copy:** o nome atual não descreve de modo confiável os dois estados. Validar auditivamente e definir o nome antes da Fase 1. |
| Estado global | `bypass` | UI Bypass | booleano; desligado | Faz crossfade de 10 ms para o sinal de entrada direto; é separado do bypass do host. | Rótulo visual pode ser `Bypass`; tooltip deve distinguir bypass interno e bypass da DAW. |
| Performance | `momentary_effect` | Momentary Switch | booleano; desligado | Com Freeze, força decay a 1.0; com Overdrive, aumenta drive; com Effect, direciona a ramificação de oitava enquanto ativo. | O toggle persistente conflita com “momentary”. Especificar botão de pressão sustentada, mantendo o ID automatizável. |

### Contratos que não podem mudar durante o redesenho

1. Os 15 IDs acima são parte do contrato de automação e de recuperação de sessão. Não os renomear, remover nem alterar faixa/padrão sem migração de estado e teste em hosts.
2. O bypass interno continua independente do bypass disponibilizado pelo host.
3. A ramificação de oitava é mono e é reamostrada internamente a 48 kHz; a interface não deve prometer imagem estéreo no octavador.
4. A ação de performance precisa continuar automatizável, mesmo quando a UI passar a usar um botão momentâneo real.

## 2. Dependências e estados a comunicar

| Condição | Efeito sonoro implementado | Resposta de interface exigida |
| --- | --- | --- |
| `effect_mode = None` | A ramificação de oitava não é processada. | Atenuar os controles de octave e explicar que os valores continuam disponíveis para automação/presets. |
| `effect_mode = Up` | Apenas a voz de oitava acima é somada. | Dar ênfase ao controle tonal associado a agudos, sem desabilitar o outro EQ. |
| `effect_mode = Down` | São somadas as vozes uma e duas oitavas abaixo; o sinal seco é acrescentado na ramificação de oitava. | Comunicar esse roteamento na ajuda contextual; não inferir que `octave_dry_mix` controla integralmente esse modo. |
| `effect_mode = Both` | As vozes acima e abaixo são somadas. | Exibir o bloco octave completo. |
| `footswitch_mode = Freeze` + ação ativa | Decay é levado a `1.0`. | Exibir `Freeze active` e indicar que o valor de Decay está temporariamente sobreposto. |
| `footswitch_mode = Overdrive` + ação ativa | O drive do reverb aumenta de `0.4` rumo a `0.6`. | Exibir `Drive active`; não apresentar como overdrive de entrada. |
| `footswitch_mode = Effect` + ação ativa | Quando há modo de octave, a entrada do reverb passa pela ramificação de oitava enquanto a ação estiver ativa. | Exibir `Octave perform active`; quando `effect_mode = None`, explicar que não há octave selecionado. |
| `bypass = On` | A saída cruza para a entrada direta em 10 ms. | LED/estado `Bypassed`, sem confundir com o bypass da DAW. |

## 3. Fluxos de tarefa a validar

| Perfil | Tarefa | Critério observável |
| --- | --- | --- |
| Guitarrista em performance | Configurar um plate grande e acionar Freeze por uma frase. | Localiza Size, Decay, Mix, Perform Action e o botão de ação sem consultar documentação. |
| Produtor em mix | Ajustar uma cauda curta, reduzir graves/agudos conforme necessário e voltar ao padrão. | Lê valores/unidades, compreende o centro de Damp e restaura defaults por gesto documentado. |
| Sound designer / automação | Criar automação de octave e de ação de performance, depois recuperar a sessão. | Encontra nomes de parâmetros estáveis no host; a UI reflete a automação e não perde valores em estados atenuados. |

## 4. Critérios de sucesso e método de medição

| Objetivo | Meta de aceite | Método |
| --- | --- | --- |
| Descoberta | Pelo menos 4 de 5 participantes localizam e ajustam Size, Decay e Mix em até 30 segundos, sem ajuda. | Teste moderado com tarefas e cronômetro; registrar sucesso e tempo. |
| Semântica de roteamento | Pelo menos 4 de 5 participantes descrevem corretamente o resultado de cada estado do controle de dry/octave antes de ouvi-lo. | Mostrar o controle e as opções; pedir previsão verbal; comparar com o comportamento implementado. |
| Estado crítico | 5 de 5 participantes identificam Bypass e a ação ativa em menos de 1 segundo. | Mostrar capturas estáticas dos estados Active, Bypassed, Freeze e Overdrive; medir identificação. |
| Legibilidade | Todos os controles essenciais permanecem legíveis em escalas de 100%, 125%, 150% e 200%. | Capturas de regressão por escala e inspeção manual em host suportado. |
| Acessibilidade | Todos os controles recebem foco por teclado, têm foco visível e não dependem apenas de cor. | Checklist manual e navegação Tab/Shift+Tab. |
| Não regressão | Todas as automações e sessões de teste preservam os 15 IDs e os valores após reabrir. | Projeto de sessão por formato suportado; comparação de parâmetros antes/depois. |

## 5. Protocolo de descoberta antes de desenhar

1. **Confirmar escala e unidade.** Medir/inspecionar `setPreDelay`, `setTankModSpeed` e o filtro Damp para decidir se os valores visíveis serão ms, Hz, dB ou percentual. Não inventar unidade onde o DSP expõe somente uma escala normalizada.
2. **Resolver `octave_dry_mix`.** Preparar quatro clipes curtos (Up/Down/Both com cada estado booleano), ouvir e documentar o caminho de sinal. A nomenclatura da Fase 1 depende desse resultado.
3. **Validar a ação de performance.** Testar mouse down, mouse up, automação e MIDI mapping no host-alvo para especificar o comportamento de um botão momentâneo sem quebrar a automação booleana.
4. **Estabelecer baseline visual.** Capturar a UI atual em 100%, 150% e 200%, com octave None/Both, Freeze ativo, Overdrive ativo e bypass ativo. Guardar as capturas no material de QA, não no bundle do plugin.
5. **Recrutar e executar os três fluxos.** Usar ao menos cinco participantes, incluindo pelo menos um representante de cada perfil da seção 3; priorizar observação e perguntas de compreensão, não preferência estética.

## 6. Questões abertas e responsáveis

| Questão | Dono | Saída necessária para iniciar a Fase 1 |
| --- | --- | --- |
| Qual texto descreve corretamente os dois estados de `octave_dry_mix`, inclusive a exceção do modo Down? | DSP + produto | Tabela de roteamento aprovada e dois rótulos de estado. |
| `setTankModSpeed` usa Hz para toda a faixa? Qual curva traduz `predelay` em ms? | DSP | Fórmulas ou tabela de conversão aprovada. |
| `EQ1 Gain` / `EQ2 Gain` devem ser tratados como filtros tonais ou níveis de voz? | Produto + áudio | Nomes visíveis e tooltips aprovados. |
| Quais hosts e sistemas operacionais compõem a matriz de QA inicial? | Release/QA | Lista priorizada para VST3, AU e standalone. |
| O botão Perform deve responder apenas a mouse, ou também a teclado/foco? | UX + engenharia | Especificação de interação e acessibilidade. |

## 7. Saída para a Fase 1

A Fase 1 pode iniciar com o inventário acima como fonte de verdade. Ela deve produzir o dicionário de microcopy e o wireframe, mas só deve alterar nomes **visíveis** até que as questões abertas estejam resolvidas. Qualquer proposta de mudança de ID, faixa, padrão ou lógica de roteamento requer uma decisão separada de compatibilidade de automação.
