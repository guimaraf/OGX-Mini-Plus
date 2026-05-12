# tech-implementation.md

Documento tecnico de proposta incremental para melhorar a comunicacao Bluetooth
do OGX-Mini-Plus com controles Sony/DInput, usando o JoypadOS como referencia
tecnica.

Escopo:

- Branch de trabalho: `mix`
- Objetivo primario: sair do comportamento de 15-21 Hz com DS4/DualSense Bluetooth em DInput.
- Objetivo minimo aceitavel: atingir patamar estavel proximo ao pior caso do JoypadOS, cerca de 250 Hz.
- Objetivo ideal: permitir que modos USB favoraveis cheguem acima de 700 Hz quando o host/driver permitir.
- Nao executar build automaticamente neste documento.
- Nao implementar tudo de uma vez.

Status de implementacao:

- 2026-05-11: iniciado Patch 1/controle incremental na branch `mix`.
- As otimizacoes Sony/Bluepad32 experimentais devem ficar desligadas por padrao e ser ativadas uma por vez nos testes.
- Adicionada a proposta de flag `OGXM_USB_TIGHT_POLL` para remover o `sleep_ms(1)` do loop principal somente quando o teste pedir.
- 2026-05-11: iniciado Patch 2 com `OGXM_BLUEPAD32_DS4_CONTROL_ENABLE=ON` por padrao para testar a ativacao DS4 via SET_REPORT no canal Control.
- 2026-05-11: apos teste parcial, o DS4 saiu de ~66 ms para pacotes de 2-5 ms depois de alguns segundos; `OGXM_BLUEPAD32_DS4_DELAY_INITIAL_ENABLE=ON` foi ativado por padrao para testar se o modo rapido entra de forma mais deterministica no inicio.
- 2026-05-11: teste com `OGXM_BLUEPAD32_DS4_DELAY_INITIAL_ENABLE=ON` piorou o DS4; a opcao voltou para `OFF`. Manter apenas `OGXM_BLUEPAD32_DS4_CONTROL_ENABLE=ON` como estado base atual.
- 2026-05-11: novo teste com delay `OFF` continuou majoritariamente em ~66 ms; `OGXM_BLUEPAD32_SONY_FAST_INPUT=ON` foi ativado por padrao para testar o impacto do parser/hot path Sony sem mexer em SDP, sniff ou USB.

## Resumo executivo

O problema nao deve ser tratado como "mudar o USB" ou "mudar o Bluepad32" de forma isolada.
O JoypadOS mostra que a Pi Pico 2W e o DS4 conseguem entregar taxas altas, mas isso acontece porque a cadeia completa foi bem resolvida:

```text
Sony reconhecido cedo
    -> SDP evitado no CYW43 quando perigoso
    -> L2CAP HID Control/Interrupt aberto corretamente
    -> DS4 ativado em report rapido 0x11
    -> parser Sony leve
    -> estado interno latest-state-wins
    -> USB consome evento rapidamente
    -> modo USB com descriptor favoravel
```

No OGX-Mini-Plus atual, o DInput USB ja anuncia intervalo de 1 ms. Logo, se DS4/DualSense Bluetooth em DInput cai para 15-21 Hz, o gargalo inicial mais provavel esta antes do USB: caminho Sony no Bluepad32, ativacao do report rapido, link policy/sniff, SDP/L2CAP ou cadencia de callbacks ate `Gamepad::set_pad_in()`.

O plano abaixo separa as mudancas em etapas para descobrir onde esta o gargalo real.

## Estado atual da branch mix

O working tree ja contem alteracoes experimentais anteriores.

Alteracoes relevantes ja presentes:

- `Firmware/RP2040/CMakeLists.txt`
  - opcoes de debug por taxa;
  - opcoes Sony/Bluepad32;
  - opcao para desligar BLE server durante testes.
- `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`
  - contadores `BT_RATE` quando `OGXM_BT_RATE_DEBUG` esta habilitado.
- `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`
  - BLE server pode ser desligado por macro.
- `Firmware/RP2040/src/USBDevice/DeviceDriver/DInput/DInput.cpp`
  - contadores `USB_RATE` para DInput.
- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
  - skip SDP para Sony;
  - desativacao de sniff;
  - opcao de forcar active link policy;
  - tentativa de adiar L2CAP para depois de encryption.
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
  - DS4 SET_REPORT via control channel;
  - delay/retry de enable report;
  - fast input sem motion/touch;
  - controle opcional da request de calibration.
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`
  - fast input;
  - retries de enable lightbar/report.
- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt.c`
  - encaminha `HCI_EVENT_ENCRYPTION_CHANGE` para BR/EDR.

Importante:

- Essas alteracoes nao devem ser assumidas como solucao final.
- O historico ja mostrou que `OGXM_BLUEPAD32_SONY_L2CAP_AFTER_ENCRYPTION` pode quebrar sync Sony.
- O plano deve transformar essas ideias em testes isolados.

## Principios da implementacao

