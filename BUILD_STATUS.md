# Estado da compilação

- Windows x64 Release VST3: **compilado com sucesso no MSVC**.
- Commit compilado: `3fb8e01`.
- Testes DSP no Windows: passaram.
- Empacotamento: pasta `Guitar Rack.vst3` verificada e compactada em `Guitar-Rack-Windows-x64-VST3.zip`.
- Artefato GitHub: `Guitar-Rack-Windows-x64-VST3`.
- Execução: https://github.com/aulamix001/CLA-GUITARS/actions/runs/43800282515
- Download: abra a execução, entre no GitHub e selecione o artefato na seção Artifacts. Extraia o ZIP interno e copie a pasta VST3 inteira para `C:\Program Files\Common Files\VST3\`.
- Linux x86_64 Release VST3 também compilou; testes do núcleo passaram a 44.1 / 48 / 96 kHz.
- Carregamento manual em DAW Windows e audição ainda não foram realizados.

## Atualização da interface

- Editor JUCE atualizado com tecido procedural vintage, metal escovado, chave iluminada, dial serrilhado, faders e medidores L/R independentes.
- Teste local do editor: Color, Undo, Redo e restauração A/B passaram.
- Windows: compilação, testes DSP, verificação do binário VST3 e upload passaram na execução vinculada acima.
- `docs/interface.png`: captura do editor real. `docs/visual-reference.png`: imagem gerada de direção visual, não implementada como interface.
- Não há comprovação de igualdade pixel a pixel ou sonora ao produto Waves.

## Banco de 60 presets

- Compilação Windows x64: passou para o commit `3fb8e01`.
- `dsp_tests` e `preset_tests`: passaram no Linux e na execução Windows acima.
- 60 presets embutidos, seis categorias com 10 itens cada; preservados os 18 índices anteriores.
- Mapeamento de faders, Colors, estados On/Off, modo, Re-Amplify e ganhos validado; restauração de sessão e processamento estéreo testados para cada preset.
- ZIP Windows contém `Guitar Rack.vst3`, os 60 XMLs em `Presets/`, `presets.csv` e instruções de instalação.
- Nomes de presets descrevem usos sugeridos; audição e teste manual em DAW continuam pendentes.
