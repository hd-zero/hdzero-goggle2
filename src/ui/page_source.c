#include "page_source.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <minIni.h>

#include "core/app_state.h"
#include "core/common.hh"
#include "core/dvr.h"
#include "core/osd.h"
#include "core/settings.h"
#include "driver/beep.h"
#include "driver/hardware.h"
#include "driver/it66121.h"
#include "driver/oled.h"
#include "lang/language.h"
#include "ui/page_common.h"
#include "ui/page_scannow.h"
#include "ui/ui_main_menu.h"
#include "ui/ui_porting.h"
#include "ui/ui_style.h"
#include <log/log.h>

// local
static lv_coord_t col_dsc[] = {160, 200, 200, 160, 160, 160, LV_GRID_TEMPLATE_LAST};
static lv_coord_t row_dsc[] = {60, 60, 60, 60, 60, 60, 60, 60, 60, 60, LV_GRID_TEMPLATE_LAST};

typedef enum {
    LABEL_HDZERO,
    LABEL_ANALOG,
    LABEL_HDMI,
    LABEL_AVIN,
    LABEL_OLED,

    LABEL_MAX,
} source_type_t;

static lv_obj_t *label[LABEL_MAX];

static uint8_t oled_tst_mode = 0; // 0=Normal,1=CB; 2-Grid; 3=All Black; 4=All White,5=Boot logo
static bool in_sourcepage = false;
static btn_group_t btn_group0, btn_group1, btn_group2, btn_group3, btn_group4;

static lv_obj_t *page_source_create(lv_obj_t *parent, panel_arr_t *arr) {
    char buf[128];
    int row = 0;

    lv_obj_t *page = lv_menu_page_create(parent, NULL);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(page, 1053, 900);
    lv_obj_add_style(page, &style_subpage, LV_PART_MAIN);
    lv_obj_set_style_pad_top(page, 94, 0);

    lv_obj_t *section = lv_menu_section_create(page);
    lv_obj_add_style(section, &style_submenu, LV_PART_MAIN);
    lv_obj_set_size(section, 1053, 894);

    snprintf(buf, sizeof(buf), "%s:", _lang("Source"));
    create_text(NULL, section, false, buf, LV_MENU_ITEM_BUILDER_VARIANT_2);

    lv_obj_t *cont = lv_obj_create(section);
    lv_obj_set_size(cont, 960, 894);
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont, &style_context, LV_PART_MAIN);

    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);

    create_select_item(arr, cont);

    label[LABEL_HDZERO] = create_label_item(cont, "HDZero", 1, row++, 3);
    if (GOGGLE_VER_2) {
        label[LABEL_ANALOG] = create_label_item(cont, "Analog", 1, row++, 3);
    } else {
        label[LABEL_ANALOG] = create_label_item(cont, _lang("Expansion Module"), 1, row++, 3);
    }
    snprintf(buf, sizeof(buf), "HDMI %s", _lang("In"));
    label[LABEL_HDMI] = create_label_item(cont, buf, 1, row++, 3);
    snprintf(buf, sizeof(buf), "AV %s", _lang("In"));
    label[LABEL_AVIN] = create_label_item(cont, buf, 1, row++, 3);

    create_btn_group_item(&btn_group0, cont, 2, _lang("HDZero Band"), _lang("Raceband"), _lang("Lowband"), "", "", row++);
    btn_group_set_sel(&btn_group0, g_setting.source.hdzero_band);

    create_btn_group_item(&btn_group1, cont, 2, _lang("HDZero BW"), _lang("Wide"), _lang("Narrow"), "", "", row++);
    btn_group_set_sel(&btn_group1, g_setting.source.hdzero_bw);

    if (GOGGLE_VER_2 == 1) {
        create_btn_group_item(&btn_group2, cont, 2, _lang("Analog Module"), _lang("Built-in"), _lang("Expansion"), "", "", row++);
        btn_group_set_sel(&btn_group2, g_setting.source.analog_module);
    } else {
        create_btn_group_item(&btn_group2, cont, 2, _lang("Analog Video"), _lang("NTSC"), _lang("PAL"), "", "", row++);
        btn_group_set_sel(&btn_group2, g_setting.source.analog_format);
    }

    create_btn_group_item(&btn_group3, cont, 2, _lang("Analog Ratio"), _lang("4:3"), _lang("16:9"), "", "", row++);
    btn_group_set_sel(&btn_group3, g_setting.source.analog_ratio);

    snprintf(buf, sizeof(buf), "< %s", _lang("Back"));
    if (g_setting.storage.selftest) {
        label[LABEL_OLED] = create_label_item(cont, "OLED Pattern: Normal", 1, row++, 3);
        create_label_item(cont, buf, 1, row++, 3);
    } else {
        label[LABEL_OLED] = NULL;
        create_label_item(cont, buf, 1, row++, 3);
    }
    pp_source.p_arr.max = row;
    return page;
}

