#include "outbox.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <cstring>

#include "sensor_community.h"

namespace {

static constexpr uint16_t kMaxJobs = 32;
static constexpr uint32_t kBackoffBaseMs = 5000;
static constexpr uint32_t kBackoffMaxMs = 300000;
static constexpr uint32_t kQueueMagic = 0x53454E51; // "SENQ"
static constexpr uint8_t kQueueVersion = 1;
static constexpr char kPrefsNamespace[] = "http_outbox";
static constexpr char kPrefsKey[] = "queue";
static constexpr uint8_t kPinBmp280 = 11;
static constexpr uint8_t kPinDust = 1;

#pragma pack(push, 1)
struct PersistedJob {
    uint32_t device_id;
    int16_t temperature_centi;
    uint16_t pressure_hpa;
    uint16_t pm25_deci;
    uint8_t pin;
    uint8_t attempts;
};

struct PersistedQueue {
    uint32_t magic;
    uint8_t version;
    uint8_t reserved;
    uint16_t count;
    PersistedJob jobs[kMaxJobs];
};
#pragma pack(pop)

static Preferences prefs;
static PersistedJob jobs[kMaxJobs];
static uint32_t nextAttemptAt[kMaxJobs];
static uint16_t jobCount = 0;
static bool prefsReady = false;

static uint32_t backoffDelayMs(uint8_t attempts) {
    uint32_t delay = kBackoffBaseMs;
    for (uint8_t i = 1; i < attempts; ++i) {
        if (delay >= kBackoffMaxMs) return kBackoffMaxMs;
        delay *= 2;
    }
    return delay > kBackoffMaxMs ? kBackoffMaxMs : delay;
}

static bool isDue(uint32_t deadline) {
    return (int32_t)(millis() - deadline) >= 0;
}

static void saveQueue() {
    if (!prefsReady) return;

    PersistedQueue state{};
    state.magic = kQueueMagic;
    state.version = kQueueVersion;
    state.count = jobCount;

    for (uint16_t i = 0; i < jobCount; ++i) {
        state.jobs[i] = jobs[i];
    }

    prefs.putBytes(kPrefsKey, &state, sizeof(state));
}

static void resetQueue(bool persist) {
    jobCount = 0;
    memset(jobs, 0, sizeof(jobs));
    memset(nextAttemptAt, 0, sizeof(nextAttemptAt));

    if (persist) saveQueue();
}

static void loadQueue() {
    resetQueue(false);

    size_t len = prefs.getBytesLength(kPrefsKey);
    if (len != sizeof(PersistedQueue)) return;

    PersistedQueue state{};
    if (prefs.getBytes(kPrefsKey, &state, sizeof(state)) != sizeof(state)) return;
    if (state.magic != kQueueMagic || state.version != kQueueVersion) return;

    if (state.count > kMaxJobs) state.count = kMaxJobs;
    jobCount = state.count;

    for (uint16_t i = 0; i < jobCount; ++i) {
        jobs[i] = state.jobs[i];
        nextAttemptAt[i] = 0;
    }
}

static PersistedJob makeJob(uint32_t deviceId, uint8_t pin, int16_t tempCenti, uint16_t pressureHpa, uint16_t pm25Deci) {
    PersistedJob job{};
    job.device_id = deviceId;
    job.temperature_centi = tempCenti;
    job.pressure_hpa = pressureHpa;
    job.pm25_deci = pm25Deci;
    job.pin = pin;
    job.attempts = 0;
    return job;
}

static SensorCommunityJob toScJob(const PersistedJob &job) {
    SensorCommunityJob scJob{};
    scJob.device_id = job.device_id;
    scJob.pin = job.pin;
    scJob.temperature_centi = job.temperature_centi;
    scJob.pressure_hpa = job.pressure_hpa;
    scJob.pm25_deci = job.pm25_deci;
    return scJob;
}

static void dropOldestJob() {
    if (jobCount == 0) return;

    for (uint16_t i = 1; i < jobCount; ++i) {
        jobs[i - 1] = jobs[i];
        nextAttemptAt[i - 1] = nextAttemptAt[i];
    }
    --jobCount;
}

static void enqueueJob(const PersistedJob &job) {
    if (jobCount >= kMaxJobs) {
        Serial.println("[Outbox] file pleine, suppression du plus ancien job");
        dropOldestJob();
    }

    jobs[jobCount] = job;
    nextAttemptAt[jobCount] = 0;
    ++jobCount;
    saveQueue();
}

static void scheduleRetry(uint16_t index) {
    jobs[index].attempts++;
    nextAttemptAt[index] = millis() + backoffDelayMs(jobs[index].attempts);
    saveQueue();
}

} // namespace

void httpOutboxInit() {
    if (!prefsReady) {
        prefsReady = prefs.begin(kPrefsNamespace, false);
    }

    if (!prefsReady) {
        Serial.println("[Outbox] preferences indisponibles");
        return;
    }

    loadQueue();
    Serial.printf("[Outbox] %u job(s) charges\n", jobCount);
}

void httpOutboxEnqueueMeasurement(const SensorPayload &payload) {
    int16_t tempCenti = (int16_t)(payload.temperature * 100.0f);
    uint16_t pressureHpa = (uint16_t)(payload.pressure);
    uint16_t pm25Deci = (uint16_t)(payload.pm25 * 10.0f);

    enqueueJob(makeJob(payload.device_id, kPinBmp280, tempCenti, pressureHpa, pm25Deci));
    enqueueJob(makeJob(payload.device_id, kPinDust, tempCenti, pressureHpa, pm25Deci));

    Serial.printf("[Outbox] mesure %08X mise en file (%u job(s))\n", payload.device_id, jobCount);
}

void httpOutboxTick() {
    if (jobCount == 0) return;
    if (WiFi.status() != WL_CONNECTED) return;
    if (!isDue(nextAttemptAt[0])) return;

    SensorCommunityJob job = toScJob(jobs[0]);
    if (scSendJob(job)) {
        for (uint16_t i = 1; i < jobCount; ++i) {
            jobs[i - 1] = jobs[i];
            nextAttemptAt[i - 1] = nextAttemptAt[i];
        }
        --jobCount;
        saveQueue();
        Serial.printf("[Outbox] job pin %u envoye, reste %u\n", job.pin, jobCount);
        return;
    }

    scheduleRetry(0);
    uint32_t delay = nextAttemptAt[0] - millis();
    Serial.printf("[Outbox] job pin %u en echec, retry dans %lu ms\n", job.pin, (unsigned long)delay);
}

uint16_t httpOutboxPendingCount() {
    return jobCount;
}
