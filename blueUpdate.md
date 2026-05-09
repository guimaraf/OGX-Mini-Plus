# Plano de melhoria Bluetooth DInput

## Objetivo

Melhorar a comunicacao Bluetooth de controles DInput no OGX-Mini-Plus, principalmente DualShock 4, DualSense, Nintendo Switch Pro e clones/licenciados, preservando o funcionamento atual dos drivers e evitando uma reescrita ampla de arquitetura.

Este plano usa o JoypadOS como referencia tecnica, mas a implementacao deve ser incremental. Ao fim de cada etapa, o firmware deve ser testado em hardware real antes de seguir para a proxima.

O assistente nao deve fazer build. O build e os testes em hardware ficam a cargo do usuario.

## Hipotese principal

O JoypadOS consegue taxa melhor porque:

- processa reports Bluetooth de forma curta e direta
- evita filas profundas no hot path
- usa politica de estado mais recente
- evita trabalho periferico no caminho critico
- mantem o loop principal sem sleeps desnecessarios
- no Pico W, trata Sony Classic com cuidado especial para evitar problemas de SDP/CYW43

No OGX-Mini-Plus atual, o caminho Pico W usa Bluepad32 no core 1 e USB device no core 0. O projeto ja trabalha, em parte, com estado mais recente via `Gamepad::set_pad_in()`, entao a causa mais provavel nao e uma fila explicita grande no codigo local. Os pontos mais suspeitos sao:

- custo do parser Bluepad32 para DS4/DS5, incluindo motion, calibracao e touchpad/mouse virtual
- scheduling do loop USB no Pico W com `sleep_ms(1)`
- BLE server/servico paralelo competindo com Bluetooth Classic
- caminho Bluepad32 de conexao Sony usando SDP onde o JoypadOS evita SDP no CYW43
- possivel gargalo entre callback Bluetooth e envio USB, ainda sem medicao objetiva no projeto atual

## Regras de implementacao

- Uma etapa por vez.
- Nao misturar otimizacao, refatoracao e mudanca de comportamento na mesma etapa.
- Cada etapa deve ser facil de reverter.
- Preferir flags de compilacao ou helpers pequenos para experimentos.
- Preservar drivers existentes.
- Evitar alterar descritores ou comportamento de USB output ate confirmar que o gargalo esta ali.
- Evitar mexer em rumble/LED antes de estabilizar input.
- Nao alterar `Firmware/external/bluepad32` sem isolar a mudanca com macro `OGXM_...` ou comentario claro, porque e codigo externo.

## Controles de teste padrao

Sempre que possivel, testar:

- DualSense original
- DualShock 4 original
- Nintendo Switch Pro Controller ou clone ja conhecido
- Xbox One S ou Xbox Series como controle de referencia

Ferramentas de teste:

- Polling Rate no Windows
- Gamepad Test via navegador
- Gerenciador de controles do Windows
- teste real em jogo sensivel a analogico e direcional
- logs seriais quando a etapa incluir instrumentacao

Resultados devem ser anotados com:

- controle usado
- modo USB do OGX
- taxa media
- taxa minima/maxima aproximada
- estabilidade percebida
- se houve travamento, desconexao, atraso, perda de input ou rumble quebrado

---

# Etapa 1 - Instrumentacao de taxa BT e USB

## Objetivo

Descobrir onde a taxa cai:

- antes de chegar ao `controller_data_cb`
- dentro do caminho Bluepad32/parser
- entre `set_pad_in()` e USB
- no envio HID USB

Esta etapa deve medir, nao otimizar.

## Arquivos provaveis

- `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/DInput/DInput.cpp`
- opcionalmente `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`

## Implementacao proposta

Adicionar contadores leves, preferencialmente protegidos por macro, por exemplo:

- `OGXM_BT_RATE_DEBUG`
- contador de chamadas de `controller_data_cb`
- contador de chamadas aceitas como `UNI_CONTROLLER_CLASS_GAMEPAD`
- contador de `gamepad->set_pad_in(gp_in)`
- contador de reports enviados por `tud_hid_n_report()`
- janela de 1000 ms usando tempo monotonic
- log resumido 1 vez por segundo

Nao imprimir por report. Log por report destruiria a medicao.

## Testes

1. Compilar com instrumentacao.
2. Conectar DualSense via Bluetooth.
3. Mover analogico continuamente por 15 a 30 segundos.
4. Repetir com Xbox One S/Series.
5. Repetir com DS4, se disponivel.

## Resultado esperado

Precisamos separar estes casos:

- Se `controller_data_cb` ja fica perto de 33 Hz no DualSense, o gargalo esta no Bluetooth/parser Bluepad32.
- Se `controller_data_cb` fica alto, mas `tud_hid_n_report()` fica baixo, o gargalo esta no core 0/USB/output.
- Se Xbox fica alto e DualSense baixo no mesmo firmware, o problema e especifico do caminho DInput/Sony/Switch, nao do USB geral.

