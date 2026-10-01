// ======================= PIN DEFINISI =======================
#define TRIG_PIN   5
#define ECHO_PIN  18
#define ENCODER_PIN 34
#define ENA_PWM_PIN 4
#define IN1_PIN     16
#define IN2_PIN     17

// ===================== RULE BASE (5x5) ====================
// Baris = Himpunan Jarak (SD, D, S, J, SJ)
// Kolom = Himpunan Kecepatan (SL, L, S, C, SC)
// Nilai = Output PWM yang direkomendasikan (%)
const float rule[5][5] = {
    // Kolom:  SL,  L,  S,  C,  SC
    {  0, 0, 5, 10, 15 }, // Baris: SD (Sangat Dekat)
    { 27,30,35, 40, 40 }, // Baris: D  (Dekat)
    { 30,35,40, 45, 45 }, // Baris: S  (Sedang)
    { 35,40,40, 45, 50 }, // Baris: J  (Jauh)
    { 55,60,65, 70, 75 }  // Baris: SJ (Sangat Jauh)
};

// Label himpunan fuzzy
String setJarak[] = {"SD","D","S","J","SJ"};  // Input 1: Jarak
String setKec[]   = {"SL","L","S","C","SC"};  // Input 2: Kecepatan

// ===== 1. FUZZIFIKASI - Fungsi Keanggotaan Jarak =====
// Mengubah nilai crisp jarak (cm) → derajat keanggotaan (0-1)
float muJarak(float x, String himp) {
    if (himp == "SD") {                       // Sangat Dekat: 0-35 cm
        if (x <= 25) return 1.0;
        else if (x > 25 && x <= 35) return (35 - x) / 10.0;
        else return 0.0;
        
    } else if (himp == "D") {                 // Dekat: 25-65 cm
        if (x <= 25 || x >= 65) return 0.0;
        else if (x > 25 && x <= 35) return (x - 25) / 10.0;
        else if (x > 35 && x <= 55) return 1.0;
        else if (x > 55 && x < 65) return (65 - x) / 10.0;
        else return 0.0;
        
    } else if (himp == "S") {                 // Sedang: 55-95 cm
        if (x <= 55 || x >= 95) return 0.0;
        else if (x > 55 && x <= 65) return (x - 55) / 10.0;
        else if (x > 65 && x <= 85) return 1.0;
        else if (x > 85 && x < 95) return (95 - x) / 10.0;
        else return 0.0;
        
    } else if (himp == "J") {                 // Jauh: 85-125 cm
        if (x <= 85 || x >= 125) return 0.0;
        else if (x > 85 && x <= 95) return (x - 85) / 10.0;
        else if (x > 95 && x <= 115) return 1.0;
        else if (x > 115 && x < 125) return (125 - x) / 10.0;
        else return 0.0;
        
    } else if (himp == "SJ") {                // Sangat Jauh: >115 cm
        if (x <= 115) return 0.0;
        else if (x > 115 && x <= 125) return (x - 115) / 10.0;
        else return 1.0;
    }
    return 0.0;
}

// ===== 1. FUZZIFIKASI - Fungsi Keanggotaan Kecepatan =====
// Mengubah nilai crisp RPM → derajat keanggotaan (0-1)
float muKecepatan(float x, String himp) {
    if (himp == "SL") {                       // Sangat Lambat: 0-250 RPM
        if (x <= 150) return 1.0;
        else if (x > 150 && x <= 250) return (250 - x) / 100.0;
        else return 0.0;
        
    } else if (himp == "L") {                 // Lambat: 250-500 RPM
        if (x <= 250 || x >= 500) return 0.0;
        else if (x > 250 && x <= 300) return (x - 250) / 50.0;
        else if (x > 300 && x <= 400) return 1.0;
        else if (x > 400 && x < 500) return (500 - x) / 100.0;
        else return 0.0;
        
    } else if (himp == "S") {                 // Sedang: 500-650 RPM
        if (x <= 500 || x >= 650) return 0.0;
        else if (x > 500 && x <= 530) return (x - 500) / 30.0;
        else if (x > 530 && x <= 620) return 1.0;
        else if (x > 620 && x < 650) return (650 - x) / 30.0;
        else return 0.0;
        
    } else if (himp == "C") {                 // Cepat: 650-800 RPM
        if (x <= 650 || x >= 800) return 0.0;
        else if (x > 650 && x <= 690) return (x - 650) / 40.0;
        else if (x > 690 && x <= 760) return 1.0;
        else if (x > 760 && x < 800) return (800 - x) / 40.0;
        else return 0.0;
        
    } else if (himp == "SC") {                // Sangat Cepat: 800-950 RPM
        if (x <= 800) return 0.0;
        else if (x > 800 && x <= 900) return (x - 800) / 100.0;
        else if (x > 900) return 1.0;
        else return 0.0;
    }
    return 0.0;
}