char *state2string(uint8_t status) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "#%s %s#", status ? "00FF00" : "C0C0C0", status ? _lang("Connected") : _lang("Disconnected"));
    return buf;
}

void source_status_timer() {
    char band_str[7] = "ABEFRL";
    char buf[64];
    int band, ch;

    if (!in_sourcepage)
        return;

    // hdzero
    ch = g_setting.scan.channel & 0x7F;
    if (g_setting.source.hdzero_band == SETTING_SOURCES_HDZERO_BAND_RACEBAND) {
        if (ch <= 8) {
            snprintf(buf, sizeof(buf), "HDZero: R%d", ch);
        } else if (ch <= 12) {
            snprintf(buf, sizeof(buf), "HDZero: F%d", (ch - 8) * 2);
        } else {
            g_setting.scan.channel = 1;
            snprintf(buf, sizeof(buf), "HDZero: R1");
        }
    } else {
        if (ch > 8) {
            g_setting.scan.channel = 1;
        }
        snprintf(buf, sizeof(buf), "HDZero: L%d", ch);
    }
    lv_label_set_text(label[LABEL_HDZERO], buf);

    // analog
    if (GOGGLE_VER_2) {
        if (g_setting.source.analog_module == SETTING_SOURCES_ANALOG_MODULE_INTERNAL) {
            snprintf(buf, sizeof(buf), "%s: %s", _lang("Analog"), channel2str(0, 0, g_setting.source.analog_channel));
        } else {
            snprintf(buf, sizeof(buf), "%s: %s", _lang("Analog"), _lang("Expansion Module"));
        }
    } else {
        snprintf(buf, sizeof(buf), "%s: %s", _lang("Expansion Module"), state2string(g_source_info.av_bay_status));
    }
    lv_label_set_text(label[LABEL_ANALOG], buf);

    // hdmi in
    snprintf(buf, sizeof(buf), "HDMI %s: %s", _lang("In"), state2string(g_source_info.hdmi_in_status));
    lv_label_set_text(label[LABEL_HDMI], buf);

    // av in
    snprintf(buf, sizeof(buf), "AV %s: %s", _lang("In"), state2string(g_source_info.av_in_status));
    lv_label_set_text(label[LABEL_AVIN], buf);

    if (g_setting.storage.selftest && label[LABEL_AVIN]) {
        uint8_t oled_tm = oled_tst_mode & 0x0F;
        char *pattern_label[6] = {"Normal", "Color Bar", "Grid", "All Black", "All White", "Boot logo"};
        char str[32];
        snprintf(str, sizeof(buf), "OLED Pattern: %s", pattern_label[oled_tm]);
        lv_label_set_text(label[LABEL_OLED], str);
    }
}

static void page_source_select_hdzero() {
    progress_bar.start = 1;
    app_switch_to_hdzero(true);
    app_state_push(APP_STATE_VIDEO);
    g_source_info.source = SOURCE_HDZERO;
    dvr_select_audio_source(g_setting.record.audio_source);
    dvr_enable_line_out(true);
}

static void page_source_select_hdmi_in() {
    if (g_source_info.hdmi_in_status)
        app_switch_to_hdmi_in();
}

static void page_source_select_av_in() {
    app_switch_to_av_in();
    app_state_push(APP_STATE_VIDEO);
    g_source_info.source = SOURCE_AV_IN;
    dvr_select_audio_source(g_setting.record.audio_source);
    dvr_enable_line_out(true);
}

static void page_source_select_analog(bool force_external) {
    app_switch_to_analog(force_external);
    app_state_push(APP_STATE_VIDEO);
    g_source_info.source = SOURCE_ANALOG;
    dvr_select_audio_source(g_setting.record.audio_source);
    dvr_enable_line_out(true);
}

void source_toggle() {
    beep_dur(BEEP_SHORT);
    switch (g_source_info.source) {
    case SOURCE_HDZERO:
        page_source_select_analog(false);
        break;
    case SOURCE_ANALOG:
        page_source_select_hdzero();
        break;
    case SOURCE_AV_IN:
        page_source_select_hdzero();
        break;
    case SOURCE_HDMI_IN:
        page_source_select_hdzero();
        break;
    }
}

