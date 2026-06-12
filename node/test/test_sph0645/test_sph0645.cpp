//
// Created by Moolinex on 11/06/2026.
// Test unitaire SPH0645 — à placer dans test/test_sph0645/test_sph0645.cpp
//

#include <Arduino.h>
#include <unity.h>
#include "sensors/sph0645.h"

// ─── Helpers ─────────────────────────────────────────────────────────────────

#define TEST_SAMPLES        MIC_BUFFER_SIZE
#define RMS_SILENCE_MAX     0.01f   // seuil max en silence complet
#define RMS_NOISE_MIN       0.001f  // seuil min pour considérer une lecture valide
#define REPEAT_COUNT        10      // nombre de lectures pour les tests de stabilité

// ─── Tests ───────────────────────────────────────────────────────────────────

// 1. Init du driver I2S sans erreur
void test_mic_init() {
    bool ok = micInit();
    TEST_ASSERT_TRUE_MESSAGE(ok, "[MIC] micInit() a retourne false");
}

// 2. micRead retourne un nombre de samples > 0
void test_mic_read_returns_samples() {
    int32_t buf[TEST_SAMPLES];
    size_t n = micRead(buf, TEST_SAMPLES);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, n, "[MIC] micRead() a retourne 0 samples");
}

// 3. micRead retourne exactement le nombre de samples demandes
void test_mic_read_exact_count() {
    int32_t buf[TEST_SAMPLES];
    size_t n = micRead(buf, TEST_SAMPLES);
    TEST_ASSERT_EQUAL_MESSAGE(TEST_SAMPLES, n, "[MIC] micRead() n'a pas retourne le bon nombre de samples");
}

// 4. Le buffer n'est pas entierement a zero (le micro envoie des donnees)
void test_mic_buffer_not_all_zero() {
    int32_t buf[TEST_SAMPLES];
    micRead(buf, TEST_SAMPLES);

    bool all_zero = true;
    for (size_t i = 0; i < TEST_SAMPLES; i++) {
        if (buf[i] != 0) { all_zero = false; break; }
    }
    TEST_ASSERT_FALSE_MESSAGE(all_zero, "[MIC] Tous les samples sont a zero — micro non connecte ou SEL flottant");
}

// 5. micReadRMS retourne une valeur >= 0
void test_mic_rms_non_negative() {
    float rms = micReadRMS(TEST_SAMPLES);
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0.0f, rms, "[MIC] RMS negatif — erreur de calcul");
}

// 6. micReadRMS retourne une valeur <= 1.0
void test_mic_rms_in_range() {
    float rms = micReadRMS(TEST_SAMPLES);
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(1.0f, rms, "[MIC] RMS > 1.0 — saturation ou decalage incorrect");
}

// 7. micReadRMS > 0 (le micro capte quelque chose, meme le bruit ambiant)
void test_mic_rms_above_noise_floor() {
    float rms = micReadRMS(TEST_SAMPLES);
    TEST_ASSERT_GREATER_THAN_MESSAGE(RMS_NOISE_MIN, rms, "[MIC] RMS trop bas — verifier capa 100nF et SEL=GND");
}

// 8. Stabilite : sur REPEAT_COUNT lectures, au moins 80% doivent etre > 0
void test_mic_stability() {
    int valid = 0;
    for (int i = 0; i < REPEAT_COUNT; i++) {
        float rms = micReadRMS(TEST_SAMPLES);
        if (rms > RMS_NOISE_MIN) valid++;
        delay(100);
    }
    int threshold = (REPEAT_COUNT * 80) / 100;  // 80%
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(threshold, valid,
        "[MIC] Trop de lectures nulles — instabilite hardware (capa manquant ?)");
}

// 9. Double init : reinitialiser le driver ne doit pas planter
void test_mic_reinit() {
    bool ok = micInit();
    TEST_ASSERT_TRUE_MESSAGE(ok, "[MIC] Deuxieme micInit() a echoue");
    float rms = micReadRMS(TEST_SAMPLES);
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0.0f, rms, "[MIC] RMS invalide apres reinit");
}

// ─── Entry point ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(2000);  // laisse le temps au moniteur serie de s'ouvrir

    Wire.begin(21, 22);  // necessaire si BMP280 partage le bus

    UNITY_BEGIN();

    RUN_TEST(test_mic_init);
    RUN_TEST(test_mic_read_returns_samples);
    RUN_TEST(test_mic_read_exact_count);
    RUN_TEST(test_mic_buffer_not_all_zero);
    RUN_TEST(test_mic_rms_non_negative);
    RUN_TEST(test_mic_rms_in_range);
    RUN_TEST(test_mic_rms_above_noise_floor);
    RUN_TEST(test_mic_stability);
    RUN_TEST(test_mic_reinit);

    UNITY_END();
}

void loop() {}
