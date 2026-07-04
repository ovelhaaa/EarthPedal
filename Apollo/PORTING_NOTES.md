# Apollo Porting Notes

Este documento detalha as alterações feitas no comportamento original do EarthPedal ao ser portado para o formato plugin (VST3/AU) "Apollo".

## 1. Alterações em Controles e Interface

* **Bypass (Novo Parâmetro de UI)**
  * **O que mudou:** Foi adicionado um parâmetro extra `bypass` no APVTS, independente do bypass do host (DAW).
  * **Por quê:** O plugin não possui footswitches físicos. Quando o bypass da UI é ativado, o áudio é roteado diretamente (dry) ignorando a engine DSP, utilizando um smoothing de 5-10ms para evitar cliques (comportamento inexistente no hardware original, onde o bypass era físico).
  * **Nota:** O parâmetro original `bypass` na DAW repassará o áudio integralmente.

* **Footswitch 2 (Efeito Momentâneo)**
  * **O que mudou:** O comportamento de "segurar" e os diferentes modos do Footswitch 2 (para Freeze, Overdrive ou Octave) foram transformados em um parâmetro *toggle* simples (booleano) no APVTS chamado `momentary_effect`.
  * **Por quê:** Não existe o conceito de footswitch físico ou "manter pressionado" num plugin. Usuários podem automatizar esse parâmetro no DAW para emular o efeito desejado.

* **Bypass Host (Footswitch 1 Original)**
  * **O que mudou:** O Footswitch 1 físico (que acionava a variável `bypass`) foi removido. A lógica de crossfade constante via energia (`dryMix` / `wetMix`) original permanece no DSP para evitar quebra do comportamento do algoritmo original, mas o acionamento via footswitch não existe mais.

* **LEDs Físicos**
  * **O que mudou:** Lógica relacionada aos componentes físicos `Led led1, led2` foi removida.
  * **Por quê:** Resposta visual será dada pelos componentes de UI do JUCE.

## 2. Funcionalidades Removidas

* **Expression Pedal (ExpressionHandler)**
  * **O que mudou:** Toda a lógica do pedal de expressão (calibração, "Set Mode", definição de ranges min/max) foi **removida**.
  * **Por quê:** Funcionalidade descontinuada nesta versão. DAWs lidam nativamente com macros e pedais de expressão através da automação (mapping MIDI ou automação de parâmetros), tornando redundante recriar um modo de calibração ("Expression Set Mode") internamente no plugin.

## 3. Substituições de Dependências

* **DaisyPetal, DaisySP, Funbox, Hardware GPIOs/ADC**
  * Toda comunicação via hardware (`hw.ProcessAnalogControls`, `hw.switches`, `hw.knobs`) foi convertida para a arquitetura baseada no `AudioProcessorValueTreeState` (APVTS) do JUCE.
  * O código original assume a taxa de amostragem dependendo do hardware; no plugin, o sample rate e buffers serão fornecidos em tempo real pelas propriedades do host no método `prepareToPlay` e repassados para a engine e para instâncias DSP (como o OctaveGenerator, onde se usará o valor correto via parameter binding ao invés de hardcode).

* **GCEM e cycfi Q**
  * Dependências de processamento matemático (gcem) utilizadas apenas no construtor de classes foram trocadas pelas funções padrão `<cmath>` (`std::pow`), e filtros da bibliotecas `cycfi q` (`highshelf`, `lowshelf`) serão substituídos por `juce::dsp::IIR::Filter` para evitar importação desnecessária de bibliotecas grandes.

## 4. Valores Padr�o (Defaults) de APVTS
* **predelay**: 0.0
* **mix**: 0.5 (sem correspond�ncia direta inicial no hardware, mas � um meio-termo seguro)
* **decay**: 0.877 (hardcoded inicial via reverb.setDecay(0.877465))
* **moddepth**: 0.0625 (reverb.setTankModDepth(0.5))
* **modspeed**: 0.0466 (reverb.setTankModSpeed(1.0))
* **damp**: 0.5

## 5. Resampling do OctaveGenerator (Generaliza��o de Sample Rate)
* **O que mudou:** O c�digo original operava estritamente a 48kHz, possuindo coeficientes FIR hardcoded para essa taxa na etapa de decima��o/oitavador. Para suportar qualquer sample rate de DAW (ex: 44.1k, 96k) sem alterar o timbre (frequ�ncia de corte dos filtros) ou exigir uma complexa recria��o din�mica dos filtros, o sinal da ramifica��o do OctaveGenerator passa por um re-sampler interno (juce::LagrangeInterpolator) para 48kHz, processa o oitavador, e retorna � taxa original. 
* **Nota Pragmatica:** Essa � uma escolha pragm�tica. O hardware rodava a 48kHz nativamente, logo n�o sofria altera��es por reamostragem extra. Aqui, h� uma lev�ssima altera��o n�o bit-exata introduzida pelo interpolador, mas o escopo � restrito APENAS � ramifica��o do oitavador. O Dattorro Reverb e filtros Damp operam 100% na taxa nativa do host, garantindo m�xima fidelidade.
