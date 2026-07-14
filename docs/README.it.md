# FindMyAdv — guida italiana

FindMyAdv è una libreria Arduino riutilizzabile che aggiunge l'advertising BLE
Apple Find My e Google Find Hub a un firmware ESP32 già esistente. La libreria
lavora in un task FreeRTOS indipendente: normalmente basta una chiamata a
`FindMyAdv.begin(config)` dentro `setup()` e non occorre modificare `loop()`.

Il progetto è indipendente e non ufficiale; non è affiliato né approvato da
Apple, Google o Espressif.

## Compatibilità

Sono verificati ESP32 classico, ESP32-C3, ESP32-S3 ed ESP32-C6. La selezione
automatica dello stack permette di usare anche ESP32-C2, C5, C61 e H2 quando
il relativo core Arduino espone NimBLE o Bluedroid.
ESP8266, ESP32-S2 ed ESP32-P4 non possono essere supportati perché non hanno
una radio Bluetooth.

## Installazione

Con PlatformIO puoi installare direttamente la libreria pubblica:

```ini
lib_deps = https://github.com/mattbox03/Find_My_adv_ESP_library.git
```

ESP32, ESP32-C3 ed ESP32-S3 possono usare `espressif32@7.0.1`. Per ESP32-C6
serve Arduino-ESP32 3.x; questa configurazione è verificata:

```ini
[env:esp32c6]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.39/platform-espressif32.zip
platform_packages =
    framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32/releases/download/3.3.9/esp32-core-3.3.9.tar.xz
board = esp32-c6-devkitc-1
framework = arduino
lib_deps = https://github.com/mattbox03/Find_My_adv_ESP_library.git
```

Se su Windows è già presente Arduino-ESP32 2.x, PlatformIO può riutilizzare
per errore quel pacchetto omonimo al posto del core C6. Usa una cache isolata:

```powershell
$env:PLATFORMIO_PACKAGES_DIR="$PWD\.pio-packages-c6"
pio run -e esp32c6
```

Il primo avvio scarica l'intera toolchain e può richiedere diversi minuti; le
compilazioni successive riutilizzano `.pio-packages-c6`.

Per un'installazione locale puoi invece copiare l'intera cartella `FindMyAdv`
nella cartella `lib` del progetto. Con Arduino IDE, installa lo ZIP della
libreria tramite **Sketch > Include Library > Add .ZIP Library**.

## Uso minimo

```cpp
#include <FindMyAdv.h>

void setup() {
    FindMyAdvConfig config;
    config.appleAdvertisementKeyBase64 = ""; // chiave Base64 o vuota
    config.googleAdvertisementEidHex = "";   // EID esadecimale o vuoto
    config.advertisingIntervalMs = 2000;
    config.txPowerDbm = 0;

    FindMyAdv.begin(config);
}

void loop() {
    // Il resto del firmware non cambia.
}
```

Si può impostare Apple, Google oppure entrambi. Quando sono presenti entrambi,
la libreria alterna automaticamente i due frame pubblicitari.

## Generazione e registrazione delle chiavi

FindMyAdv gestisce soltanto l'advertising BLE. La generazione delle chiavi, la
registrazione del tracker, l'autenticazione, il recupero delle posizioni e la
decifratura dei report sono compiti dei progetti collegati descritti qui sotto.
La libreria non contatta i servizi Apple o Google e non richiede le credenziali
dei relativi account.

### Apple Find My / Haystack

