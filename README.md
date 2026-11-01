# ssv6x5x-original: driver da Icomm para SSV6256, intocado, no kernel 7.2

Mesma proposta do wrapper do SSV6051: roda o driver **original** do SSV6256
(família ssv6x5x/6006C) num kernel atual, para servir de **referência**
contra o `ssv6xxx` escrito do zero.

## Origem do código

`vendor/` é o `paolosabatino/ssv6x5x`, commit `4594793` ("Initial commit"),
**sem alteração**. É o drop da Icomm `SMAC.0000.1807.12.SPUR.90`, rev 17680,
compilado em 07/04/2019. É a mesma versão que serviu de base para o teu
`6256` (o commit `inicial` dele já vinha com o port de outra pessoa para
6.12). Os commits seguintes do paolosabatino são adaptações para RK322x e não
entram aqui.

## Como está organizado

Igual ao do 6051:

| caminho | o que é |
|---|---|
| `vendor/` | código da Icomm, intocado |
| `compat/compat.h` | incluído à força antes de cada arquivo: refaz a API antiga em cima do 7.2 |
| `compat/compat.c` | callbacks do mac80211, controle de taxa, procfs, notificador de CPU, crypto |
| `compat/platform.c` | substitui `ssv6x5x-generic-wlan.c` (que chamava `rockchip_wifi_power`) |
| `patches/` | 9 patches mecânicos, 92 linhas |
| `Kbuild` | flags do fabricante (`platform-config.mak` → `config_common.mak`), trocando só as de plataforma |

Diferenças em relação ao 6051: este driver é de 2019 e já usa
`ieee80211_ampdu_params`, `skcipher` e `ahash`. O `compat` deixa o
`ampdu_action` passar direto e reimplementa `skcipher`/`ahash` sobre a mesma
base (`lib/crypto` e Michael MIC local), porque `michael_mic` não existe mais
como hash no kernel e `ecb(arc4)` só existe com
`CRYPTO_USER_API_ENABLE_OBSOLETE`.

### Os patches

| patch | por quê |
|---|---|
| 0001 `sta-deflink` | `sta->ht_cap`/`supp_rates` → `sta->deflink` |
| 0002 `vif-cfg` | `bss_conf.assoc`/`aid` → `vif->cfg` |
| 0003 `timer-setup` | `init_timer`/`.data` → `timer_setup` |
| 0004 `task-state` | `task->state` → `task->__state` |
| 0005 `platform-remove` | `.remove` retorna `void` |
| 0006 `rx-status` | `RX_FLAG_HT`/`40MHZ`/`SHORT_GI`/`SHORTPRE` → `encoding`/`bw`/`enc_flags` |
| 0007 `usb-drvwrap` | `usb_driver.drvwrap.driver` → `usb_driver.driver` |
| 0008 `driver-find-device` | a função de comparação recebe `const void *` |
| 0009 `file-read-write` | `fp->f_op->read/write` é NULL no ext4 atual (leitura e gravação do MAC) |

### Configuração de compilação

`config_common.mak` do fabricante como está (HAL, SSV6006C, crypto local,
minstrel próprio, HT/AMPDU) mais o `ssv6x5x.cfg` original **sem**
`CONFIG_SSV_SUPPORT_ANDROID` e **com** `CONFIG_SSV_BUILD_AS_ONE_KO`. O
suporte a USB do fabricante (`hwif/usb`) vem junto porque faz parte do
SSV6006 na configuração dele.

## Compilar e instalar (na própria caixa)

```sh
make
sudo install -D -m644 build/ssv6x5x_original.ko /lib/modules/$(uname -r)/updates/ssv6x5x_original.ko
sudo install -D -m644 vendor/image/ssv6x5x-sw.bin /lib/firmware/ssv6x5x-original/ssv6x5x-sw.bin
sudo install -D -m644 config/ssv6x5x-wifi.cfg   /lib/firmware/ssv6x5x-original/ssv6x5x-wifi.cfg
sudo install -m644 config/ssv6x5x-original.conf /etc/modprobe.d/
sudo depmod -a
sudo rmmod ssv6xxx ssv6x5x 2>/dev/null; sudo modprobe ssv6x5x_original
```

O módulo se chama `ssv6x5x_original` para não colidir com o teu `ssv6x5x`.

## Para a comparação ser justa

- **Firmware:** aqui é o mesmo. O `ssv6x5x-sw.bin` do fabricante (9/ago/2018)
  é idêntico, byte a byte, ao do `ssv6xxx` e do `6256`.
- Um driver por vez: todos atendem 0x3030:0x3030.
- O `.cfg` do fabricante liga HT e AMPDU. Para comparar com o `ssv6xxx` (só
  legacy), meça os dois modos: `hw_cap_ht = off` no cfg dá o caso "legacy
  contra legacy".

## O que foi verificado e o que não foi

- Compila sem erro contra o **7.2.15** em `arm` e `arm64` (GCC 14), sem aviso
  de tipo de ponteiro, conversão ou função implícita.
- Todos os símbolos que o módulo usa são exportados pelo 7.2.15.
- **Ainda não rodou em hardware.** Os pontos de atenção são os mesmos do 6051:
  `ieee80211_tx_status` fora de softirq (se o lockdep reclamar, troque para
  `_ni` em `compat.h`), a fila de TX e o contexto de canal emulados, e a
  crypto em software reimplementada.
