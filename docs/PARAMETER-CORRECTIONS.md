# Correções dos controles — 16/11/2026

Referências: manual CLA Guitars fornecido pelo usuário (páginas 8–13), XML
`Parameter_List`, XML `GUITree` e banco de 48 presets `PresetXMLTree`.
Os anexos foram tratados como dados de referência.

| Controle | Comportamento implementado |
| --- | --- |
| Input, Output e seis faders | −10 a +10, resolução 0,1, inicial/reset 0 |
| Guitar Mode | Clean inicial; Clean/Crunch/Heavy/Bypass preservados |
| Re-Amplify | Desligado inicialmente e no reset |
| Color | Verde inicial: Sub/Bite/Push/Club/Eight/Stereo |
| Off/Clear | Estados On independentes preservados; ciclo verde/azul/vermelho/desligado |
| Balance | −6 a +6 dB, resolução 0,1, centro 0; apenas estéreo |
| Delay | BPM do host; Quarter 1/4, Edge 1/8 pontuada, Eight 1/8 |
| Sem BPM válido | 120 BPM; atraso máximo de 2 segundos |
| Medidores | Escala −26 a 0 dBFS; clip acima de 0 dBFS retido até clique |

Balance atenua R com valores negativos e L com positivos, com suavização de
aproximadamente 10 ms. Seu parâmetro foi acrescentado ao fim da lista; os IDs,
ordem e índices anteriores foram preservados. O duplo clique dos faders e o
reset do menu seguem os novos valores iniciais. Foi removida a exceção visual
que pintava Lower de verde em vez de azul.

Os 60 presets próprios, seus arquivos e valores não foram substituídos.
A resolução de 0,1 também se aplica a valores recebidos por automação.
O sincronismo do Delay altera o tempo do eco em sessões anteriores, que
antes usavam taps fixos. Mudanças abruptas de BPM/Color ainda não têm
crossfade entre tempos de delay.

## Banco fornecido

`scripts/convert-cla-presets.py` converte um banco de controles públicos usando
sua definição `Parameter_List` correspondente. Foram convertidos 48 presets,
incluindo Full Reset, em `/workspace/converted-presets`; o pacote está em
`/workspace/Presets-convertidos.zip`. Use **Load → Load user preset…**.

São transferidos Re-Amplify, Guitar Mode, Balance, os seis faders e os seis
seletores Color. Clear desliga o módulo; 1/2/3 correspondem a verde/azul/vermelho,
convertidos para a ordem interna legada 2/1/0. `*` não altera o parâmetro.
Input/Output são omitidos para preservar os trims atuais, conforme a indicação
`IgnoreOnPresetLoad` dos arquivos de referência.

O conversor valida quantidade de campos, faixas, valores finitos e seletores
antes de gravar. Exige diretório de saída vazio. Os arquivos `1010.xml` e
`1011.xml` descrevem configurações internas de modos/cores; não são bancos de
controles públicos compatíveis com essa conversão. O arquivo `1100` é um ícone
Windows; outros XMLs descrevem componentes, interface e barra de ferramentas.
Eles não foram incorporados como código ou recursos comerciais do plugin.

## Verificação

O fluxo Linux usa GCC 14.2, C++17, CMake 3.31.6, Ninja e JUCE no commit fixado
pelo projeto. Os testes CTest exercitam DSP, Pitch, 60 presets e editor real.
Foram acrescentadas verificações de defaults/resolução, Balance e sessão,
impulsos nas três divisões de delay em diferentes BPMs/sample rates, BPM
fornecido por um host simulado, automação do Balance e cliques independentes
nos resets de clip. O conversor tem três testes Python para cores, trims,
asteriscos, dados inválidos e proteção contra sobrescrita.

Os logs da instalação/validação ficam em `/workspace/toolchain`; os builds
estão em `build-cloud`. `install_script` e `start_skill` foram salvos no
rascunho do ambiente. Salvar o rascunho não publica o ambiente nem verifica
sua restauração em uma tarefa nova.

## Limites

Não houve execução de Windows/MSVC, Studio One, pluginval ou audição com
hardware nesta tarefa. O Standalone inicia sob Xvfb, mas a máquina não oferece
dispositivo físico ALSA. Os testes de editor e processamento validam o fluxo
sem comprovar desempenho numa DAW.

O DSP continua sendo uma implementação própria. Os anexos não fornecem o
código proprietário; importar valores não reproduz seus processadores
internos. Modelos de amp, frequências de EQ, dinâmica, reverberação, pitch,
curva/limiares do LED e processamento fixo por modo não foram igualados ao
produto de referência. A latência de Pitch permanece em 120 ms. Os presets
convertidos transferem posições de controles, sem equivalência sonora.
