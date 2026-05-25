/**
 * @file lv_i18n.h
 * @brief Internationalization - lightweight gettext-style _(msgid) macro system
 */
#ifndef LV_I18N_H
#define LV_I18N_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

typedef enum {
    LV_I18N_PLURAL_TYPE_ZERO,
    LV_I18N_PLURAL_TYPE_ONE,
    LV_I18N_PLURAL_TYPE_TWO,
    LV_I18N_PLURAL_TYPE_FEW,
    LV_I18N_PLURAL_TYPE_MANY,
    LV_I18N_PLURAL_TYPE_OTHER,
    _LV_I18N_PLURAL_TYPE_NUM,
} lv_i18n_plural_type_t;

typedef struct {
    const char * msg_id;
    const char * translation;
} lv_i18n_phrase_t;

typedef struct {
    const char * locale_name;
    lv_i18n_phrase_t * singulars;
    lv_i18n_phrase_t * plurals[_LV_I18N_PLURAL_TYPE_NUM];
    uint8_t (*locale_plural_fn)(int32_t num);
} lv_i18n_lang_t;

typedef const lv_i18n_lang_t * lv_i18n_language_pack_t;

extern const lv_i18n_language_pack_t lv_i18n_language_pack[];

int lv_i18n_init(const lv_i18n_language_pack_t * langs);
int lv_i18n_set_locale(const char * l_name);
const char * lv_i18n_get_text(const char * msg_id);
const char * lv_i18n_get_text_plural(const char * msg_id, int32_t num);
const char * lv_i18n_get_current_locale(void);

void __lv_i18n_reset(void);

#ifdef __cplusplus
}
#endif

#endif
