# tech-joypad.md

Documento tecnico de referencia sobre como o JoypadOS trata comunicacao Bluetooth, roteamento interno e saida USB.

Escopo atual:

- Firmware analisado: `!joypadOS/`
- Placa de interesse: Pi Pico 2W / CYW43
- Controle de referencia: DualShock 4 via Bluetooth Classic HID
- Foco: entender por que JoypadOS alcanca taxas altas com DS4 Bluetooth
- Estado: investigacao, sem plano de port neste documento
- Build: nao executar build automatico

## Resumo executivo

O JoypadOS usa uma cadeia curta e orientada a eventos:

```text
CYW43 / BTstack
        -> L2CAP HID Control + Interrupt
        -> bthid
        -> driver Sony DS4/DS5
        -> router_submit_input()
        -> router latest-state-wins
        -> usbd pending_events
        -> modo USB ativo
        -> TinyUSB
        -> Windows / host USB
```

Nos testes registrados em `ds4Test.txt`, usando o mesmo DS4 e a mesma Pi Pico 2W:

| Modo JoypadOS | Taxa media | Intervalo medio |
|---|---:|---:|
| PS3 / DInput | 248.76 Hz | 4.02 ms |
| Razer Pantera / PS4 | 778.21 Hz | 1.28 ms |
| Pokken / Switch | 775.19 Hz | 1.29 ms |
| Xbox 360 / XInput | 249.00 Hz | 4.02 ms |

Leitura inicial:

- O hardware nao e o gargalo principal.
- O modo USB exposto ao Windows influencia fortemente a taxa medida.
- JoypadOS prova que DS4 Bluetooth na Pico 2W pode alimentar modos USB acima de 700 Hz.
- PS4/Razer e Switch/Pokken sao os modos mais rapidos nos testes.

## Arquivos centrais

Bluetooth / BTstack / CYW43:

- `!joypadOS/src/bt/btstack/btstack_host.c`
- `!joypadOS/src/bt/transport/bt_transport_cyw43.c`
- `!joypadOS/src/bt/btstack/bt_device_db.c`
- `!joypadOS/src/bt/btstack/bt_device_db.h`

Bluetooth HID:

- `!joypadOS/src/bt/bthid/bthid.c`
- `!joypadOS/src/bt/bthid/bthid_registry.c`
- `!joypadOS/src/bt/bthid/devices/vendors/sony/ds4_bt.c`
- `!joypadOS/src/bt/bthid/devices/vendors/sony/ds5_bt.c`

Roteamento:

- `!joypadOS/src/core/router/router.c`

App BT para USB:

- `!joypadOS/src/apps/bt2usb/app.c`
- `!joypadOS/src/main.c`

USB device:

- `!joypadOS/src/usb/usbd/usbd.c`
- `!joypadOS/src/usb/usbd/usbd_mode.h`
- `!joypadOS/src/usb/usbd/modes/ps4_mode.c`
- `!joypadOS/src/usb/usbd/modes/switch_mode.c`
- `!joypadOS/src/usb/usbd/modes/ps3_mode.c`
- `!joypadOS/src/usb/usbd/modes/xinput_mode.c`
- `!joypadOS/src/usb/usbd/drivers/tud_xinput.c`

Descritores:

- `!joypadOS/src/usb/usbd/descriptors/ps4_descriptors.h`
- `!joypadOS/src/usb/usbd/descriptors/switch_descriptors.h`
- `!joypadOS/src/usb/usbd/descriptors/ps3_descriptors.h`
- `!joypadOS/src/usb/usbd/descriptors/xinput_descriptors.h`

## Reconhecimento Sony

O JoypadOS reconhece Sony por nome/perfil e por VID/PID quando disponivel.

Em `bt_device_db.c`:

- `"Wireless Controller"` aponta para perfil Sony.
- `"DualSense"` aponta para perfil Sony.
- perfis relevantes sao `classic_only`.

Isso e importante porque permite tratar DS4/DS5 como dispositivos conhecidos mesmo quando SDP e incompleto, lento ou problematico.

## SDP e L2CAP no CYW43

No caminho do CYW43, o JoypadOS possui logica explicita para forcar direct L2CAP em Sony e pular SDP.

