# tech-ogx.md

Documento tecnico de referencia sobre como o OGX-Mini-Plus trata comunicacao Bluetooth, estado interno e saida USB.

Escopo atual:

- Firmware alvo: `Firmware/RP2040/`
- Placa de interesse: Pico W / Pico 2W, tratadas pelo codigo como `PI_PICOW`
- Foco: caminho Bluetooth DInput/Sony ate USB device
- Estado: investigacao, sem plano de implementacao neste documento
- Build: nao executar build automatico

## Resumo executivo

O OGX-Mini-Plus usa uma arquitetura em dois nucleos no Pico W/Pico 2W:

```text
Bluepad32 / BTstack no core1
        -> Gamepad compartilhado
        -> loop USB no core0
        -> DeviceDriver::process()
        -> TinyUSB device
        -> Windows / console / host USB
```

O ponto central e que o USB nao recebe diretamente todos os reports Bluetooth.
O Bluepad32 transforma cada callback de controle em um `Gamepad::PadIn`.
Esse estado e sobrescrito em um buffer compartilhado por gamepad. O core USB
consome esse estado quando seu loop roda.

Isso torna o pipeline simples, mas cria tres efeitos importantes:

- Se chegarem varios frames Bluetooth antes do USB consumir, so o ultimo estado sobrevive.
- A taxa USB percebida depende do loop do core0, do endpoint estar pronto e do descriptor do modo USB.
- Cada modo USB tem politica propria de envio; alguns reenviam estado sempre que possivel, outros so enviam quando ha `new_pad_in()`.

## Arquivos centrais

Bluetooth / Bluepad32:

- `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`
- `Firmware/RP2040/src/Bluepad32/Bluepad32.h`
- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`

Estado interno:

- `Firmware/RP2040/src/Gamepad/Gamepad.h`

Loop da placa:

- `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`
- `Firmware/RP2040/src/OGXMini/OGXMini.cpp`

USB device:

- `Firmware/RP2040/src/USBDevice/DeviceManager.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceManager.h`
- `Firmware/RP2040/src/USBDevice/tud_callbacks.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/DeviceDriver.h`

Drivers USB relevantes:

- `Firmware/RP2040/src/USBDevice/DeviceDriver/DInput/DInput.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/DS4/DS4.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/PS3/PS3.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/PS4/PS4.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/Switch/Switch.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/XInput.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.cpp`

Descritores USB:

- `Firmware/RP2040/src/Descriptors/DInput.h`
- `Firmware/RP2040/src/Descriptors/DS4.h`
- `Firmware/RP2040/src/Descriptors/PS3.h`
- `Firmware/RP2040/src/Descriptors/PS4.h`
- `Firmware/RP2040/src/Descriptors/SwitchWired.h`
- `Firmware/RP2040/src/Descriptors/XInput.h`
- `Firmware/RP2040/src/tusb_config.h`

## Entrada Bluetooth

No Pico W/Pico 2W, o core1 inicia Bluetooth e executa o Bluepad32:

- `core1_task()` chama `board_api::init_bluetooth()`
- inicializa BLE server, se habilitado
- chama `bluepad32::run_task(_gamepads)`
- `bluepad32::run_task()` termina em `btstack_run_loop_execute()`

Referencia:

- `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`
- `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`

O callback quente e `controller_data_cb()`.
Ele recebe `uni_controller_t`, converte para `Gamepad::PadIn` e chama:

```cpp
gamepad->set_pad_in(gp_in);
```

Esse ponto e o cruzamento entre Bluetooth e USB. O USB nao sabe de DS4, DS5,
Xbox ou Switch nesse nivel. Ele so ve `Gamepad::PadIn`.

## Estado compartilhado Gamepad

`Gamepad` e o buffer interno entre o core Bluetooth e o core USB.

Semantica principal:

- `set_pad_in(pad_in)` copia o estado e marca `new_pad_in_ = true`
- `new_pad_in()` apenas consulta a flag atomica
- `get_pad_in()` copia o estado e limpa `new_pad_in_ = false`
- o estado e protegido por mutex de Pico SDK

Consequencia:

```text
BT frame 1 -> set_pad_in(A)
BT frame 2 -> set_pad_in(B)
BT frame 3 -> set_pad_in(C)
USB loop   -> get_pad_in() retorna C
```

O desenho e latest-state-wins. Isso e bom para reduzir fila e manter o estado
mais atual, mas nao preserva todos os frames Bluetooth recebidos. Para medicao
de taxa, isso pode esconder a frequencia real do Bluetooth se o USB consumir
mais devagar.

## Loop USB no Pico W/Pico 2W

O loop critico fica em `pico_w::run()`:

```cpp
while (true) {
    TaskQueue::Core0::process_tasks();

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
        device_driver->process(i, _gamepads[i]);
        tud_task();
    }
    sleep_ms(1);
}
```

Caracteristicas:

- USB roda no core0.
- Bluepad32 roda no core1.
- `tud_task()` e chamado dentro do loop por gamepad.
- existe `sleep_ms(1)` fixo ao final de cada iteracao.
- `MAX_GAMEPADS` normalmente e 1 para este caso, mas o loop suporta mais.

Leitura tecnica:

- O `sleep_ms(1)` cria uma cadencia artificial no core USB.
- Mesmo com endpoint `bInterval=1`, o firmware nao tenta processar TinyUSB em loop sem descanso.
- Isso contrasta com o JoypadOS, que usa loop mais agressivo para BT/USB.

## DeviceManager e callbacks TinyUSB

`DeviceManager` apenas instancia o driver USB escolhido:

```text
DINPUT -> DInputDevice
PS3    -> PS3Device
DS4    -> DS4Device
PS4    -> PS4Device
SWITCH -> SwitchDevice
XINPUT -> XInputDevice
```

Depois disso, `tud_callbacks.cpp` delega os callbacks TinyUSB para o driver ativo:

- `usbd_app_driver_get_cb()`
- `tud_hid_get_report_cb()`
- `tud_hid_set_report_cb()`
- `tud_vendor_control_xfer_cb()`
- descriptors device/config/report/string

Ou seja, a politica de envio de input nao esta no `DeviceManager`.
Ela esta nos metodos `process()` de cada `DeviceDriver`.

## Politica de envio por modo USB

### DInput

Arquivo:

- `Firmware/RP2040/src/USBDevice/DeviceDriver/DInput/DInput.cpp`

Comportamento:

- Se `gamepad.new_pad_in()` for verdadeiro, consome `get_pad_in()` e atualiza `in_report`.
- Depois, independentemente de ter havido input novo, se `tud_hid_n_ready(idx)` estiver pronto, chama `tud_hid_n_report(...)`.

Resumo:

```text
input novo atualiza report
endpoint pronto envia ultimo report
```

Isso permite reenviar estado em alta frequencia, limitado por endpoint, `tud_task()` e loop.

### PS3

Arquivo:

- `Firmware/RP2040/src/USBDevice/DeviceDriver/PS3/PS3.cpp`

Comportamento:

- Atualiza `report_in_` apenas quando `new_pad_in()` e verdadeiro.
- Reenvia `report_in_` sempre que `tud_hid_ready()` permite.
- Comentario local indica que PS3 pode usar dado velho se nao receber report todo frame.

Resumo:

```text
input novo atualiza report
endpoint pronto envia ultimo report
```

### DS4

Arquivo:

- `Firmware/RP2040/src/USBDevice/DeviceDriver/DS4/DS4.cpp`

Comportamento:

- Atualiza `report_in_` apenas quando `new_pad_in()` e verdadeiro.
- Reenvia `report_in_` sempre que `tud_hid_ready()` permite.
- Usa report de 64 bytes com `report_id = 0x01`.
- Trata output report para rumble/LED.

Resumo:

```text
input novo atualiza report
endpoint pronto envia ultimo report
```

Limitacao importante:

- Descriptor DS4 do OGX anuncia `bInterval = 5 ms`.
- Isso cria teto teorico em torno de 200 Hz no host USB, antes de qualquer gargalo Bluetooth.

### PS4

Arquivo:

- `Firmware/RP2040/src/USBDevice/DeviceDriver/PS4/PS4.cpp`

Comportamento:

- Similar ao DS4.
- Atualiza com `new_pad_in()`.
- Envia quando `tud_hid_ready()` permite.
- Descriptor PS4 tambem usa `bInterval = 5 ms`.

### Switch / Pokken

Arquivo:

- `Firmware/RP2040/src/USBDevice/DeviceDriver/Switch/Switch.cpp`

Comportamento:

- Atualiza report quando `new_pad_in()`.
- Reenvia ultimo report quando `tud_hid_n_ready(idx)`.
- Report pequeno, 8 bytes.
- Descriptor usa HID com intervalo 1 ms.

### XInput

Arquivo:

- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/XInput.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.cpp`