## Criterio para avancar

So avancar depois de registrar numeros de pelo menos DualSense e Xbox.

---

# Etapa 2 - Remover atraso artificial do loop Pico W

## Objetivo

Testar se o `sleep_ms(1)` no loop USB do Pico W esta reduzindo margem ou introduzindo jitter.

## Arquivo provavel

- `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`

## Implementacao proposta

Experimentar uma das opcoes, em ordem conservadora:

1. remover `sleep_ms(1)` do loop principal do core 0
2. ou trocar por uma politica condicional, dormindo apenas quando nao houver trabalho USB pendente
3. manter `tud_task()` chamado com alta frequencia

Como o Bluetooth roda no core 1, a remocao do sleep no core 0 nao deve bloquear diretamente o BTstack, mas precisa ser testada.

## Testes

1. Testar DualSense no Polling Rate.
2. Testar Xbox One S/Series para confirmar ausencia de regressao.
3. Testar troca de modo USB pelo combo atual, porque `TaskQueue::Core0::process_tasks()` tambem roda nesse loop.
4. Testar por pelo menos 5 minutos para observar aquecimento, travamento ou watchdog.

## Resultado esperado

- Xbox deve permanecer estavel.
- Se a queda estiver no lado USB, DualSense deve subir de forma perceptivel.
- Se DualSense continuar em torno de 33 Hz e a instrumentacao da Etapa 1 mostrar callback BT baixo, esta etapa nao resolve a causa principal.

## Criterio para avancar

Avancar se nao houver regressao de estabilidade. Manter a mudanca apenas se melhorar taxa ou reduzir jitter.

---

# Etapa 3 - Reduzir trabalho local no callback Bluepad32

## Objetivo

Garantir que o callback do OGX faca apenas conversao minima para `Gamepad::PadIn` e nao carregue trabalho desnecessario.

## Arquivo provavel

- `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`

## Implementacao proposta

Revisar `controller_data_cb` para:

- retornar imediatamente se `idx` for invalido
- retornar imediatamente se `bt_devices_[idx].connected` for falso
- evitar qualquer trabalho para classes nao gamepad
- evitar conversoes nao usadas
- opcionalmente ignorar dados de motion/touch que chegam no `uni_controller_t`, mantendo somente botoes, dpad, sticks e triggers

Esta etapa nao mexe ainda no parser Bluepad32. Ela apenas deixa o wrapper OGX o mais barato possivel.

## Testes

1. Repetir medicao da Etapa 1.
2. Confirmar mapeamento de todos os botoes principais.
3. Confirmar triggers analogicos no DualSense/DS4.
4. Confirmar Switch Pro digital ZL/ZR.
5. Confirmar rumble basico quando aplicavel.

## Resultado esperado

Ganho pequeno a moderado. Se o gargalo principal estiver dentro do parser Bluepad32, esta etapa sozinha pode nao mudar muito.

## Criterio para avancar

Avancar se nao houver regressao de mapeamento.

---

# Etapa 4 - Fast path Sony no parser Bluepad32

## Objetivo

Aplicar a logica mais importante do JoypadOS para Sony: processar apenas o necessario para jogo e evitar custo de motion/touch no hot path.

## Arquivos provaveis

- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`

## Implementacao proposta

Adicionar uma macro local, por exemplo:

- `OGXM_BLUEPAD32_SONY_FAST_INPUT`

Quando ativa:

- DS4 report `0x11`: parsear somente botoes, dpad, sticks, triggers e bateria basica
- DS5 report `0x31`: parsear somente botoes, dpad, sticks, triggers e bateria basica
- pular loops de gyro
- pular loops de accel
- pular parser de touchpad/mouse virtual
- manter a chamada normal para `uni_hid_device_process_controller()` do dispositivo gamepad
- nao alterar rumble/LED nesta etapa

Esta etapa deve preservar o comportamento padrao quando a macro estiver desativada.

## Testes

1. DualSense no Polling Rate com analogico em movimento constante.
2. DS4 no Polling Rate.
3. Gamepad Test para todos os botoes/sticks/triggers.
4. Teste de rumble.
5. Teste de reconexao.
6. Comparar com Xbox como controle de referencia.

## Resultado esperado

Se o custo do parser for o principal gargalo, esta etapa deve produzir o maior salto de taxa para DS4/DS5.

Meta realista inicial:

- sair de aproximadamente 33 Hz para algo claramente acima de 100 Hz
- idealmente aproximar ou passar de 200 Hz no DualSense

Nao esperar necessariamente os 700 Hz do adaptador Bluetooth 4.0 do PC, porque o OGX ainda esta convertendo e enviando por USB device no RP2040.

## Criterio para avancar

Avancar se:

- taxa melhorar sem quebrar input principal
- rumble nao regredir de forma grave
- reconexao continuar funcional

Se a taxa nao melhorar, o gargalo provavelmente esta antes do parser ou em configuracao/link Bluetooth.

---

# Etapa 5 - Desabilitar mouse virtual Sony no Bluepad32

## Objetivo

Remover a criacao e processamento de dispositivo virtual de mouse para touchpad DS4/DS5 quando o OGX estiver usando esses controles apenas como gamepad.

## Arquivos provaveis

- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds4.c`
- `Firmware/external/bluepad32/src/components/bluepad32/parser/uni_hid_parser_ds5.c`
- possivelmente headers/config do Bluepad32 se houver flag existente para virtual devices

