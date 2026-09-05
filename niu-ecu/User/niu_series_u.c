
#include "niu_series_u.h"

#include "niu_controller.h"
#include "rs485.h"
#include "global_data.h"
#include "user_common.h"
#include "adc_util.h"


uint32_t g_mileage = 0;
uint32_t g_last_time = 0;

const struct PayloadFOCUnlockRequest PayloadFOCUnlockRequestDefault = {
    0x03
};

const struct PayloadFOCUnlock2Request PayloadFOCUnlock2RequestDefault = {
    0x73, 0x80
};

const struct PayloadFOCInfoRequest PayloadFOCInfoRequestDefault = {
    0x05, 0x08
};

const struct PayloadBMSInfoRequest PayloadBMSInfoRequestDefault = {
    0x2D, 0x0F
};


int transact_foc_unlock(struct PayloadFOCUnlockResponse *p_response) {

    int out_payload_length = transact_frame(
        DEVICE_FOC, FOC_Unlock_Operation, (uint8_t*)&PayloadFOCUnlockRequestDefault, sizeof(PayloadFOCUnlockRequestDefault),
        (uint8_t*)p_response, sizeof(*p_response)
    );
    if (out_payload_length <= 0) {
        return out_payload_length;
    }
    if (out_payload_length != sizeof(*p_response)) {
        return -10;
    }

    return 1;  // success
}

int transact_foc_unlock2(const struct PayloadFOCUnlock2Request *p_request) {
    struct PayloadFOCUnlock2Response response = {0};

    int out_payload_length = transact_frame(
        DEVICE_FOC, FOC_Unlock2_Operation, (uint8_t*)p_request, sizeof(*p_request),
        (uint8_t*)&response, sizeof(response)
    );
    if (out_payload_length <= 0) {
        return out_payload_length;
    }
    if (out_payload_length != sizeof(response)) {
        return -10;
    }

    return 1;  // success
}

int transact_foc_info(struct PayloadFOCInfoResponse *p_response) {

    int out_payload_length = transact_frame(
        DEVICE_FOC, FOC_Info_Operation, (uint8_t*)&PayloadFOCInfoRequestDefault, sizeof(PayloadFOCInfoRequestDefault),
        (uint8_t*)p_response, sizeof(*p_response)
    );
    if (out_payload_length <= 0) {
        return out_payload_length;
    }
    if (out_payload_length != sizeof(*p_response)) {
        return -10;
    }

    return 1;  // success
}


int transact_bms_info(struct PayloadBMSInfoResponse *p_response) {

    int out_payload_length = transact_frame(
        DEVICE_BMS, BMS_Info_Operation, (uint8_t*)&PayloadBMSInfoRequestDefault, sizeof(PayloadBMSInfoRequestDefault),
        (uint8_t*)p_response, sizeof(*p_response)
    );
    if (out_payload_length <= 0) {
        return out_payload_length;
    }
    if (out_payload_length != sizeof(*p_response)) {
        return -10;
    }

    return 1;  // success
}


int transact_dash_display(struct PayloadDashDisplayRequest *p_request) {
    struct PayloadDashDisplayResponse response = {0};

    int out_payload_length = transact_frame(
        DEVICE_DASH, Dash_Display_Operation, (uint8_t*)p_request, sizeof(*p_request),
        (uint8_t*)&response, sizeof(response)
    );
    if (out_payload_length <= 0) {
        return out_payload_length;
    }
    if (out_payload_length != sizeof(response)) {
        return -10;
    }

    return 1;  // success
}


bool g_foc_unlocked = false;
bool g_bms_ok = false;


int niu_try_unlock(uint8_t try_times, uint32_t retry_duration) {
    int result = 0;
    bool unlock1_success = false;

    struct PayloadFOCUnlockResponse response = {0};

    for (uint8_t i = 0; i < try_times; ++i) {
        result = transact_foc_unlock(&response);
        if (result > 0) {
            unlock1_success = true;
            break;
        }
        delay_ms(retry_duration);
    }

    if (!unlock1_success) {
        return -1;
    }

    uint16_t serial_number_crc16 = calc_serial_number_crc16(response.serial_number, sizeof(response.serial_number));
    struct PayloadFOCUnlock2Request request = {0};
    request.crc16_0 = (serial_number_crc16 >> 8) & 0xFF;
    request.crcl6_1 = serial_number_crc16 & 0xFF;

    delay_ms(100);  // NOTE this delay is needed! Otherwise the following unlock2 receives no response from foc.

    result = transact_foc_unlock2(&request);
    if (result > 0) {
        g_foc_unlocked = true;
        return 1;
    }
    return -2;
}


