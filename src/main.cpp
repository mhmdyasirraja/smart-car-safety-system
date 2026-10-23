#include <Arduino.h>

namespace Pin
{
    constexpr uint8_t MQ7 = 34; // input-only, ADC1
    constexpr uint8_t LED_HIJAU = 27;
    constexpr uint8_t LED_KUNING = 26;
    constexpr uint8_t LED_MERAH = 25;
} // namespace Pin

namespace Config
{
    constexpr uint32_t INTERVAL_BACA_MS = 5000; // FA01
    constexpr uint32_t WARMUP_MS = 60000;       // sementara; MQ-7 idealnya lebih lama
    constexpr uint32_t KEDIP_MS = 500;
    constexpr uint8_t JUMLAH_SAMPEL = 16;

    // Divider: 100Ω (atas) + 220Ω (bawah). Ubah kalau resistor berbeda.
    constexpr float RASIO_DIVIDER = (100.0f + 220.0f) / 220.0f;

    // Threshold dalam mV di sisi sensor (sebelum divider). SEMENTARA, belum ppm.
    constexpr float BATAS_WASPADA_MV = 1000.0f;
    constexpr float BATAS_BAHAYA_MV = 2000.0f;
    constexpr float HISTERESIS_MV = 100.0f;
    constexpr float BATAS_ERROR_MV = 50.0f; // nyangkut ~0 V => sensor/kabel bermasalah
} // namespace Config

enum class Status
{
    PEMANASAN,
    AMAN,
    WASPADA,
    BAHAYA,
    SENSOR_ERROR
};

const char *namaStatus(Status s)
{
    switch (s)
    {
    case Status::PEMANASAN:
        return "PEMANASAN";
    case Status::AMAN:
        return "AMAN [Udara Bersih]";
    case Status::WASPADA:
        return "WASPADA [Gas CO Terdeteksi!]";
    case Status::BAHAYA:
        return "BAHAYA [Kadar CO Tinggi!]";
    case Status::SENSOR_ERROR:
        return "SENSOR ERROR";
    }
    return "?";
}

// Rata-rata beberapa sampel untuk meredam noise ADC, lalu kembalikan ke skala sensor.
float bacaSensorMv()
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < Config::JUMLAH_SAMPEL; i++)
    {
        total += analogReadMilliVolts(Pin::MQ7);
        delayMicroseconds(200);
    }
    float mvPin = static_cast<float>(total) / Config::JUMLAH_SAMPEL;
    return mvPin * Config::RASIO_DIVIDER;
}

// Histeresis: naik status langsung, turun status harus lewat margin.
Status tentukanStatus(float mv, Status sebelumnya)
{
    if (mv < Config::BATAS_ERROR_MV)
        return Status::SENSOR_ERROR;

    const bool sedangBahaya = (sebelumnya == Status::BAHAYA);
    const bool sedangWaspada = (sebelumnya == Status::WASPADA) || sedangBahaya;

    const float batasBahaya = Config::BATAS_BAHAYA_MV - (sedangBahaya ? Config::HISTERESIS_MV : 0);
    const float batasWaspada = Config::BATAS_WASPADA_MV - (sedangWaspada ? Config::HISTERESIS_MV : 0);

    if (mv >= batasBahaya)
        return Status::BAHAYA;
    if (mv >= batasWaspada)
        return Status::WASPADA;
    return Status::AMAN;
}

void setLed(bool hijau, bool kuning, bool merah)
{
    digitalWrite(Pin::LED_HIJAU, hijau);
    digitalWrite(Pin::LED_KUNING, kuning);
    digitalWrite(Pin::LED_MERAH, merah);
}

// Dipanggil tiap loop (non-blocking) supaya kedipan tetap jalan di antara pembacaan.
void perbaruiLed(Status s)
{
    static uint32_t terakhirGanti = 0;
    static bool fase = false;

    if (millis() - terakhirGanti >= Config::KEDIP_MS)
    {
        terakhirGanti = millis();
        fase = !fase;
    }

    switch (s)
    {
    case Status::PEMANASAN:
        setLed(fase, false, false);
        break; // hijau berkedip
    case Status::AMAN:
        setLed(true, false, false);
        break;
    case Status::WASPADA:
        setLed(false, true, false);
        break;
    case Status::BAHAYA:
        setLed(false, false, true);
        break;
    case Status::SENSOR_ERROR:
        setLed(fase, fase, fase);
        break; // semua berkedip
    }
}

Status statusSekarang = Status::PEMANASAN;
uint32_t terakhirBaca = 0;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(Pin::LED_HIJAU, OUTPUT);
    pinMode(Pin::LED_KUNING, OUTPUT);
    pinMode(Pin::LED_MERAH, OUTPUT);
    analogSetPinAttenuation(Pin::MQ7, ADC_11db); // range ~0–3.1 V

    // Tes LED sekali saat boot: gampang cek wiring
    setLed(true, true, true);
    delay(800);
    setLed(false, false, false);

    Serial.println("\n=== MONITORING MQ-7 - ESP32 ===");
}

void loop()
{
    const uint32_t now = millis();

    if (now < Config::WARMUP_MS)
    {
        statusSekarang = Status::PEMANASAN;
    }
    else if (now - terakhirBaca >= Config::INTERVAL_BACA_MS || terakhirBaca == 0)
    {
        terakhirBaca = now;

        const float mv = bacaSensorMv();
        statusSekarang = tentukanStatus(mv, statusSekarang);

        Serial.printf("Sensor: %.0f mV | STATUS: %s\n", mv, namaStatus(statusSekarang));
    }

    perbaruiLed(statusSekarang);
}