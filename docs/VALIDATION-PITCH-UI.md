# Validação local de Pitch e interface — 16/11/2026

Alterações locais sobre o checkout `813daaa6df81d3ed5f5c15e5a2bf89a2504054ab`, ainda sem commit/push. Máquina Linux x86_64, GCC 14.2, CMake 3.31.6, C++17, JUCE 8.0.6 no commit já fixado pelo projeto. A compilação Windows e a execução no Studio One desta revisão não foram realizadas nesta máquina.

## Arquivos modificados

- DSP e integração: `Source/PitchShifter.h` (novo), `Source/GuitarDSP.h`, `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`.
- Interface: `Source/PluginEditor.cpp`, `Source/PluginEditor.h`, `docs/interface.png` e `docs/interface-off.png` (novo).
- Build e CI: `CMakeLists.txt`, `.github/workflows/windows.yml`.
- Testes: `tests/DspTests.cpp`, `tests/PitchTests.cpp` (novo), `tests/Preview.cpp`, `tests/AllocationChecks.h` (novo).
- Documentação e licenças: `README.md`, `BUILD_STATUS.md`, este relatório e os dois avisos em `docs/licenses/` (novos).

## Causas e correções

As setas chamavam apenas `cycleColor`; os estados On já existiam, mas só eram acessíveis no menu. O timer copiava On para a aparência da seta, o que não significava que clicar nela mudasse On. Não havia uma rotina reaplicando continuamente presets ao DSP. Agora o clique consulta os parâmetros atuais, percorre os três modos e desliga. Os indicadores recebem atualizações por ParameterAttachment; o APVTS continua sendo a única fonte dos estados. Não foram alterados IDs, versões, faixas ou valores dos 60 presets.

O ciclo começa no estado atual carregado pelo preset, não reinicia a cada clique:

| Seta | Primeiro modo | Segundo | Terceiro | Clique seguinte | Função no áudio |
|---|---|---|---|---|---|
| Bass | SUB | LOWER | UPPER | Apaga | Shelf grave: 65 / 100 / 160 Hz |
| Treble | BITE | TOP | ROOF | Apaga | Shelf agudo: 6500 / 4000 / 2500 Hz |
| Compress | PUSH | SPANK | WALL | Apaga | Compressor: razões 6 / 4 / 2 |
| Reverb | CLUB | HALL | ARENA | Apaga | Tamanho e feedback dos combs existentes |
| Delay | EIGHT | EDGE | QUARTER | Apaga | Taps de 62,5 / 125 / 250 ms |
| Pitch | STEREO | WIDE | SPREADER | Apaga | Detune: razões 0,9 / 0,95 / 0,975 |

Depois de apagado, outro clique ativa o primeiro modo. Não foi adicionado texto OFF aos visores das seis setas. O OFF do Re-Amplify, já existente, permanece. Os nomes Color e suas associações originais foram mantidos; não representam especificações de outro fabricante. O menu e os seletores no visor continuam disponíveis. As mudanças de parâmetros notificam o host com gestos completos.

Foi encontrado também um problema de layout: os sliders calculavam inicialmente seu curso com o LookAndFeel padrão antes de herdarem o RackLook. A pintura usava outra posição, tornando possível um descompasso de arraste, escala e cap. O RackLook agora é aplicado explicitamente antes da configuração; o curso visual e a interação usam o mesmo `sliderPos`. O duplo clique volta ao default real de cada parâmetro (Pitch = −6 dB wet).

O pluginval revelou um defeito de restauração já permitido pelo AudioParameterBool do JUCE fixado: valores fracionários normalizados podiam permanecer no parâmetro, enquanto o APVTS salvava 0/1. Os switches agora normalizam a entrada para dois estados, mantendo os mesmos IDs, tipo booleano, passos e defaults. Ponteiros atômicos dos parâmetros são resolvidos no construtor; processBlock não constrói Strings nem procura parâmetros.

## Pitch: finalidade e algoritmo

O controle original é **detune/doubling**, não transposição em semitons nem velocidade. O fader −10…+10 controla o nível do efeito, misturado ao sinal seco. Transformá-lo silenciosamente em semitons mudaria os presets e as automações. A função foi preservada.

O algoritmo anterior usava dois taps de delay com janelas complementares, interpolação linear, rampa periódica de 80 ms e profundidades de 2/4/8 ms. A fase persistia entre blocos: não foi encontrado reset de fase a cada buffer. Entretanto, o reinício periódico dos taps e o alinhamento imperfeito entre fases dos grãos quantizavam o espectro e criavam modulação. Melhorar apenas o fader não resolveria esse efeito.

