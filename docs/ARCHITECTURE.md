# Arquitetura e decisões

O BlackVoice é um aplicativo desktop Windows escrito em C++20, JUCE 8.0.13 e CMake. O processamento é local: o executável não grava áudio, não envia áudio à internet e não instala drivers.

## Áudio em tempo real

`AudioEngine` mantém uma única callback duplex do `AudioDeviceManager`. Os buffers de trabalho, monitoramento e soundboard são dimensionados em `audioDeviceAboutToStart`; a callback reutiliza esses buffers e não acessa GUI, JSON, logs ou disco.

`VoiceProcessor` usa parâmetros `std::atomic`, rampas para ganho/mistura e `ScopedNoDenormals`. O fluxo atual é:

`entrada → ganho → HP/LP + redução de ruído → EQ tonal de 3 bandas → gate → compressor → pitch/formante → efeitos → dry/wet → limitador/DC block → saída`

Os efeitos implementados são distorção, ring modulation, bit crusher, flanger, chorus, delay e reverb. O custo por bloco é linear em canais × amostras. A UI lê apenas picos, CPU e underruns atômicos.

O soundboard usa `AudioTransportSource` com read-ahead de 32.768 amostras e uma `TimeSliceThread` dedicada. Os pads da UI apenas selecionam/carregam o arquivo na message thread e acionam o transporte; a leitura de disco não ocorre diretamente na callback.

O monitoramento usa outro `AudioDeviceManager` e um ring buffer pré-alocado. A opção deve ser usada com fones para evitar feedback.

## Shell e navegação

`MainComponent` é o shell: sidebar, topbar, notificações e barra inferior. Ele não constrói cards de voz nem conteúdo específico de módulos. `PageRouter` controla a página atual/anterior e delega para componentes persistentes:

- Início (`ModulePage::Home`)
- Vozes (`VoicesPage`)
- Efeitos, Soundboard, Favoritos, Equalizador e Presets (`ModulePage`)
- Dispositivos (seletor nativo do JUCE)
- Integrações (`IntegrationsPage`)
- Diagnóstico
- Configurações (`SettingsPage`)
- Administração (`AdminPage`)

O layout suporta 1024×768 até 2200×1400. A sidebar entra em modo compacto, a grade recalcula colunas, as páginas longas usam viewport e as áreas de detalhes mantêm dimensões estáveis durante a troca de acordeões.

## Biblioteca de vozes

`PresetManager` fornece 12 presets-base, 1.000 variações determinísticas e presets JSON do usuário. `VoicesPage` é a única responsável por busca, filtros, favoritos, cards, painel de detalhes e criação/exclusão de vozes personalizadas.

Para evitar mais de mil componentes simultâneos, apenas 72 cards são materializados por página. “Mostrar mais” amplia o limite sob demanda. Os presets de fábrica são calculados quando selecionados; não há arquivos redundantes para cada variação.

As transformações são multilíngues porque trabalham sobre o sinal e não reconhecem texto. A UI identifica isso explicitamente, em vez de atribuir idiomas fictícios.

## Tema e acessibilidade

`Theme.h` centraliza cores, raios, espaçamentos, fundo, painéis e preferências globais. `AppLookAndFeel` desenha estados normal, hover, pressionado, desabilitado, foco, sucesso e perigo.

Alto contraste, foco visível, áreas de clique maiores, redução de animações, modo compacto, frequência dos medidores e tooltips são aplicados de verdade. Opções sem backend — bandeja, atualização automática e qualidade adaptativa — ficam desabilitadas com uma explicação.

## Persistência

`SettingsManager` usa JSON e gravações atômicas por `TemporaryFile`. Preferências são carregadas em lote e alterações da página de configurações são persistidas em uma única escrita, evitando dezenas de leituras/gravações por clique. Parâmetros de áudio são limitados a faixas seguras ao importar JSON inválido ou extremo.

Favoritos, preferências, dispositivo, monitoramento e presets têm arquivos separados. I/O ocorre somente fora da callback.

## Integrações

`IntegrationsPage` configura endpoints normais do Windows. Um cabo virtual externo, como VB-CABLE ou VoiceMeeter, continua necessário para expor a saída processada como microfone em Discord, FiveM, OBS, jogos ou chamadas.

`ApplicationDetector` verifica apenas se processos conhecidos estão abertos. Esse sinal nunca é apresentado como prova de que o roteamento funciona. O teste de integração valida dispositivos, estado do engine e rota local; a seleção final do microfone no aplicativo de destino continua sendo responsabilidade do usuário.

## Administração e segurança

O painel administrativo é local. Na primeira execução sem armazenamento, `UserManager` cria o proprietário associado ao login atual do Windows. Depois disso, uma identidade desconhecida é negada; ela não é promovida automaticamente.

`AdminAccessController` aplica permissões por função. Um administrador não pode conceder função acima da própria, alterar/bloquear/remover um superadministrador nem mudar a própria função/status. Exclusões, bloqueios e limpeza de dados exigem confirmação.

Essa camada não substitui autenticação remota, banco de dados, criptografia de servidor ou trilha de auditoria corporativa.

## Verificação

`BlackVoiceTests` cobre DSP, limites e JSON, presets, roteador, detectores e autorização administrativa. O argumento `--ui-smoke-test` percorre todas as rotas e testa layouts-alvo, incluindo 1024×768, na message thread.

Comandos de referência:

```powershell
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\BlackVoice_artefacts\Release\BlackVoice.exe --ui-smoke-test
```