void source_cycle() {
    beep_dur(BEEP_SHORT);
    switch (g_source_info.source) {
    case SOURCE_HDZERO:
        if (g_source_info.hdmi_in_status) {
            page_source_select_hdmi_in();
        } else {
            page_source_select_av_in();
        }
        break;
    case SOURCE_ANALOG:
        page_source_select_hdzero();
        break;
    case SOURCE_AV_IN:
        page_source_select_analog(false);
        break;
    case SOURCE_HDMI_IN:
        page_source_select_av_in();
        break;
    }
}

#include "driver/rtc6715.h"
void source_toggle_cam() {
    static bool camera_enabled = false;
    static source_info_t source_info_backup;

    if (!camera_enabled) {
        camera_enabled = true;
        source_info_backup = g_source_info;

        if (g_source_info.source == SOURCE_ANALOG) {
            RTC6715_Open(0, 0);
        } else {
            page_source_select_analog(true);
        }
    } else {
        camera_enabled = false;
        g_source_info = source_info_backup;
        switch (g_source_info.source) {
        case SOURCE_HDZERO:
            page_source_select_hdzero();
            break;
        case SOURCE_ANALOG:
            // page_source_select_analog(false);
            RTC6715_Open(1, g_setting.record.audio_source == SETTING_RECORD_AUDIO_SOURCE_AV_IN);
            RTC6715_SetCH(g_setting.source.analog_channel - 1);
            break;
        case SOURCE_AV_IN:
            page_source_select_av_in();
            break;
        case SOURCE_HDMI_IN:
            page_source_select_hdmi_in();
            break;
        }
    }
}

static void page_source_on_click(uint8_t key, int sel) {
    switch (sel) {
    case 0: // HDZero in
        page_source_select_hdzero();
        break;

    case 1: // Analog
        page_source_select_analog(false);
        break;

    case 2: // hdmi_in
        page_source_select_hdmi_in();
        break;

    case 3: // av in
        page_source_select_av_in();
        break;

    case 4: // HDZero band format
        btn_group_toggle_sel(&btn_group0);
        g_setting.source.hdzero_band = btn_group_get_sel(&btn_group0);
        page_scannow_set_channel_label();
        ini_putl("source", "hdzero_band", g_setting.source.hdzero_band, SETTING_INI);
        break;

    case 5: // HDZero bw format
        btn_group_toggle_sel(&btn_group1);
        g_setting.source.hdzero_bw = btn_group_get_sel(&btn_group1);
        ini_putl("source", "hdzero_bw", g_setting.source.hdzero_bw, SETTING_INI);
        break;

    // 1V0: Analog format
    // 1V1: Analog module
    case 6:
        if (GOGGLE_VER_2) {
            btn_group_toggle_sel(&btn_group2);
            g_setting.source.analog_module = btn_group_get_sel(&btn_group2);
            LOGI("g_setting.source.analog_module = %d", g_setting.source.analog_module);
            ini_putl("source", "analog_module", g_setting.source.analog_module, SETTING_INI);
            Analog_Module_Power(0, 0);
        } else {
            btn_group_toggle_sel(&btn_group2);
            g_setting.source.analog_format = btn_group_get_sel(&btn_group2);
            ini_putl("source", "analog_format", g_setting.source.analog_format, SETTING_INI);
        }
        break;

    case 7: // analog ratio
        btn_group_toggle_sel(&btn_group3);
        g_setting.source.analog_ratio = btn_group_get_sel(&btn_group3);
        ini_putl("source", "analog_ratio", g_setting.source.analog_ratio, SETTING_INI);
        break;

    case 8:
        if (g_setting.storage.selftest && label[LABEL_OLED]) {
            uint8_t oled_te = (oled_tst_mode != 0);
            uint8_t oled_tm = (oled_tst_mode & 0x0F) - 1;
            // LOGI("OLED TE=%d,TM=%d",oled_te,oled_tm);
            OLED_Pattern(oled_te, oled_tm, 4);
            oled_tst_mode++;
            if (oled_tst_mode >= 6)
                oled_tst_mode = 0;
            break;
        }
    }
}

static void page_source_enter() {
    in_sourcepage = true;
}

static void page_source_exit() {
    // LOGI("page_source_exit %d",oled_tst_mode);
    if ((oled_tst_mode != 0) && g_setting.storage.selftest) {
        OLED_Pattern(0, 0, 4);
        oled_tst_mode = 0;
    }
    in_sourcepage = false;
}

page_pack_t pp_source = {
    .p_arr = {
        .cur = 0,
        .max = 6, // .max will update when page_source_create()
    },

    .name = "Source",
    .create = page_source_create,
    .enter = page_source_enter,
    .exit = page_source_exit,
    .on_created = NULL,
    .on_update = NULL,
    .on_roller = NULL,
    .on_click = page_source_on_click,
    .on_right_button = NULL,
};
