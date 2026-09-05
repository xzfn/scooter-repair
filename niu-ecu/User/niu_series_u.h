#ifndef NIU_SERIES_U_H_
#define NIU_SERIES_U_H_

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

enum {
    DEVICE_BMS = 0x31,
    DEVICE_FOC = 0x20,
    DEVICE_DASH = 0x10
};


// FOC activate step 1
// example: 68 20 df 68 01 01 36 07 16
// 20 01 01 | 03
#define FOC_Unlock_Operation 0x01
struct PayloadFOCUnlockRequest {
    uint8_t req_0;
};
_Static_assert(sizeof(struct PayloadFOCUnlockRequest) == 1, "bad size");
extern const struct PayloadFOCUnlockRequest PayloadFOCUnlockRequestDefault;
struct PayloadFOCUnlockResponse {
    uint8_t serial_number[16];
};


// FOC activate step 2
// example: 68 20 df 68 05 02 a6 b3 2f 16
// 20 05 02 | 73 80
#define FOC_Unlock2_Operation 0x05
struct PayloadFOCUnlock2Request {
    uint8_t crc16_0;
    uint8_t crcl6_1;
};
_Static_assert(sizeof(struct PayloadFOCUnlock2Request) == 2, "bad size");
extern const struct PayloadFOCUnlock2Request PayloadFOCUnlock2RequestDefault;
struct PayloadFOCUnlock2Response {
    uint8_t result;
};


// Dash display
// example: 68 10 ef 68 04 0f 39 23 34 33 5a 61 33 33 33 33 97 33 ff ff 33 27 16
// 10 04 0f | 06 f0 01 00 27 2e 00 00 00 00 64 00 cc cc 00
#define Dash_Display_Operation 0x04
struct PayloadDashDisplayRequest {
    uint8_t flags_0;
    uint8_t flags_1;
    uint8_t drive_mode;
    uint8_t speed;
    uint8_t mileage_0;
    uint8_t mileage_1;
    uint8_t mileage_2;
    uint8_t reserved_0;
    uint8_t reserved_1;
    uint8_t reserved_2;
    uint8_t soc_percent;
    uint8_t reserved_3;
    uint8_t time_hour;
    uint8_t time_minute;
    uint8_t error_code;
};
_Static_assert(sizeof(struct PayloadDashDisplayRequest) == 0x0f, "bad size");
struct PayloadDashDisplayResponse {
    uint8_t result;
};
enum {
    DASH_FLAG_0_CHARGING_MODE = 0x01,
    DASH_FLAG_0_PARKING_LED = 0x04,
    DASH_FLAG_0_ENERGY_RECOVERY_ICON = 0x10,
    DASH_FLAG_0_ECO_MODE_ICON = 0x20,
    DASH_FLAG_0_SHOW_ERROR_CODE = 0x80,  // parking led flash and show error code (last byte in payload)
    DASH_FLAG_1_CRUISE_ICON = 0x01,
    DASH_FLAG_1_UPDATE_MODE = 0x02,  // error code show "UP" and show percent "%"
    DASH_FLAG_1_AUTO_LIGHT_ICON = 0x08,
    DASH_FLAG_1_GPS_ICON_CONSTANT_WITH_SIGNAL = 0x10,
    DASH_FLAG_1_GPS_ICON_SEARCHING_SIGNAL = 0x20,
    DASH_FLAG_1_GPS_ICON_WITHOUT_SIGNAL = 0x30,
    DASH_FLAG_1_GPRS_ICON_CONSTANT_WITH_SIGNAL = 0x40,
    DASH_FLAG_1_GPRS_ICON_SEARCHING_SIGNAL = 0x80,
    DASH_FLAG_1_GPRS_ICON_WITHOUT_SIGNAL = 0xC0
};


// FOC info
// example: 68 20 df 68 02 02 38 3b 46 16
// 20 02 02 | 05 08
#define FOC_Info_Operation 0x02
struct PayloadFOCInfoRequest {
    uint8_t req_0;
    uint8_t req_1;
};
_Static_assert(sizeof(struct PayloadFOCInfoRequest) == 2, "bad size");
extern const struct PayloadFOCInfoRequest PayloadFOCInfoRequestDefault;
// TODO FOC: speed, drive_mode flag, parking flag, enery recovery flag, eco flag, cruise flag
struct PayloadFOCInfoResponse {
    uint8_t reserved_0;
    uint8_t reserved_1;
    uint8_t reserved_2;
    uint8_t flag_0;
    uint8_t reserved_4;
    uint8_t reserved_5;
    uint8_t reserved_6;
    uint8_t reserved_7;
    uint8_t reserved_8;
    uint8_t speed;  // signed int8_t, will be negative when reverse
    uint8_t flag_1;
    uint8_t reserved_11;
};
_Static_assert(sizeof(struct PayloadFOCInfoResponse) == 0x0c, "bad size");
enum {
    FOC_INFO_FLAG_0_DRIVE_MODE_1 = 0x01,
    FOC_INFO_FLAG_0_DRIVE_MODE_2 = 0x02,
    FOC_INFO_FLAG_1_PARKING = 0x08,
    FOC_INFO_FLAG_1_ENERGY_RECOVERY = 0x10
};


// BMS info
// example: 68 31 ce 68 02 02 60 42 75 16
// 31 02 02 | 2d 0f
#define BMS_Info_Operation 0x02
struct PayloadBMSInfoRequest {
    uint8_t req_0;
    uint8_t req_1;
};
_Static_assert(sizeof(struct PayloadFOCInfoRequest) == 2, "bad size");
extern const struct PayloadBMSInfoRequest PayloadBMSInfoRequestDefault;
struct PayloadBMSInfoResponse {
    uint8_t voltage_0;
    uint8_t voltage_1;
    uint8_t reserved_0;
    uint8_t reserved_1;
    uint8_t reserved_2;
    uint8_t current;
    uint8_t soc_percent;
    uint8_t reserved_3;
    uint8_t reserved_4;
    uint8_t reserved_5;
    uint8_t temperature_0;
    uint8_t temperature_1;
    uint8_t temperature_2;
    uint8_t temperature_3;
    uint8_t temperature_4;
};
_Static_assert(sizeof(struct PayloadBMSInfoResponse) == 0x0f, "bad size");


void niu_main();

#endif
