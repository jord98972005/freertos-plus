#include "osdebug.h"
#include "filesystem.h"
#include "fio.h"

#include <stdint.h>
#include <string.h>
#include <hash-djb2.h>

#define MAX_FS 16

struct fs_t {
    uint32_t hash;
    fs_open_t cb;
	fs_show_files_t show_files_cb;
    void * opaque;
};

static struct fs_t fss[MAX_FS];

__attribute__((constructor)) void fs_init() {
    memset(fss, 0, sizeof(fss));
}

int register_fs(const char * mountpoint, fs_open_t callback, fs_show_files_t show_files_callback, void * opaque) {
    int i;
    DBGOUT("register_fs(\"%s\", %p, %p)\r\n", mountpoint, callback, opaque);
    
    for (i = 0; i < MAX_FS; i++) {
         //why use MAX_FS=16
        if (!fss[i].cb) {
            fss[i].hash = hash_djb2((const uint8_t *) mountpoint, -1);
            fss[i].cb = callback;
	  fss[i].show_files_cb = show_files_callback;
            fss[i].opaque = opaque;
            //so fss[i].hash record the hash of mountpoint,and opaque record the file hash value,right?
            return 0;
        }
    }
    
    return -1;
}

int fs_open(const char * path, int flags, int mode) {
    const char * slash;
    uint32_t hash;
    int i;
//    DBGOUT("fs_open(\"%s\", %i, %i)\r\n", path, flags, mode);
    
    while (path[0] == '/')
        path++;
    
    slash = strchr(path, '/');
    
    if (!slash)
        return -2;

    hash = hash_djb2((const uint8_t *) path, slash - path);// is slash-path lenth= mountpoint lenth?
    path = slash + 1;

    for (i = 0; i < MAX_FS; i++) {
        if (fss[i].hash == hash)
            return fss[i].cb(fss[i].opaque, path, flags, mode);//open right filesystem 
    }
    
    return -2;
}

static struct fs_t *fs_get_fss(const char * fs_name) {
	uint32_t hash;
	int i;

	hash = hash_djb2((const uint8_t *) fs_name, -1); //why -1?

	for (i=0; i<MAX_FS && fss[i].cb; i++) {
		if (fss[i].hash == hash) {
			return &fss[i];
		}
	}
	return NULL;
}

int fs_show_files() {
	struct fs_t *fs_ptr = fs_get_fss("romfs");
	
	if (!fs_ptr || !fs_ptr->show_files_cb) {
		return -1;
	}

	fs_ptr->show_files_cb(fs_ptr->opaque);
	return 0;
}