Ponto tecnico importante em `btstack_host.c`:

- comentario indica que respostas SDP de DS4/DS5 podem travar o barramento SPI do CYW43.
- para Sony no CYW43, o firmware forca direct L2CAP.
- a intencao e abrir canais HID Control e HID Interrupt sem depender do SDP problematico.

Fluxo conceitual:

```text
nome/perfil indica Sony
        -> perfil conhecido
        -> no CYW43, usa direct L2CAP
        -> evita SDP problematico
        -> abre HID Control PSM 0x11
        -> abre HID Interrupt PSM 0x13
```

Observacao:

- O JoypadOS tambem tem caminhos para reconexao e HID Host.
- A regra mais importante para o caso testado e o tratamento especial Sony/CYW43 para evitar SDP.

## Entrada HID Bluetooth

O caminho esperado para reports HID e:

```text
L2CAP Interrupt
        -> pacote HID DATA | INPUT, header 0xA1
        -> bthid
        -> driver vendor Sony
        -> parser DS4/DS5
```

Em `bthid.c`, reports Sony podem ser reclassificados por report id:

- DS4 report `0x11`
- DS5 report `0x31`

Isso reduz dependencia de classificacao perfeita no inicio da conexao.

## Ativacao DS4 em modo rapido

Arquivo:

- `!joypadOS/src/bt/bthid/devices/vendors/sony/ds4_bt.c`

O DS4 Bluetooth precisa ser colocado em modo enhanced report para emitir o report completo rapido.

No JoypadOS:

- o output inicial usa `SET_REPORT | Output`;
- byte inicial `0x52`;
- report id `0x11`;
- flags de Bluetooth incluem `0x80 0x00 0xFF`;
- o envio e feito pelo canal Control;
- ha um delay inicial de aproximadamente 100 ms antes do primeiro envio.

Formato conceitual:

```text
[0] 0x52  SET_REPORT | OUTPUT
[1] 0x11  report id DS4 Bluetooth output
[2] 0x80  BT flags
[3] 0x00
[4] 0xFF  enable flags
...
```

Leitura tecnica:

- O primeiro SET_REPORT ativa ou estabiliza o modo `0x11`.
- O JoypadOS evita depender apenas de output via interrupt para esse passo.
- Isso coincide com a observacao de que DS4 precisa ser "cutucado" corretamente para sair do report basico.

## Parser DS4

Arquivo:

- `!joypadOS/src/bt/bthid/devices/vendors/sony/ds4_bt.c`

O parser DS4 aceita o report Bluetooth `0x11`.
O report recebido pelo parser ignora o header HID externo e processa o payload Sony.

Campos relevantes:

- sticks
- dpad
- botoes
- triggers analogicos
- touchpad, quando presente
- bateria/status, quando usado
- motion, quando presente

Depois de montar `input_event_t`, chama:

```c
router_submit_input(&ds4->event);
```

Esse e o ponto de saida do driver DS4 para o roteador interno.

## Parser DualSense

Arquivo:

- `!joypadOS/src/bt/bthid/devices/vendors/sony/ds5_bt.c`

O DualSense usa report `0x31` no Bluetooth.
Tambem passa pelo roteador com `router_submit_input()`.

Importante para cruzamento futuro:

- DS4 e DS5 tem ativacoes e formatos diferentes.
- O comportamento ruim no OGX para DS4 e DualSense pode ter causas parecidas no scheduling ou L2CAP, mas os detalhes de enable report nao sao identicos.

## Router interno

Arquivo:

- `!joypadOS/src/core/router/router.c`

O roteador usa politica latest-state-wins por output/player.

Padrao observado:

```c
router_outputs[output][player].current_state = *final_event;
router_outputs[output][player].updated = true;
```

Ao consumir:

```c
if (router_outputs[output][player_id].updated) {
    router_outputs[output][player_id].updated = false;
    router_output_copy[output][player_id] = router_outputs[output][player_id].current_state;
}
```

Leitura tecnica:

- JoypadOS nao precisa preservar todos os frames em uma fila profunda para o USB.
- Ele preserva o estado mais recente e marca que existe update.
- Isso reduz backlog e favorece latencia.
- O comportamento e parecido conceitualmente com `Gamepad::PadIn` do OGX, mas o JoypadOS tem roteamento e consumo USB mais integrados.

