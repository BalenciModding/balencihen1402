/* Local download-file reset only. No claim of server-side ban removal.
 * The two files are moved into a same-directory recoverable backup. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "../../common/syscall.h"

#ifndef BH_APPMETA_ROOT
#define BH_APPMETA_ROOT "/user/appmeta"
#endif
#ifndef BH_DOWNLOAD_ROOT
#define BH_DOWNLOAD_ROOT "/user/download"
#endif
static uint16_t le16(const unsigned char *p) {return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t le32(const unsigned char *p) {return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int title_id(const char *s) {
    if(!s || strlen(s)!=9 || strncmp(s,"CUSA",4)) return 0;
    for(int i=4;i<9;++i) if(!isdigit((unsigned char)s[i])) return 0;
    return 1;
}
static int directory(const char *p) {struct stat s;return !lstat(p,&s) && S_ISDIR(s.st_mode);}
static const char *sfo_string(const unsigned char *b,size_t n,const char *key) {
    if(n<20 || le32(b)!=0x46535000) return NULL;
    uint32_t keys=le32(b+8),data=le32(b+12),count=le32(b+16);
    if(count>1024 || count>(n-20)/16 || keys>=n || data>=n) return NULL;
    for(uint32_t i=0;i<count;++i) {
        const unsigned char *entry=b+20+16*i;
        uint32_t ko=le16(entry),len=le32(entry+4),value=le32(entry+12);
        if(ko>=n-keys || !memchr(b+keys+ko,0,n-keys-ko)) return NULL;
        if(strcmp((const char *)b+keys+ko,key)) continue;
        if(le16(entry+2)!=0x0204 || !len || value>=n-data || len>n-data-value || !memchr(b+data+value,0,len)) return NULL;
        return (const char *)b+data+value;
    } return NULL;
}
static int is_fortnite(const char *id) {
    char path[512];snprintf(path,sizeof(path),BH_APPMETA_ROOT "/%s",id);
    if(!directory(path)) return 0;
    snprintf(path,sizeof(path),BH_APPMETA_ROOT "/%s/param.sfo",id);
    struct stat s;if(lstat(path,&s) || !S_ISREG(s.st_mode) || s.st_size<20 || s.st_size>262144) return 0;
    FILE *f=fopen(path,"rb");if(!f) return 0;
    size_t n=(size_t)s.st_size;unsigned char *b=malloc(n);
    if(!b) {fclose(f);return 0;}
    size_t got=fread(b,1,n,f);fclose(f);int match=0;
    if(got==n) {
        const char *name=sfo_string(b,n,"TITLE"),*actual=sfo_string(b,n,"TITLE_ID");
        match=name && actual && !strcmp(actual,id) &&
              (!strcmp(name,"Fortnite") || !strcmp(name,"Fortnite\xe2\x84\xa2") || !strcmp(name,"Fortnite\xc2\xae"));
    } free(b);return match;
}
int bh_fortnite_find_title(char *id,size_t size) {
    if(!id || size<10 || !directory(BH_APPMETA_ROOT)) return -1;
    id[0]=0;DIR *dir=opendir(BH_APPMETA_ROOT);if(!dir) return -1;
    struct dirent *e;int count=0;
    while((e=readdir(dir))) if(title_id(e->d_name) && is_fortnite(e->d_name)) {
        snprintf(id,size,"%s",e->d_name);++count;
    }
    closedir(dir);
    if(count!=1) {id[0]=0;return -1;} return 0;
}
static int same_file(const char *p,const struct stat *before) {
    struct stat now;
    return !lstat(p,&now) && S_ISREG(now.st_mode) && now.st_dev==before->st_dev &&
           now.st_ino==before->st_ino && now.st_size==before->st_size && now.st_mtime==before->st_mtime;
}
static int game_running(const char *title) {
    struct reset_proc {char name[32];int pid;} __attribute__((packed));
    struct reset_info {int pid;char name[40],path[64],title[16],content[64];} __attribute__((packed));
    uint64_t count=0;
    if(orbis_syscall(107,NULL,&count) || !count || count>2048) return -1;
    struct reset_proc *list=calloc((size_t)count,sizeof(*list));if(!list) return -1;
    if(orbis_syscall(107,list,&count)) {free(list);return -1;}
    int result=0;
    for(size_t i=0;i<count;++i) {
        if(list[i].pid<=0) continue;
        struct reset_info info;memset(&info,0,sizeof(info));
        if(orbis_syscall(109,(uint64_t)list[i].pid,(uint64_t)8,&info)) {result=-1;break;}
        info.title[15]=0;
        if(!strcmp(info.title,title)) {result=1;break;}
    }
    free(list);return result;
}
int bh_fortnite_reset(char *message,size_t size) {
    char id[16],base[512],paths[2][640],backup[640],dest[2][768];
    static const char *files[]={"download0.dat","download0_information.dat"};
    struct stat before[2];
    if(bh_fortnite_find_title(id,sizeof(id))) {
        snprintf(message,size,"Cannot identify one installed Fortnite title from param.sfo. No files changed.");return -1;
    }
    int running=game_running(id);
    if(running) {
        snprintf(message,size,"%s",running>0?"Close Fortnite before resetting its local files. No files changed.":"Cannot verify that Fortnite is closed. No files changed.");return -1;
    }
    if(!directory(BH_DOWNLOAD_ROOT)) {snprintf(message,size,"Download directory unavailable. No files changed.");return -1;}
    snprintf(base,sizeof(base),BH_DOWNLOAD_ROOT "/%s",id);
    if(!directory(base)) {snprintf(message,size,"No download directory for %s. No files changed.",id);return -1;}
    for(int i=0;i<2;++i) {
        snprintf(paths[i],sizeof(paths[i]),"%s/%s",base,files[i]);
        if(lstat(paths[i],&before[i]) || !S_ISREG(before[i].st_mode)) {
            snprintf(message,size,"Both regular download files must exist for %s. No files changed.",id);return -1;
        }
    }
    int created=0;
    for(unsigned i=0;i<100;++i) {
        snprintf(backup,sizeof(backup),"%s/.BalenciHEN-backup-%llu-%d-%u",base,(unsigned long long)time(NULL),getpid(),i);
        if(!mkdir(backup,0700)) {created=1;break;}
    }
    if(!created) {snprintf(message,size,"Cannot create a recovery directory. No files changed.");return -1;}
    for(int i=0;i<2;++i) snprintf(dest[i],sizeof(dest[i]),"%s/%s",backup,files[i]);
    if(!same_file(paths[0],&before[0]) || !same_file(paths[1],&before[1])) {
        rmdir(backup);snprintf(message,size,"Download files changed during inspection. Pause downloads and retry.");return -1;
    }
    if(rename(paths[0],dest[0])) {
        rmdir(backup);snprintf(message,size,"First file move failed. No files changed.");return -1;
    }
    if(!same_file(paths[1],&before[1]) || rename(paths[1],dest[1])) {
        /* Never overwrite a newly recreated first file while rolling back. */
        if(link(dest[0],paths[0])) {
            snprintf(message,size,"Incomplete reset. Recovery files remain in %s",backup);return -2;
        }
        unlink(dest[0]);
        rmdir(backup);snprintf(message,size,"Second file move failed; first file restored. No reset performed.");return -1;
    }
    snprintf(message,size,"Local files reset for %s. Backup: %s. Ban status was not checked.",id,backup);
    return 0;
}
