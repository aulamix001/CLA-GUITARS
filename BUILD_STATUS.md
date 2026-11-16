# Estado da compilação

- Núcleo DSP: testes passaram a 44.1 / 48 / 96 kHz.
- Linux x86_64 Release VST3: compilado com sucesso; manifesto gerado por juce_vst3_helper.
- Windows x64: bloqueado. JUCE 8.0.6 rejeita MinGW. Download do SDK Microsoft via xwin 0.10.0 recebeu HTTP 403 ao redirecionar de aka.ms para download.visualstudio.microsoft.com.
- Rede: domínio Microsoft adicionado ao rascunho; requer revisão, salvar e publicação nas configurações do ambiente.
- Windows MSVC: workflow e script PowerShell preparados, ainda não executados no Windows.
- Nenhum VST3 Windows é incluído no pacote de código-fonte.
