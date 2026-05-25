#include "lv_i18n.h"

#define UNUSED(x) (void)(x)

static inline uint32_t op_n(int32_t val) { return (uint32_t)(val < 0 ? -val : val); }
static inline uint32_t op_i(uint32_t val) { return val; }
static inline uint32_t op_v(uint32_t val) { UNUSED(val); return 0;}
static inline uint32_t op_w(uint32_t val) { UNUSED(val); return 0; }
static inline uint32_t op_f(uint32_t val) { UNUSED(val); return 0; }
static inline uint32_t op_t(uint32_t val) { UNUSED(val); return 0; }

static lv_i18n_phrase_t en_us_singulars[] = {
    /* --- X-TRACK original strings --- */
    {"EPHEMERIS", "Ephemeris"},
    {"LOAD_SUCCESS", "Load Success"},
    {"LOW_BATTERY", "Low Battery"},
    {"SHUTDOWN?", "Shutdown?"},
    {"NO_OPERATION", "No Operation"},
    {"NO", "No"},
    {"YES", "Yes"},
    {"START_RECORD", "Start Record"},
    {"STOP_RECORD", "Stop Record"},
    {"GNSS_NOT_READY", "GNSS Not Ready"},
    {"OPEN_FILE_FAILED", "Open File Failed"},
    {"AVG_SPEED", "AVG"},
    {"TIME", "Time"},
    {"DISTANCE", "Distance"},
    {"CALORIES", "Calories"},
    {"LOADING...", "Loading..."},
    {"POWER_OFF", "Power Off"},
    {"SPORT", "Sport"},
    {"GNSS", "GNSS"},
    {"BATTERY", "Battery"},
    {"ABOUT", "About"},
    {"TOTAL_TRIP", "Total Trip"},
    {"TOTAL_TIME", "Total Time"},
    {"MAX_SPEED", "Max Speed"},
    {"LATITUDE", "Latitude"},
    {"LONGITUDE", "Longitude"},
    {"ALTITUDE", "Altitude"},
    {"UTC_TIME", "UTC Time"},
    {"COURSE", "Course"},
    {"SPEED", "Speed"},
    {"DATE", "Date"},
    {"USAGE", "Usage"},
    {"VOLTAGE", "Voltage"},
    {"STATUS", "Status"},
    {"CHARGE", "Charge"},
    {"DISCHARGE", "Discharge"},
    {"NAME", "Name"},
    {"AUTHOR", "Author"},
    {"GRAPHICS", "GUI"},
    {"COMPILER", "Compiler"},
    {"BUILD_DATE", "Build"},
    /* --- Demo app strings --- */
    {"LVGL Framework Demo", "LVGL Framework Demo"},
    {"Comprehensive Feature Demo", "Comprehensive Feature Demo"},
    {"PageManager + DataBroker + DeviceManager", "PageManager + DataBroker + DeviceManager"},
    {"Dashboard", "Dashboard"},
    {"Device List", "Device List"},
    {"Chart", "Chart"},
    {"Settings", "Settings"},
    {"Back", "Back"},
    {"Device Dashboard", "Device Dashboard"},
    {"FPS:", "FPS:"},
    {"GNSS Receiver", "GNSS Receiver"},
    {"Battery Level", "Battery Level"},
    {"Heart Rate", "Heart Rate"},
    {"Speed Sensor", "Speed Sensor"},
    {"Temperature", "Temperature"},
    {"Initialized", "Initialized"},
    {"Real-time Chart", "Real-time Chart"},
    {"GNSS Altitude", "GNSS Altitude"},
    {"Heart Rate (filtered)", "Heart Rate (filtered)"},
    {"Device Toggle", "Device Toggle"},
    {"Language", "Language"},
    {"About Framework", "About Framework"},
    {"Framework Version", "Framework Version"},
    {"Built with LVGL v8.2", "Built with LVGL v8.2"},
    {"SDL2 PC Simulator", "SDL2 PC Simulator"},
    {"Screen: 480x320", "Screen: 480x320"},
    {"Toast demo", "Toast demo"},
    {"Show Toast", "Show Toast"},
    {"Hello from LVGL Framework!", "Hello from LVGL Framework!"},
    {"Refresh", "Refresh"},
    {"Click Me!", "Click Me!"},
    {"Click Count:", "Click Count:"},
    {"Framework Modules:", "Framework Modules:"},
    {"Simulated Devices:", "Simulated Devices:"},
    {NULL, NULL}
};

