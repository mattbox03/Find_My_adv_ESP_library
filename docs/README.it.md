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

Con PlatformIO, copia l'intera cartella `FindMyAdv` nella cartella `lib` del
progetto. Con Arduino IDE, installa lo ZIP della libreria tramite **Sketch >
Include Library > Add .ZIP Library**.

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

La descrizione completa di tutti i campi e metodi è in
[`API.md`](API.md).
