#include "engine.h"
#include "cJSON.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define BH_MODS 256
#define BH_PATCHES 64
#define BH_BYTES 4096
struct patch { uint64_t offset; size_t length; unsigned char *on, *off, *saved; };
struct mod { char name[256]; int supported, enabled; size_t count; struct patch *patches; };
struct bh_db { size_t count; int poisoned; struct mod *mods; };
static int fail(char *s, size_t n, const char *why) { if(s && n) snprintf(s,n,"%s",why); return -1; }
static const cJSON *get(const cJSON *o, const char *k) { return cJSON_GetObjectItemCaseSensitive(o,k); }
static const char *str(const cJSON *o, const char *k) { const cJSON *v=get(o,k); return cJSON_IsString(v) ? v->valuestring : NULL; }
static int unhex(const char *s, unsigned char **out, size_t *n) {
    if(!s || !*s || strlen(s)%2 || strlen(s)>BH_BYTES*2) return -1;
    *n=strlen(s)/2; *out=malloc(*n); if(!*out) return -1;
    for(size_t i=0;i<*n;++i) {
        unsigned x=0;
        for(int j=0;j<2;++j) { unsigned char c=(unsigned char)s[2*i+j];
            if(!isxdigit(c)) { free(*out); *out=NULL; return -1; }
            x=x*16+(c<='9'?c-'0':tolower(c)-'a'+10);
        } (*out)[i]=(unsigned char)x;
    } return 0;
}
static int offset_of(const char *s, uint64_t *out) {
    if(!s || !*s) return -1;
    if(s[0]=='0' && (s[1]=='x' || s[1]=='X')) s+=2;
    if(!*s) return -1;
    uint64_t value=0;
    for(;*s;++s) {
        unsigned char c=(unsigned char)*s;
        unsigned digit;
        if(c>='0' && c<='9') digit=c-'0';
        else if(c>='a' && c<='f') digit=c-'a'+10;
        else if(c>='A' && c<='F') digit=c-'A'+10;
        else return -1;
        if(value>(UINT64_MAX-digit)/16) return -1;
        value=value*16+digit;
    }
    *out=value; return 0;
}
static int overlap(const struct patch *a, const struct patch *b) {
    return a->offset < b->offset+b->length && b->offset < a->offset+a->length;
}
void bh_db_close(bh_db *d) {
    if(!d) return;
    for(size_t i=0;i<d->count;++i) { struct mod *m=&d->mods[i];
        for(size_t j=0;j<m->count;++j) {free(m->patches[j].on); free(m->patches[j].off); free(m->patches[j].saved);}
        free(m->patches);
    } free(d->mods); free(d);
}
bh_db *bh_db_open(const char *json,const char *id,const char *version,const char *process,char *err,size_t errn) {
    if(!json || !id || !version || !process) {fail(err,errn,"Missing identity");return NULL;}
    cJSON *root=cJSON_ParseWithOpts(json,NULL,1);
    if(!root) {fail(err,errn,"Invalid JSON");return NULL;}
    const char *ri=str(root,"id"), *rv=str(root,"version"), *rp=str(root,"process");
    const cJSON *mods=get(root,"mods");
    if(!ri || !rv || !rp || strcmp(ri,id) || strcmp(rv,version) || strcmp(rp,process)) {
        fail(err,errn,"Title, version or process mismatch"); cJSON_Delete(root); return NULL;
    }
    int count=cJSON_GetArraySize(mods);
    if(!cJSON_IsArray(mods) || count<1 || count>BH_MODS) {fail(err,errn,"Unsupported cheat count");cJSON_Delete(root);return NULL;}
    bh_db *d=calloc(1,sizeof(*d)); if(!d) {cJSON_Delete(root);return NULL;}
    d->count=(size_t)count; d->mods=calloc(d->count,sizeof(*d->mods));
    if(!d->mods) {free(d);cJSON_Delete(root);return NULL;}
    for(size_t i=0;i<d->count;++i) {
        struct mod *m=&d->mods[i]; const cJSON *v=cJSON_GetArrayItem(mods,(int)i);
        const char *name=str(v,"name"), *type=str(v,"type");
        snprintf(m->name,sizeof(m->name),"%s",name?name:"Unnamed cheat");
        const cJSON *memory=get(v,"memory"); int n=cJSON_GetArraySize(memory);
        if(!type || strcmp(type,"checkbox") || !cJSON_IsArray(memory) || n<1 || n>BH_PATCHES) continue;
        m->patches=calloc((size_t)n,sizeof(*m->patches)); if(!m->patches) continue;
        m->count=(size_t)n; m->supported=1;
        for(size_t j=0;j<m->count;++j) {
            const cJSON *item=cJSON_GetArrayItem(memory,(int)j); struct patch *p=&m->patches[j];
            size_t offlen=0;
            /* Section-indexed patches need a different address resolver. */
            if(get(item,"section") || offset_of(str(item,"offset"),&p->offset) ||
               unhex(str(item,"on"),&p->on,&p->length) || unhex(str(item,"off"),&p->off,&offlen) ||
               offlen!=p->length || p->offset>UINT64_MAX-p->length) {m->supported=0;break;}
            for(size_t k=0;k<j;++k) if(overlap(p,&m->patches[k])) m->supported=0;
            if(!m->supported) break;
        }
    }
    cJSON_Delete(root); if(err && errn) *err=0; return d;
}
size_t bh_db_count(const bh_db *d) {return d?d->count:0;}
const char *bh_db_name(const bh_db *d,size_t i) {return d && i<d->count?d->mods[i].name:"";}
int bh_db_supported(const bh_db *d,size_t i) {return d && !d->poisoned && i<d->count && d->mods[i].supported;}
int bh_db_enabled(const bh_db *d,size_t i) {return d && i<d->count && d->mods[i].enabled;}
int bh_db_toggle(bh_db *d,size_t i,bh_mem_fn rw,void *ctx,char *err,size_t errn) {
    if(!d || !rw || i>=d->count || !bh_db_supported(d,i)) return fail(err,errn,"Unsupported cheat or engine locked");
    struct mod *m=&d->mods[i];
    for(size_t a=0;a<m->count;++a) for(size_t j=0;j<d->count;++j) {
        if(j==i || !d->mods[j].enabled) continue;
        for(size_t b=0;b<d->mods[j].count;++b) if(overlap(&m->patches[a],&d->mods[j].patches[b]))
            return fail(err,errn,"Overlaps an enabled cheat; disable it first");
    }
    unsigned char **before=calloc(m->count,sizeof(*before));
    if(!before) return fail(err,errn,"Out of memory");
    int result=-1;
    for(size_t j=0;j<m->count;++j) {
        struct patch *p=&m->patches[j]; before[j]=malloc(p->length);
        if(!before[j] || rw(ctx,p->offset,before[j],p->length,0)) {fail(err,errn,"Read failed or address not in executable mapping");goto done;}
        if(memcmp(before[j],m->enabled?p->on:p->off,p->length)) {fail(err,errn,"Memory differs from expected bytes; no writes made");goto done;}
    }
    for(size_t j=0;j<m->count;++j) {
        struct patch *p=&m->patches[j]; unsigned char check[BH_BYTES];
        unsigned char *wanted=m->enabled?p->saved:p->on;
        if(!wanted || rw(ctx,p->offset,wanted,p->length,1) || rw(ctx,p->offset,check,p->length,0) || memcmp(check,wanted,p->length)) {
            int rollback_failed=0;
            for(size_t k=0;k<=j;++k) {
                struct patch *q=&m->patches[k];
                if(rw(ctx,q->offset,before[k],q->length,1) || rw(ctx,q->offset,check,q->length,0) || memcmp(check,before[k],q->length)) rollback_failed=1;
            }
            if(rollback_failed) {d->poisoned=1;fail(err,errn,"Rollback failed; engine locked. Restart game.");}
            else fail(err,errn,"Write failed; original bytes restored");
            goto done;
        }
    }
    if(!m->enabled) for(size_t j=0;j<m->count;++j) {free(m->patches[j].saved);m->patches[j].saved=before[j];before[j]=NULL;}
    m->enabled=!m->enabled; result=0; if(err && errn) snprintf(err,errn,"%s",m->enabled?"Enabled and verified":"Disabled; original bytes restored");
done:
    for(size_t j=0;j<m->count;++j) free(before[j]);
    free(before);
    return result;
}