Foi integrado Signalsmith Stretch 1.3.2, commit `88061fd4eda56a2353c5d65c74aa3f4ccf76f9da`, com Signalsmith Linear 0.6.6, commit `7bb1c8f36794d6980080ce817c91f8da8fe56a66`. Ambos MIT; avisos completos em `docs/licenses/`. O processamento espectral usa detecção de picos, previsão de fase e ligação entre canais. Entradas e saídas têm o mesmo comprimento, preservando a duração. Não foi adicionado oversampling.

A configuração padrão da biblioteca não passou inicialmente no critério de afinação desta tarefa. A configuração final usa janelas de 120 ms e salto de 5 ms, com maior sobreposição e custo de CPU. O cálculo interno é double; buffers e planos são preparados fora do processamento. A conversão é `ratio = pow(2, semitones / 12)`, com `semitones = 12 * log2(ratioColor)`. Mudanças de modo são suavizadas em 30 ms; nível e ativação do Pitch em aproximadamente 10 ms, sem reiniciar buffers. O algoritmo continua processando quando o módulo está apagado, mantendo seu estado e a latência.

Latência fixa de **120 ms**: 5292 / 5760 / 11520 samples em 44,1 / 48 / 96 kHz. O sinal seco é atrasado igualmente; processBlockBypassed também preserva a latência e o roteamento mono→estéreo. Não há mudança de latência ao selecionar modos ou desligar o módulo. O host recebe a latência em prepareToPlay. Isso permite compensação na DAW, mas não elimina o atraso sentido na monitoração ao vivo.

Não foi implementado modo Formant Preserve: o produto continua sendo um efeito de guitarra/detune. O núcleo pode transpor oitavas e foi testado assim, mas isso não torna o fader de nível uma transposição de ±12 semitons. Não há validação de naturalidade vocal/formantes, gravações reais de piano/violão ou audição humana comprovada nesta tarefa.

## Evidências executadas

- Build Release Linux x86_64 de VST3, Standalone e RackPreview: passou.
- CTest: **4/4 suítes passaram**, incluindo DSP, Pitch, 60 presets e editor.
- Pluginval 1.0.4, nível 10, seed 42: **SUCCESS**, exit 0. Rates padrão 44,1/48/96 kHz e buffers 64/128/256/512/1024. Incluiu restauração de estado, automação, editor, threads, buses e fuzzing. O validador oficial Steinberg foi explicitamente ignorado pelo pluginval por não estar instalado; não equivale a certificação VST3 nem a teste no Studio One.
- Presets: 60 programas, seis categorias, parâmetros, sessão e processamento estéreo.
- Editor: cliques reais de mouse nos seis ciclos após três presets distintos, independência entre módulos, gestos ao host, automação→GUI, edição→parâmetro, persistência, ausência de sobrescrita após processar áudio, Undo/Redo e A/B. Cada seta também foi testada de ponta a ponta contra uma referência seca: seu acionamento alterou o áudio do módulo correspondente.
- Pitch fader: mínimo, centro e máximo verificam o ganho wet real; duplo clique verifica default −6. Curso do mouse e pintura verificados nos extremos.
- Pitch: senoide de 440 Hz em −12/0/+12 semitons e nos três detunes, em três sample rates; erro absoluto <1 cent. Identidade a zero e atraso do caminho seco medidos. Canais mono duplicados permanecem coerentes após o início; tolerâncias de diferença de amplitude de 1% na partida e 0,1% em regime. Transientes de impulso e automação de saltos/lenta foram medidos. Processamento idêntico para a mesma sequência de samples com buffers 1/17/64/127/256/1024.
- Ausência de alocações C++ `new`/`new[]` durante o processamento instrumentado de Engine/Pitch, processBlock e bypass. Não é uma certificação geral de deadlines em todos os hosts.
- Mono→estéreo: áudio efetivamente sai após a latência, com buffers 17/64/257/1024 e três rates; bypass tem atraso exato.
- GUI: dez ciclos de abrir/fechar, resize 840×540 e render real 1680×1080. Standalone abriu no Xvfb; a máquina não tem dispositivo físico de áudio para monitoração.

O impulso a transposição zero apresentou pico de 0,2 e energia 0,04 exatamente na latência declarada. Em ±12 semitons houve redistribuição temporal/espectral esperada do impulso: picos aproximadamente 0,092–0,144 e energias 0,019–0,032. Isso mostra que o algoritmo **não preserva perfeitamente transientes**; a medição não deve ser apresentada como fidelidade absoluta.

### Medições de afinação e modulação