1. Medir antes de otimizar.
2. Separar Bluetooth Sony de USB device.
3. Nao ligar varias flags novas no mesmo teste.
4. Manter Xbox Series como controle de regressao.
5. Validar DS4 e DualSense separadamente.
6. DInput e o primeiro alvo, porque ja usa endpoint 1 ms no OGX.
7. PS4/DS4 USB descriptor de 5 ms so deve ser alterado depois que o Bluetooth estiver bom.
8. Mudancas no submodulo Bluepad32 devem ser pequenas, comentadas e reversiveis.

## Metricas obrigatorias por teste

Para cada etapa, registrar:

- controle usado: DS4, DualSense, Xbox Series;
- modo USB: DInput, PS3, Switch, XInput, DS4/PS4 se aplicavel;
- taxa media no Gamepad Polling Rate;
- intervalo minimo, medio e maximo;
- jitter;
- outliers;
- se pareou/sincronizou;
- se reconectou apos power cycle;
- se rumble/LED ainda funciona;
- se Xbox Series ainda sincroniza.

Quando `OGXM_BT_RATE_DEBUG` estiver ativo, registrar tambem:

- `BT_RATE cb`;
- `BT_RATE gp`;
- `BT_RATE set`;
- `USB_RATE process`;
- `USB_RATE new`;
- `USB_RATE ready`;
- `USB_RATE sent`;
- `USB_RATE fail`.

Interpretacao rapida:

| Sintoma | Leitura |
|---|---|
| `BT_RATE set` perto de 15-21/s | gargalo ainda no Bluetooth/parser Sony |
| `BT_RATE set` alto, `USB_RATE new` baixo | consumo/flag `Gamepad` ou loop USB perdendo updates |
| `USB_RATE ready` alto, `sent` baixo | problema de envio TinyUSB ou endpoint ocupado |
| `USB_RATE sent` alto, Windows mede baixo | driver/API/descriptor do modo USB |
| Xbox normal, Sony ruim | problema especifico Sony |
| Xbox tambem piora | regressao global BT/USB |

## Etapa 0 - Baseline limpo e matriz de controle

Objetivo:

Estabelecer a linha de base na branch `mix` antes de novas alteracoes funcionais.

Arquivos a alterar:

- nenhum, se as flags atuais ja permitirem build de teste;
- caso precise ampliar logs, isso fica para a Etapa 1.

Configuracao sugerida para o primeiro baseline:

- `OGXM_ENABLE_DEBUG_LOG=ON`
- `OGXM_ENABLE_BT_RATE_DEBUG=ON`
- `OGXM_DISABLE_BLE_SERVER_FOR_BT_PERF_TEST=ON`
- manter `OGXM_BLUEPAD32_SONY_L2CAP_AFTER_ENCRYPTION=OFF`

Testes:

1. DS4 Bluetooth em DInput.
2. DualSense Bluetooth em DInput.
3. Xbox Series Bluetooth em DInput ou XInput.
4. DS4 Bluetooth em Switch/Pokken, se modo disponivel.
5. DS4 Bluetooth em XInput, apenas como comparacao.

Criterio de sucesso da etapa:

- Obter logs suficientes para dizer se o gargalo de 15-21 Hz esta antes ou depois de `gamepad->set_pad_in()`.

Criterio de falha:

- Sem logs uteis.
- Sony nao sincroniza.
- Xbox regride.

## Etapa 1 - Instrumentacao completa sem mudar comportamento

Objetivo:

Adicionar visibilidade em todos os pontos da cadeia antes de mexer em comportamento.

Arquivos provaveis:

- `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`
- `Firmware/RP2040/src/Gamepad/Gamepad.h`
- `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/DInput/DInput.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/Switch/Switch.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/PS3/PS3.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/XInput.cpp`
- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`

Mudancas propostas:

1. Expandir `BT_RATE` para incluir:
   - tipo do controle;
   - report id recebido, quando disponivel;
   - tamanho do report;
   - intervalo medio entre callbacks;
   - maior intervalo observado.

2. Expandir `USB_RATE` para todos os modos usados em teste:
   - DInput;
   - Switch;
   - PS3;
   - XInput;
   - DS4/PS4 apenas se forem testados.

3. Adicionar contador leve no `Gamepad`:
   - numero de `set_pad_in()`;
   - numero de `get_pad_in()`;
   - numero de sobrescritas enquanto `new_pad_in_` ainda estava true.

4. Adicionar log de estado Sony no Bluepad32:
   - remote name;
   - VID/PID inferido;
   - se SDP foi pulado;
   - quando Control CID abriu;
   - quando Interrupt CID abriu;
   - quando `uni_hid_device_set_ready()` ocorreu;
   - quando report 0x11/0x31 comecou a chegar.

Cuidados:

- Logs devem ser condicionais por macro.
- Nao deixar logging pesado sempre ligado.
- Evitar `printf` em cada report; agregar por janela de 1 segundo.

Criterio de sucesso:

- Saber em uma execucao se DS4 esta entregando 15-21 reports/s ou se entrega mais e o USB perde.

## Etapa 2 - Estabilizar o caminho Sony sem alterar USB

Objetivo:

Fazer DS4/DualSense entregar callbacks em taxa alta ate `set_pad_in()`.
Nesta etapa nao se mexe em descriptor USB nem no loop USB.

Arquivos provaveis:

- `Firmware/RP2040/CMakeLists.txt`
- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`
- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt.c`
- `Firmware/external/bluepad32/src/components/bluepad32/include/bt/uni_bt_bredr.h`

### 2A - Fast parser Sony

Status:

- iniciado em 2026-05-11;
- `OGXM_BLUEPAD32_SONY_FAST_INPUT` passou a ficar `ON` por padrao;
- estado combinado atual: `DS4_CONTROL=ON`, `DS4_DELAY_INITIAL=OFF`, `SONY_FAST_INPUT=ON`.

Manter ou isolar:

- `OGXM_BLUEPAD32_SONY_FAST_INPUT=ON`

Justificativa:

- JoypadOS processa o hot path DS4 focado no que interessa para controle.
- Motion/touch/bateria nao devem bloquear ou encarecer o caminho de input basico.

Teste:

- DS4 DInput.
- DualSense DInput.
- Verificar se `BT_RATE set` sobe.

Risco:

- Perda de touch/motion no output do Bluepad32, aceitavel para teste de taxa.

### 2B - DS4 SET_REPORT no canal Control

Manter ou ajustar:

- `OGXM_BLUEPAD32_DS4_CONTROL_ENABLE=ON`
- report `0x52`, id `0x11`, flags `0x80 0x00 0xFF`

Justificativa:

- JoypadOS ativa DS4 com SET_REPORT Output no Control channel.
- O envio por interrupt com `0xA2` pode nao ativar corretamente o report enhanced em alguns DS4.

Teste:

- Confirmar nos logs quando report `0x11` comeca a chegar.
- Se report `0x01` continuar predominando, o enable nao funcionou.

Risco:

- Enviar cedo demais pode falhar se Control channel ainda nao estiver pronto.

### 2C - Timing do enable DS4

Status:

- iniciado em 2026-05-11;
- `OGXM_BLUEPAD32_DS4_DELAY_INITIAL_ENABLE` foi testado em `ON` e voltou para `OFF`;
- resultado observado: piora da media e aumento da predominancia de pacotes em ~66 ms;
- decisao: nao manter delay/retry inicial no estado base atual.

Testar em separado:

- `OGXM_BLUEPAD32_DS4_DELAY_INITIAL_ENABLE=ON`
- delay inicial de 100 ms;
- ate 4 tentativas com intervalo de 150 ms.

Justificativa:

- JoypadOS usa atraso inicial antes do primeiro SET_REPORT.
- Repetir algumas vezes pode cobrir DS4 que ignora o primeiro comando.

Teste A:

- delay ON, calibration ON.

Teste B:

- delay OFF, calibration ON.

Teste C:

- delay ON, calibration OFF.

Criterio:

- Escolher a combinacao que sincroniza e mantem maior `BT_RATE set`.

### 2D - Calibration GET_REPORT do DS4

Tratar como variavel de teste:

- `OGXM_BLUEPAD32_DS4_SKIP_INIT_CALIBRATION=OFF` inicialmente.

Justificativa:

- No Bluepad32 original, a calibration request tambem pode ajudar a iniciar report `0x11`.
- No JoypadOS, o SET_REPORT control parece ser o caminho principal observado.

Plano:

1. Primeiro manter calibration ON.
2. So testar skip calibration depois que SET_REPORT control estiver comprovado.

Risco:

- Pular calibration pode reduzir inicializacao de alguns modelos DS4.

## Etapa 3 - Politica L2CAP/SDP Sony no Bluepad32

Objetivo:

Eliminar SDP problematico para Sony no CYW43 sem quebrar sincronizacao.

Arquivos provaveis:

- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt.c`
- `Firmware/external/bluepad32/src/components/bluepad32/include/bt/uni_bt_bredr.h`
- `Firmware/RP2040/CMakeLists.txt`

### 3A - Skip SDP por nome Sony

Opcao:

- `OGXM_BLUEPAD32_DS4_SKIP_SDP=ON`

Comportamento atual da branch:

- reconhece `"Wireless Controller"` e `"DualSense"`;
- seta VID Sony `0x054c`;
- seta PID `0x09cc` para DS4 ou `0x0ce6` para DualSense;
- marca `SDP_QUERY_NOT_NEEDED`;
- move estado para `SDP_HID_DESCRIPTOR_FETCHED`.

Justificativa:

- JoypadOS evita SDP Sony no CYW43 por risco de travar o barramento SPI.
- Para taxa e estabilidade, evitar SDP tambem reduz uma fase lenta e fragil.

Teste:

- DS4 pareando do zero.
- DS4 reconectando depois de desligar controle.
- DualSense pareando do zero.
- DualSense reconectando.

Criterio de sucesso:

- Sony sincroniza consistentemente e `BT_RATE set` melhora.

### 3B - Nao usar L2CAP after encryption por padrao

Opcao:

- manter `OGXM_BLUEPAD32_SONY_L2CAP_AFTER_ENCRYPTION=OFF`

Justificativa:

- O historico do projeto ja mostrou que ligar esta opcao restaurou ou quebrou sync dependendo do experimento, e a observacao mais recente diz que desligar restaurou sync Sony.
- O JoypadOS cria L2CAP depois de encryption em certos caminhos, mas a maquina de estados do Bluepad32 nao e identica.

