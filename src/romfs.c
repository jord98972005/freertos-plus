#include <string.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <unistd.h>
#include "fio.h"
#include "filesystem.h"
#include "romfs.h"
#include "osdebug.h"
#include "hash-djb2.h"
#include "clib.h"

struct romfs_fds_t {
    const uint8_t * file;   //record how many files in space
    uint32_t cursor;
};

static struct romfs_fds_t romfs_fds[MAX_FDS];

static uint32_t get_unaligned(const uint8_t * d) {
    return ((uint32_t) d[0]) | ((uint32_t) (d[1] << 8)) | ((uint32_t) (d[2] << 16)) | ((uint32_t) (d[3] << 24));
}

static ssize_t romfs_read(void * opaque, void * buf, size_t count) {
    struct romfs_fds_t * f = (struct romfs_fds_t *) opaque;
    const uint8_t * size_p = f->file - 4;
    uint32_t size = get_unaligned(size_p);
    
    if ((f->cursor + count) > size)
        count = size - f->cursor;

    memcpy(buf, f->file + f->cursor, count);
    f->cursor += count;

    return count;
}

static off_t romfs_seek(void * opaque, off_t offset, int whence) {
    struct romfs_fds_t * f = (struct romfs_fds_t *) opaque;
    const uint8_t * size_p = f->file - 4;
    uint32_t size = get_unaligned(size_p);
    uint32_t origin;
    
    switch (whence) {
    case SEEK_SET:
        origin = 0;
        break;
    case SEEK_CUR:
        origin = f->cursor;
        break;
    case SEEK_END:
        origin = size;
        break;
    default:
        return -1;
    }

    offset = origin + offset;

    if (offset < 0)
        return -1;
    if (offset > size)
        offset = size;

    f->cursor = offset;

    return offset;
}

const uint8_t * romfs_get_file_by_hash(const uint8_t * romfs, uint32_t h, uint32_t * len) {
     /* ptr: content of ptr
     * meta: hash
     * meta + 4: filename_len(1 byte)
     * meta + 5: filename(m bytes)
     * meta + 5 + m: file_len(4 bytes)
     * meta + 5 + m + 4: content(n bytes)
     * meta + 5 + m + 4 + n: next file record
     */
    const uint8_t * meta=romfs;
    uint8_t filename_len;
    uint32_t hash, content_len;

     while ((hash = get_unaligned(meta))) {

        filename_len = *(meta + 4);
        content_len = (uint32_t)get_unaligned(meta + 5 + filename_len);
    
            if (hash == h) {
                 if (len) {
                    *len = content_len;
            }
            return (meta + 5 + filename_len + 4);

            } else {
            meta = meta + 5 + filename_len + 4 + content_len;                   //if hash is correspond ,take out the file content,otherwise,keep searching until find the true file block 
            }
     }
    return NULL;
}

static int romfs_open(void * opaque, const char * path, int flags, int mode) {
    uint32_t h = hash_djb2((const uint8_t *) path, -1);
    const uint8_t * romfs = (const uint8_t *) opaque;
    const uint8_t * file;
    int r = -1;

    file = romfs_get_file_by_hash(romfs, h, NULL);

    if (file) {
        r = fio_open(romfs_read, NULL, romfs_seek, NULL, NULL); //read,write,seek,close,opaque  r=0~15(MAX_FS-1)
        if (r > 0) { //why not >=0 ??
            romfs_fds[r].file = file;
            romfs_fds[r].cursor = 0; // 0 =file head
            fio_set_opaque(r, romfs_fds + r);
        }
    }
    return r;
}

int romfs_show_files(void * opaque) {
	uint8_t * meta = (uint8_t *) opaque;
	uint8_t filename_len, buf[256];
	

	if (!meta) {
		return -1;
	}

	memset(buf, 0, sizeof(buf));

	fio_printf(1, "\r\n"); //why 1?
		
	while (get_unaligned(meta)) {
		/* move to next field, the name of file name */
		meta += 4;

		/* get the length of file */
		filename_len = *meta;

		/* move to next field, filename*/
		meta++;

		memcpy(buf, meta, filename_len);//copy filename into buffer
		buf[filename_len] = '\0';

		fio_printf(1, "%s ", buf);

		/* move to next field, content of the file len */
		meta += filename_len;

		/* move to nextfield, next file meta*/
		meta += 4 + get_unaligned(meta);		
	}

	fio_printf(1, "\r\n");
	return 0;
}

void register_romfs(const char * mountpoint, const uint8_t * romfs) {
//    DBGOUT("Registering romfs `%s' @ %p\r\n", mountpoint, romfs);
    register_fs(mountpoint, romfs_open, romfs_show_files, (void *) romfs);
}
