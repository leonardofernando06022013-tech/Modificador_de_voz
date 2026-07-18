# Changelog do DSP vocal

## Cadeia de voz profissional

Esta revisão substitui o caminho de voz básico por uma cadeia causal e segura para tempo real, mantendo compatibilidade com presets JSON anteriores.

### Alterações perceptivas

- Pitch estéreo independente, rampa de 35–40 ms e preservação opcional do envelope por LPC de ordem 10.
- Limite natural de ±6 semitons com LPC; rota granular criativa continua disponível até ±12.
- Redução de ruído com estimativa contínua do piso e ganho suave, sem fechamento abrupto.
- Gate com detector separado, histerese de 4 dB, hold configurável e release por envoltória.
- De-esser linkado em estéreo na região de 5–9 kHz, detector RMS de 8 ms e até 8 dB de redução.
- AGC lento e opcional com alvo de −18 dBFS e ganho limitado entre −5,2 e +6 dB.
- Compressor multibanda opcional de duas bandas, separado em 260 Hz.
- EQ tonal limitado internamente a ±6 dB nos presets naturais.
- Distorção forte com interpolação 2× e filtro de descida leve; bit crusher com suavização.
- Chorus mais lento, delay com damping/ducking e reverb limitado a 15% de wet.
- Limitador final em −0,5 dBFS e remoção de DC.

### Segurança e desempenho

- `AudioEngine` processa diretamente o buffer preparado e informa apenas a região ativa; não constrói um `AudioBuffer` referenciado dentro da callback.
- Buffers de DSP são dimensionados em `prepare()`/`audioDeviceAboutToStart()`.
- O soundboard continua usando `AudioTransportSource` com read-ahead de 32768 amostras em `TimeSliceThread` dedicada. A callback usa somente `tryEnter()` no lock externo e pula o bloco do pad caso a UI esteja trocando o arquivo, impedindo espera bloqueante.
- O modo **Baixo consumo** desliga LPC, AGC, multibanda e oversampling de distorção.
- Não há I/O de arquivo, GUI, log ou criação explícita de objetos no caminho por amostra.

### Validação recomendada

1. Execute `ctest --test-dir build -C Release --output-on-failure`.
2. Execute o smoke test da UI com `BlackVoice.exe --ui-smoke-test`.
3. Grave a mesma leitura de 10 segundos em bypass e nos presets Conversação Natural/Podcast.
4. Rode `scripts/audio_metrics.py` usando uma captura limpa como referência.
5. Faça A/B cego com volume igualado e registre sibilância, pumping, cortes de palavras, alteração de timbre e preferência.

PESQ e STOI não foram incorporados porque exigem dependências/licenças externas. Eles podem complementar o script padrão em um ambiente de avaliação separado.