Plano:

- Nao misturar esta opcao com os testes principais.
- So criar uma etapa dedicada caso skip SDP + SET_REPORT control nao resolvam.

Teste dedicado, se necessario:

- ligar apenas `SONY_L2CAP_AFTER_ENCRYPTION`;
- manter todas as demais flags iguais ao melhor teste anterior;
- medir sincronizacao antes de medir taxa.

Criterio de abortar:

- DS4 ou DualSense deixa de sincronizar.

### 3C - Separar DS4 e DualSense

Problema:

- A opcao se chama `DS4_SKIP_SDP`, mas a implementacao atual tambem inclui `DualSense`.

Proposta:

- renomear futuramente para algo como `OGXM_BLUEPAD32_SONY_SKIP_SDP`;
- ou criar flags separadas:
  - `OGXM_BLUEPAD32_DS4_SKIP_SDP`;
  - `OGXM_BLUEPAD32_DS5_SKIP_SDP`.

Justificativa:

- DS4 e DualSense tem ativacao diferente.
- Separar flags facilita identificar regressao especifica.

## Etapa 4 - Link policy, sniff e concorrencia Bluetooth

Objetivo:

Evitar que o link Bluetooth Classic entre em modo de baixa atividade durante teste de latencia.

Arquivos provaveis:

- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
- `Firmware/RP2040/CMakeLists.txt`
- `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`

### 4A - Desligar sniff mode durante testes

Opcao:

- `OGXM_BLUEPAD32_DISABLE_SNIFF_FOR_BT_PERF_TEST=ON`

Comportamento:

- usar `LM_LINK_POLICY_ENABLE_ROLE_SWITCH`;
- nao habilitar `LM_LINK_POLICY_ENABLE_SNIFF_MODE`.

Justificativa:

- Sniff mode pode criar cadencia baixa e blocos longos.
- O sintoma de 65-70 ms e compativel com link dormindo/agrupando trafego.

Teste:

- DS4 DInput segurando stick em movimento continuo.
- DS4 parado e depois movimento rapido.
- DualSense no mesmo fluxo.

Criterio:

- `BT_RATE set` deve subir e os intervalos maximos devem cair.

### 4B - Forcar active link policy apos conexao

Opcao:

- `OGXM_BLUEPAD32_FORCE_ACTIVE_LINK_POLICY=ON` por padrao na etapa atual.
- Foi ligado depois que as etapas USB nao reduziram os pacotes de 60-75 ms.

Comportamento:

- chamar `hci_write_link_policy_settings`;
- chamar `gap_sniff_mode_exit(handle)`.

Risco:

- Pode interferir em alguns controles.
- Deve ser testado com Xbox Series como regressao.

### 4C - Desligar BLE server no teste

Opcao:

- `OGXM_DISABLE_BLE_SERVER_FOR_BT_PERF_TEST=ON`

Justificativa:

- Reduz concorrencia local durante diagnostico.
- Nao deve ser necessariamente uma decisao final de produto.

Teste:

- comparar DS4 DInput com BLE server ON e OFF depois que o caminho Sony estiver estavel.

## Etapa 5 - Loop USB do Pico W/Pico 2W

Objetivo:

Remover gargalo artificial no core0 depois que o Bluetooth Sony estiver entregando updates em taxa boa.

Arquivos provaveis:

- `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`
- possivelmente `Firmware/RP2040/CMakeLists.txt`

Problema atual:

```cpp
while (true) {
    TaskQueue::Core0::process_tasks();
    for (...) {
        device_driver->process(i, _gamepads[i]);
        tud_task();
    }
    sleep_ms(1);
}
```

Proposta incremental:

1. Criar flag de build:
   - `OGXM_USB_TIGHT_POLL`

2. Quando ligada:
   - remover `sleep_ms(1)` do hot path;
   - chamar `tud_task()` antes e depois de `device_driver->process()` ou manter uma ordem medida;
   - chamar `tight_loop_contents()` no final da iteracao, se necessario.

3. Quando desligada:
   - manter comportamento atual.

Justificativa:

- JoypadOS nao tem `sleep_ms(1)` equivalente no hot path.
- Com endpoint 1 ms, dormir 1 ms a cada loop reduz margem de envio e aumenta jitter.

Teste:

- So aplicar depois que `BT_RATE set` ja estiver alto.
- Medir `USB_RATE process`, `ready` e `sent`.
- Confirmar que `TaskQueue::Core0::process_tasks()` continua funcionando.
- Confirmar troca de modo por combo, se aplicavel.

Criterio de sucesso:

- `USB_RATE process` aumenta.
- `USB_RATE sent` aumenta ou jitter cai.
- Nao quebra sincronizacao Bluetooth.

## Etapa 6 - Politica de envio USB por driver

Objetivo:

Padronizar envio USB para que endpoint pronto e input novo sejam conceitos separados.

Arquivos provaveis:

- `Firmware/RP2040/src/USBDevice/DeviceDriver/DInput/DInput.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/Switch/Switch.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/PS3/PS3.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/DS4/DS4.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/PS4/PS4.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/XInput.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.cpp`

