# Arquitetura e decisões

O aplicativo usa uma única callback duplex de `AudioDeviceManager`, evitando uma fila entre entrada e saída quando o driver fornece ambos no mesmo dispositivo lógico. A área de trabalho e os buffers dry/wet são dimensionados em `audioDeviceAboutToStart`. Parâmetros são `std::atomic`; ganhos e mistura usam rampas de 20 ms. `ScopedNoDenormals` evita penalidade de números denormais.

A UI nunca toca no buffer. Ela publica parâmetros e lê picos/CPU atômicos em 20 Hz. JSON e caixas de diálogo ficam na message thread. Presets são aplicados por várias escritas atômicas; isso é seguro, embora não seja uma troca transacional de todos os parâmetros no mesmo sample.

O custo é O(canais × amostras), com buffers fixos e sem GPU. Pitch granular usa uma linha mono compartilhada, escolha deliberada para entrada de microfone e estabilidade de fase nas saídas estéreo.

## Arquitetura visual

A interface usa `AppLookAndFeel` e a paleta centralizada em `Theme.h`. `SidebarComponent`, `VoiceCardComponent`, `AudioLevelMeter` e `NotificationComponent` são componentes reutilizáveis sem acesso direto à callback. `MainComponent` organiza barra superior, navegação, banner, filtros, grade rolável, painel de parâmetros e barra inferior. Cards publicam presets no `PresetManager`; sliders escrevem apenas em parâmetros atômicos; medidores, CPU, latência e diagnóstico são consultados por timer a 30 Hz.

Avatares e ondas são desenhados por `juce::Graphics`, evitando imagens externas, marcas de terceiros e carregamento repetido de recursos. O layout recalcula colunas conforme a largura e recolhe o painel direito abaixo de 1250 px.

## Navegação por páginas

`PageRouter` mantém página atual/anterior, ignora navegação duplicada e publica uma única transição. As dez rotas têm componentes persistentes: Início, Vozes, Efeitos, Painel de Som, Favoritos, Equalizador, Dispositivos, Presets, Diagnóstico e Configurações. `SettingsPage` fica em arquivo próprio e usa viewport com cartões, debounce de 550 ms para salvar parâmetros, backup antes da importação e controles reais de dispositivo.

O executável aceita `--ui-smoke-test`: percorre todas as rotas na message thread e encerra. Isso permite testar navegação sem depender de coordenadas ou de overlays de outros aplicativos.
