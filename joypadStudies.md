# JoypadOS Studies

## Objetivo

Este documento substitui os guias anteriores sobre Bluetooth DInput. A partir de agora, o foco nao e implementar diretamente no OGX-Mini-Plus, mas investigar a fundo como o JoypadOS consegue taxas tao altas com DS4 Bluetooth na mesma Pi Pico 2W.

Depois da investigacao e da documentacao tecnica, sera criado outro plano separado para portar ao OGX-Mini-Plus somente as otimizacoes comprovadas.

## Decisao de direcao

Os testes recentes mudam a premissa do trabalho:

- o mesmo DS4 foi usado
- a mesma Pi Pico 2W foi usada
- o JoypadOS foi compilado localmente e testado em varios modos
- o JoypadOS atingiu resultados acima de 700 Hz em alguns modos USB
- portanto, o hardware nao e o gargalo principal

O problema atual do OGX-Mini-Plus deve ser investigado como diferenca de arquitetura, pipeline, stack Bluetooth, parser, scheduling ou saida USB.

## Evidencia atual

Arquivo de referencia: `ds4test.txt`

Resultados registrados com JoypadOS:

| Modo JoypadOS | Taxa media | Intervalo medio | Maximo | Jitter | Outliers |
|---|---:|---:|---:|---:|---:|
| PS3 / DInput | 248.76 Hz | 4.02 ms | 11.90 ms | 0.33 ms | 0.60% |
| Razer Pantera / PS4 | 778.21 Hz | 1.28 ms | 4.00 ms | 0.48 ms | 0.60% |
| Pokken / Switch | 775.19 Hz | 1.29 ms | 4.04 ms | 0.51 ms | 0.95% |
| Xbox 360 / XInput | 249.00 Hz | 4.02 ms | 11.87 ms | 0.28 ms | 0.60% |

Leitura inicial:

- JoypadOS consegue receber e entregar DS4 Bluetooth com latencia muito baixa.
- O modo USB escolhido altera fortemente a taxa final percebida no Windows.
- Razer Pantera / PS4 e Pokken / Switch ficam perto de 1.28 ms por update.
- PS3 / DInput e Xbox 360 / XInput ficam perto de 4 ms por update.
- Mesmo o pior modo JoypadOS testado ainda e muito superior ao comportamento de 65-70 ms visto no OGX-Mini-Plus.

## Regras desta fase

- Nao fazer build automaticamente.
- Nao alterar o OGX-Mini-Plus durante a investigacao.
- Nao tentar portar codigo antes de entender o caminho completo.
- Tratar `!joypadOS/` como referencia principal.
- Tratar o OGX-Mini-Plus como alvo de comparacao, nao como alvo de edicao nesta fase.
- Toda conclusao deve apontar arquivos e funcoes concretas.
- Separar fatos observados de hipoteses.

## Hipoteses principais

### H1 - O JoypadOS usa caminho Bluetooth Classic HID mais direto

Investigar se o JoypadOS evita camadas genericas pesadas e processa DS4 em um caminho curto:

`BTstack -> L2CAP HID -> bthid -> ds4_bt -> input_event -> USB output`

Arquivos iniciais:

- `!joypadOS/src/bt/btstack/btstack_host.c`
- `!joypadOS/src/bt/transport/bt_transport_cyw43.c`
- `!joypadOS/src/bt/bthid/bthid.c`
- `!joypadOS/src/bt/bthid/devices/vendors/sony/ds4_bt.c`
- `!joypadOS/src/bt/bthid/bthid_registry.c`

### H2 - O JoypadOS evita ou reduz SDP problematico no CYW43

Investigar se o JoypadOS reconhece Sony por nome/perfil e abre L2CAP direto em casos especificos, evitando a sequencia SDP usada por stacks mais genericas.

Arquivos iniciais:

- `!joypadOS/src/bt/btstack/bt_device_db.c`
- `!joypadOS/src/bt/btstack/bt_device_db.h`
- `!joypadOS/src/bt/btstack/btstack_host.c`

Perguntas:

- Como `Wireless Controller` e classificado?
- Como VID/PID sao definidos quando nao ha SDP completo?
- Quando o JoypadOS pula SDP?
- Quando abre HID Control e HID Interrupt?
- Existe espera por encryption antes de L2CAP?
- O comportamento e diferente para CYW43?

