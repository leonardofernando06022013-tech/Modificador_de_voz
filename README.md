# BlackVoice

BlackVoice é um modificador de voz local para Windows 10/11 x64, escrito em C++20 com JUCE 8.0.13, CMake e WASAPI. Captura um microfone, aplica DSP em tempo real e reproduz o resultado em uma saída física ou virtual já instalada. Não instala driver, não grava áudio, não usa internet em execução e não coleta telemetria.

> Use de forma responsável, com consentimento. O programa não é uma ferramenta de autenticação nem de clonagem de identidade.

## Arquitetura

`AudioEngine` gerencia o `AudioDeviceManager`/WASAPI e a callback. `VoiceProcessor` executa ganho, limpeza espectral DSP, gate, compressor, pitch granular, efeitos, mistura e limitador. Os parâmetros são atômicos e suavizados onde a alteração produziria degraus. `PresetManager` e `SettingsManager` fazem I/O JSON somente fora da thread de áudio. A UI lê medidores atômicos por timer. A callback não acessa disco, GUI ou logs e reutiliza buffers preparados previamente.

Fluxo implementado:

`microfone → HP/LP + redução de ruído → EQ tonal de 3 bandas → gate → compressor → pitch/formante → distorção/ring/bit-crusher/flanger/chorus/delay/reverb → dry/wet → limitador → saída`

O pitch usa duas cabeças de atraso granular defasadas, janelas Hann e crossfade. Assim, muda a altura sem mudar a velocidade global. A qualidade é apropriada para fala e baixa latência, mas não equivale a algoritmos comerciais de preservação espectral de formantes.

## Recursos

- WASAPI, seleção de entrada/saída, sample rate, buffer e canais pela tela Dispositivos.
- 44,1/48 kHz e buffers 128/256/512/1024 quando oferecidos pelo driver.
- Pitch ±12 semitons e ajuste fino, dry/wet, ganhos e inversão de fase.
- Redução de ruído DSP, gate, passa-altas/baixas, compressor e limitador.
- Distorção, chorus, delay, reverb, ring modulation e bit crusher.
- 12 presets-base, 1.000 variações determinísticas e presets JSON do usuário, carregados sob demanda na UI.
- Soundboard com pads dinâmicos e leitura antecipada em thread de background.
- Favoritos, busca, categorias, paginação e teste de voz em tempo real.
- Medidores, clipping, CPU aproximada, latência e underruns estimados.
- Reconexão periódica ao último dispositivo após desconexão.
- Tema escuro, bypass geral e relatório copiável.

## Requisitos de desenvolvimento

1. Windows 10 ou 11 x64.
2. Visual Studio 2022: no Visual Studio Installer marque **Desenvolvimento para Desktop com C++**, MSVC v143 e Windows 10/11 SDK.
3. CMake 3.25 ou posterior. Marque “Add CMake to PATH” no instalador.
4. Git, usado pelo CMake para baixar a tag fixa do JUCE na primeira configuração.
5. Internet somente no primeiro build automático. O executável não depende de internet.

JUCE 8.0.13 é obtido por `FetchContent`. Para uma cópia local, instale o JUCE e configure com `-DVOXFORGE_FETCH_JUCE=OFF -DCMAKE_PREFIX_PATH=C:\caminho\JUCE\lib\cmake\JUCE`.

## Compilar

