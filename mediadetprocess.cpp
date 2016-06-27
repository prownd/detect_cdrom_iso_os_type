/***
 *author:hanjinpeng
 *email:545751287@qq.com
 */

extern "C"
{
#include "mediadetglobal.h"

}
#include"cdromisoprocess.h"


static void analyze_file(const char *filename,CdromIsoInfo *info);
static void print_kind(int filekind, u8 size, int size_known);
static void get_path(char *,char *);
static void get_name(char *, char *);
static void get_parent_path(char *,char *);
static bool str_endswith(char *, char *);
static bool str_startswith(char * ,char * );

int detect_media(char * path,CdromIsoInfo *info)
{
  memset(info,0,sizeof(CdromIsoInfo));
  char name[100]={0};
  if(str_endswith(path,".iso")){
      get_name(path,name);
      strcpy(info->name,name);
  }else{
      strcpy(info->name,"cdrom");
  }
  char * mediapath=NULL;
  if(str_startswith(path,"/dev/cdrom")){
     //mediapath="/dev/sr0";
      mediapath=path;
  }else{
      mediapath=path;
  }
  analyze_file(path,info);
  return 0;
}

static void analyze_file(const char *filename,CdromIsoInfo *info)
{
  int fd, filekind;
  u8 filesize;
  struct stat sb;
  char *reason;
  SOURCE *s;
  //print_line(0, "--- %s", filename);
  if (stat(filename, &sb) < 0) {
    errore("Can't stat %.300s", filename);
    return;
  }

  filekind = 0;
  filesize = 0;
  reason = NULL;
  if (S_ISREG(sb.st_mode)) {
    filesize = sb.st_size;
    info->size=filesize;
    //print_kind(filekind, filesize, 1);
  } else if (S_ISBLK(sb.st_mode))
    filekind = 1;
  else if (S_ISCHR(sb.st_mode))
    filekind = 2;
  else if (S_ISDIR(sb.st_mode))
    reason = "Is a directory";
  else if (S_ISFIFO(sb.st_mode))
    reason = "Is a FIFO";
#ifdef S_ISSOCK
  else if (S_ISSOCK(sb.st_mode))
    reason = "Is a socket";
#endif
  else
    reason = "Is an unknown kind of special file";

  if (reason != NULL) {
    error("%.300s: %s", filename, reason);
    return;
  }

  if (filekind == 0 && filesize == 0)
    return;

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    errore("Can't open %.300s", filename);
    return;
  }

  if (filekind == 2) {
    if (isatty(fd)) {
      error("%.300s: Is a TTY device", filename);
      return;
    }
  }

  s = init_file_source(fd, filekind);
  s->info=info;

  if (filekind != 0){
     info->size=s->size;
     //print_kind(filekind, s->size, s->size_known);
  }
  analyze_source(s, 0);
  close_source(s);
}

static void print_kind(int filekind, u8 size, int size_known)
{
  char buf[256], *kindname;

  if (filekind == 0)
    kindname = "Regular file";
  else if (filekind == 1)
    kindname = "Block device";
  else if (filekind == 2)
    kindname = "Character device";
  else
    kindname = "Unknown kind";

  if (size_known) {
    format_size_verbose(buf, size);
    print_line(0, "%s, size %s", kindname, buf);
  } else {
    print_line(0, "%s, unknown size", kindname);
  }
}

static void get_path(char *filepath,char *filedir)
{
    while(*(filedir++)=*(filepath++));
    while(*(--filedir)!='/');
    *filedir='\0';
}


static void get_name(char *filepath, char *filename)
{
    while(*(filepath++));
    while(*(--filepath)!='/');
    while(*(filename++)=*(++filepath));
}

static void get_parent_path(char *filepath,char *parentdir)
{
    while(*(parentdir++)=*(filepath++));
    while(*(--parentdir)!='/');
    *parentdir='\0';
    while(*(--parentdir))
    {
        if(*parentdir == '/' )
        {
           *parentdir='\0';
            break;
        }
    }
}

static bool str_endswith(char *str, char *reg)
{
    int l1 = strlen(str), l2 = strlen(reg);
    if (l1 < l2) return false;
    str += l1 - l2;
    while (*str && *reg && *str == *reg) {
        str ++;
        reg ++;
    }
    if (!*str && !*reg) return true;
    return false;
}
static bool str_startswith(char * str,char * reg){
    if(strncmp(str, reg, strlen(reg)) == 0)
        return true;
    return false;
}


void detect_dos_partmap(SECTION *section, int level);

void detect_gpt_partmap(SECTION *section, int level);

void detect_dos_loader(SECTION *section, int level);

void detect_iso(SECTION *section, int level);

void detect_cdrom_misc(SECTION *section, int level);

void detect_linux_loader(SECTION *section, int level);


void detect_cdimage(SECTION *section, int level);

/*
 * list
 */

DETECTOR detectors[] = {
  /*  image formats */
  detect_cdimage,           /* may stop */
  /*   code */
  detect_linux_loader,
  detect_dos_loader,
  /*  tables */
  detect_dos_partmap,
  //detect_gpt_partmap,
  //detect_cdrom_misc,
  detect_iso,
 NULL };


/*
 * internal stuff
 */

static void detect(SECTION *section, int level);

static int stop_flag = 0;

/*
 * analyze
 */

void analyze_source(SOURCE *s, int level)
{
  SECTION section;

  /* Allow custom analyzing using special info available to the
     data source implementation. The analyze() function must either
     call through to analyze_source_special() or return zero. */
  if (s->analyze != NULL) {
    if ((*s->analyze)(s, level))
      return;
  }

  section.source = s;
  section.pos = 0;
  section.size = s->size_known ? s->size : 0;
  section.flags = 0;
  section.info=s->info;

  detect(&section, level);
}

/*
 * analyze continue
 */

void analyze_source_special(SOURCE *s, int level, u8 pos, u8 size)
{
  SECTION section;

  section.source = s;
  section.pos = pos;
  section.size = size;
  section.flags = 0;
  section.info=s->info;

  detect(&section, level);
}

/*
 *
 */

void analyze_recursive(SECTION *section, int level,
                       u8 rel_pos, u8 size, int flags)
{
  SOURCE *s;
  SECTION rs;

  /* sanity */
  if (rel_pos == 0 && (flags & FLAG_IN_DISKLABEL) == 0)
    return;
  s = section->source;
  if (s->size_known && (section->pos + rel_pos >= s->size))
    return;

  rs.source = s;
  rs.pos = section->pos + rel_pos;
  rs.size = size;
  rs.flags = section->flags | flags;
  rs.info=section->info;

  detect(&rs, level);
}

/*
 * detection
 */

static void detect(SECTION *section, int level)
{
  int i;

  /* run the d */
  for (i = 0; detectors[i] && !stop_flag; i++)
    (*detectors[i])(section, level);
  stop_flag = 0;
}

/*
 * break the detection loop
 */

void stop_detect(void)
{
  stop_flag = 1;
}


extern "C"
{
#define USE_BINARY_SEARCH 0
#define DEBUG_SIZE 0
#ifdef USE_IOCTL_LINUX
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif
}


typedef struct file_source {
  SOURCE c;
  int fd;
} FILE_SOURCE;

/*
 * helper functions
 */

static int analyze_file(SOURCE *s, int level);
static u8 read_file(SOURCE *s, u8 pos, u8 len, void *buf);
static void close_file(SOURCE *s);

#if USE_BINARY_SEARCH
static int check_position(int fd, u8 pos);
#endif

/*
 * initialize the file source
 */