// ======================= ENCODER (RPM) ======================
volatile unsigned long pulseCount = 0;
unsigned long lastPulseCount = 0;
unsigned long lastTime = 0;
float rpmMotorRaw = 0.0;
float rpmMotor = 0.0;
const float ppr = 10.0;
const float faktorKalibrasi = 1.2;           // Faktor koreksi RPM (sesuaikan)

void IRAM_ATTR hitungPulse() {
    pulseCount++;
}

void hitungRPM() {
    unsigned long now = millis();
    if (now - lastTime >= 1000) {
        unsigned long currentCount;
        noInterrupts();                       // Baca pulse secara aman
        currentCount = pulseCount;
        interrupts();

        unsigned long delta = currentCount - lastPulseCount;
        rpmMotorRaw = (delta / ppr) * 60.0;   // RPM mentah
        rpmMotor = rpmMotorRaw * faktorKalibrasi;  // RPM terkalibrasi

        lastPulseCount = currentCount;
        lastTime = now;
    }
}

// ======================= JARAK (HC-SR04) ====================
float bacaJarak() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long durasi = pulseIn(ECHO_PIN, HIGH, 30000);
    float jarak = durasi * 0.0343 / 2.0;
    if (jarak == 0) jarak = 200;               // Timeout → anggap 200 cm
    return jarak;
}

// ===== 2. RULE BASE + INFERENSI + 3. DEFUZZIFIKASI =====
// Inferensi: Operator AND = MIN
// Defuzzifikasi: Weighted Average
float hitungPWM(float jarak, float rpm) {
    float num = 0.0, den = 0.0;               // Pembilang & penyebut
    
    // Iterasi 25 aturan (5 himpunan jarak × 5 himpunan kecepatan)
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            // 1. FUZZIFIKASI: Hitung derajat keanggotaan
            float muJ = muJarak(jarak, setJarak[i]);
            float muK = muKecepatan(rpm, setKec[j]);
            
            // 2. INFERENSI: Operator AND (MIN) → Fire Strength
            float fire = min(muJ, muK);
            
            // 3. RULE BASE + DEFUZZIFIKASI (Weighted Average)
            if (fire > 0.0) {
                float z = rule[i][j];          // Nilai dari rule base
                num += fire * z;               // Σ (fire × z)
                den += fire;                   // Σ (fire)
        
            }
        }
    }
    
    // 3. DEFUZZIFIKASI: Output crisp (PWM akhir)
    if (den == 0.0) return 0.0;               // Tidak ada rule aktif
    return num / den;                          // PWM dalam %
}

// ======================= SETUP ==============================
void setup() {
    Serial.begin(115200);
    
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    pinMode(ENCODER_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), hitungPulse, RISING);
    lastTime = millis();

    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    digitalWrite(IN1_PIN, HIGH);              // Arah motor: maju
    digitalWrite(IN2_PIN, LOW);

    ledcAttach(ENA_PWM_PIN, 5000, 8);         // PWM: 5kHz, 8-bit
    ledcWrite(ENA_PWM_PIN, 0);
}

// ======================= LOOP UTAMA =========================
float pwmFiltered = 0.0;
const float alpha = 0.3;                      // Koefisien filter (0-1)

void loop() {
    // === INPUT SENSOR ===
    float jarak = bacaJarak();
    hitungRPM();
    
    // === 4. OUTPUT FUZZY ===
    float pwmRaw = hitungPWM(jarak, rpmMotor);  // Hasil defuzzifikasi
    
    // === POST-PROCESSING ===
    pwmFiltered = alpha * pwmRaw + (1 - alpha) * pwmFiltered;  // Low-pass filter
    int pwmVal = constrain((int)(pwmFiltered * 2.55), 0, 255); // % → 0-255
    ledcWrite(ENA_PWM_PIN, pwmVal);           // Output ke motor
    
    // === KATEGORI OUTPUT ===
    String kategori;
    if      (pwmFiltered <= 0.0)  kategori = "STOP";
    else if (pwmFiltered <= 15.0) kategori = "SANGAT LAMBAT";
    else if (pwmFiltered <= 30.0) kategori = "LAMBAT";
    else if (pwmFiltered <= 40.0) kategori = "SEDANG";
    else if (pwmFiltered <= 50.0) kategori = "CEPAT";
    else if (pwmFiltered <= 75.0) kategori = "SANGAT CEPAT";
    else;
    
    // === DEBUG SERIAL ===
    Serial.print("| J:");
    Serial.print(jarak, 1);
    Serial.print("cm RPM_raw:");
    Serial.print(rpmMotorRaw, 1);
    Serial.print(" RPM_kal:");
    Serial.print(rpmMotor, 1);
    Serial.print(" PWM:");
    Serial.print(pwmRaw, 1);
    Serial.print("%->");
    Serial.print(pwmFiltered, 1);
    Serial.print("%(");
    Serial.print(pwmVal);
    Serial.print("/255)");
    Serial.print(" [");
    Serial.print(kategori);
    Serial.println("]");

    delay(100);
}