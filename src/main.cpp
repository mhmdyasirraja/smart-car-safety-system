#include <Arduino.h>

// Definisikan pin ADC yang digunakan
const int MQ7_PIN = 34;

void setup()
{
    // Inisialisasi komunikasi Serial dengan baud rate 115200
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n======================================");
    Serial.println("   MONITORING SENSOR MQ-7 - ESP32     ");
    Serial.println("======================================");
    Serial.println("Catatan: Sensor membutuhkan pre-heating agar stabil.");
}

void loop()
{
    // 1. Membaca nilai analog dari pin GPIO 34 (Nilai ADC: 0 - 4095)
    int nilaiAnalog = analogRead(MQ7_PIN);

    // 2. Mengonversi nilai ADC ke Voltase (0V - 3.3V)
    // Sesuai rangkaian langsung Anda saat ini
    float voltase = (nilaiAnalog * 3.3) / 4095.0;

    // 3. Menampilkan data ke Serial Monitor PlatfromIO
    Serial.print("ADC: ");
    Serial.print(nilaiAnalog);
    Serial.print(" | Voltase: ");
    Serial.print(voltase, 2); // Menampilkan 2 angka di belakang koma
    Serial.print(" V -> ");

    // 4. Logika indikator sederhana berdasarkan voltase/ADC
    // Anda bisa mengubah angka batasan (threshold) ini nanti setelah kalibrasi
    if (nilaiAnalog < 1200)
    {
        Serial.println("STATUS: AMAN [Udara Bersih]");
    }
    else if (nilaiAnalog >= 1200 && nilaiAnalog < 2500)
    {
        Serial.println("STATUS: WASPADA [Gas CO Terdeteksi!]");
    }
    else
    {
        Serial.println("STATUS: BAHAYA [Kadar CO Tinggi!]");
    }

    // Tunggu 1,5 detik sebelum pembacaan berikutnya
    delay(1500);
}