## Implementacao proposta

Adicionar macro local, por exemplo:

- `OGXM_BLUEPAD32_DISABLE_SONY_VIRTUAL_MOUSE`

Quando ativa:

- nao criar `uni_hid_device_create_virtual(d)` para DS4/DS5
- nao chamar `ds4_parse_mouse()` / `ds5_parse_mouse()`
- manter touchpad click mapeado como botao se ja estiver no gamepad, se necessario

## Testes

1. DualSense e DS4 conectam normalmente.
2. Nenhum dispositivo extra estranho aparece no fluxo interno.
3. Botoes, sticks e triggers continuam corretos.
4. Taxa no Polling Rate melhora ou permanece igual.
5. Rumble/LED continuam aceitaveis.

## Resultado esperado

Ganho pequeno a moderado, principalmente por reduzir callback extra e processamento de touchpad.

## Criterio para avancar

Manter se nao quebrar DS4/DS5 e se simplificar o hot path. Reverter se afetar conexao ou ready state.

---

# Etapa 6 - Isolar BLE Server durante testes DInput Classic

## Objetivo

Verificar se o BLE server local ou anuncios BLE do OGX estao competindo com Bluetooth Classic no Pico W.

## Arquivos provaveis

- `Firmware/RP2040/src/OGXMini/Board/PicoW.cpp`
- `Firmware/RP2040/src/BLEServer/BLEServer.cpp`
- `Firmware/RP2040/src/sdkconfig.h`

## Implementacao proposta

Criar uma flag temporaria, por exemplo:

- `OGXM_DISABLE_BLE_SERVER_FOR_BT_PERF_TEST`

Quando ativa:

- nao chamar `BLEServer::init_server(_gamepads)` durante testes
- se necessario, desabilitar BLE por configuracao Bluepad32 enquanto o foco for BT Classic

## Testes

1. Testar DualSense/DS4/Switch Pro com BLE server desativado.
2. Testar Xbox One S/Series, porque alguns Xbox podem depender de BLE.
3. Confirmar se recursos que usam BLE server deixam de funcionar apenas quando a flag esta ativa.
4. Medir estabilidade por 10 minutos.

## Resultado esperado

Se houver disputa de radio/stack, DualSense/DS4 podem ganhar estabilidade ou taxa. Se Xbox BLE deixar de conectar, isso e esperado para esta flag de teste, mas nao pode virar padrao sem uma solucao seletiva.

## Criterio para avancar

Se melhorar DInput Classic, planejar uma politica dinamica:

- desativar BLE server durante sessao Classic HID
- ou pausar anuncios BLE quando um controle Classic estiver conectado
- ou deixar como opcao de configuracao

---

# Etapa 7 - Caminho Sony sem SDP no CYW43

## Objetivo

Portar com cuidado a ideia do JoypadOS de evitar SDP para Sony no CYW43, porque o JoypadOS indica que respostas SDP de DS4/DS5 podem derrubar ou degradar o barramento SPI/CYW43.

## Arquivos provaveis

- `Firmware/external/bluepad32/src/components/bluepad32/bt/uni_bt_bredr.c`
- `Firmware/external/bluepad32/src/components/bluepad32/uni_hid_device.c`
- possivelmente arquivos de conexao L2CAP/SDP do Bluepad32

## Implementacao proposta

Esta e uma etapa mais arriscada. Antes de alterar, estudar exatamente:

- como Bluepad32 decide `controller_type`
- quando ele exige SDP para DS4/DS5
- se DS4 e DS5 podem ser reclassificados pelo primeiro report, como no JoypadOS
- como abrir canais HID L2CAP direto sem quebrar outros dispositivos

Possivel abordagem:

- macro `OGXM_CYW43_SONY_SKIP_SDP`
- se nome for `Wireless Controller` ou `DualSense`, evitar SDP
- abrir L2CAP HID control/interrupt direto
- classificar DS4/DS5 pelo report ID (`0x11` para DS4, `0x31` para DS5), se necessario

## Testes