static uint8_t en_us_plural_fn(int32_t num)
{
    uint32_t n = op_n(num); UNUSED(n);
    uint32_t i = op_i(n); UNUSED(i);
    uint32_t v = op_v(n); UNUSED(v);
    if ((i == 1 && v == 0)) return LV_I18N_PLURAL_TYPE_ONE;
    return LV_I18N_PLURAL_TYPE_OTHER;
}

static const lv_i18n_lang_t en_us_lang = {
    .locale_name = "en-US",
    .singulars = en_us_singulars,
    .locale_plural_fn = en_us_plural_fn
};

static lv_i18n_phrase_t zh_cn_singulars[] = {
    /* --- X-TRACK original strings --- */
    {"EPHEMERIS", "[CN] Ephemeris"},
    {"LOAD_SUCCESS", "[CN] Load Success"},
    {"LOW_BATTERY", "[CN] Low Battery"},
    {"SHUTDOWN?", "[CN] Shutdown?"},
    {"NO_OPERATION", "[CN] No Operation"},
    {"NO", "[CN] No"},
    {"YES", "[CN] Yes"},
    {"START_RECORD", "[CN] Start Record"},
    {"STOP_RECORD", "[CN] Stop Record"},
    {"GNSS_NOT_READY", "[CN] GNSS Not Ready"},
    {"OPEN_FILE_FAILED", "[CN] Open File Failed"},
    {"AVG_SPEED", "[CN] AVG"},
    {"TIME", "[CN] Time"},
    {"DISTANCE", "[CN] Distance"},
    {"CALORIES", "[CN] Calories"},
    {"LOADING...", "[CN] Loading..."},
    {"POWER_OFF", "[CN] Power Off"},
    {"SPORT", "[CN] Sport"},
    {"GNSS", "[CN] GNSS"},
    {"BATTERY", "[CN] Battery"},
    {"ABOUT", "[CN] About"},
    {"TOTAL_TRIP", "[CN] Total Trip"},
    {"TOTAL_TIME", "[CN] Total Time"},
    {"MAX_SPEED", "[CN] Max Speed"},
    {"LATITUDE", "[CN] Latitude"},
    {"LONGITUDE", "[CN] Longitude"},
    {"ALTITUDE", "[CN] Altitude"},
    {"UTC_TIME", "[CN] UTC Time"},
    {"COURSE", "[CN] Course"},
    {"SPEED", "[CN] Speed"},
    {"DATE", "[CN] Date"},
    {"USAGE", "[CN] Usage"},
    {"VOLTAGE", "[CN] Voltage"},
    {"STATUS", "[CN] Status"},
    {"CHARGE", "[CN] Charge"},
    {"DISCHARGE", "[CN] Discharge"},
    {"NAME", "[CN] Name"},
    {"AUTHOR", "[CN] Author"},
    {"GRAPHICS", "[CN] GUI"},
    {"COMPILER", "[CN] Compiler"},
    {"BUILD_DATE", "[CN] Build"},
    /* --- Demo app strings --- */
    {"LVGL Framework Demo", "[CN] LVGL Framework Demo"},
    {"Comprehensive Feature Demo", "[CN] Comprehensive Feature Demo"},
    {"PageManager + DataBroker + DeviceManager", "[CN] PageManager + DataBroker + DeviceManager"},
    {"Dashboard", "[CN] Dashboard"},
    {"Device List", "[CN] Device List"},
    {"Chart", "[CN] Chart"},
    {"Settings", "[CN] Settings"},
    {"Back", "[CN] Back"},
    {"Device Dashboard", "[CN] Device Dashboard"},
    {"FPS:", "[CN] FPS:"},
    {"GNSS Receiver", "[CN] GNSS Receiver"},
    {"Battery Level", "[CN] Battery Level"},
    {"Heart Rate", "[CN] Heart Rate"},
    {"Speed Sensor", "[CN] Speed Sensor"},
    {"Temperature", "[CN] Temperature"},
    {"Initialized", "[CN] Initialized"},
    {"Real-time Chart", "[CN] Real-time Chart"},
    {"GNSS Altitude", "[CN] GNSS Altitude"},
    {"Heart Rate (filtered)", "[CN] Heart Rate (filtered)"},
    {"Device Toggle", "[CN] Device Toggle"},
    {"Language", "[CN] Language"},
    {"About Framework", "[CN] About Framework"},
    {"Framework Version", "[CN] Framework Version"},
    {"Built with LVGL v8.2", "[CN] Built with LVGL v8.2"},
    {"SDL2 PC Simulator", "[CN] SDL2 PC Simulator"},
    {"Screen: 480x320", "[CN] Screen: 480x320"},
    {"Toast demo", "[CN] Toast demo"},
    {"Show Toast", "[CN] Show Toast"},
    {"Hello from LVGL Framework!", "[CN] Hello from LVGL Framework!"},
    {"Refresh", "[CN] Refresh"},
    {"Click Me!", "[CN] Click Me!"},
    {"Click Count:", "[CN] Click Count:"},
    {"Framework Modules:", "[CN] Framework Modules:"},
    {"Simulated Devices:", "[CN] Simulated Devices:"},
    {NULL, NULL}
};