void niu_start() {
    adc_init();

    niu_try_unlock(6, 500);

    int result = 0;
    struct PayloadBMSInfoResponse bms_response = {0};
    result = transact_bms_info(&bms_response);
    if (result > 0) {
        g_bms_ok = true;
    }

    g_last_time = get_ms();
}


void niu_update() {
    struct PayloadFOCInfoResponse foc_response = {0};
    struct PayloadBMSInfoResponse bms_response = {0};

    bool foc_info_ok = false;
    bool bms_info_ok = false;
    int result = 0;

    uint32_t now = get_ms();
    uint32_t delta_time_ms = now - g_last_time;
    g_last_time = now;

    if (!g_foc_unlocked) {
        niu_try_unlock(1, 0);
    }
    else {
        result = transact_foc_info(&foc_response);
        if (result > 0) {
            foc_info_ok = true;
        }
    }

    if (g_bms_ok) {
        result = transact_bms_info(&bms_response);
        if (result > 0) {
            bms_info_ok = true;
        }
    }

    struct PayloadDashDisplayRequest dash_request = {0};

    // build dash display request
    uint8_t foc_speed = 0;

    if (foc_info_ok) {
        // speed
        if ((int8_t)foc_response.speed >= 0) {
            foc_speed = foc_response.speed;
        }
        else {
            foc_speed = 0;
        }
        dash_request.speed = foc_speed * 12 / 25;
        // drive mode
        if (foc_response.flag_0 & FOC_INFO_FLAG_0_DRIVE_MODE_1) {
            dash_request.drive_mode = 1;
        }
        else if (foc_response.flag_0 & FOC_INFO_FLAG_0_DRIVE_MODE_2) {
            dash_request.drive_mode = 2;
        }
        // parking led
        if (foc_response.flag_1 & FOC_INFO_FLAG_1_PARKING) {
            dash_request.flags_0 |= DASH_FLAG_0_PARKING_LED;
        }
        // energy recovery icon
        if (foc_response.flag_1 & FOC_INFO_FLAG_1_ENERGY_RECOVERY) {
            dash_request.flags_0 |= DASH_FLAG_0_ENERGY_RECOVERY_ICON;
        }
    }
    else {
        dash_request.speed = 0;
        dash_request.flags_0 |= DASH_FLAG_0_SHOW_ERROR_CODE;
        dash_request.error_code = 190;  // FOC communication error
    }

    if (bms_info_ok) {
        // battery soc percent
        dash_request.soc_percent = bms_response.soc_percent;
    }
    else {
        // use adc
        uint32_t adc_val = (uint32_t)adc_measure();
        uint32_t voltage = adc_val / 10;
        if (voltage > 100) {
            voltage = 100;
        }
        if (voltage <= 0) {
            voltage = 1;
        }
        dash_request.soc_percent = (uint8_t)voltage;
    }

    // mileage
    g_mileage += (uint32_t)foc_speed * 12 * delta_time_ms * 10 / 36 / 25;  // (foc_speed * 12 / 25) km/h / 3.6 * dt
    uint32_t mileage_m = g_mileage / 1000 + 1;
    dash_request.mileage_0 = mileage_m & 0xff;
    dash_request.mileage_1 = (mileage_m >> 8) & 0xff;
    dash_request.mileage_2 = (mileage_m >> 16) & 0xff;

    // time
    uint32_t seconds = get_ms() / 1000;
    uint32_t minutes = seconds / 60 + 1;
    dash_request.time_hour = minutes / 60;
    dash_request.time_minute = minutes % 60;

    result = transact_dash_display(&dash_request);
    if (result > 0) {
        // dash display ok
    }

}

void niu_main() {
    niu_start();
    while (1) {
        uint32_t t_begin = get_ms();
        niu_update();
        uint32_t t_end = get_ms();

        if (t_end - t_begin < 200) {
            delay_ms(200 - (t_end - t_begin));
        }
    }
}
