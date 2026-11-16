# Guitar Rack — C++ / JUCE VST3

Plugin independente para guitarra, inspirado no fluxo de controles das referências fornecidas. Alvo: **Windows 10/11 x64, VST3**. Não é um produto Waves, não inclui seus binários, marcas gráficas ou presets oficiais e não reproduz comprovadamente seus algoritmos.

## Compilar no Windows

Instale Visual Studio 2022 com **Desktop development with C++**, Windows SDK, CMake >= 3.22 e Git. No Developer PowerShell:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

O CMake obtém JUCE 8.0.6 fixado em commit. Alternativamente use `-DJUCE_PATH=C:/deps/JUCE`. Consulte a licença AGPL/comercial do JUCE antes de distribuir.

Resultado: `build/CLAInspiredGuitar_artefacts/Release/VST3/Guitar Rack.vst3`. Copie **a pasta inteira** para `C:\Program Files\Common Files\VST3\` e reescaneie na DAW. Também é gerado um aplicativo Standalone para testes. Interface e processamento aceitam entrada mono/estéreo e saída estéreo.

O workflow `.github/workflows/windows.yml` compila com MSVC x64, executa testes e disponibiliza a pasta VST3 como artefato. Precisa estar publicado no GitHub e executar com sucesso; não foi executado pelo agente.

## Controles e presets

Input/Output e seis faders: −10 a +10. Re-Amplify: on/off; Clean, Crunch, Heavy, Bypass. Clique nos botões coloridos para alternar três modos; ON desativa cada módulo. Os parâmetros estão expostos à automação da DAW. Os medidores exibem picos de entrada/saída e o LED de sensibilidade sinaliza o nível.

Load contém 18 presets originais em Acoustic, Clean, Crunch, Heavy, Lead / Solo e FX / Special. A seleção atualiza todos os controles públicos, sem parâmetros sonoros ocultos. Input/Output são zerados e os seis módulos ativados. Save grava XML, Load user preset recupera XML. O estado completo é salvo na sessão da DAW. Não há presets atribuídos a artistas convidados sem valores oficiais fornecidos.

## Implementação e limites de fidelidade

Re-Amplify usa saturação tanh e filtro de gabinete simples; Bass/Treble usam shelves; Compress usa envelope e ganho dependente do nível; Reverb usa quatro combs por canal; Delay usa taps e feedback filtrado; Pitch usa dois taps modulados com crossfade. As cores selecionam frequências, razão, tamanho ou tap conforme o módulo. Não há modelo de amplificador medido, IR de gabinete, oversampling, sincronismo de tempo da DAW ou pitch shifter de qualidade comercial.

As páginas do manual vieram em imagens de baixa resolução. Nomes/valores que precisam de confirmação: especialmente os três modos de Delay/Pitch, frequências, tempos, regras exatas do LED e valores padrão. Os valores internos são **aproximações próprias**, não especificações extraídas do original. A interface é desenhada em vetores com tecido procedural, metal, sombras e faders móveis; não é cópia pixel a pixel. Não existe processamento de áudio dentro das imagens geradas anteriormente.

## Validação

Teste do núcleo independente do JUCE:

```sh
cmake -S . -B build-dsp -DBUILD_PLUGIN=OFF
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

Testa bypass unitário, medidores, estabilidade dos efeitos/cores, cauda do delay e ganho de saída a 44.1, 48 e 96 kHz. O plugin VST3 Linux completo, incluindo interface e wrapper JUCE, foi compilado com sucesso em Release. O manifesto VST3 foi gerado carregando o módulo Linux. Ainda é necessário compilar para Windows e validar em host/DAW, automação, salvar/reabrir sessão, mono→estéreo, presets, redimensionamento e audição. JUCE 8 rejeita MinGW; a alternativa Clang/MSVC SDK está bloqueada pelo proxy HTTP 403 ao obter o SDK em `download.visualstudio.microsoft.com`. A inclusão desse domínio foi salva no rascunho do ambiente e ainda precisa ser aplicada/publicada pelo usuário. Não há binário Windows validado nesta entrega.