Comparação isolada do componente wet, senoide de 440 Hz, Color WIDE, 48 kHz, mesma entrada e parâmetros; janela de medição entre 2 e 4 s. Referência ideal: 418 Hz. Ajuste de senoide por mínimos quadrados no fundamental medido; o resíduo inclui modulação, ruído e componentes espúrios, não apenas THD.

| Medida | Antes | Depois |
|---|---:|---:|
| Fundamental wet dominante | 415,000 Hz | 418,044 Hz |
| Erro em relação a 418 Hz | −12,470 cents | +0,183 cent |
| Resíduo relativo ao fundamental ajustado | −23,567 dB | −141,062 dB |

Esse resíduo muito baixo é um resultado de **senoide estacionária**, próximo do limite numérico do ensaio, e não descreve a qualidade de música/voz. Os ensaios gerais de senoide do novo núcleo apresentaram erros máximos de aproximadamente 0,90 cent. O salto máximo entre samples na automação testada ficou em cerca de 0,0252 para sinal de entrada de pico 0,2; isso não prova ausência de todos os artefatos audíveis.

O benchmark sintético de três segundos, incluindo geração de senoide e verificações, usou aproximadamente 5–8% de um núcleo a 44,1/48 kHz e 11–15% a 96 kHz nesta máquina. É tempo médio de parede, sujeito à concorrência do ambiente; não é medição de CPU em uma DAW ou do pior deadline.

Os arquivos `/workspace/validation/source.wav`, `before.wav` e `after.wav` usam exatamente o mesmo sinal musical sintético de cordas/notas harmônicas, 48 kHz, oito segundos, RMS igualado a −20 dBFS e latência removida apenas para comparação. Não são gravações reais nem substituem audição crítica. Scripts e logs desse ensaio foram retidos em `/workspace/validation`.

## Visual implementado

As capturas `docs/interface.png` e `docs/interface-off.png` vêm do editor real; os medidores foram alimentados pelo teste. Chave vetorial com base concêntrica, cavidade, haste projetada com perspectiva variável, metal escovado, iluminação superior esquerda, sombras e indicador; transição de 100 ms somente na message thread. Setas quadradas com moldura metálica, oliva, relevo e estado apagado. Faders com faces metálicas, grão, bordas especulares e sombras. O painel estático tem cache ajustado à escala; controles móveis são vetoriais. Repaints periódicos ficam nas áreas de visor, indicadores e medidores. Não foi usada imagem comercial nem uma interface integral em 8K; não há alegação de equivalência fotográfica a render 8K.

## CI e ambiente reutilizável

O workflow Windows usa windows-latest e gerador Visual Studio 17 2022 x64, CTest, pluginval nível 10, ZIP com os 60 presets e avisos MIT; tags v*.*.* publicam Release apenas após sucesso do build. O workflow foi preparado, mas **não foi executado remotamente nesta tarefa**. Nada foi enviado ao GitHub nem publicado como Release.

O ambiente atual tem toolchain em `/workspace/toolchain`, fontes de dependências retidas e artefatos em `build-cloud`. O script completo foi executado novamente com sucesso e `install_script`/`start_skill` foram salvos no rascunho da configuração. A publicação do ambiente é uma ação separada nas configurações. Restauração em uma nova tarefa ainda não foi testada.

Pendências: MSVC/Windows, Studio One, validador oficial Steinberg, audição nivelada de gravações reais e adequação dos 120 ms de latência à monitoração. Preservação dos valores de presets não significa identidade sonora com o algoritmo anterior: a sonoridade do Pitch mudou deliberadamente.

## Refinamento da chave a partir da imagem fornecida

A chave Re-Amplify foi redesenhada no RackLook a partir da nova referência: arruela metálica larga, chanfro com reflexos, aro interno, cavidade escura, pivô, haste de metal escovado e ponta arredondada em marfim. A representação é vetorial com aparência tridimensional; não é uma malha 3D nem uma foto ampliada. A perspectiva e a iluminação variam durante a transição existente de 100 ms. O rótulo ON foi afastado da ponta para evitar sobreposição. Não foram alterados DSP, IDs ou valores de parâmetros nesta revisão visual.

Build Release Linux de VST3/Standalone passou. editor_tests passou novamente, incluindo cliques reais do switch atualizando reamp nos dois sentidos, chegada da animação aos dois extremos, automação, estados, ciclos dos módulos e abertura/fechamento do editor. As capturas foram atualizadas; docs/switch-detail.png é um detalhe renderizado diretamente pelo editor em escala 6×, não uma imagem conceitual. Compilação Windows desta revisão permanece pendente.
