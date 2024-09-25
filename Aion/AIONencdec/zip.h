#ifndef __zip_h__
#define __zip_h__

#include <stdint.h>

#define ZLFH_magic 0x04034b50
typedef struct __attribute__((packed)) _ZLFH {
	uint32_t magic;
	uint16_t ver, flags, meth, mtime, mdate;
	uint32_t crc32, csize, usize;
	uint16_t namelen, extralen;
} ZLFH;

#if 0
structFileHeader = "<4s2B4HlLL2H"      # 12 items, file header record, 30 bytes


_FH_SIGNATURE = 0            4
_FH_EXTRACT_VERSION = 1      1
_FH_EXTRACT_SYSTEM = 2       1           # is this meaningful?
_FH_GENERAL_PURPOSE_FLAG_BITS = 3  2
_FH_COMPRESSION_METHOD = 4         2
_FH_LAST_MOD_TIME = 5              2
_FH_LAST_MOD_DATE = 6              2
_FH_CRC = 7                        4
_FH_COMPRESSED_SIZE = 8            4    x
_FH_UNCOMPRESSED_SIZE = 9          4
_FH_FILENAME_LENGTH = 10           2
_FH_EXTRA_FIELD_LENGTH = 11        2
#endif





#define ZDD_magic 0x08074b50
typedef struct __attribute__((packed)) _ZDD {
	uint32_t crc32, csize, usize;
} ZDD;

//no magic, determined by flags
typedef struct __attribute__((packed)) _ZADH {
	uint16_t ivlen;
	uint8_t data[];
} ZADH;

//extra data record
#define ZEDR_magic 0x08064b50
typedef struct __attribute__((packed)) _ZEDR {
	uint32_t magic, len;
} ZEDR;

//central directory file header
#define ZCD_FH_magic 0x02014b50
typedef struct __attribute__((packed)) _ZCD_FH {
	uint32_t magic;
	uint16_t verby, ver, flags, meth, mtime, mdate;
	uint32_t crc32, csize, usize;
	uint16_t namelen, extralen, commlen, diskstart, iattr;
	uint32_t eattr, off;
	
} ZCD_FH;

//central directory digital signature
#define ZCD_DS_magic 0x05054b50
typedef struct __attribute__((packed)) _ZCD_DS {
	uint32_t magic;
	uint16_t len;
} ZCD_DS;

//Zip64 end of central directory record, unsupported
#define Z64EOCD_magic 0x06064b50
//Zip64 end of central directory locator, unsupported
#define Z64EOCDL_magic 0x07064b50

//end of central directory
#define ZEOCD_magic 0x06054b50
typedef struct __attribute__((packed)) _ZEOCD {
	uint32_t magic;
	uint16_t disknr, disknrcd, cntdisk, cnt;
	uint32_t cdlen, offcd;
	uint16_t commlen;
} ZEOCD;


#endif