SOURCE *init_file_source(int fd, int filekind)
{
  FILE_SOURCE *fs;
  off_t result;

  fs = (FILE_SOURCE *)malloc(sizeof(FILE_SOURCE));
  if (fs == NULL)
    bailout("Out of memory");
  memset(fs, 0, sizeof(FILE_SOURCE));

  if (filekind != 0)  /* special treatment hook for devices */
    fs->c.analyze = analyze_file;
  fs->c.read_bytes = read_file;
  fs->c.close = close_file;
  fs->fd = fd;

  /*
   * lseek() to the end:
   * Works on files. On some systems (Linux), this also works on devices.
   */
  if (!fs->c.size_known) {
    result = lseek(fd, 0, SEEK_END);
#if DEBUG_SIZE
    printf("Size: lseek returned %lld\n", result);
#endif
    if (result > 0) {
      fs->c.size_known = 1;
      fs->c.size = result;
    }
  }

#ifdef USE_IOCTL_LINUX
  /*
   * ioctl, Linux style:
   * Works on certain devices.
   */
#ifdef BLKGETSIZE64
#define u64 __u64   /* workaround for broken header file */
  if (!fs->c.size_known && filekind != 0) {
    u8 devsize;
    if (ioctl(fd, BLKGETSIZE64, (void *)&devsize) >= 0) {
      fs->c.size_known = 1;
      fs->c.size = devsize;
#if DEBUG_SIZE
      printf("Size: Linux 64-bit ioctl reports %llu\n", fs->c.size);
#endif
    }
  }
#undef u64
#endif

  if (!fs->c.size_known && filekind != 0) {
    u4 blockcount;
    if (ioctl(fd, BLKGETSIZE, (void *)&blockcount) >= 0) {
      fs->c.size_known = 1;
      fs->c.size = (u8)blockcount * 512;
#if DEBUG_SIZE
      printf("Size: Linux 32-bit ioctl reports %llu (%lu blocks)\n",
         fs->c.size, blockcount);
#endif
    }
  }
#endif


#if USE_BINARY_SEARCH
  /*
   * binary search:
   * Works on anything that can seek, but is quite expensive.
   */
  if (!fs->c.size_known) {
    u8 lower, upper, current;

    /* TODO: check that the target can seek at all */

#if DEBUG_SIZE
    printf("Size: Doing a binary search\n");
#endif

    /* first, find an upper bound starting from a reasonable guess */
    lower = 0;
    upper = 1024 * 1024;  /* start with 1MB */
    while (check_position(fd, upper)) {
      lower = upper;
      upper <<= 2;
    }

    /* second, nail down the size between the lower and upper bounds */
    while (upper > lower + 1) {
      current = (lower + upper) >> 1;
      if (check_position(fd, current))
    lower = current;
      else
    upper = current;
    }
    fs->c.size_known = 1;
    fs->c.size = lower + 1;

#if DEBUG_SIZE
    printf("Size: Binary search reports %llu\n", fs->c.size);
#endif
  }
#endif

  return (SOURCE *)fs;
}



static int analyze_file(SOURCE *s, int level)
{
  if (analyze_cdaccess(((FILE_SOURCE *)s)->fd, s, level))
    return 1;

  return 0;
}

/*
 * raw read
 */

static u8 read_file(SOURCE *s, u8 pos, u8 len, void *buf)
{
  off_t result_seek;
  ssize_t result_read;
  char *p;
  u8 got;
  int fd = ((FILE_SOURCE *)s)->fd;

  /* seek to the requested position */
  result_seek = lseek(fd, pos, SEEK_SET);
  if (result_seek != pos) {
    errore("Seek to %llu failed", pos);
    return 0;
  }

  /* read from there */
  p = (char *)buf;
  got = 0;
  while (len > 0) {
    result_read = read(fd, p, len);
    if (result_read < 0) {
      if (errno == EINTR || errno == EAGAIN)
    continue;
      errore("Data read failed at position %llu", pos + got);
      break;
    } else if (result_read == 0) {
      /* simple EOF, no message */
      break;
    } else {
      len -= result_read;
      got += result_read;
      p += result_read;
    }
  }

  return got;
}


static void close_file(SOURCE *s)
{
  int fd = ((FILE_SOURCE *)s)->fd;

  if (fd >= 0)
    close(fd);
}

/*
 * check if the given position is inside the file's size
 */

#if USE_BINARY_SEARCH
static int check_position(int fd, u8 pos)
{
  char buf[2];

#if DEBUG_SIZE
    printf("      Probing %llu\n", pos);
#endif

  if (lseek(fd, pos, SEEK_SET) != pos)
    return 0;
  if (read(fd, buf, 1) != 1)
    return 0;
  return 1;
}
#endif





extern "C"
{
#include <stdarg.h>
}
/*
 * output functions
 */

#define LEVELS (8)

static const char *insets[LEVELS] = {
  "",
  "  ",
  "    ",
  "      ",
  "        ",
  "          ",
  "            ",
  "              ",
};
static char line_akku[4096];

void print_line(int level, const char *fmt, ...)
{
  va_list par;

  va_start(par, fmt);
  vsnprintf(line_akku, 4096, fmt, par);
  va_end(par);

  if (level >= LEVELS)
    bailout("Recursion loop caught");
  printf("%s%s\n", insets[level], line_akku);
}

void start_line(const char *fmt, ...)
{
  va_list par;

  va_start(par, fmt);
  vsnprintf(line_akku, 4096, fmt, par);
  va_end(par);
}

void continue_line(const char *fmt, ...)
{
  va_list par;
  int len = strlen(line_akku);

  va_start(par, fmt);
  vsnprintf(line_akku + len, 4096 - len, fmt, par);
  va_end(par);
}

void finish_line(int level)
{
  if (level >= LEVELS)
    bailout("Recursion loop caught");
  printf("%s%s\n", insets[level], line_akku);
}

static int format_raw_size(char *buf, u8 size)
{
  int unit_index, dd;
  u8 unit_size, card;
  const char *unit_names[] =
    { "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", NULL };
  const int dd_mult[4] = { 1, 10, 100, 1000 };

  /* just a few bytes */
  if (size < 1024) {
    sprintf(buf, "%llu bytes", size);
    return 2;
  }

  /* find a suitable unit */
  for (unit_index = 0, unit_size = 1024;
       unit_names[unit_index] != NULL;
       unit_index++, unit_size <<= 10) {

    /* size is at least one of the next unit -> use that */
    if (size >= 1024 * unit_size)
      continue;

    /* check integral multiples */
    if ((size % unit_size) == 0) {
      card = size / unit_size;
      sprintf(buf, "%d %s",
          (int)card,
          unit_names[unit_index]);
      return unit_index ? 0 : 1;
    }

    /* find suitable number of decimal digits */
    for (dd = 3; dd >= 1; dd--) {
      card = (size * dd_mult[dd] + (unit_size >> 1)) / unit_size;
      if (card >= 10000)
    continue;  /* more than four significant digits */

      sprintf(buf, "%d.%0*d %s",
              (int)(card / dd_mult[dd]),
          dd, (int)(card % dd_mult[dd]),
          unit_names[unit_index]);
      return 0;
    }
  }

  /* fallback (something wrong with the numbers?) */
  strcpy(buf, "off the scale");
  return 0;
}

void format_blocky_size(char *buf, u8 count, u4 blocksize,
            const char *blockname, const char *append)
{
  int used;
  u8 total_size;
  char *p;
  char blocksizebuf[32];

  total_size = count * blocksize;
  used = format_raw_size(buf, total_size);
  p = strchr(buf, 0);

  *p++ = ' ';
  *p++ = '(';

  if (used != 2) {
    sprintf(p, "%llu bytes, ", total_size);
    p = strchr(buf, 0);
  }

  if (blocksize == 512 && strcmp(blockname, "sectors") == 0) {
    sprintf(p, "%llu %s", count, blockname);
  } else {
    if (blocksize < 64*1024 && (blocksize % 1024) != 0)
      sprintf(blocksizebuf, "%lu bytes", blocksize);
    else
      format_raw_size(blocksizebuf, blocksize);
    sprintf(p, "%llu %s of %s", count, blockname, blocksizebuf);
  }
  p = strchr(buf, 0);

  if (append != NULL) {
    strcpy(p, append);
    p = strchr(buf, 0);
  }

  *p++ = ')';
  *p++ = 0;
}