Estado atual:

- DInput, PS3, DS4, PS4 e Switch atualizam em `new_pad_in()` mas reenviam quando endpoint esta pronto.
- XInput so envia dentro de `new_pad_in()`.

Proposta:

1. Manter DInput como referencia.
2. Para XInput:
   - atualizar report quando `new_pad_in()`;
   - tentar enviar ultimo report sempre que `tud_xinput::send_report_ready()` estiver true;
   - preservar recebimento de output/rumble.

3. Criar contadores por driver para:
   - input novo consumido;
   - endpoint ready;
   - report enviado;
   - falha de envio.

Justificativa:

- O USB nao deve depender exclusivamente da chegada de input novo.
- Isso ajuda a comparar com JoypadOS, que usa pending event e readiness do modo.

Prioridade:

- Baixa para corrigir DS4/DualSense em DInput, porque DInput ja reenvia.
- Alta para melhorar XInput depois.

## Etapa 7 - Descritores USB de alta taxa

Objetivo:

Depois que o Bluetooth estiver correto, ajustar modos USB que hoje tem teto baixo.

Arquivos provaveis:

- `Firmware/RP2040/src/Descriptors/DS4.h`
- `Firmware/RP2040/src/Descriptors/PS4.h`
- possivelmente `Firmware/RP2040/src/USBDevice/DeviceDriver/DS4/DS4.cpp`
- possivelmente `Firmware/RP2040/src/USBDevice/DeviceDriver/PS4/PS4.cpp`

Estado atual:

- DInput: 1 ms.
- PS3: 1 ms.
- Switch/Pokken: 1 ms.
- XInput: IN 1 ms no OGX, mas XInput no Windows pode se comportar diferente.
- DS4: 5 ms.
- PS4: 5 ms.

Proposta:

1. Nao mexer em DS4/PS4 enquanto DInput Sony estiver em 15-21 Hz.
2. Depois que DInput Sony passar de 200 Hz, testar:
   - DS4 `bInterval` 5 ms -> 1 ms;
   - PS4 `bInterval` 5 ms -> 1 ms.

Justificativa:

- JoypadOS PS4/Razer usa 1 ms e atinge ~778 Hz.
- OGX DS4/PS4 com 5 ms nao tem como atingir esse patamar.

Risco:

- Alguns hosts podem esperar comportamento mais proximo de DS4 real.
- Alterar VID/PID/descriptor pode afetar compatibilidade.

Criterio:

- Se DInput ja estiver bom, esta etapa e otimizacao final, nao correcao primaria.

## Etapa 8 - Se Bluepad32 continuar limitando Sony

Objetivo:

Ter um plano de fallback caso o Bluepad32 nao consiga entregar Sony em alta taxa de forma estavel.

Opcao A - Mini hot path Sony dentro do Bluepad32

Arquivos provaveis:

- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`
- `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`

Ideia:

- manter conexao e lifecycle do Bluepad32;
- reduzir parser Sony ao minimo no hot path;
- evitar virtual mouse/touch/motion em modo performance;
- preservar apenas gamepad basico, triggers e sticks.

Opcao B - Bypass Sony inspirado no JoypadOS

Arquivos provaveis:

- novos arquivos em `Firmware/RP2040/src/Bluepad32/` ou modulo separado;
- alteracoes em `Firmware/external/bluepad32`;
- possivel integracao direta com BTstack/L2CAP.

Ideia:

- para DS4/DS5 no CYW43, criar caminho especifico de L2CAP HID;
- enviar SET_REPORT correto;
- parsear report 0x11/0x31 diretamente para `Gamepad::PadIn`;
- deixar Bluepad32 para outros controles.

Risco:

- Muito maior.
- Pode conflitar com ownership de conexoes dentro do Bluepad32.
- So deve ser considerado se etapas 2-4 falharem.

Recomendacao:

- Nao iniciar por aqui.
- Primeiro provar com contadores que o gargalo esta dentro do Bluepad32 e nao no USB.

## Ordem recomendada dos patches

### Patch 1 - Observabilidade

Status:

- iniciado em 2026-05-11;
- a instrumentacao de taxa Bluetooth/USB ja estava parcialmente presente na branch;
- o primeiro ajuste aplicado foi transformar as otimizacoes experimentais em flags desligadas por padrao e adicionar `OGXM_USB_TIGHT_POLL` para testes controlados.

Arquivos:

- `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`
- `Firmware/RP2040/src/Gamepad/Gamepad.h`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/DInput/DInput.cpp`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`
- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`

Resultado esperado:

- Logs de 1 segundo mostrando onde a taxa cai.

### Patch 2 - Sony activation DS4

Status:

- iniciado em 2026-05-11;
- `OGXM_BLUEPAD32_DS4_CONTROL_ENABLE` passou a ficar `ON` por padrao;
- as demais opcoes Sony continuam desligadas para isolar o efeito do SET_REPORT Control.

Arquivos:

- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/RP2040/CMakeLists.txt`

Resultado esperado:

- DS4 passa a reportar `0x11` consistentemente.

### Patch 3 - Sony fast parser DS4/DS5

Arquivos:

- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`
- `Firmware/RP2040/CMakeLists.txt`