### H3 - O driver DS4 ativa corretamente o modo rapido

Investigar como o JoypadOS envia o primeiro output/SET_REPORT do DS4 e se isso e suficiente para manter report `0x11` em alta taxa.

Arquivos iniciais:

- `!joypadOS/src/bt/bthid/devices/vendors/sony/ds4_bt.c`
- `!joypadOS/src/bt/bthid/bthid.c`
- `!joypadOS/src/bt/transport/bt_transport_cyw43.c`

Perguntas:

- Quando o primeiro `SET_REPORT` e enviado?
- Ele vai pelo canal Control ou Interrupt?
- Quantas vezes e enviado?
- Ha delay antes do primeiro output?
- O JoypadOS usa `0x52` ou `0xA2` para o DS4 em cada caminho?
- O report enviado bate com o formato usado pelo kernel Linux?

### H4 - O JoypadOS usa politica latest-state-wins

Investigar se o JoypadOS descarta samples antigos e preserva apenas o estado mais recente antes de enviar USB.

Arquivos a localizar durante a investigacao:

- roteador de input
- estrutura `input_event_t`
- servico de players
- camada de saida USB

Pontos de busca:

- `router_submit_input`
- `input_event_t`
- `players`
- `feedback`
- `usbd`

### H5 - A saida USB do JoypadOS e mais agressiva

Os testes mostram dois grupos:

- modos em torno de 250 Hz
- modos em torno de 775 Hz

Isso pode estar ligado aos descritores USB, ao intervalo de endpoint, ao modo de envio ou a politica de flush.

Arquivos iniciais:

- `!joypadOS/src/usb/usbd/usbd.c`
- `!joypadOS/src/usb/usbd/usbd_mode.h`
- `!joypadOS/src/usb/usbd/modes/ps4_mode.c`
- `!joypadOS/src/usb/usbd/modes/switch_mode.c`
- `!joypadOS/src/usb/usbd/modes/ps3_mode.c`
- `!joypadOS/src/usb/usbd/modes/xinput_mode.c`
- `!joypadOS/src/usb/usbd/descriptors/ps4_descriptors.h`
- `!joypadOS/src/usb/usbd/descriptors/switch_descriptors.h`
- `!joypadOS/src/usb/usbd/descriptors/ps3_descriptors.h`
- `!joypadOS/src/usb/usbd/descriptors/xinput_descriptors.h`
- `!joypadOS/src/usb/usbd/drivers/tud_xinput.c`

Perguntas:

- Qual endpoint interval cada modo anuncia?
- Qual tamanho de report cada modo envia?
- O envio acontece a cada evento novo ou em loop fixo?
- Existe limitador de taxa por modo?
- Existe chamada a `tud_task_ext()` mais agressiva que no OGX?
- O modo PS4/Razer e Pokken/Switch usam endpoint de 1 ms?

### H6 - O loop principal favorece BT e USB sem sleeps ruins

Investigar como JoypadOS intercala:

- `cyw43_arch_poll()`
- BTstack
- parser HID
- roteador de input
- TinyUSB device
- aplicacao `bt2usb`

Arquivos iniciais:

- `!joypadOS/src/apps/bt2usb/app.c`
- `!joypadOS/src/bt/transport/bt_transport_cyw43.c`
- `!joypadOS/src/usb/usbd/usbd.c`
- `!joypadOS/src/tusb_config.h`

Perguntas:

- Ha `sleep_ms()` no loop critico?
- Qual frequencia de `tud_task_ext()`?
- O BT roda no mesmo core que USB?
- Ha filas profundas entre BT e USB?
- Ha tarefas perifericas competindo com o hot path?

## Plano de investigacao

### Etapa 1 - Mapear o caminho DS4 Bluetooth no JoypadOS

Objetivo:

Entender, com referencias de arquivo e funcao, o fluxo desde a conexao Bluetooth ate a chegada do report DS4.

Saida esperada:

- diagrama textual do fluxo
- lista de funcoes chamadas
- explicacao de como DS4 e reconhecido
- explicacao de como L2CAP HID e aberto
- confirmacao se SDP e usado, pulado ou condicionado

### Etapa 2 - Mapear ativacao e parsing do DS4