## Ponte para USB

Arquivo:

- `!joypadOS/src/usb/usbd/usbd.c`

O USB mantem:

```c
static input_event_t pending_events[USB_MAX_PLAYERS];
static bool pending_flags[USB_MAX_PLAYERS];
```

Quando recebe evento:

```c
pending_events[player_index] = *event;
pending_flags[player_index] = true;
```

Quando envia:

```c
if (!pending_flags[player_index]) return false;
const input_event_t* event = &pending_events[player_index];
pending_flags[player_index] = false;
return mode->send_report(...);
```

Leitura tecnica:

- Existe uma segunda camada latest-state-wins na saida USB.
- O firmware envia quando ha evento pendente e o modo esta pronto.
- Cada modo USB converte `input_event_t` para seu report nativo.

## Scheduling do JoypadOS

Arquivos:

- `!joypadOS/src/main.c`
- `!joypadOS/src/apps/bt2usb/app.c`
- `!joypadOS/src/usb/usbd/usbd.c`

O `main.c` chama tarefas de input, output e `app_task()` em loop.
No app `bt2usb`, `app_task()` chama `bt_task()`.
No USB, existem chamadas a `tud_task()` e, em alguns caminhos, `tud_task_ext(1, false)`.

Ponto importante:

- Nao foi identificado no hot path um `sleep_ms(1)` equivalente ao do OGX Pico W.
- O JoypadOS mantem BT e USB com polling mais agressivo.
- O design reduz tempo parado entre chegada de report BT e oportunidade de envio USB.

## Saida USB por modo

### PS4 / Razer Panthera

Arquivos:

- `!joypadOS/src/usb/usbd/modes/ps4_mode.c`
- `!joypadOS/src/usb/usbd/descriptors/ps4_descriptors.h`

Caracteristicas:

- VID/PID Razer Panthera.
- Report input de 64 bytes.
- Endpoint IN interrupt 64 bytes.
- `bInterval = 1 ms`.
- `ps4_mode_is_ready()` usa `tud_hid_ready()`.
- `ps4_mode_send_report()` envia via `tud_hid_report(...)`.

Resultado observado:

- 778.21 Hz medio.
- intervalo medio 1.28 ms.

Leitura:

- Este e um modo USB rapido no Windows.
- O descriptor de 1 ms e a politica de envio por evento pendente explicam a taxa alta.

### Switch / Pokken

Arquivos:

- `!joypadOS/src/usb/usbd/modes/switch_mode.c`
- `!joypadOS/src/usb/usbd/descriptors/switch_descriptors.h`

Caracteristicas:

- VID/PID HORI Pokken Controller.
- Report input pequeno.
- Endpoint interrupt com `bInterval = 1 ms`.
- `switch_mode_is_ready()` usa `tud_hid_ready()`.
- Envio via `tud_hid_report(...)`.

Resultado observado:

- 775.19 Hz medio.
- intervalo medio 1.29 ms.

Leitura:

- Tambem e um modo USB rapido no Windows.
- Report menor pode ajudar, mas o fator principal parece ser modo/driver/endpoint de 1 ms.

### PS3 / DInput

Arquivos:

- `!joypadOS/src/usb/usbd/modes/ps3_mode.c`
- `!joypadOS/src/usb/usbd/descriptors/ps3_descriptors.h`

Caracteristicas:

- VID/PID Sony DualShock 3.
- Endpoint IN/OUT via HID.
- Descriptor indica intervalo de 1 ms.
- Envio via `tud_hid_report(...)`.

Resultado observado:

- 248.76 Hz medio.
- intervalo medio 4.02 ms.

Leitura:

- Apesar do descriptor permitir 1 ms, no Windows o caminho medido ficou perto de 250 Hz.
- Isso sugere limite do host, driver, API de medicao ou caminho HID/PS3 no Windows.
- Ainda assim e muito superior aos 65-70 ms observados no OGX Sony Bluetooth.

### Xbox 360 / XInput

Arquivos:

- `!joypadOS/src/usb/usbd/modes/xinput_mode.c`
- `!joypadOS/src/usb/usbd/descriptors/xinput_descriptors.h`
- `!joypadOS/src/usb/usbd/drivers/tud_xinput.c`

