# Estado da compilação

- Windows x64 Release VST3: **compilado com sucesso no MSVC**.
- Commit compilado: `58be716`.
- Testes DSP no Windows: passaram.
- Empacotamento: pasta `Guitar Rack.vst3` verificada e compactada em `Guitar-Rack-Windows-x64-VST3.zip`.
- Artefato GitHub: `Guitar-Rack-Windows-x64-VST3`, 2.58 MB.
- Execução: https://github.com/aulamix001/CLA-GUITARS/actions/runs/43795023814
- Download: abra a execução, entre no GitHub e selecione o artefato na seção Artifacts. Extraia o ZIP interno e copie a pasta VST3 inteira para `C:\Program Files\Common Files\VST3\`.
- Linux x86_64 Release VST3 também compilou; testes do núcleo passaram a 44.1 / 48 / 96 kHz.
- Carregamento manual em DAW Windows e audição ainda não foram realizados.