Resultado esperado:

- Reduz custo por report.
- Nao deve alterar pareamento.

### Patch 4 - Sony skip SDP no CYW43

Arquivos:

- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
- `Firmware/RP2040/CMakeLists.txt`

Resultado esperado:

- Pareamento/reconexao Sony mais estavel.
- Sem travas ligadas a SDP.

### Patch 5 - Link policy performance

Arquivos:

- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
- `Firmware/RP2040/CMakeLists.txt`

Resultado esperado:

- Reducao de blocos longos e queda de jitter.

### Patch 6 - Loop USB tight poll

Arquivos:

- `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`
- `Firmware/RP2040/CMakeLists.txt`

Resultado esperado:

- Maior frequencia de `process()` e `tud_task()`.
- Melhor aproveitamento de endpoints 1 ms.

### Patch 7 - XInput resend policy

Arquivos:

- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/XInput.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.h`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.cpp`

Resultado esperado:

- XInput deixa de depender apenas de `new_pad_in()`.

### Patch 8 - Descritores DS4/PS4 1 ms

Arquivos:

- `Firmware/RP2040/src/Descriptors/DS4.h`
- `Firmware/RP2040/src/Descriptors/PS4.h`

Resultado esperado:

- DS4/PS4 USB deixam de ter teto de 5 ms.

## Matriz de validacao por etapa

| Etapa | DS4 DInput | DualSense DInput | Xbox Series | Objetivo |
|---|---:|---:|---:|---|
| Baseline | medir | medir | medir | estado inicial |
| Observabilidade | medir + logs | medir + logs | medir + logs | localizar gargalo |
| DS4 activation | subir BT_RATE | sem regressao | sem regressao | report 0x11 |
| Fast parser | subir/estabilizar | subir/estabilizar | sem regressao | custo menor |
| Skip SDP | sync estavel | sync estavel | sem regressao | conexao Sony |
| Link policy | menos blocos | menos blocos | sem regressao | evitar sniff |
| USB tight poll | subir USB_RATE | subir USB_RATE | sem regressao | menor latencia USB |
| Descritor 1 ms | nao foco DInput | nao foco DInput | sem regressao | modos DS4/PS4 |

## Criterios de parada

Parar e nao empilhar novas mudancas se:

- DS4 para de sincronizar.
- DualSense para de sincronizar.
- Xbox Series regride.
- `BT_RATE set` cai apos uma mudanca.
- `USB_RATE sent` cai apos uma mudanca USB.
- ha hard fault, travamento ou perda de reconnect.

Quando isso acontecer:

1. Reverter apenas a ultima etapa.
2. Manter logs do teste ruim.
3. Comparar com o teste anterior.
4. So entao tentar a variacao seguinte.

## Resultado esperado final

Meta minima:

- DS4 Bluetooth em DInput acima de 200 Hz, estavel.
- DualSense Bluetooth em DInput acima de 200 Hz, estavel.
- Xbox Series sem regressao.
- Reconexao Sony funcionando apos power cycle.

Meta ideal:

- DS4/DualSense em modos USB de 1 ms com taxa percebida acima de 700 Hz quando o host permitir.
- DInput sem blocos sustentados de 65-70 ms.
- USB e Bluetooth instrumentados o bastante para diagnosticos futuros.

## Recomendacao principal

A primeira mudanca funcional nao deve ser descriptor USB.

O primeiro alvo real deve ser:

```text
Sony Bluetooth no Bluepad32
    -> ativacao correta do report rapido
    -> evitar SDP problematico
    -> impedir sniff/baixa atividade
    -> confirmar callbacks por segundo antes de chegar ao USB
```

Somente depois disso faz sentido otimizar:

```text
loop USB do Pico W
    -> politica de envio dos drivers
    -> descriptor DS4/PS4 de 1 ms
```

Essa ordem evita confundir sintomas. Se o Bluetooth ainda entrega so 15-21 updates por segundo, nenhum descriptor USB de 1 ms vai tornar DInput jogavel.

## Estado aplicado - DS4 direct input

Alteracao aplicada para o proximo teste no modo Pokken:

- `OGXM_BLUEPAD32_DS4_DIRECT_INPUT=ON` por padrao no CMake.
- Em `uni_bt_bredr.c`, quando o pacote Bluetooth Classic e de um DS4, o firmware agora:
  - executa o parser bruto do DS4;
  - chama diretamente o callback `on_controller_data`;
  - pula `uni_hid_device_process_controller()`.
- Em `uni_hid_parser_ds4.c`, quando `OGXM_BLUEPAD32_DS4_DIRECT_INPUT=ON`, o DS4 nao cria mais o dispositivo virtual de mouse/touchpad.

Objetivo tecnico:

- remover custo por pacote que nao existe no caminho essencial de input de jogo;
- evitar processamento de touchpad/mouse virtual;
- testar se os blocos de 65-70 ms estao ligados ao pipeline generico do Bluepad32 depois do parser DS4.

Risco esperado:

- algum mapeamento de botao pode mudar, porque `uni_gamepad_remap()` fica fora do caminho do DS4 nesse teste;
- funcoes especiais do botao PS/Home podem nao passar pelo mesmo tratamento interno do Bluepad32;
- o objetivo deste teste e medir cadencia, nao finalizar mapeamento.

Validacao esperada:

- usar DS4 em modo USB Pokken;
- comparar a proporcao de pacotes abaixo de 5 ms e abaixo de 10 ms contra o teste anterior;
- observar se a proporcao de pacotes entre 60-75 ms cai de forma clara;
- se houver regressao forte de botoes, sync ou enumeracao USB, desligar com `-DOGXM_BLUEPAD32_DS4_DIRECT_INPUT=OFF`.

## Estado aplicado - Pokken send-on-change

Alteracao aplicada para o proximo teste no modo Pokken:

- `OGXM_SWITCH_SEND_ON_CHANGE_ONLY=ON` por padrao no CMake.
- Em `SwitchDevice`, o firmware agora mantem um report pendente por controle e envia HID IN somente quando:
  - chegou um novo `PadIn`; ou
  - ja havia um report pendente que ainda nao conseguiu ser enviado porque o endpoint nao estava pronto.
- Antes dessa etapa, o modo Pokken reenviava o mesmo report em todo ciclo quando `tud_hid_n_ready()` estava verdadeiro.
- Tambem foram adicionados logs `[USB_RATE] switch` quando `OGXM_BT_RATE_DEBUG=ON`.

Objetivo tecnico:

- reduzir trafego USB redundante no modo Pokken;
- liberar tempo de CPU para Bluetooth/TinyUSB quando nao houve mudanca real de input;
- separar nos logs se o gargalo restante vem de `new_pad_in` baixo ou de endpoint USB sem prontidao.

Validacao esperada:

- manter DS4 em modo USB Pokken;
- gerar build Release com `OGXM_ENABLE_DEBUG_LOG=ON` e `OGXM_ENABLE_BT_RATE_DEBUG=ON`;
- comparar `[BT_RATE] idx=... set=...` com `[USB_RATE] switch idx=... new=... sent=...`;
- se `[BT_RATE] set` ja vier baixo ou alternando, o gargalo restante esta antes do USB;
- se `[BT_RATE] set` vier alto e `[USB_RATE] switch sent` vier baixo, o gargalo esta no driver/endpoint USB.

## Estado aplicado - USB tight poll default

Alteracao aplicada para o proximo teste:

- `OGXM_USB_TIGHT_POLL=ON` por padrao no CMake.
- O loop principal do Pico W/Pico 2W deixa de executar `sleep_ms(1)` ao fim de cada ciclo e passa a executar `tight_loop_contents()`.

Objetivo tecnico:

- reduzir buracos artificiais de 1 ms no loop que chama `device_driver->process()` e `tud_task()`;
- testar se a alternancia entre pacotes rapidos e pacotes de 60-75 ms esta sendo amplificada pela cadencia do loop USB;
- manter a mudanca reversivel com `-DOGXM_USB_TIGHT_POLL=OFF`.

Validacao esperada:

- repetir o teste DS4 no modo Pokken;
- comparar contra o teste anterior:
  - media: 31,53 ms;
  - `<5 ms`: 50,0%;
  - `<10 ms`: 54,2%;
  - `60-75 ms`: 44,8%.
- se `60-75 ms` cair de forma clara, o loop USB ainda era parte do gargalo;
- se ficar parecido, o alvo seguinte deve ser o caminho Bluetooth/Gamepad antes do USB.

## Estado aplicado - interval telemetry

Correcao aplicada depois do teste com `OGXM_USB_TIGHT_POLL=ON`:

- `OGXM_USB_TIGHT_POLL` voltou para `OFF` por padrao, pois o teste nao mostrou melhora.
- `Bluepad32.cpp` agora emite `[BT_GAP]` junto de `[BT_RATE]`, medindo intervalos reais entre `set_pad_in`.
- `Switch.cpp` agora emite `[USB_GAP] switch`, medindo intervalos reais entre:
  - `new_pad_in` consumidos pelo driver Pokken;
  - reports HID realmente enviados para o host.

Objetivo tecnico:

- descobrir onde nasce o intervalo de 60-75 ms;
- se `[BT_GAP]` ja mostrar muitos `60_75`, o gargalo esta antes do USB;
- se `[BT_GAP]` estiver rapido, mas `[USB_GAP] new_60_75` ou `sent_60_75` estiver alto, o gargalo esta no `Gamepad` ou no driver USB.

Build recomendada para esta etapa:

```bat
cd /d F:\GitRevised\OGX-Mini-Plus\Firmware\RP2040
cmake -S . -B build_pico2w_debuglog -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICO2W -DOGXM_ENABLE_DEBUG_LOG=ON -DOGXM_ENABLE_BT_RATE_DEBUG=ON
cmake --build build_pico2w_debuglog -j
```

## Estado aplicado - BT active link policy

Alteracao aplicada para o proximo teste:

- `OGXM_BLUEPAD32_DISABLE_SNIFF_FOR_BT_PERF_TEST=ON` por padrao.
- `OGXM_BLUEPAD32_FORCE_ACTIVE_LINK_POLICY=ON` por padrao.