1. Apagar pareamento antigo e parear DS4.
2. Apagar pareamento antigo e parear DualSense.
3. Testar reconexao apos reboot.
4. Medir taxa no Polling Rate.
5. Confirmar que Xbox e Switch Pro nao foram afetados.
6. Testar com controle clone/licenciado que anuncia nome semelhante.

## Resultado esperado

Esta etapa deve melhorar robustez de conexao e talvez reduzir problemas iniciais de DS4/DS5 no Pico W. Nao e garantido que melhore taxa em runtime se a conexao ja estava estabelecida corretamente.

## Criterio para avancar

So manter se pairing/reconnection ficarem iguais ou melhores. Reverter imediatamente se quebrar deteccao de DS4/DS5.

---

# Etapa 8 - Revisao de intervalo e envio USB por modo

## Objetivo

Garantir que o USB output nao esteja limitando indevidamente os testes.

## Arquivos provaveis

- `Firmware/RP2040/src/Descriptors/DInput.h`
- `Firmware/RP2040/src/Descriptors/DS4.h`
- `Firmware/RP2040/src/Descriptors/PS4.h`
- `Firmware/RP2040/src/USBDevice/DeviceDriver/DInput/DInput.cpp`
- outros drivers USB device usados nos testes

## Observacao atual

O descritor DInput atual usa intervalo 1 ms. Portanto, para testes em DInput, o descritor nao parece ser a causa de 33 Hz.

Os descritores DS4/PS4 locais usam 5 ms, o que limita esses modos a cerca de 200 Hz. Isso pode ser aceitavel por compatibilidade, mas precisa ser conhecido durante testes.

## Implementacao proposta

Somente se os testes mostrarem gargalo USB:

- medir reports efetivamente enviados por `tud_hid_n_report()`
- evitar envio duplicado desnecessario se isso estiver prejudicando o host
- avaliar `bInterval` de modos especificos com muito cuidado

## Testes

1. Testar em modo DInput.
2. Testar em modo XInput.
3. Testar em modo PS4/DS4 se usado.
4. Comparar taxa reportada pelo Polling Rate com contador interno de envio USB.

## Resultado esperado

Confirmar que a taxa baixa dos controles DInput Bluetooth nao esta sendo causada pelo USB device.

---

# Etapa 9 - Fast path dedicado para controles DInput Classic

## Objetivo

Se as etapas anteriores nao forem suficientes, implementar um caminho mais parecido com o JoypadOS para alguns controles DInput Classic, sem remover Bluepad32 para o resto do projeto.

## Ideia

Criar um caminho dedicado para:

- DS4
- DualSense
- Switch Pro

Esse caminho deve:

- receber report Classic HID
- parsear somente o conjunto minimo util
- atualizar `Gamepad::PadIn` diretamente
- manter Bluepad32 para controles ja estaveis, como Xbox, 8BitDo e outros

## Arquivos provaveis

Ainda depende da decisao tecnica, mas pode envolver:

- `Firmware/RP2040/src/Bluepad32/Bluepad32.cpp`
- novos arquivos em `Firmware/RP2040/src/Bluepad32/` ou `Firmware/RP2040/src/Bluetooth/`
- partes selecionadas de parser inspiradas em `!joypadOS/src/bt/bthid/devices/vendors/sony/`
- partes selecionadas de parser inspiradas em `!joypadOS/src/bt/bthid/devices/vendors/nintendo/`

## Testes

1. Todos os testes anteriores.
2. Pareamento novo e reconexao.
3. Teste com mais de um controle, se o firmware suportar.
4. Teste prolongado em jogo real.
5. Teste de regressao com Xbox.

## Resultado esperado

Essa etapa e a mais proxima de portar a logica do JoypadOS. Deve ser usada apenas se as otimizacoes localizadas no Bluepad32 nao resolverem.

---

# Ordem recomendada

1. Etapa 1 - Instrumentacao.
2. Etapa 2 - Remover atraso artificial do core 0.
3. Etapa 3 - Enxugar wrapper OGX do Bluepad32.
4. Etapa 4 - Fast path Sony sem motion/touch no parser.
5. Etapa 5 - Desabilitar mouse virtual Sony.
6. Etapa 6 - Isolar BLE Server.
7. Etapa 7 - Skip SDP Sony no CYW43.
8. Etapa 8 - Revisao USB por modo.
9. Etapa 9 - Caminho dedicado inspirado no JoypadOS.

## Primeira implementacao recomendada

A primeira mudanca real deve ser a Etapa 1. Sem essa medicao, qualquer port do JoypadOS vira tentativa cega.

O primeiro resultado importante sera responder:

- o DualSense chega ao `controller_data_cb` a 33 Hz ou mais alto?
- o USB envia reports a 33 Hz ou mais alto?
- o Xbox passa pelos mesmos pontos com taxa maior?

Com essas tres respostas, a etapa seguinte fica objetiva.