static uint8_t zh_cn_plural_fn(int32_t num)
{
    return LV_I18N_PLURAL_TYPE_OTHER;
}

static const lv_i18n_lang_t zh_cn_lang = {
    .locale_name = "zh-CN",
    .singulars = zh_cn_singulars,
    .locale_plural_fn = zh_cn_plural_fn
};

const lv_i18n_language_pack_t lv_i18n_language_pack[] = {
    &en_us_lang,
    &zh_cn_lang,
    NULL
};

static const lv_i18n_language_pack_t * current_lang_pack;
static const lv_i18n_lang_t * current_lang;

void __lv_i18n_reset(void)
{
    current_lang_pack = NULL;
    current_lang = NULL;
}

int lv_i18n_init(const lv_i18n_language_pack_t * langs)
{
    if(langs == NULL) return -1;
    if(langs[0] == NULL) return -1;
    current_lang_pack = langs;
    current_lang = langs[0];
    return 0;
}

int lv_i18n_set_locale(const char * l_name)
{
    if(current_lang_pack == NULL) return -1;
    uint16_t i;
    for(i = 0; current_lang_pack[i] != NULL; i++) {
        if(strcmp(current_lang_pack[i]->locale_name, l_name) == 0) {
            current_lang = current_lang_pack[i];
            return 0;
        }
    }
    return -1;
}

static const char * __lv_i18n_get_text_core(lv_i18n_phrase_t * trans, const char * msg_id)
{
    uint16_t i;
    for(i = 0; trans[i].msg_id != NULL; i++) {
        if(strcmp(trans[i].msg_id, msg_id) == 0) {
            if(trans[i].translation) return trans[i].translation;
        }
    }
    return NULL;
}

const char * lv_i18n_get_text(const char * msg_id)
{
    if(current_lang == NULL) return msg_id;
    const lv_i18n_lang_t * lang = current_lang;
    const void * txt;

    if(lang->singulars != NULL) {
        txt = __lv_i18n_get_text_core(lang->singulars, msg_id);
        if (txt != NULL) return txt;
    }

    if(lang == current_lang_pack[0]) return msg_id;
    lang = current_lang_pack[0];

    if(lang->singulars != NULL) {
        txt = __lv_i18n_get_text_core(lang->singulars, msg_id);
        if (txt != NULL) return txt;
    }
    return msg_id;
}

const char * lv_i18n_get_text_plural(const char * msg_id, int32_t num)
{
    if(current_lang == NULL) return msg_id;
    const lv_i18n_lang_t * lang = current_lang;
    const void * txt;
    lv_i18n_plural_type_t ptype;

    if(lang->locale_plural_fn != NULL) {
        ptype = lang->locale_plural_fn(num);
        if(lang->plurals[ptype] != NULL) {
            txt = __lv_i18n_get_text_core(lang->plurals[ptype], msg_id);
            if (txt != NULL) return txt;
        }
    }

    if(lang == current_lang_pack[0]) return msg_id;
    lang = current_lang_pack[0];

    if(lang->locale_plural_fn != NULL) {
        ptype = lang->locale_plural_fn(num);
        if(lang->plurals[ptype] != NULL) {
            txt = __lv_i18n_get_text_core(lang->plurals[ptype], msg_id);
            if (txt != NULL) return txt;
        }
    }
    return msg_id;
}

const char * lv_i18n_get_current_locale(void)
{
    if(!current_lang) return NULL;
    return current_lang->locale_name;
}