Caracteristicas:

- VID/PID Microsoft Xbox 360.
- Endpoint IN principal de 32 bytes.
- `xinput_descriptors.h` declara IN principal com intervalo 4 ms.
- `xinput_mode_is_ready()` usa `tud_xinput_ready()`.
- Envio via `tud_xinput_send_report(...)`.

Resultado observado:

- 249.00 Hz medio.
- intervalo medio 4.02 ms.

Leitura:

- Este resultado bate diretamente com endpoint IN de 4 ms.
- Aqui a taxa menor parece esperada pelo descriptor/modo XInput.

## Tabela de modos testados

| Modo | Arquivo descriptor | Identidade USB | Endpoint IN | Resultado |
|---|---|---|---:|---:|
| PS4/Razer | `ps4_descriptors.h` | Razer Panthera | 1 ms | ~778 Hz |
| Switch/Pokken | `switch_descriptors.h` | HORI Pokken | 1 ms | ~775 Hz |
| PS3 | `ps3_descriptors.h` | DualShock 3 | 1 ms declarado | ~249 Hz medido |
| XInput | `xinput_descriptors.h` | Xbox 360 | 4 ms | ~249 Hz |

## Pontos fortes da arquitetura JoypadOS

1. Reconhecimento Sony especifico no CYW43.
2. Evita SDP problematico em DS4/DS5 no CYW43.
3. Abre HID Control/Interrupt de forma direta quando apropriado.
4. Envia SET_REPORT DS4 correto no canal Control para ativar report `0x11`.
5. Usa parser vendor especifico e direto.
6. Usa latest-state-wins sem filas profundas.
7. USB consome eventos pendentes e envia pelo modo ativo.
8. Modos PS4/Razer e Switch/Pokken usam descriptors favoraveis a alta taxa.
9. Loop nao tem descanso fixo de 1 ms no hot path como observado no OGX Pico W.

## O que ainda precisa ser confirmado

Ainda faltam medicoes instrumentadas para separar perfeitamente:

- taxa real de reports Bluetooth recebidos;
- taxa de eventos que chegam ao `router_submit_input()`;
- taxa de updates consumidos por `usbd`;
- taxa real de reports USB aceitos pelo host;
- diferenca entre DS4 e DualSense no mesmo modo USB.

Sem esses contadores, a medicao do Windows prova a taxa final observada, mas nao mostra isoladamente a taxa de cada etapa.

## Comparacao tecnica contra OGX

| Tema | JoypadOS |
|---|---|
| Stack BT | BTstack/CYW43 integrado diretamente |
| Sony no CYW43 | tratamento explicito para pular SDP |
| DS4 enable | SET_REPORT `0x52`, report `0x11`, canal Control |
| Parser DS4 | driver `ds4_bt.c`, chama `router_submit_input()` |
| Estado interno | latest-state-wins no router |
| USB pending | `pending_events` + `pending_flags` |
| Loop | tarefas em loop sem `sleep_ms(1)` critico identificado |
| PS4 USB | Razer Panthera, `bInterval=1` |
| Switch USB | Pokken, `bInterval=1` |
| XInput USB | endpoint IN 4 ms, coerente com ~250 Hz |

## Conclusao tecnica

O JoypadOS combina duas coisas:

- caminho Bluetooth Sony mais especifico para CYW43;
- saida USB com modos e descriptors que o Windows trata em alta frequencia.

O ganho acima de 700 Hz nao vem apenas de "Bluetooth rapido" nem apenas de "USB rapido".
Ele vem da cadeia completa:

```text
Sony reconhecido cedo
    -> SDP evitado quando perigoso no CYW43
    -> L2CAP HID estabelecido
    -> DS4 ativado em report 0x11
    -> parser direto
    -> estado recente no router
    -> USB pending event
    -> modo USB com endpoint/driver favoravel
```

Para portar ideias ao OGX-Mini-Plus, o primeiro cuidado e nao copiar uma unica mudanca isolada.
O comportamento do JoypadOS depende do conjunto: conexao Sony, ativacao DS4, scheduler, buffer interno e descriptor USB.