Comportamento:

- So entra no bloco de envio se `gamepad.new_pad_in()` for verdadeiro.
- Dentro desse bloco, atualiza `in_report_` e chama `tud_xinput::send_report(...)`.
- Se nao houver `new_pad_in()`, nao reenvia o ultimo estado.

Resumo:

```text
input novo atualiza report
input novo tenta enviar
sem input novo nao reenvia
```

Isso acopla a taxa USB a frequencia com que o Bluepad32 entrega e marca input novo.

## Descritores USB e teto teorico

Os intervalos declarados importam porque o host USB usa `bInterval` para agendar endpoints interrupt.

| Modo OGX | Arquivo | Endpoint IN | Report | Observacao |
|---|---|---:|---:|---|
| DInput | `Descriptors/DInput.h` | 1 ms | 19 bytes | usa `TUD_HID_DESCRIPTOR` |
| PS3 | `Descriptors/PS3.h` | 1 ms | 49 bytes | IN/OUT 64 bytes |
| Switch/Pokken | `Descriptors/SwitchWired.h` | 1 ms | 8 bytes | IN 64 via CFG |
| DS4 | `Descriptors/DS4.h` | 5 ms | 64 bytes | teto USB ~200 Hz |
| PS4 | `Descriptors/PS4.h` | 5 ms | 64 bytes | teto USB ~200 Hz |
| XInput | `Descriptors/XInput.h` | 1 ms | 20 bytes | OUT 8 ms |

`CFG_TUD_HID_EP_BUFSIZE` esta em 64 bytes em `tusb_config.h`.

Conclusao:

- DS4/PS4 no OGX ja partem de um descriptor mais lento que JoypadOS PS4/Razer.
- DInput, PS3 e Switch nao tem esse limite de 5 ms no descriptor.
- O padrao observado de 65-70 ms nao e explicado pelo USB descriptor sozinho.

## Sony Bluetooth no Bluepad32

Arquivos principais:

- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`

Existem opcoes experimentais no `CMakeLists.txt`:

- `OGXM_BLUEPAD32_SONY_FAST_INPUT`
- `OGXM_BLUEPAD32_DS4_CONTROL_ENABLE`
- `OGXM_BLUEPAD32_DS4_DELAY_INITIAL_ENABLE`
- `OGXM_BLUEPAD32_DS4_SKIP_INIT_CALIBRATION`
- `OGXM_BLUEPAD32_DS4_SKIP_SDP`
- `OGXM_BLUEPAD32_SONY_L2CAP_AFTER_ENCRYPTION`
- `OGXM_BLUEPAD32_DISABLE_SNIFF_FOR_BT_PERF_TEST`
- `OGXM_BLUEPAD32_FORCE_ACTIVE_LINK_POLICY`

Leitura tecnica:

- Algumas dessas opcoes tentam aproximar o Bluepad32 do comportamento observado no JoypadOS.
- `SONY_L2CAP_AFTER_ENCRYPTION` ja foi observado causando regressao de sync Sony quando ligado.
- Portanto estas opcoes devem ser tratadas como investigacao, nao como solucao final.

## Comparacao interna: onde a ineficiencia pode nascer

### Cenario A: gargalo no USB puro

Sinais:

- `tud_task()` pouco frequente.
- `sleep_ms(1)` fixo no loop do core0.
- descriptor com `bInterval` alto.
- driver so envia sob `new_pad_in()`.

Modos mais afetados:

- DS4/PS4 pelo `bInterval=5`.
- XInput por enviar apenas em `new_pad_in()`.

### Cenario B: gargalo antes do USB

Sinais:

- DInput tem `bInterval=1`, mas Sony Bluetooth cai para 65-70 ms.
- Mesmo quando USB poderia enviar mais, `Gamepad::PadIn` so recebe atualizacao lenta.
- Esse caso aponta para Bluepad32, L2CAP, modo Sony, parser DS4/DS5, sniff ou estado de conexao.

### Cenario C: perda por condensacao de estado

Sinais:

- Bluetooth pode receber reports em taxa maior que USB consome.
- `Gamepad` guarda so o ultimo estado.
- O polling tester pode medir mudancas aplicadas ao USB, nao todos os reports BT recebidos.

Isso nao e necessariamente ruim para jogabilidade, mas atrapalha diagnostico se nao houver contadores separados.

## Diferencas relevantes contra JoypadOS

| Tema | OGX-Mini-Plus |
|---|---|
| Stack BT | Bluepad32 sobre BTstack |
| Parser Sony | Bluepad32 DS4/DS5 |
| Estado interno | `Gamepad::PadIn` compartilhado |
| Politica interna | latest-state-wins por gamepad |
| USB loop | `process()` + `tud_task()` + `sleep_ms(1)` |
| DS4 USB | descriptor Sony DS4 com `bInterval=5` |
| PS4 USB | descriptor generico PS4 com `bInterval=5` |
| Switch USB | Pokken/HORI com intervalo 1 ms |
| XInput USB | endpoint IN 1 ms, mas envio so em input novo |
| Risco atual | alteracoes experimentais no Bluepad32 podem afetar sync |

## Hipoteses para investigacao futura

Estas hipoteses ainda precisam ser confirmadas com instrumentacao:

1. O gargalo de 65-70 ms vem do caminho Sony Bluetooth no Bluepad32, nao do USB device.
2. O loop USB com `sleep_ms(1)` impede aproximar o comportamento do JoypadOS em modos de 1 ms.
3. DS4/PS4 no OGX nao podem atingir >700 Hz enquanto `bInterval=5` permanecer.
4. XInput precisa separar "report USB pronto" de "input novo" para nao depender exclusivamente do callback Bluetooth.
5. A politica latest-state-wins deve ser mantida, mas acompanhada de contadores de entrada/saida para medir perda entre BT e USB.

## Pontos de verificacao recomendados

Sem alterar comportamento, os contadores mais uteis seriam:

- callbacks Bluetooth por segundo em `controller_data_cb()`
- chamadas `set_pad_in()` por segundo
- chamadas `DeviceDriver::process()` por segundo
- `new_pad_in()` consumidos por segundo
- endpoint ready por segundo
- reports USB enviados por segundo
- falhas de envio por endpoint ocupado

Ja existe estrutura experimental de debug em `DInput.cpp` e `Bluepad32.cpp` sob `OGXM_BT_RATE_DEBUG`, mas ela deve ser tratada como parte das alteracoes experimentais atuais.

## Conclusao tecnica

O OGX-Mini-Plus nao tem apenas um gargalo. Ele combina:

- stack Bluetooth generica via Bluepad32;
- estado compartilhado latest-state-wins;
- loop USB com `sleep_ms(1)`;
- descritores com intervalos diferentes por modo;
- politicas de envio diferentes por driver.

O resultado e que a taxa final observada pelo Windows pode ser limitada em qualquer uma destas etapas.
Para DS4/DualSense Bluetooth especificamente, o comportamento de 65-70 ms e forte demais para ser explicado so por USB. O USB do OGX tem ineficiencias claras, mas o gargalo principal Sony provavelmente esta antes da camada USB ou na forma como o USB consome os updates vindos do Bluepad32.