Usa [OpenHaystack](https://github.com/seemoo-lab/openhaystack) oppure un sistema
Haystack compatibile come
[macless-haystack](https://github.com/dchristl/macless-haystack):

1. Crea un nuovo accessorio in OpenHaystack e copia la sua **chiave pubblica di
   advertising**; in alternativa esegui `generate_keys.py` di macless-haystack
   e preleva la chiave Base64 dal file `.keys` generato.
2. Inserisci quel valore pubblico in `appleAdvertisementKeyBase64`. Deve
   decodificare esattamente 28 byte (normalmente 40 caratteri Base64, padding
   compreso).
3. Conserva la chiave privata nel client/backend Haystack che decifra i report.
   Non inserire mai nel firmware chiave privata, credenziali Apple o dati 2FA.

FindMyAdv trasmette soltanto la chiave pubblica. Per visualizzare le posizioni
serve comunque un servizio OpenHaystack/macless-haystack configurato
correttamente.

### Google Find Hub / Google Find My Tools

Usa [GoogleFindMyTools](https://github.com/leonboe1/GoogleFindMyTools) per
autenticare l'account e registrare il tracker personalizzato:

1. Segui le istruzioni aggiornate del progetto, installa i requisiti Python ed
   esegui `main.py`.
2. Seleziona la registrazione del tracker (`r` nella versione attuale), completa
   la procedura e copia la chiave di advertising/EID mostrata.
3. Inseriscila in `googleAdvertisementEidHex`: esattamente 40 caratteri
   esadecimali (20 byte), senza `0x`, spazi, due punti o trattini.

I segreti di autenticazione restano sul computer che esegue GoogleFindMyTools e
non vanno copiati sull'ESP32. Anche la registrazione e gli eventuali annunci
periodici richiesti dal servizio restano responsabilità di GoogleFindMyTools.
Il protocollo è sperimentale e può cambiare: prima di creare un tracker verifica
sempre il README aggiornato del progetto originale.

Si può configurare una sola rete oppure entrambe:

```cpp
FindMyAdvConfig config;
config.appleAdvertisementKeyBase64 = "CHIAVE_PUBBLICA_APPLE_BASE64"; // o ""
config.googleAdvertisementEidHex = "0123456789abcdef0123456789abcdef01234567"; // o ""

if (!FindMyAdv.begin(config)) {
    Serial.println(FindMyAdv.lastError());
}
```

Non copiare chiavi da tracker commerciali e non riutilizzare la stessa identità
su dispositivi indipendenti. Registra e traccia soltanto hardware tuo o per cui
hai ricevuto autorizzazione.

## Configurazione dinamica

Le chiavi possono provenire dalla pagina web già presente nel firmware, MQTT,
Preferences/NVS o qualsiasi altro sistema di configurazione:

```cpp
FindMyAdv.setKeys(nuovaChiaveApple.c_str(), nuovoEidGoogle.c_str());
```

Per aggiornare contemporaneamente chiavi, intervalli, potenza TX e sensore:

```cpp
FindMyAdvConfig nuova = config;
nuova.advertisingIntervalMs = 1000;
nuova.txPowerDbm = 3;
FindMyAdv.reconfigure(nuova);
```

La libreria copia internamente le stringhe. Il salvataggio permanente rimane
volutamente responsabilità del firmware principale, così FindMyAdv non impone
un portale Wi-Fi, un filesystem o uno specifico formato di credenziali.

## Accelerometro opzionale

Sono supportati LIS3DH, LIS2DH e LIS2DH12 agli indirizzi I2C `0x18` e `0x19`.
Con indirizzo `0` il rilevamento è automatico. Durante il movimento si può usare
un intervallo più rapido e tornare a quello di riposo dopo `motionHoldMs`.

Per qualsiasi altro accelerometro, IMU, sensore a vibrazione o stato del
programma si può assegnare una funzione a `motionDetectedCallback`: non occorre
modificare la libreria.

## Limiti importanti

- Il firmware può continuare a usare Wi-Fi, MQTT, sensori, display e web server.
- FindMyAdv controlla il legacy advertising BLE. Se il programma pubblicizza
  già un servizio GATT, i due moduli devono coordinare esplicitamente l'unico
  advertising set disponibile.
- Durante il deep sleep la radio è spenta e non può trasmettere.
- Le chiavi reali non devono essere pubblicate in repository, log o binari.
- Chiavi private, password, dati di autenticazione del browser e segreti 2FA non
  devono mai essere memorizzati nel firmware.
- Un identificatore pubblico fisso può essere riconoscibile da osservatori BLE
  nelle vicinanze: valuta questo aspetto di privacy per l'uso previsto.

La descrizione completa di tutti i campi e metodi è in
[`API.md`](API.md).
