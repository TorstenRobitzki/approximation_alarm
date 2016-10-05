#include <nrf.h>
#include <nrf_sdm.h>
#include <app_error.h>
#include <ble_gap.h>
#include <ble_advdata.h>
#include <softdevice_handler.h>

#include <string.h>

#define DEVICE_NAME                      "Approx Beacon"                            /**< Name of device. Will be included in the advertising data. */

#define MIN_CONN_INTERVAL                MSEC_TO_UNITS(400, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.4 seconds). */
#define MAX_CONN_INTERVAL                MSEC_TO_UNITS(650, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                    0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                 MSEC_TO_UNITS(4000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */

#define ADVERTING_INTERVAL               MSEC_TO_UNITS(50, UNIT_0_625_MS)

#define CENTRAL_LINK_COUNT              0                                           /**< The number of central links used by the application. When changing this number remember to adjust the RAM settings. */
#define PERIPHERAL_LINK_COUNT           1                                           /**< The number of peripheral links used by the application. When changing this number remember to adjust the RAM settings. */
#define VENDOR_SPECIFIC_UUID_COUNT      1                                           /**< The number of vendor specific UUIDs used by this example. */

static const uint8_t beacon_id[] = { 0x01, 0x00, 0x00, 0x00 };

// clock settings
#define NRF_CLOCK_LFCLKSRC  { \
    .source        = NRF_CLOCK_LF_SRC_XTAL,            \
    .rc_ctiv       = 0,                                \
    .rc_temp_ctiv  = 0,                                \
    .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM \
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    for ( ;; )
        ;
}

void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    for ( ;; )
        ;
}

//    app_error_handler(0xDEADBEEF, line_num, p_file_name);

static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    /*
    dm_ble_evt_handler(p_ble_evt);
    ble_db_discovery_on_ble_evt(&m_ble_db_discovery, p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_ancs_c_on_ble_evt(&m_ancs_c, p_ble_evt);
    bsp_btn_ble_on_ble_evt(p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
    */
}

static void sys_evt_dispatch(uint32_t sys_evt)
{
    // pstorage_sys_event_handler(sys_evt);
    // ble_advertising_on_sys_evt(sys_evt);
}


static void ble_stack_init(void)
{
    uint32_t err_code;

    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);

    ble_enable_params.common_enable_params.vs_uuid_count = VENDOR_SPECIFIC_UUID_COUNT;

    //Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);

    // Enable BLE stack.
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for System events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}

static void gap_params_init(void)
{
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    APP_ERROR_CHECK( sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME)) );

    APP_ERROR_CHECK( sd_ble_gap_appearance_set(BLE_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT) );

    static const ble_gap_conn_params_t   gap_conn_params = {
        .min_conn_interval = MIN_CONN_INTERVAL,
        .max_conn_interval = MAX_CONN_INTERVAL,
        .slave_latency     = SLAVE_LATENCY,
        .conn_sup_timeout  = CONN_SUP_TIMEOUT
    };

    APP_ERROR_CHECK( sd_ble_gap_ppcp_set(&gap_conn_params) );
}

static void advertising_start(void)
{
    static const ble_advdata_manuf_data_t identity = {
        .company_identifier = 0x0269, // Torrox GmbH & Co KG
        .data = {
            .p_data = (uint8_t*)beacon_id,
            .size   = sizeof( beacon_id )
        }
    };

    static const ble_advdata_t advertising_data = {
        .name_type              = BLE_ADVDATA_FULL_NAME,
        .flags                  = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED,
        .p_manuf_specific_data  = (ble_advdata_manuf_data_t*)&identity
    };

    APP_ERROR_CHECK( ble_advdata_set( &advertising_data, NULL ) );

    static const ble_gap_adv_params_t params = {
        .interval = ADVERTING_INTERVAL
    };

    APP_ERROR_CHECK( sd_ble_gap_adv_start( &params ) );
}

int main()
{
    ble_stack_init();
    gap_params_init();

    advertising_start();

    for (;;)
    {
        APP_ERROR_CHECK( sd_app_evt_wait() );
    }

    return 0;
}