Abra **Developer PowerShell for VS 2022**, entre na pasta do projeto e execute:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Debug
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Release
```

Equivalente manual:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Executável: `build\BlackVoice_artefacts\Release\BlackVoice.exe`.

Limpar: `powershell -ExecutionPolicy Bypass -File .\scripts\clean.ps1`.

Portátil: compile Release e execute `powershell -ExecutionPolicy Bypass -File .\scripts\portable.ps1`.

## Instalador

Instale Inno Setup 6, compile Release e abra `installer\BlackVoice.iss`, ou rode:

```powershell
& "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe" .\installer\BlackVoice.iss
```

O instalador em `dist` inclui executável, README, licença, atalhos e desinstalador.

## Configuração de cabo virtual

Instale um dispositivo como VB-CABLE conforme as instruções do fabricante e reinicie se solicitado. O BlackVoice não inclui nem instala esse driver.

Para Discord ou FiveM:

1. Selecione o microfone real como entrada no BlackVoice.
2. Selecione **CABLE Input** (ou entrada de outro cabo virtual) como saída no BlackVoice.
3. No Discord/FiveM, selecione **CABLE Output** como microfone.
4. Use fones de ouvido para evitar feedback.

No Discord, desative processamento automático agressivo caso ele altere o efeito e mantenha 48 kHz em toda a rota. No FiveM, altere o dispositivo de entrada nas opções de voz e reinicie o cliente se ele não atualizar a lista.

No OBS, adicione **Captura de entrada de áudio** e escolha `CABLE Output`. Evite monitorar essa fonte de volta ao mesmo cabo.

## Latência, eco e microfonia

- Comece em 48 kHz/256 amostras. Tente 128 se o sistema ficar estável; aumente para 512/1024 se houver cortes.
- Sample rate é a quantidade de amostras por segundo. Todos os dispositivos na rota devem preferencialmente usar a mesma taxa.
- Buffer menor reduz latência e aumenta a exigência de CPU. Buffer maior é mais estável, mas atrasa a fala.
- Use fones, não caixas de som. Não escolha como saída do BlackVoice o mesmo dispositivo que retorna à entrada.
- Desative “Ouvir este dispositivo” no Windows se houver voz duplicada.
- O “monitoramento” ocorre ao escolher uma saída audível; para uso com cabo virtual, monitore pelo VoiceMeeter/OBS sem fechar um loop.

## Arquivos e privacidade

Configurações: `%APPDATA%\BlackVoice\settings.json`. Presets: `%APPDATA%\BlackVoice\Presets`. Logs: `%LOCALAPPDATA%\BlackVoice\Logs`. Em modo portátil, são usadas as pastas `Data` e `Presets` ao lado do executável. JSON inválido é movido para backup e valores seguros permanecem ativos. Nenhum áudio é salvo automaticamente.

## Testes

`ctest` cobre buffer vazio, bypass, limitador, limites de parâmetros (incluindo EQ), JSON inválido, round-trip de preset, roteamento de páginas, detecção de cabos/aplicativos e autorização administrativa. Para teste manual sem microfone, use um cabo virtual ou reprodutor roteado como entrada WASAPI; o modo WAV offline ainda não está disponível nesta versão.

O teste do roteador valida página atual, página anterior e supressão de transições duplicadas. Para percorrer todas as páginas do aplicativo automaticamente:

```powershell
.\build\BlackVoice_artefacts\Release\BlackVoice.exe --ui-smoke-test
```

Checklist manual:

- abrir sem dispositivo e conferir mensagem;
- selecionar entrada/saída e iniciar/parar;
- testar 44,1 e 48 kHz em 128–1024;
- alterar pitch/efeitos sem estalos;
- salvar/reabrir preset e configurações;
- desconectar/reconectar USB;
- verificar bypass, limitador e indicador de clip;
- rotear a Discord, FiveM e OBS com fones.

## Solução de problemas

- **Nenhum microfone:** Configurações do Windows → Privacidade e segurança → Microfone → permita acesso a aplicativos desktop.
- **Dispositivo ocupado:** feche DAWs/jogos em modo exclusivo ou desative “Permitir que aplicativos assumam controle exclusivo” nas propriedades avançadas do dispositivo.
- **Taxa incompatível:** escolha uma taxa oferecida na tela e iguale o Formato padrão do Windows.
- **Estalos:** aumente o buffer, feche efeitos/overlays pesados e confira o diagnóstico de CPU.
- **Sem áudio no Discord:** confira que o BlackVoice sai em `CABLE Input` e Discord recebe de `CABLE Output`.
- **Caminho/CMake:** use uma pasta comum sem permissões administrativas e confirme `cmake --version`, `git --version` e `where cl`.

## Limitações conhecidas

- Redução de ruído é DSP determinístico simples, não uma “IA”; reduz piso contínuo moderado, mas não separa fala de música/vozes.
- Pitch e formante usam algoritmos voltados a fala e baixa latência; não equivalem a processamento espectral comercial de estúdio.
- O equalizador implementado possui HP/LP e graves/médios/agudos com cruzamentos fixos; filtros peak paramétricos livres, de-esser, AGC, pan, gate hold, knee/makeup e modo WAV offline não estão implementados.
- Presets podem ser criados, salvos, copiados e excluídos; renomeação e importação/exportação em lote de presets individuais ainda não estão expostas.
- WASAPI é escolhido pelo JUCE no Windows, mas o driver disponível e as taxas/buffers dependem do hardware.
- Reconexão usa o último dispositivo do JUCE; a troca durante callback depende do backend e deve ser validada com o hardware alvo.
- Bandeja do sistema, atualização automática e qualidade adaptativa aparecem desativadas e identificadas como indisponíveis enquanto não houver backend próprio.
- O painel administrativo autentica apenas a identidade local do Windows; não substitui autenticação, autorização ou auditoria de um servidor.

Esses itens são declarados para não confundir código presente com comportamento ainda não verificado.

## Licenças

O código do BlackVoice usa MIT. JUCE 8.0.13 é uma dependência externa com licenciamento próprio (AGPLv3 ou licença comercial JUCE, conforme o uso). Antes de distribuir um binário fechado, obtenha uma licença JUCE compatível. Não há biblioteca de pitch adicional.

## Pacote completo, assinatura e SmartScreen

Com Visual Studio 2022, CMake, Git e Inno Setup no PATH, execute:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-all.ps1
```

O comando compila Release x64, executa os testes, cria `dist\portable`, cria `dist\installer\BlackVoice-Setup-x64.exe` e grava `dist\checksums\SHA256.txt`. Ele interrompe imediatamente se uma ferramenta, teste ou artefato estiver ausente.

O projeto não contém certificado. Binários sem assinatura podem acionar o Windows SmartScreen. Para assinar futuramente, instale o Windows SDK e use `scripts\sign.ps1` com o thumbprint de um certificado de assinatura de código legítimo. Não tente contornar o SmartScreen.

## Integrações com Discord, FiveM, jogos e OBS

Abra **Integrações** no menu lateral, escolha o microfone físico e selecione uma saída virtual já instalada, como `CABLE Input`, `VoiceMeeter Input` ou outro endpoint reconhecido. Clique em **Aplicar roteamento** e depois em **Testar integração**.

No aplicativo de destino, selecione o endpoint de gravação correspondente (por exemplo, `CABLE Output`) como microfone. A integração usa somente os dispositivos normais do Windows; não há injeção, hooks ou alteração de processos. Caso nenhum cabo virtual seja detectado, a página oferece acesso às Configurações de Som do Windows e instruções de instalação/configuração.