void format_size(char *buf, u8 size)
{
  int used;

  used = format_raw_size(buf, size);
  if (used > 0)
    return;

  sprintf(strchr(buf, 0), " (%llu bytes)", size);
}

void format_size_verbose(char *buf, u8 size)
{
  int used;

  used = format_raw_size(buf, size);
  if (used == 2)
    return;

  sprintf(strchr(buf, 0), " (%llu bytes)", size);
}

void format_ascii(void *from, char *to)
{
  u1 *p = (u1 *)from;
  u1 *q = (u1 *)to;
  int c;

  while ((c = *p++)) {
    if (c >= 127 || c < 32) {
      *q++ = '<';
      *q++ = "0123456789ABCDEF"[c >> 4];
      *q++ = "0123456789ABCDEF"[c & 15];
      *q++ = '>';
    } else {
      *q++ = c;
    }
  }
  *q = 0;
}

void format_utf16_be(void *from, u4 len, char *to)
{
  u2 *p = (u2 *)from;
  u2 *p_end;
  u1 *q = (u1 *)to;
  u2 c;

  if (len)
    p_end = (u2 *)(((u1 *)from) + len);
  else
    p_end = NULL;

  while (p_end == NULL || p < p_end) {
    c = get_be_short(p);
    if (c == 0)
      break;
    p++;  /* advance 2 bytes */

    if (c >= 127 || c < 32) {
      *q++ = '<';
      *q++ = "0123456789ABCDEF"[c >> 12];
      *q++ = "0123456789ABCDEF"[(c >> 8) & 15];
      *q++ = "0123456789ABCDEF"[(c >> 4) & 15];
      *q++ = "0123456789ABCDEF"[c & 15];
      *q++ = '>';
    } else {
      *q++ = (u1)c;
    }
  }
  *q = 0;
}

void format_utf16_le(void *from, u4 len, char *to)
{
  u2 *p = (u2 *)from;
  u2 *p_end;
  u1 *q = (u1 *)to;
  u2 c;

  if (len)
    p_end = (u2 *)(((u1 *)from) + len);
  else
    p_end = NULL;

  while (p_end == NULL || p < p_end) {
    c = get_le_short(p);
    if (c == 0)
      break;
    p++;  /* advance 2 bytes */

    if (c >= 127 || c < 32) {
      *q++ = '<';
      *q++ = "0123456789ABCDEF"[c >> 12];
      *q++ = "0123456789ABCDEF"[(c >> 8) & 15];
      *q++ = "0123456789ABCDEF"[(c >> 4) & 15];
      *q++ = "0123456789ABCDEF"[c & 15];
      *q++ = '>';
    } else {
      *q++ = (u1)c;
    }
  }
  *q = 0;
}