Objetivo:

Entender como o JoypadOS faz o DS4 entrar e permanecer em modo de report rapido.

Saida esperada:

- formato exato do output report inicial
- canal usado para envio
- momento em que o envio ocorre
- caminho do report `0x11`
- campos processados e ignorados
- custo estimado do parser

### Etapa 3 - Mapear roteamento interno de input

Objetivo:

Descobrir se JoypadOS preserva todos os reports ou se sempre usa o estado mais recente.

Saida esperada:

- estrutura de dados entre DS4 e USB
- politica de sobrescrita, fila ou dispatch
- onde ocorrem copias de dados
- se ha lock, mutex, circular buffer ou fila

### Etapa 4 - Mapear saida USB dos quatro modos testados

Objetivo:

Explicar por que PS4/Razer e Pokken/Switch passam de 700 Hz enquanto PS3 e XInput ficam em 250 Hz.

Modos:

- PS3 / DInput
- Razer Pantera / PS4
- Pokken / Switch
- Xbox 360 / XInput

Saida esperada:

- descritor usado por modo
- endpoint interval por modo
- funcao que envia report por modo
- tamanho do report por modo
- condicao de envio por modo
- possivel limitador de taxa por modo

### Etapa 5 - Mapear scheduling e concorrencia

Objetivo:

Entender por que BT e USB conseguem coexistir na Pico 2W sem cair para 15 Hz.

Saida esperada:

- loop principal do app `bt2usb`
- chamadas de poll relevantes
- uso de core 0/core 1, se houver
- pontos onde delays existem ou nao existem
- comparacao com a arquitetura atual do OGX-Mini-Plus

### Etapa 6 - Comparar com OGX-Mini-Plus sem editar

Objetivo:

Criar uma matriz de diferencas entre JoypadOS e OGX-Mini-Plus.

Comparar:

- BTstack direto versus Bluepad32
- reconhecimento Sony
- SDP
- abertura L2CAP
- ativacao DS4
- parser DS4
- filas ou estado mais recente
- loop principal
- TinyUSB/device task
- descritores e intervalos USB

Saida esperada:

- tabela JoypadOS vs OGX
- gargalos provaveis
- gargalos descartados
- riscos de portar cada parte

### Etapa 7 - Gerar plano de implementacao separado

Objetivo:

Somente depois da investigacao, criar um plano novo para implementar no OGX-Mini-Plus.

Esse plano deve ser outro documento, criado depois, com etapas testaveis e reversiveis.

## O que nao fazer nesta fase

- Nao portar `ds4_bt.c` ainda.
- Nao substituir Bluepad32 ainda.
- Nao alterar descritores USB do OGX ainda.
- Nao mexer em CMake ou scripts de build agora.
- Nao assumir que o ganho vem de uma unica linha.
- Nao tratar taxa USB alta como prova automatica de taxa Bluetooth alta sem rastrear o pipeline.

## Medicoes adicionais desejadas

Se forem feitos novos testes no JoypadOS, registrar:

- modo USB
- controle usado
- taxa media
- intervalo minimo
- intervalo medio
- intervalo maximo
- jitter
- outliers
- observacao de estabilidade em jogo

Preferencia de testes:

1. DS4 no modo Razer Pantera / PS4
2. DS4 no modo Pokken / Switch
3. DS4 no modo PS3 / DInput
4. DS4 no modo Xbox 360 / XInput
5. DualSense nos mesmos modos, se suportado
6. Switch Pro Controller, se disponivel

## Criterios para encerrar a investigacao

A investigacao so deve ser considerada suficiente quando soubermos responder:

- como o JoypadOS reconhece DS4 no CYW43
- como ele abre L2CAP HID
- se ele pula SDP e em quais condicoes
- como ele ativa report rapido
- se ele processa todos os reports ou so o estado mais recente
- como cada modo USB limita ou nao limita a taxa
- por que Razer/Pokken passam de 700 Hz
- qual diferenca concreta explica o OGX ficar em 65-70 ms

## Resultado esperado desta fase

Ao final, este documento deve conter uma analise tecnica com evidencias suficientes para orientar um plano de implementacao real no OGX-Mini-Plus.

O proximo documento, se necessario, sera um plano de port incremental das otimizacoes confirmadas.
