/* BalenciHEN Settings cheat menu. Uses the HEN's own 107/108/109 syscalls.
 * No hardcoded game addresses or writes to other processes' arbitrary maps. */
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include "../../../common/syscall.h"
#include "../../../plugin_shellcore/source/local_appinfo.h"

#ifndef CHEAT_ROOT
#define CHEAT_ROOT "/data/BalenciHEN/cheats"
#endif
#ifndef BH_APPINFO_ROOT
#define BH_APPINFO_ROOT "/user" TEMP_INFO_PATH
#endif
struct bh_proc { char name[32]; int pid; } __attribute__((packed));
struct bh_info { int pid; char name[40], path[64], title[16], content[64]; } __attribute__((packed));
struct bh_map { char name[32]; uint64_t start,end,offset; uint16_t prot; } __attribute__((packed));
struct bh_maps { struct bh_map *maps; uint64_t count; } __attribute__((packed));
_Static_assert(sizeof(struct bh_proc)==36, "Process list ABI");
_Static_assert(sizeof(struct bh_map)==58, "Memory map ABI");
_Static_assert(sizeof(struct bh_info)==188, "Process identity ABI");
static bh_db *active_db;
static unsigned long generation;
static int active_pid;
static char active_title[16], active_version[8];
static struct bh_map active_maps[512];
static size_t active_map_count;
static uint64_t active_base;
static char last_result[256]="No cheat selected";
static char menu[524288];
static size_t used;
static void emit(const char *fmt,...) {
    if(used>=sizeof(menu)-1) return;
    va_list ap;va_start(ap,fmt);int n=vsnprintf(menu+used,sizeof(menu)-used,fmt,ap);va_end(ap);
    if(n>0) used+=(size_t)n<sizeof(menu)-used?(size_t)n:sizeof(menu)-used-1;
}
static void escaped(const char *s) {
    for(;s && *s;++s) switch(*s) {
        case '&':emit("&amp;");break;case '<':emit("&lt;");break;case '>':emit("&gt;");break;
        case '"':emit("&quot;");break;case '\'':emit("&apos;");break;
        default: if((unsigned char)*s>=32) emit("%c",*s);break;
    }
}
static void message(const char *s) {emit("<message title=\"");escaped(s);emit("\"/>\n");}
static int live_info(int pid,struct bh_info *info) {
    memset(info,0,sizeof(*info));
    if(orbis_syscall(109,(uint64_t)pid,(uint64_t)8,info)) return -1;
    info->name[39]=0;info->title[15]=0;info->path[63]=0;
    return 0;
}
static int valid_title(const char *s) {
    if(strlen(s)!=9 || strncmp(s,"CUSA",4)) return 0;
    for(int i=4;i<9;++i) if(!isdigit((unsigned char)s[i])) return 0;
    return 1;
}
static int read_identity(int pid,const char *title,struct disk_appinfo *disk) {
    char path[256];snprintf(path,sizeof(path),BH_APPINFO_ROOT "/%s/" TEMP_INFO_FILE,title);
    FILE *f=fopen(path,"rb");if(!f) return -1;
    size_t n=fread(disk,1,sizeof(*disk),f);fclose(f);
    if(n!=sizeof(*disk) || !memchr(disk->m_titleid,0,sizeof(disk->m_titleid)) ||
       !memchr(disk->m_appver,0,sizeof(disk->m_appver)) || disk->pid!=pid || strcmp(title,disk->m_titleid)) return -1;
    const char *v=disk->m_appver;
    return strlen(v)==5 && isdigit(v[0]) && isdigit(v[1]) && v[2]=='.' && isdigit(v[3]) && isdigit(v[4])?0:-1;
}
static int game_map(const struct bh_map *m) {
    return !strncmp(m->name,"executable",10) || !strcmp(m->name,"eboot.bin") || !strcmp(m->name,"eboot");
}
static int refresh_maps(int pid) {
    struct bh_maps maps={NULL,0};
    if(orbis_syscall(109,(uint64_t)pid,(uint64_t)4,&maps) || maps.count<1 || maps.count>512) return -1;
    memset(active_maps,0,sizeof(active_maps)); maps.maps=active_maps;
    if(orbis_syscall(109,(uint64_t)pid,(uint64_t)4,&maps) || maps.count>512) return -1;
    active_map_count=(size_t)maps.count;active_base=0;
    for(size_t i=0;i<active_map_count;++i) {
        struct bh_map *m=&active_maps[i];m->name[31]=0;
        if(game_map(m) && !m->offset && (m->prot&4) && m->end>m->start &&
           (!active_base || m->start<active_base)) active_base=m->start;
    }
    return active_base?0:-1;
}
static void forget_game(void) {
    ++generation;bh_db_close(active_db);active_db=NULL;active_pid=0;active_title[0]=active_version[0]=0;
}
static int attach_game(void) {
    uint64_t count=0;
    if(orbis_syscall(107,NULL,&count) || !count || count>2048) {forget_game();snprintf(last_result,sizeof(last_result),"Process listing unavailable; use this build after a fresh reboot");return -1;}
    struct bh_proc *procs=calloc((size_t)count,sizeof(*procs));if(!procs) return -1;
    if(orbis_syscall(107,procs,&count)) {free(procs);forget_game();return -1;}
    struct bh_info chosen={0};struct disk_appinfo disk={0};int candidates=0;
    for(size_t i=0;i<count;++i) {
        procs[i].name[31]=0;
        if(strcmp(procs[i].name,"eboot.bin") && strcmp(procs[i].name,"eboot")) continue;
        struct bh_info info;struct disk_appinfo di;
        if(live_info(procs[i].pid,&info) || !valid_title(info.title) || read_identity(procs[i].pid,info.title,&di)) continue;
        chosen=info;chosen.pid=procs[i].pid;disk=di;++candidates;
    } free(procs);
    if(candidates!=1) {forget_game();snprintf(last_result,sizeof(last_result),"Launch one supported game (eboot.bin); exact title/version metadata is required");return -1;}
    uint64_t old_base=active_base;
    if(refresh_maps(chosen.pid)) {forget_game();snprintf(last_result,sizeof(last_result),"Cannot identify the game's executable mapping; no memory writes permitted");return -1;}
    if(active_db && active_pid==chosen.pid && !strcmp(active_title,chosen.title) && !strcmp(active_version,disk.m_appver) && old_base==active_base) return 0;
    forget_game();active_pid=chosen.pid;snprintf(active_title,sizeof(active_title),"%s",chosen.title);snprintf(active_version,sizeof(active_version),"%s",disk.m_appver);
    char path[256];snprintf(path,sizeof(path),CHEAT_ROOT "/json/%s_%s.json",active_title,active_version);
    FILE *f=fopen(path,"rb");if(!f) {snprintf(last_result,sizeof(last_result),"No exact JSON match: %s_%s.json",active_title,active_version);return -1;}
    if(fseek(f,0,SEEK_END)) {fclose(f);return -1;}long n=ftell(f);rewind(f);
    if(n<2 || n>2*1024*1024) {fclose(f);snprintf(last_result,sizeof(last_result),"Cheat file size unsupported");return -1;}
    char *json=calloc((size_t)n+1,1);if(!json) {fclose(f);return -1;}
    size_t got=fread(json,1,(size_t)n,f);fclose(f);
    if(got==(size_t)n) active_db=bh_db_open(!memcmp(json,"\xef\xbb\xbf",3)?json+3:json,active_title,active_version,"eboot.bin",last_result,sizeof(last_result));
    free(json);
    if(active_db) snprintf(last_result,sizeof(last_result),"Ready; verify game version before selecting a cheat");
    return active_db?0:-1;
}
static int remote_memory(void *unused,uint64_t offset,unsigned char *bytes,size_t n,int write) {
    (void)unused;
    if(offset>UINT64_MAX-active_base || !n) return -1;
    uint64_t address=active_base+offset;if(address>UINT64_MAX-n) return -1;
    for(size_t i=0;i<active_map_count;++i) {
        struct bh_map *m=&active_maps[i];
        if(game_map(m) && (m->prot&1) && address>=m->start && address+n<=m->end)
            return orbis_syscall(108,(uint64_t)active_pid,address,bytes,(uint64_t)n,(uint64_t)write)?-1:0;
    } return -1;
}
const char *bh_cheat_menu_xml(void) {
    attach_game();used=0;menu[0]=0;
    emit("<?xml version=\"1.0\" encoding=\"utf-8\"?><system_settings version=\"1.0\" plugin=\"settings_root_plugin\"><setting_list id=\"id_bh_cheats\" title=\"BalenciHEN Cheats\">\n");
    if(active_title[0]) {char id[64];snprintf(id,sizeof(id),"%s / %s / pid %d",active_title,active_version,active_pid);message(id);}
    message(last_result);message("JSON checkboxes only. Return to this page to refresh state.");
    for(size_t i=0;i<bh_db_count(active_db);++i) {
        if(bh_db_supported(active_db,i)) {
            emit("<button id=\"bhcheat_%zu\" title=\"[%s] ",i,bh_db_enabled(active_db,i)?"ON":"OFF");escaped(bh_db_name(active_db,i));
            emit("\" description=\"Press to %s; requires matching original bytes.\"/>\n",bh_db_enabled(active_db,i)?"disable":"enable");
        } else {emit("<message title=\"Unsupported: ");escaped(bh_db_name(active_db,i));emit("\"/>\n");}
    }
    emit("</setting_list></system_settings>");return menu;
}
int bh_cheat_menu_press(const char *id,char *result,size_t size) {
    if(!id || strncmp(id,"bhcheat_",8)) return 0;
    int old_pid=active_pid;unsigned long old_generation=generation;
    if(attach_game() || !active_db || generation!=old_generation || active_pid!=old_pid) snprintf(last_result,sizeof(last_result),"Game session changed; reopen Cheats before applying");
    else {
        char *end=NULL;unsigned long index=strtoul(id+8,&end,10);
        if(!*(id+8) || !end || *end || index>=bh_db_count(active_db)) snprintf(last_result,sizeof(last_result),"Invalid cheat selection");
        else bh_db_toggle(active_db,(size_t)index,remote_memory,NULL,last_result,sizeof(last_result));
    }
    if(result && size) snprintf(result,size,"%s",last_result);
    return 1;
}
