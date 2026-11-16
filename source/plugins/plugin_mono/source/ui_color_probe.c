/* Optional metadata inspection only. No UI property setters or patches.
 * Create /data/BalenciHEN/enable_ui_probe before starting this test build. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mono.h"

extern void mono_assembly_foreach(void (*callback)(void *, void *), void *data);
extern const char *mono_image_get_name(void *image);
extern const void *mono_image_get_table_info(void *image, int table);
extern int mono_table_info_get_rows(const void *table);
extern void *mono_class_get(void *image, uint32_t token);
extern const char *mono_class_get_name(void *klass);
extern const char *mono_class_get_namespace(void *klass);
extern void *mono_class_get_methods(void *klass, void **iter);
extern void *mono_class_get_properties(void *klass, void **iter);
extern void *mono_class_get_fields(void *klass, void **iter);
extern const char *mono_method_get_name(void *method);
extern const char *mono_property_get_name(void *property);
extern const char *mono_field_get_name(void *field);
extern void *mono_method_signature(void *method);
extern unsigned mono_signature_get_param_count(void *signature);

struct probe { FILE *log; unsigned lines; unsigned assemblies; };
static int interesting(const char *name) {
    if (!name) return 0;
    return strstr(name, "Theme") || strstr(name, "Color") ||
           strstr(name, "Font") || strstr(name, "Label") ||
           strstr(name, "Style") || strstr(name, "Text");
}
static void inspect_assembly(void *assembly, void *data) {
    struct probe *p = data;
    if (p->lines >= 6000 || p->assemblies++ >= 256) return;
    void *image = mono_assembly_get_image(assembly);
    if (!image) return;
    const char *name = mono_image_get_name(image);
    if (!name || (!strstr(name, "Sce.") && !strstr(name, "app"))) return;
    const void *table = mono_image_get_table_info(image, 2); /* TypeDef */
    if (!table) return;
    int rows = mono_table_info_get_rows(table);
    if (rows < 0 || rows > 50000) return;
    fprintf(p->log, "ASSEMBLY %s types=%d\n", name, rows); ++p->lines;
    for (int row = 1; row <= rows && p->lines < 6000; ++row) {
        void *klass = mono_class_get(image, 0x02000000u | (uint32_t)row);
        if (!klass) continue;
        const char *cn = mono_class_get_name(klass);
        const char *ns = mono_class_get_namespace(klass);
        if (!interesting(cn)) continue;
        fprintf(p->log, "CLASS %s.%s\n", ns ? ns : "", cn ? cn : ""); ++p->lines;
        void *iter = NULL, *member;
        unsigned count = 0;
        while (p->lines < 6000 && count++ < 1024 &&
               (member = mono_class_get_methods(klass, &iter))) {
            const char *mn = mono_method_get_name(member);
            if (!interesting(mn) && !strstr(cn, "Theme")) continue;
            void *sig = mono_method_signature(member);
            fprintf(p->log, " METHOD %s argc=%u\n", mn ? mn : "?",
                    sig ? mono_signature_get_param_count(sig) : 0); ++p->lines;
        }
        iter = NULL; count = 0;
        while (p->lines < 6000 && count++ < 1024 &&
               (member = mono_class_get_properties(klass, &iter))) {
            const char *pn = mono_property_get_name(member);
            if (!interesting(pn) && !strstr(cn, "Theme")) continue;
            fprintf(p->log, " PROPERTY %s\n", pn ? pn : "?"); ++p->lines;
        }
        iter = NULL; count = 0;
        while (p->lines < 6000 && count++ < 1024 &&
               (member = mono_class_get_fields(klass, &iter))) {
            const char *fn = mono_field_get_name(member);
            if (!interesting(fn) && !strstr(cn, "Theme")) continue;
            fprintf(p->log, " FIELD %s\n", fn ? fn : "?"); ++p->lines;
        }
    }
}
void balencihen_probe_ui(void) {
    if (access("/data/BalenciHEN/enable_ui_probe", F_OK) != 0) return;
    unlink("/data/BalenciHEN/enable_ui_probe"); /* One shot, including failed attempts. */
    mkdir("/data/BalenciHEN", 0777);
    FILE *log = fopen("/data/BalenciHEN/ui_color_probe.txt", "w");
    if (!log) return;
    fprintf(log, "BalenciHEN optional UI metadata probe\nNo properties changed. No managed methods invoked.\n");
    struct probe state = { log, 2, 0 };
    mono_assembly_foreach(inspect_assembly, &state);
    fprintf(log, "END lines=%u visited=%u%s\n", state.lines, state.assemblies,
            state.lines >= 6000 ? " (output truncated)" : "");
    fclose(log);
}