void format_uuid(void *uuid, char *to)
{
  u1 *from = (u1*)uuid;
  int i, c, variant, version;

  if (memcmp(uuid, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0) {
    strcpy(to, "nil");
    return;
  }

  variant = from[8] >> 5;
  version = from[6] >> 4;

  for (i = 0; i < 16; i++) {
    c = *from++;
    *to++ = "0123456789ABCDEF"[c >> 4];
    *to++ = "0123456789ABCDEF"[c & 15];
    if (i == 3 || i == 5 || i == 7 || i == 9)
      *to++ = '-';
  }

  if ((variant & 4) == 0) {         /* 0 x x */
    strcpy(to, " (NCS)");
  } else if ((variant & 2) == 0) {  /* 1 0 x */
    sprintf(to, " (DCE, v%1.1d)", version);
  } else if ((variant & 1) == 0) {  /* 1 1 0 */
    strcpy(to, " (MS GUID)");
  } else {                          /* 1 1 1 */
    strcpy(to, " (Reserved)");
  }
}

void format_uuid_lvm(void *uuid, char *to)
{
  char *from = (char*)uuid;
  int i;

  for (i = 0; i < 32; i++) {
    *to++ = *from++;
    if ((i & 3) == 1 && i > 1 && i < 29)
      *to++ = '-';
  }
  *to = 0;
}

void format_guid(void *guid, char *to)
{
  u1 *from = (u1*)guid;
  int i, c;

  if (memcmp(guid, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0) {
    strcpy(to, "nil");
    return;
  }

  for (i = 0; i < 16; i++) {
    c = *from++;
    *to++ = "0123456789ABCDEF"[c >> 4];
    *to++ = "0123456789ABCDEF"[c & 15];
    if (i == 3 || i == 5 || i == 7 || i == 9)
      *to++ = '-';
  }
  *to = 0;
}

/*
 * endian-aware data access
 */

u2 get_be_short(void *from)
{
  u1 *p = (u1*)from;
  return ((u2)(p[0]) << 8) +
    (u2)p[1];
}

u4 get_be_long(void *from)
{
  u1 *p = (u1*)from;
  return ((u4)(p[0]) << 24) +
    ((u4)(p[1]) << 16) +
    ((u4)(p[2]) << 8) +
    (u4)p[3];
}

u8 get_be_quad(void *from)
{
  u1 *p = (u1*)from;
  return ((u8)(p[0]) << 56) +
    ((u8)(p[1]) << 48) +
    ((u8)(p[2]) << 40) +
    ((u8)(p[3]) << 32) +
    ((u8)(p[4]) << 24) +
    ((u8)(p[5]) << 16) +
    ((u8)(p[6]) << 8) +
    (u8)p[7];
}

u2 get_le_short(void *from)
{
  u1 *p = (u1*)from;
  return ((u2)(p[1]) << 8) +
    (u2)p[0];
}

u4 get_le_long(void *from)
{
  u1 *p = (u1*)from;
  return ((u4)(p[3]) << 24) +
    ((u4)(p[2]) << 16) +
    ((u4)(p[1]) << 8) +
    (u4)p[0];
}

u8 get_le_quad(void *from)
{
  u1 *p = (u1*)from;
  return ((u8)(p[7]) << 56) +
    ((u8)(p[6]) << 48) +
    ((u8)(p[5]) << 40) +
    ((u8)(p[4]) << 32) +
    ((u8)(p[3]) << 24) +
    ((u8)(p[2]) << 16) +
    ((u8)(p[1]) << 8) +
    (u8)p[0];
}

u2 get_ve_short(int endianness, void *from)
{
  if (endianness)
    return get_le_short(from);
  else
    return get_be_short(from);
}

u4 get_ve_long(int endianness, void *from)
{
  if (endianness)
    return get_le_long(from);
  else
    return get_be_long(from);
}

u8 get_ve_quad(int endianness, void *from)
{
  if (endianness)
    return get_le_quad(from);
  else
    return get_be_quad(from);
}

const char * get_ve_name(int endianness)
{
  if (endianness)
    return "little-endian";
  else
    return "big-endian";
}

/*
 * more data access
 */

void get_string(void *from, int len, char *to)
{
  if (len > 255)
    len = 255;
  memcpy(to, from, len);
  to[len] = 0;
}

void get_pstring(void *from, char *to)
{
  int len = *(unsigned char *)from;
  memcpy(to, (char *)from + 1, len);
  to[len] = 0;
}

void get_padded_string(void *from, int len, char pad, char *to)
{
  int pos;

  get_string(from, len, to);

  for (pos = strlen(to) - 1; pos >= 0 && to[pos] == pad; pos--)
    to[pos] = 0;
}

int find_memory(void *haystack, int haystack_len,
                 const void *needle, int needle_len)
{
  int searchlen = haystack_len - needle_len + 1;
  int pos = 0;
  void *p;

  while (pos < searchlen) {
    p = memchr((char *)haystack + pos, *(unsigned char *)needle,
           searchlen - pos);
    if (p == NULL)
      return -1;
    pos = (char *)p - (char *)haystack;
    if (memcmp(p, needle, needle_len) == 0)
      return pos;
    pos++;
  }

  return -1;
}

/*
 * error functions
 */

void error(const char *msg, ...)
{
  va_list par;
  char buf[4096];

  va_start(par, msg);
  vsnprintf(buf, 4096, msg, par);
  va_end(par);

  fprintf(stderr, PROGNAME ": %s\n", buf);
}

void errore(const char *msg, ...)
{
  va_list par;
  char buf[4096];

  va_start(par, msg);
  vsnprintf(buf, 4096, msg, par);
  va_end(par);

  fprintf(stderr, PROGNAME ": %s: %s\n", buf, strerror(errno));
}

void bailout(const char *msg, ...)
{
  va_list par;
  char buf[4096];

  va_start(par, msg);
  vsnprintf(buf, 4096, msg, par);
  va_end(par);

  fprintf(stderr, PROGNAME ": %s\n", buf);
  exit(1);
}

void bailoute(const char *msg, ...)
{
  va_list par;
  char buf[4096];

  va_start(par, msg);
  vsnprintf(buf, 4096, msg, par);
  va_end(par);

  fprintf(stderr, PROGNAME ": %s: %s\n", buf, strerror(errno));
  exit(1);
}


struct systypes {
  unsigned char type;
  char *name;
};

struct systypes i386_sys_types[] = {
  { 0x00, "Empty" },
  { 0x01, "FAT12" },
  { 0x02, "XENIX root" },
  { 0x03, "XENIX usr" },
  { 0x04, "FAT16 <32M" },
  { 0x05, "Extended" },
  { 0x06, "FAT16" },
  { 0x07, "HPFS/NTFS" },
  { 0x08, "AIX" },
  { 0x09, "AIX bootable" },
  { 0x0a, "OS/2 Boot Manager" },
  { 0x0b, "Win95 FAT32" },
  { 0x0c, "Win95 FAT32 (LBA)" },
  { 0x0e, "Win95 FAT16 (LBA)" },
  { 0x0f, "Win95 Ext'd (LBA)" },
  { 0x10, "OPUS" },
  { 0x11, "Hidden FAT12" },
  { 0x12, "Compaq diagnostics" },
  { 0x14, "Hidden FAT16 <32M" },
  { 0x16, "Hidden FAT16" },
  { 0x17, "Hidden HPFS/NTFS" },
  { 0x18, "AST SmartSleep" },
  { 0x1b, "Hidden Win95 FAT32" },
  { 0x1c, "Hidden Win95 FAT32 (LBA)" },
  { 0x1e, "Hidden Win95 FAT16 (LBA)" },
  { 0x24, "NEC DOS" },
  { 0x39, "Plan 9" },
  { 0x3c, "PartitionMagic recovery" },
  { 0x40, "Venix 80286" },
  { 0x41, "PPC PReP Boot" },
  { 0x42, "SFS / MS LDM" },
  { 0x4d, "QNX4.x" },
  { 0x4e, "QNX4.x 2nd part" },
  { 0x4f, "QNX4.x 3rd part" },
  { 0x50, "OnTrack DM" },
  { 0x51, "OnTrack DM6 Aux1" },
  { 0x52, "CP/M" },
  { 0x53, "OnTrack DM6 Aux3" },
  { 0x54, "OnTrackDM6" },
  { 0x55, "EZ-Drive" },
  { 0x56, "Golden Bow" },
  { 0x5c, "Priam Edisk" },
  { 0x61, "SpeedStor" },
  { 0x63, "GNU HURD or SysV" },
  { 0x64, "Novell Netware 286" },
  { 0x65, "Novell Netware 386" },
  { 0x70, "DiskSecure Multi-Boot" },
  { 0x75, "PC/IX" },
  { 0x78, "XOSL" },
  { 0x80, "Old Minix" },
  { 0x81, "Minix / old Linux" },
  { 0x82, "Linux swap / Solaris" },
  { 0x83, "Linux" },
  { 0x84, "OS/2 hidden C: drive" },
  { 0x85, "Linux extended" },
  { 0x86, "NTFS volume set" },
  { 0x87, "NTFS volume set" },
  { 0x8e, "Linux LVM" },
  { 0x93, "Amoeba" },
  { 0x94, "Amoeba BBT" },
  { 0x9f, "BSD/OS" },
  { 0xa0, "IBM Thinkpad hibernation" },
  { 0xa5, "FreeBSD" },
  { 0xa6, "OpenBSD" },
  { 0xa7, "NeXTSTEP" },
  { 0xa9, "NetBSD" },
  { 0xaf, "Mac OS X" },
  { 0xb7, "BSDI fs" },
  { 0xb8, "BSDI swap" },
  { 0xbb, "Boot Wizard hidden" },
  { 0xc1, "DRDOS/sec (FAT-12)" },
  { 0xc4, "DRDOS/sec (FAT-16 < 32M)" },
  { 0xc6, "DRDOS/sec (FAT-16)" },
  { 0xc7, "Syrinx" },
  { 0xda, "Non-FS data" },
  { 0xdb, "CP/M / CTOS / ..." },
  { 0xde, "Dell Utility" },
  { 0xdf, "BootIt" },
  { 0xe1, "DOS access" },
  { 0xe3, "DOS R/O" },
  { 0xe4, "SpeedStor" },
  { 0xeb, "BeOS fs" },
  { 0xee, "EFI GPT protective" },
  { 0xef, "EFI System (FAT)" },
  { 0xf0, "Linux/PA-RISC boot" },
  { 0xf1, "SpeedStor" },
  { 0xf4, "SpeedStor" },
  { 0xf2, "DOS secondary" },
  { 0xfd, "Linux raid autodetect" },
  { 0xfe, "LANstep" },
  { 0xff, "BBT" },
  { 0, 0 }
};


char * get_name_for_mbrtype(int type)
{
  int i;

  for (i = 0; i386_sys_types[i].name; i++)
    if (i386_sys_types[i].type == type)
      return i386_sys_types[i].name;
  return "Unknown";
}

/*
 * DOS-style partition map / MBR
 */

static void detect_dos_partmap_ext(SECTION *section, u8 extbase,
                   int level, int *extpartnum);

void detect_dos_partmap(SECTION *section, int level)
{
  unsigned char *buf;
  int i, off, used, type, types[4], bootflags[4];
  u4 start, size, starts[4], sizes[4];
  int extpartnum = 5;
  char s[256], append[64];

  /* partition maps only occur at the start of a device */
  if (section->pos != 0)
    return;

  if (get_buffer(section, 0, 512, (void **)&buf) < 512)
    return;

  /* check signature */
  if (buf[510] != 0x55 || buf[511] != 0xAA)
    return;

  /* get entries and check */
  used = 0;
  for (off = 446, i = 0; i < 4; i++, off += 16) {
    /* get data */
    bootflags[i] = buf[off];
    types[i] = buf[off + 4];
    starts[i] = get_le_long(buf + off + 8);
    sizes[i] = get_le_long(buf + off + 12);

    /* bootable flag: either on or off */
    if (bootflags[i] != 0x00 && bootflags[i] != 0x80)
      return;
    /* size non-zero -> entry in use */
    if (starts[i] && sizes[i])
      used = 1;
  }
  if (!used)
    return;

  /* parse the data for real */
  //print_line(level, "DOS/MBR partition map");
  for (i = 0; i < 4; i++) {
    start = starts[i];
    size = sizes[i];
    type = types[i];
    if (start == 0 || size == 0)
      continue;

    sprintf(append, " from %lu", start);
    if (bootflags[i] == 0x80)
      strcat(append, ", bootable");
    format_blocky_size(s, size, 512, "sectors", append);
    //print_line(level, "Partition %d: %s",
    //       i+1, s);

    //print_line(level + 1, "Type 0x%02X (%s)", type, get_name_for_mbrtype(type));

    if (type == 0x05 || type == 0x0f || type == 0x85) {
      /* extended partition */
      detect_dos_partmap_ext(section, start, level + 1, &extpartnum);
    } else if (type != 0xee) {
      /* recurse for content detection */
      analyze_recursive(section, level + 1,
            (u8)start * 512, (u8)size * 512, 0);
    }
  }
}

static void detect_dos_partmap_ext(SECTION *section, u8 extbase,
                   int level, int *extpartnum)
{
  unsigned char *buf;
  u8 tablebase, nexttablebase;
  int i, off, type, types[4];
  u4 start, size, starts[4], sizes[4];
  char s[256], append[64];

  for (tablebase = extbase; tablebase; tablebase = nexttablebase) {
    /* read sector from linked list */
    if (get_buffer(section, tablebase << 9, 512, (void **)&buf) < 512)
      return;

    /* check signature */
    if (buf[510] != 0x55 || buf[511] != 0xAA) {
      print_line(level, "Signature missing");
      return;
    }

    /* get entries */
    for (off = 446, i = 0; i < 4; i++, off += 16) {
      types[i] = buf[off + 4];
      starts[i] = get_le_long(buf + off + 8);
      sizes[i] = get_le_long(buf + off + 12);
    }

    /* parse the data for real */
    nexttablebase = 0;
    for (i = 0; i < 4; i++) {
      start = starts[i];
      size = sizes[i];
      type = types[i];
      if (size == 0)
    continue;

      if (type == 0x05 || type == 0x85) {
    /* inner extended partition */

    nexttablebase = extbase + start;

      } else {
    /* logical partition */

    sprintf(append, " from %llu+%lu", tablebase, start);
    format_blocky_size(s, size, 512, "sectors", append);
    print_line(level, "Partition %d: %s",
           *extpartnum, s);
    (*extpartnum)++;
    print_line(level + 1, "Type 0x%02X (%s)", type, get_name_for_mbrtype(type));

    /* recurse for content detection */
    if (type != 0xee) {
      analyze_recursive(section, level + 1,
                (tablebase + start) * 512, (u8)size * 512, 0);
    }
      }
    }
  }
}



/*
 * DOS/Windows boot loaders
 */

void detect_dos_loader(SECTION *section, int level)
{
  int fill;
  unsigned char *buf;

  if (section->flags & FLAG_IN_DISKLABEL)
    return;

  fill = get_buffer(section, 0, 2048, (void **)&buf);
  if (fill < 512)
    return;


  if (find_memory(buf, fill, "NTLDR", 5) >= 0)
    section->info->os_type=WINDOW;//print_line(level, "Windows NTLDR boot loader");
  else if (find_memory(buf, 512, "WINBOOT SYS", 11) >= 0)
    section->info->os_type=WINDOW;//print_line(level, "Windows 95/98/ME boot loader");
  else if (find_memory(buf, 512, "MSDOS   SYS", 11) >= 0)
    section->info->os_type=WINDOW;//print_line(level, "Windows / MS-DOS boot loader");

}

static char *levels[] = {
  "Multipath",
  "\'HSM\'",
  "\'translucent\'",
  "Linear",
  "RAID0",
  "RAID1",
  NULL, NULL,
  "RAID4(?)",
  "RAID5"
};


/*
 *  code signatures
 */

void detect_linux_loader(SECTION *section, int level)
{
  int fill, executable, id;
  unsigned char *buf;

  if (section->flags & FLAG_IN_DISKLABEL)
    return;

  fill = get_buffer(section, 0, 2048, (void **)&buf);
  if (fill < 512)
    return;

  executable = (get_le_short(buf + 510) == 0xaa55) ? 1 : 0;

  /* boot sector stuff */
  if (executable && (memcmp(buf + 2, "LILO", 4) == 0 ||
             memcmp(buf + 6, "LILO", 4) == 0)){
    print_line(level, "LILO boot loader");
    section->info->os_type=LINUX;
  }
  if (executable && memcmp(buf + 3, "SYSLINUX", 8) == 0){
    //print_line(level, "SYSLINUX boot loader");
    section->info->os_type=LINUX;
  }
  if (fill >= 1024 && find_memory(buf, fill, "ISOLINUX", 8) >= 0){
      section->info->os_type=LINUX;
      //print_line(level, "ISOLINUX boot loader");
  }

  /* we know GRUB a little better... */
  if (executable &&
      find_memory(buf, 512, "Geom\0Hard Disk\0Read\0 Error\0", 27) >= 0) {
    if (buf[0x3e] == 3) {
      print_line(level, "GRUB boot loader, compat version %d.%d, boot drive 0x%02x",
         (int)buf[0x3e], (int)buf[0x3f], (int)buf[0x40]);
    } else if (executable && buf[0x1bc] == 2 && buf[0x1bd] <= 2) {
      id = buf[0x3e];
      if (id == 0x10) {
    print_line(level, "GRUB boot loader, compat version %d.%d, normal version",
           (int)buf[0x1bc], (int)buf[0x1bd]);
      } else if (id == 0x20) {
    print_line(level, "GRUB boot loader, compat version %d.%d, LBA version",
           (int)buf[0x1bc], (int)buf[0x1bd]);
      } else {
    print_line(level, "GRUB boot loader, compat version %d.%d",
           (int)buf[0x1bc], (int)buf[0x1bd]);
      }
    } else {
      print_line(level, "GRUB boot loader, unknown compat version %d",
         buf[0x3e]);
    }
  }

  /* -- */
  if (fill >= 1024 && memcmp(buf + 512 + 2, "HdrS", 4) == 0) {
    print_line(level, "Linux kernel build-in loader");
  }

  /* (not exactly boot code, but should be detected before gzip/tar) */
  if (memcmp(buf, "Floppy split ", 13) == 0) {
    char *name = (char *)buf + 32;
    char *number = (char *)buf + 164;
    char *total = (char *)buf + 172;
    print_line(level, "Debian floppy split, name \"%s\", disk %s of %s",
           name, number, total);
  }
}


#define PROFILE 0

/* this defines 4K chunks, should only be changed upwards */
#define CHUNKBITS (12)
#define CHUNKSIZE (1<<CHUNKBITS)
#define CHUNKMASK (CHUNKSIZE-1)

/* the minimum block size for block-oriented sources, maximum is the
   chunk size */
#define MINBLOCKSIZE (256)

/* a simple hash function */
#define HASHSIZE (13)
#define HASHFUNC(start) (((start)>>CHUNKBITS) % HASHSIZE)

/* convenience */
#define MINIMUM(a,b) (((a) < (b)) ? (a) : (b))
#define MAXIMUM(a,b) (((a) > (b)) ? (a) : (b))

typedef struct chunk {
  u8 start, end, len;
  void *buf;
  /* links within a hash bucket, organized as a ring list */
  struct chunk *next, *prev;
} CHUNK;

typedef struct cache {
  /* chunks stored as a hash table of ring lists */
  CHUNK *hashtab[HASHSIZE];
  /* temporary buffer for requests involving several chunks */
  void *tempbuf;
} CACHE;

static CHUNK * ensure_chunk(SOURCE *s, CACHE *cache, u8 start);
static CHUNK * get_chunk_alloc(CACHE *cache, u8 start);

u8 get_buffer(SECTION *section, u8 pos, u8 len, void **buf)
{
  SOURCE *s;

  /* get source info */
  s = section->source;
  pos += section->pos;

  return get_buffer_real(s, pos, len, NULL, buf);
}

/*
 * actual retrieval, entry point for layering
 */

u8 get_buffer_real(SOURCE *s, u8 pos, u8 len, void *inbuf, void **outbuf)
{
  CACHE *cache;
  CHUNK *c;
  u8 end, first_chunk, last_chunk, curr_chunk, got, tocopy;
  void *mybuf;

  /* sanity check */
  if (len == 0 || (inbuf == NULL && outbuf == NULL))
    return 0;

  /* get source info */
  if (s->size_known && pos >= s->size) {
    /* error("Request for data beyond end of file (pos %llu)", pos); */
    return 0;
  }
  end = pos + len;
  if (s->size_known && end > s->size) {
    /* truncate to known end of file */
    end = s->size;
    len = end - pos;
  }

  /* get cache head */
  cache = (CACHE *)s->cache_head;
  if (cache == NULL) {
    /* allocate and initialize new cache head */
    cache = (CACHE *)malloc(sizeof(CACHE));
    if (cache == NULL)
      bailout("Out of memory");
    memset(cache, 0, sizeof(CACHE));
    s->cache_head = (void *)cache;
  }
  /* free old temp buffer if present */
  if (cache->tempbuf != NULL) {
    free(cache->tempbuf);
    cache->tempbuf = NULL;
  }

  /* calculate involved chunks */
  first_chunk = pos & ~CHUNKMASK;
  last_chunk = (end - 1) & ~CHUNKMASK;

  if (last_chunk == first_chunk) {
    /* just get the matching chunk */
    c = ensure_chunk(s, cache, first_chunk);
    /* NOTE: first_chunk == c->start */

    if (pos >= c->end)  /* chunk is incomplete and doesn't have our data */
      return 0;

    /* calculate return data */
    len = MINIMUM(len, c->end - pos);  /* guaranteed to be > 0 */
    mybuf = c->buf + (pos - c->start);
    if (inbuf)
      memcpy(inbuf, mybuf, len);
    if (outbuf)
      *outbuf = mybuf;

    return len;

  } else {

    /* prepare a buffer for concatenation */
    if (inbuf) {
      mybuf = inbuf;
    } else {

#if PROFILE
      printf("Temporary buffer for request %llu:%llu\n", pos, len);
#endif

      /* allocate one temporarily, will be free()'d at the next call */
      cache->tempbuf = malloc(len);
      if (cache->tempbuf == NULL) {
    error("Out of memory, still going");
    return 0;
      }
      mybuf = cache->tempbuf;
    }

    /* draw data from all covered chunks */
    got = 0;
    for (curr_chunk = first_chunk; curr_chunk <= last_chunk;
     curr_chunk += CHUNKSIZE) {
      /* get that chunk */
      c = ensure_chunk(s, cache, curr_chunk);
      /* NOTE: curr_chunk == c->start */

      if (pos > curr_chunk) {
    /* copy from middle of chunk */
    if (c->end > pos) {
      tocopy = c->end - pos;
      memcpy(mybuf, c->buf + (pos & CHUNKMASK), tocopy);
    } else
      tocopy = 0;
      } else {
    /* copy from start of chunk */
    tocopy = MINIMUM(c->len, len - got);  /* c->len can be zero */
    if (tocopy)
      memcpy(mybuf + got, c->buf, tocopy);
      }
      got += tocopy;

      /* stop after an incomplete chunk (possibly-okay for the last one,
     not-so-nice for earlier ones, but treated all the same) */
      if (c->len < CHUNKSIZE)
    break;
    }

    /* calculate return data */
    len = MINIMUM(len, got);  /* may be zero */
    if (outbuf)
      *outbuf = mybuf;
    return len;

  }
}

static CHUNK * ensure_chunk(SOURCE *s, CACHE *cache, u8 start)
{
  CHUNK *c;
  u8 pos, rel_start, rel_end;
  u8 toread, result, curr_chunk;

  c = get_chunk_alloc(cache, start);
  if (c->len >= CHUNKSIZE || (s->size_known && c->end >= s->size)) {
    /* chunk is complete  or  complete until EOF */
    return c;
  }

  if (s->sequential) {
    /* sequential source: ensure all data before this chunk was read */

    if (s->seq_pos < start) {
      /* try to read data between seq_pos and start */
      curr_chunk = s->seq_pos & ~CHUNKMASK;
      while (curr_chunk < start) {  /* runs at least once, due to the if()
                       and the formula of curr_chunk */
    ensure_chunk(s, cache, curr_chunk);
    curr_chunk += CHUNKSIZE;
    if (s->seq_pos < curr_chunk)
      break;  /* it didn't work out... */
      }

      /* re-check precondition since s->size may have changed */
      if (s->size_known && c->end >= s->size)
    return c;  /* there is no more data to read */
    }

    if (s->seq_pos != c->end)   /* c->end is where we'll continue reading */
      return c;  /* we're not in a sane state, give up */
  }

  /* try to read the missing piece */
  if (s->read_block != NULL) {
    /* use block-oriented read_block() method */

    if (s->blocksize < MINBLOCKSIZE ||
    s->blocksize > CHUNKSIZE ||
    ((s->blocksize & (s->blocksize - 1)) != 0)) {
      bailout("Internal error: Invalid block size %d", s->blocksize);
    }

    for (rel_start = 0; rel_start < CHUNKSIZE; rel_start = rel_end) {
      rel_end = rel_start + s->blocksize;
      if (c->len >= rel_end)
    continue;  /* already read */
      pos = c->start + rel_start;
      if (s->size_known && pos >= s->size)
    break;     /* whole block is past end of file */

      /* read it */
      if (s->read_block(s, pos, c->buf + rel_start)) {
    /* success */
    c->len = rel_end;
    c->end = c->start + c->len;
      } else {
    /* failure */
    c->len = rel_start;  /* this is safe as it can only mean a shrink */
    c->end = c->start + c->len;
    /* note the new end of file if necessary */
    if (!s->size_known || s->size > c->end) {
      s->size_known = 1;
      s->size = c->end;
    }
    break;
      }
    }

  } else {
    /* use byte-oriented read_bytes() method */

    if (s->size_known && s->size < c->start + CHUNKSIZE) {
      /* do not try to read beyond known end of file */
      toread = s->size - c->end;
      /* toread cannot be zero or negative due to the preconditions */
    } else {
      toread = CHUNKSIZE - c->len;
    }
    result = s->read_bytes(s, c->start + c->len, toread,
               c->buf + c->len);
    if (result > 0) {
      /* adjust offsets */
      c->len += result;
      c->end = c->start + c->len;
      if (s->sequential)
    s->seq_pos += result;
    }
    if (result < toread) {
      /* we fell short, so it must have been an error or end-of-file */
      /* make sure we don't try again */
      if (!s->size_known || s->size > c->end) {
    s->size_known = 1;
    s->size = c->end;
      }
    }
  }

  return c;
}

static CHUNK * get_chunk_alloc(CACHE *cache, u8 start)
{
  int hpos;
  CHUNK *chain, *trav, *c;

  /* get hash bucket (chain) */
  hpos = HASHFUNC(start);
  chain = cache->hashtab[hpos];

  if (chain == NULL) {
    /* bucket is empty, allocate new chunk */
    c = (CHUNK *)malloc(sizeof(CHUNK));
    if (c == NULL)
      bailout("Out of memory");
    c->buf = malloc(CHUNKSIZE);
    if (c->buf == NULL)
      bailout("Out of memory");
    c->start = start;
    c->end = start;
    c->len = 0;
    /* create a new ring list */
    c->prev = c->next = c;
    cache->hashtab[hpos] = c;
    return c;
  }

  /* travel the ring list, looking for the wanted chunk */
  trav = chain;
  do {
    if (trav->start == start)  /* found existing chunk */
      return trav;
    trav = trav->next;
  } while (trav != chain);

  /* not found, allocate new chunk */
  c = (CHUNK *)malloc(sizeof(CHUNK));
  if (c == NULL)
    bailout("Out of memory");
  c->buf = malloc(CHUNKSIZE);
  if (c->buf == NULL)
    bailout("Out of memory");
  c->start = start;
  c->end = start;
  c->len = 0;
  /* add to ring list before chain, becomes new head */
  c->prev = chain->prev;
  c->next = chain;
  c->prev->next = c;
  c->next->prev = c;
  cache->hashtab[hpos] = c;
  return c;
}

/*
 * dispose of a source
 */

void close_source(SOURCE *s)
{
  CACHE *cache;
  int hpos;
  CHUNK *chain, *trav, *nexttrav;

  /* drop the cache */
  cache = (CACHE *)s->cache_head;
  if (cache != NULL) {
#if PROFILE
    printf("Cache profile:\n");
#endif
    if (cache->tempbuf != NULL)
      free(cache->tempbuf);
    for (hpos = 0; hpos < HASHSIZE; hpos++) {
#if PROFILE
      printf(" hash position %d:", hpos);
#endif
      chain = cache->hashtab[hpos];
      if (chain != NULL) {
    trav = chain;
    do {
#if PROFILE
      printf(" %lluK", trav->start >> 10);
      if (trav->len != CHUNKSIZE)
        printf(":%llu", trav->len);
#endif
      nexttrav = trav->next;
      free(trav->buf);
      free(trav);
      trav = nexttrav;
    } while (trav != chain);
#if PROFILE
    printf("\n");
#endif
      }
#if PROFILE
      else
    printf(" empty\n");
#endif
    }
  }

  /* type-specific cleanup */
  if (s->close != NULL)
    (*s->close)(s);

  /* release memory for the structure */
  free(s);
}



extern "C"
{
#ifdef USE_IOCTL_LINUX
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#define DO_CDACCESS 1
#endif  /* USE_IOCTL_LINUX */
}

#ifdef DO_CDACCESS



static int cddb_sum(int n);

#define LBA_TO_SECS(lba) (((lba) + 150) / 75)


int analyze_cdaccess(int fd, SOURCE *s, int level)
{
  int i;
  int first, last, ntracks;
  int cksum, totaltime, seconds;
  u4 diskid;
  u1 ctrl[100];
  u4 lba[100], length;
  char human_readable_size[256];
#ifdef USE_IOCTL_LINUX
  struct cdrom_tochdr tochdr;
  struct cdrom_tocentry tocentry;
#endif

  /* read TOC header */
#ifdef USE_IOCTL_LINUX
  if (ioctl(fd, CDROMREADTOCHDR, &tochdr) < 0) {
    return 0;
  }
  first = tochdr.cdth_trk0;
  last  = tochdr.cdth_trk1;
#endif

  ntracks = last + 1 - first;
  if (ntracks > 99)  /* natural limit */
    return 0;

  /* read per-track data from TOC */
  for (i = 0; i <= ntracks; i++) {
#ifdef USE_IOCTL_LINUX
    if (i == ntracks)
      tocentry.cdte_track = CDROM_LEADOUT;
    else
      tocentry.cdte_track = first + i;
    tocentry.cdte_format = CDROM_LBA;
    if (ioctl(fd, CDROMREADTOCENTRY, &tocentry) < 0) {
      //printf("CDROMREADTOCENTRY: %s: ", strerror(errno));
      return 0;
    }
    ctrl[i] = tocentry.cdte_ctrl;
    lba[i] = tocentry.cdte_addr.lba;
#endif
  }

  /* System-dependent code ends here. From now on, we use the data
     in first, last, ntracks, ctrl[], and lba[]. We also assume
     all systems treat actual data access the same way... */

  /* calculate CDDB disk id */
  cksum = 0;
  for (i = 0; i < ntracks; i++) {
    cksum += cddb_sum(LBA_TO_SECS(lba[i]));
  }
  totaltime = LBA_TO_SECS(lba[ntracks]) - LBA_TO_SECS(lba[0]);
  diskid = (u4)(cksum % 0xff) << 24 | (u4)totaltime << 8 | (u4)ntracks;

  /* print disk info */
  /*
  print_line(level, "CD-ROM, %d track%s, CDDB disk ID %08lX",
         ntracks, (ntracks != 1) ? "s" : "", diskid);
  */


  /* Loop over each track */
  for (i = 0; i < ntracks; i++) {
    /* length of track in sectors */
    length = lba[i+1] - lba[i];

    if ((ctrl[i] & 0x4) == 0) {
      /* Audio track, one sector holds 2352 actual data bytes */
      seconds = length / 75;
      format_size(human_readable_size, (u8)length * 2352);

      /*
      print_line(level, "Track %d: Audio track, %s, %3d min %02d sec",
         first + i, human_readable_size,
         seconds / 60, seconds % 60);
      */
    } else {
      /* Data track, one sector holds 2048 actual data bytes */
      format_size(human_readable_size, length * 2048);

      /*
      print_line(level, "Track %d: Data track, %s",
         first + i, human_readable_size);
      */

      /* NOTE: we adjust the length to stay clear of padding or
     post-gap stuff */
      analyze_source_special(s, level + 1,
                 (u8)lba[i] * 2048, (u8)(length - 250) * 2048);
    }
  }

  return 1;
}

/*
 * helper function: calculate cddb disk id
 */

static int cddb_sum(int n)
{
  /* a number like 2344 becomes 2+3+4+4 (13) */
  int ret = 0;

  while (n > 0) {
    ret = ret + (n % 10);
    n = n / 10;
  }

  return ret;
}


#else   /* DO_CDACCESS */

/*
 * the system is not supported, so use a dummy function
 */

int analyze_cdaccess(int fd, SOURCE *s, int level)
{
  return 0;
}

#endif




typedef struct cdimage_source {
  SOURCE c;
  u8 off;
} CDIMAGE_SOURCE;

/*
 * helper functions
 */

static SOURCE *init_cdimage_source(SOURCE *foundation, u8 offset);
static int read_block_cdimage(SOURCE *s, u8 pos, void *buf);

/*
 * cd image detection
 */

static unsigned char syncbytes[12] =
  { 0, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 0 };

void detect_cdimage(SECTION *section, int level)
{
  int mode, off;
  unsigned char *buf;
  SOURCE *s;

  if (get_buffer(section, 0, 2352, (void **)&buf) < 2352)
    return;

  /* check sync bytes as signature */
  if (memcmp(buf, syncbytes, 12) != 0)
    return;

  /* get mode of the track -- this determines sector layout */
  mode = buf[15];
  if (mode == 1) {
    /* standard data track */
    print_line(level, "Raw CD image, Mode 1");
    off = 16;
  } else if (mode == 2) {
    /* free-form track, assume XA form 1 */
    print_line(level, "Raw CD image, Mode 2, assuming Form 1");
    off = 24;
  } else
    return;

  /* create and analyze wrapped source */
  s = init_cdimage_source(section->source, section->pos + off);
  analyze_source(s, level);
  close_source(s);

  /* don't run other analyzers */
  stop_detect();
}

/*
 * initialize the cd image source
 */

static SOURCE *init_cdimage_source(SOURCE *foundation, u8 offset)
{
  CDIMAGE_SOURCE *src;

  src = (CDIMAGE_SOURCE *)malloc(sizeof(CDIMAGE_SOURCE));
  if (src == NULL)
    bailout("Out of memory");
  memset(src, 0, sizeof(CDIMAGE_SOURCE));

  if (foundation->size_known) {
    src->c.size_known = 1;
    src->c.size = ((foundation->size - offset + 304) / 2352) * 2048;
    /* TODO: pass the size in from the SECTION record and use it */
  }
  src->c.blocksize = 2048;
  src->c.foundation = foundation;
  src->c.read_block = read_block_cdimage;
  src->c.close = NULL;
  src->off = offset;

  return (SOURCE *)src;
}

/*
 * raw read
 */

static int read_block_cdimage(SOURCE *s, u8 pos, void *buf)
{
  SOURCE *fs = s->foundation;
  u8 filepos;

  /* translate position */
  filepos = (pos / 2048) * 2352 + ((CDIMAGE_SOURCE *)s)->off;

  /* read from lower layer */
  if (get_buffer_real(fs, filepos, 2048, buf, NULL) < 2048)
    return 0;

  return 1;
}





static void dump_boot_catalog(SECTION *section, u8 pos, int level);
void detect_iso(SECTION *section, int level)
{
  char s[256], t[256];
  int i, sector, type;
  u4 blocksize;
  u8 blocks, bcpos;
  unsigned char *buf;

  /* get the volume descriptor */
  if (get_buffer(section, 32768, 2048, (void **)&buf) < 2048)
    return;

  /* check signature */
  if (memcmp(buf, "\001CD001", 6) != 0)
    return;

  //print_line(level, "ISO9660 file system");

  /* read Volume ID and other info */
  get_padded_string(buf + 40, 32, ' ', s);
  //print_line(level+1, "Volume name \"%s\"", s);
  strcpy(section->info->volumeLabel,s);

  get_padded_string(buf + 318, 128, ' ', s);
  if (s[0])
    ;   //print_line(level+1, "Publisher   \"%s\"", s);

  get_padded_string(buf + 446, 128, ' ', s);
  if (s[0])
    ;   //print_line(level+1, "Preparer    \"%s\"", s);

  get_padded_string(buf + 574, 128, ' ', s);
  if (s[0])
    ;   //print_line(level+1, "Application \"%s\"", s);

  blocks = get_le_long(buf + 80);
  blocksize = get_le_short(buf + 128);
  //printf("%d,%d,%ld\n",blocks,blocksize,blocks*blocksize);
  format_blocky_size(s, blocks, blocksize, "blocks", NULL);
  //print_line(level+1, "Data size %s", s);

  for (sector = 17; ; sector++) {
    /* get next descriptor */
    if (get_buffer(section, sector * 2048, 2048, (void **)&buf) < 2048)
      return;

    /* check signature */
    if (memcmp(buf + 1, "CD001", 5) != 0) {
      print_line(level+1, "Signature missing in sector %d", sector);
      return;
    }
    type = buf[0];
    if (type == 255)
      break;

    switch (type) {

    case 0:  /* El Torito */
      /* check signature */
      if (memcmp(buf+7, "EL TORITO SPECIFICATION", 23) != 0) {
    print_line(level+1, "Boot record of unknown format");
    break;
      }

      bcpos = get_le_long(buf + 0x47);
      //print_line(level+1, "El Torito boot record, catalog at %llu", bcpos);

      /* boot catalog */
      dump_boot_catalog(section, bcpos * 2048, level + 2);

      break;

    case 1:  /* Primary Volume Descriptor */
      print_line(level+1, "Additional Primary Volume Descriptor");
      break;

    case 2:  /* Supplementary Volume Descriptor, Joliet */
      /* read Volume ID */
      format_utf16_be(buf + 40, 32, t);
      for (i = strlen(t)-1; i >= 0 && t[i] == ' '; i--)
    t[i] = 0;
      //print_line(level+1, "Joliet extension, volume name \"%s\"", t);
      break;

    case 3:  /* Volume Partition Descriptor */
      print_line(level+1, "Volume Partition Descriptor");
      break;

    default:
      print_line(level+1, "Descriptor type %d at sector %d", type, sector);
      break;
    }
  }
}

static char *media_types[16] = {
  "non-emulated",
  "1.2M floppy",
  "1.44M floppy",
  "2.88M floppy",
  "hard disk",
  "reserved type 5", "reserved type 6", "reserved type 7",
  "reserved type 8", "reserved type 9", "reserved type 10",
  "reserved type 11", "reserved type 12", "reserved type 13",
  "reserved type 14", "reserved type 15"
};

static char * get_name_for_eltorito_platform(int id)
{
  if (id == 0)
    return "x86";
  if (id == 1)
    return "PowerPC";
  if (id == 2)
    return "Macintosh";
  if (id == 0xEF)
    return "EFI";
  return "unknown";
}

static void dump_boot_catalog(SECTION *section, u8 pos, int level)
{
  unsigned char *buf;
  int bootable, media, platform, system_type;
  u4 start, preload;
  int entry, maxentry, off;
  char s[256];

  /* get boot catalog */
  if (get_buffer(section, pos, 2048, (void **)&buf) < 2048)
    return;

  /* check validation entry (must be first) */
  if (buf[0] != 0x01 || buf[30] != 0x55 || buf[31] != 0xAA) {
    print_line(level, "Validation entry missing");
    return;
  }
  /* TODO: check checksum of the validation entry */
  platform = buf[1];
  /* TODO: ID string at bytes 0x04 - 0x1B */

  maxentry = 2;
  for (entry = 1; entry < maxentry + 1; entry++) {
    if ((entry & 63) == 0) {
      /* get the next CD sector */
      if (get_buffer(section, pos + (entry / 64) * 2048, 2048, (void **)&buf) < 2048)
        return;
    }
    off = (entry * 32) % 2048;

    if (entry >= maxentry) {
      if (buf[off] == 0x88)  /* more bootable entries without proper section headers */
        maxentry++;
      else
        break;
    }

    if (entry == 1) {
      if (!(buf[off] == 0x88 || buf[off] == 0x00)) {
        print_line(level, "Initial/Default entry missing");
        break;
      }
      if (buf[off + 32] == 0x90 || buf[off + 32] == 0x91)
        maxentry = 3;
    }

    if (buf[off] == 0x88 || buf[off] == 0x00) {   /* boot entry */
      bootable = (buf[off] == 0x88) ? 1 : 0;
      media = buf[off + 1] & 15;
      system_type = buf[off + 4];
      start = get_le_long(buf + off + 8);
      preload = get_le_short(buf + off + 6);

      /* print and analyze further */
      format_size(s, preload * 512);

      /*
      print_line(level, "%s %s image, starts at %lu, preloads %s",
                 bootable ? "Bootable" : "Non-bootable",
                 media_types[media], start, s);
      */

      section->info->bootable=bootable;

      /*
      print_line(level + 1, "Platform 0x%02X (%s), System Type 0x%02X (%s)",
                 platform, get_name_for_eltorito_platform(platform),
                 system_type, get_name_for_mbrtype(system_type));
      */

      if(strlen(section->info->platform)==0)
        strcpy(section->info->platform,get_name_for_eltorito_platform(platform));
      if (start > 0) {
        analyze_recursive(section, level + 1,
                          (u8)start * 2048, 0, 0);
        /* TODO: calculate size in some way */
      }

    } else if (buf[off] == 0x44) {   /* entry extension */
      maxentry++;   /* doesn't count against the entry count from the last section header */

    } else if (buf[off] == 0x90 || buf[off] == 0x91) {   /* section header */
      platform = buf[off + 1];
      maxentry = entry + 1 + get_le_short(buf + off + 2)
               + ((buf[off] == 0x90) ? 1 : 0);
      /* TODO: ID string at bytes 0x04 - 0x1F */

    } else {
      print_line(level, "Unknown entry type 0x%02X", buf[off]);
      break;
    }
  }
}

/*
 * Various CD file systems
 */

void detect_cdrom_misc(SECTION *section, int level)
{
  unsigned char *buf;

  /* get first sector */
  if (get_buffer(section, 0, 2048, (void **)&buf) < 2048)
    return;

  /* Sega Dreamcast signature */
  if (memcmp(buf, "SEGA SEGAKATANA SEGA ENTERPRISES", 32) == 0) {
    print_line(level, "Sega Dreamcast signature");
  }

  /* 3DO filesystem */
  if (memcmp(buf, "\x01\x5a\x5a\x5a\x5a\x5a\x01\x00", 8) == 0 &&
      memcmp(buf + 0x28, "CD-ROM", 6) == 0) {
    print_line(level, "3DO CD-ROM file system");
  }

  /* get sector 32 */
  if (get_buffer(section, 32*2048, 2048, (void **)&buf) < 2048)
    return;

  /* Xbox DVD file system */
  if (memcmp(buf, "MICROSOFT*XBOX*MEDIA", 20) == 0 &&
      memcmp(buf + 0x7ec, "MICROSOFT*XBOX*MEDIA", 20) == 0) {
    print_line(level, "Xbox DVD file system");
  }
}

/* EOF */