Objetivo tecnico:

- impedir que o link Bluetooth Classic use sniff/baixa atividade durante o teste de performance;
- forcar politica de link ativa apos conexao;
- verificar se os intervalos recorrentes de 60-75 ms sao causados por politica BR/EDR, e nao pelo USB.

Validacao esperada:

- repetir o DS4 no modo Pokken;
- comparar contra o melhor teste anterior:
  - media: 31,53 ms;
  - `<5 ms`: 50,0%;
  - `<10 ms`: 54,2%;
  - `60-75 ms`: 44,8%.
- se a hipotese estiver correta, `60-75 ms` deve cair e as sequencias de 2-6 ms devem ficar mais longas;
- se DS4 parar de sincronizar, desligar apenas estas flags e manter as demais mudancas.

## Estado aplicado - DS4 L2CAP interval telemetry

Alteracao aplicada para o proximo teste:

- `OGXM_BLUEPAD32_DS4_L2CAP_RATE_DEBUG=OFF` por padrao.
- A telemetria `[BT_L2CAP]` agora fica em uma flag separada de `OGXM_ENABLE_BT_RATE_DEBUG`.
- O target `bluepad32` nao recebe mais `OGXM_BT_RATE_DEBUG=1` no teste normal.
- Quando `OGXM_BLUEPAD32_DS4_L2CAP_RATE_DEBUG=ON`, `uni_bt_bredr.c` emite `[BT_L2CAP]` para DS4 antes do parser HID.
- A telemetria usa `btstack_run_loop_get_time_ms()` para evitar dependencia direta do Pico SDK dentro do arquivo C do Bluepad32.
- O log separa:
  - total de pacotes L2CAP interrupt;
  - reports `0x01`;
  - reports `0x11`;
  - reports inesperados;
  - distribuicao de intervalos antes do parser.

Objetivo tecnico:

- confirmar se o intervalo de 60-75 ms ja existe no recebimento L2CAP do DS4;
- se `[BT_L2CAP] 60_75` vier alto, o problema esta no radio/BTstack/link BR/EDR antes do parser;
- se `[BT_L2CAP]` vier rapido mas `[BT_GAP]` ou `[USB_GAP]` vier lento, o problema esta depois do recebimento Bluetooth.

Build recomendada para esta etapa:

```bat
cd /d F:\GitRevised\OGX-Mini-Plus\Firmware\RP2040
cmake -S . -B build_pico2w_debuglog -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICO2W -DOGXM_ENABLE_DEBUG_LOG=ON -DOGXM_ENABLE_BT_RATE_DEBUG=ON -DOGXM_BLUEPAD32_DISABLE_SNIFF_FOR_BT_PERF_TEST=ON -DOGXM_BLUEPAD32_FORCE_ACTIVE_LINK_POLICY=ON -DOGXM_USB_TIGHT_POLL=OFF
cmake --build build_pico2w_debuglog -j
```

## Estado aplicado - Performance sem L2CAP telemetry

Correcao aplicada para o proximo teste:

- `[BT_L2CAP]` fica desligado por padrao para nao adicionar custo/jitter dentro do caminho BR/EDR.
- Para teste normal de desempenho, usar `OGXM_ENABLE_BT_RATE_DEBUG=OFF`.
- Para diagnostico com serial, ligar explicitamente:
  - `OGXM_ENABLE_DEBUG_LOG=ON`;
  - `OGXM_ENABLE_BT_RATE_DEBUG=ON`;
  - `OGXM_BLUEPAD32_DS4_L2CAP_RATE_DEBUG=ON`.

Build recomendada para desempenho:

```bat
cd /d F:\GitRevised\OGX-Mini-Plus\Firmware\RP2040
cmake -S . -B build_pico2w_perf -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICO2W -DOGXM_ENABLE_DEBUG_LOG=OFF -DOGXM_ENABLE_BT_RATE_DEBUG=OFF -DOGXM_BLUEPAD32_DS4_L2CAP_RATE_DEBUG=OFF -DOGXM_BLUEPAD32_DISABLE_SNIFF_FOR_BT_PERF_TEST=ON -DOGXM_BLUEPAD32_FORCE_ACTIVE_LINK_POLICY=ON -DOGXM_USB_TIGHT_POLL=OFF
cmake --build build_pico2w_perf -j
```

Build recomendada apenas para diagnostico serial:

```bat
cd /d F:\GitRevised\OGX-Mini-Plus\Firmware\RP2040
cmake -S . -B build_pico2w_debuglog -G Ninja -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICO2W -DOGXM_ENABLE_DEBUG_LOG=ON -DOGXM_ENABLE_BT_RATE_DEBUG=ON -DOGXM_BLUEPAD32_DS4_L2CAP_RATE_DEBUG=ON -DOGXM_BLUEPAD32_DISABLE_SNIFF_FOR_BT_PERF_TEST=ON -DOGXM_BLUEPAD32_FORCE_ACTIVE_LINK_POLICY=ON -DOGXM_USB_TIGHT_POLL=OFF
cmake --build build_pico2w_debuglog -j
```
