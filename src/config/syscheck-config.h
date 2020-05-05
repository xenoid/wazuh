/* Copyright (C) 2015-2020, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#ifndef SYSCHECKC_H
#define SYSCHECKC_H

typedef enum fim_event_mode {
    FIM_SCHEDULED,
    FIM_REALTIME,
    FIM_WHODATA
} fim_event_mode;

typedef enum fdb_stmt {
    FIMDB_STMT_INSERT_DATA,
    FIMDB_STMT_INSERT_PATH,
    FIMDB_STMT_GET_PATH,
    FIMDB_STMT_UPDATE_DATA,
    FIMDB_STMT_UPDATE_PATH,
    FIMDB_STMT_GET_LAST_PATH,
    FIMDB_STMT_GET_FIRST_PATH,
    FIMDB_STMT_GET_ALL_ENTRIES,
    FIMDB_STMT_GET_NOT_SCANNED,
    FIMDB_STMT_SET_ALL_UNSCANNED,
    FIMDB_STMT_GET_PATH_COUNT,
    FIMDB_STMT_GET_DATA_ROW,
    FIMDB_STMT_GET_COUNT_RANGE,
    FIMDB_STMT_GET_PATH_RANGE,
    FIMDB_STMT_DELETE_PATH,
    FIMDB_STMT_DELETE_DATA,
    FIMDB_STMT_GET_PATHS_INODE,
    FIMDB_STMT_GET_PATHS_INODE_COUNT,
    FIMDB_STMT_SET_SCANNED,
    FIMDB_STMT_GET_INODE_ID,
    FIMDB_STMT_GET_COUNT_PATH,
    FIMDB_STMT_GET_COUNT_DATA,
    FIMDB_STMT_GET_INODE,
    FIMDB_STMT_SIZE
} fdb_stmt;

#define FIM_MODE(x) (x & WHODATA_ACTIVE ? FIM_WHODATA : x & REALTIME_ACTIVE ? FIM_REALTIME : FIM_SCHEDULED)

#if defined(WIN32) && defined(EVENTCHANNEL_SUPPORT)
#define WIN_WHODATA 1
#endif

#define MAX_DIR_SIZE    64
#define MAX_DIR_ENTRY   128
#define SYSCHECK_WAIT   1

/* Checking options */
#define CHECK_SIZE          00000001
#define CHECK_PERM          00000002
#define CHECK_OWNER         00000004
#define CHECK_GROUP         00000010
#define CHECK_MTIME         00000020
#define CHECK_INODE         00000040
#define CHECK_MD5SUM        00000100
#define CHECK_SHA1SUM       00000200
#define CHECK_SHA256SUM     00000400
// 0001000 0002000 0004000 Reserved for future hash functions
#define CHECK_ATTRS         00010000
#define CHECK_SEECHANGES    00020000
#define CHECK_FOLLOW        00040000
#define REALTIME_ACTIVE     00100000
#define WHODATA_ACTIVE      00200000
#define SCHEDULED_ACTIVE    00400000

#define ARCH_32BIT          0
#define ARCH_64BIT          1
#define ARCH_BOTH           2

#ifdef WIN32
/* Whodata  states */
#define WD_STATUS_FILE_TYPE 1
#define WD_STATUS_DIR_TYPE  2
#define WD_STATUS_UNK_TYPE  3
#define WD_SETUP_AUTO       0
#define WD_SETUP_SUCC       1
#define WD_SETUP_SUCC_FAIL  2
#define WD_STATUS_EXISTS    0x0000001
#define WD_CHECK_WHODATA    0x0000002
#define WD_CHECK_REALTIME   0x0000004
#define WD_IGNORE_REST      0x0000008
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

#define SK_CONF_UNPARSED    -2
#define SK_CONF_UNDEFINED   -1

#define FIM_DB_MEMORY       1
#define FIM_DB_DISK         0

#define FIM_TYPE_FILE       0
#define FIM_TYPE_REGISTRY   1

//Max allowed value for recursion
#define MAX_DEPTH_ALLOWED 320

#include "os_crypto/md5_sha1_sha256/md5_sha1_sha256_op.h"
#include "headers/integrity_op.h"
#include "external/sqlite/sqlite3.h"

#ifdef WIN32
typedef struct whodata_event_node whodata_event_node;
typedef struct whodata_dir_status whodata_dir_status;
#endif

typedef struct _rtfim {
    int fd;
    OSHash *dirtb;
#ifdef WIN32
    HANDLE evt;
#endif
} rtfim;


typedef struct whodata_evt {
    char *user_id;
    char *user_name;
    char *group_id;  // Linux
    char *group_name;  // Linux
    char *process_name;
    char *path;
    char *audit_uid;  // Linux
    char *audit_name;  // Linux
    char *effective_uid;  // Linux
    char *effective_name;  // Linux
    char *inode;  // Linux
    char *dev;  // Linux
    int ppid;  // Linux
#ifndef WIN32
    unsigned int process_id;
#else
    unsigned __int64 process_id;
    unsigned int mask;
    int dir_position;
    char deleted;
    char ignore_remove_event;
    char scan_directory;
    whodata_event_node *wnode;
#endif
} whodata_evt;

#ifdef WIN32

typedef struct whodata_dir_status {
    int status;
    char object_type;
    SYSTEMTIME last_check;
} whodata_dir_status;

typedef struct whodata_event_node {
    struct whodata_event_node *next;
    struct whodata_event_node *prev;
    char *id;
    time_t insert_time;
} whodata_event_node;

typedef struct whodata_event_list {
    whodata_event_node *first;
    whodata_event_node *last;
    union {
        struct {
            size_t current_size;
            size_t max_size;
            size_t alert_threshold;
            size_t max_remove;
            char alerted;
        };
        time_t queue_time;
    };
} whodata_event_list;

typedef struct whodata_directory {
    SYSTEMTIME timestamp;
    int position;
} whodata_directory;

typedef struct whodata {
    OSHash *fd;                         // Open file descriptors
    OSHash *directories;                // Directories checked by whodata mode
    int interval_scan;                  // Time interval between scans of the checking thread
    int whodata_setup;                  // Worth 1 when there is some directory configured with whodata
    whodata_dir_status *dirs_status;    // Status list
    char **device;                       // Hard disk devices
    char **drive;                        // Drive letter
} whodata;

#endif /* End WIN32*/

#ifdef WIN32

typedef struct registry {
    char *entry;
    int arch;
    char *tag;
} registry;

typedef struct registry_regex {
    OSMatch *regex;
    int arch;
} registry_regex;

#endif

typedef struct fim_entry_data {
    // Checksum attributes
    unsigned int size;
    char * perm;
    char * attributes;
    char * uid;
    char * gid;
    char * user_name;
    char * group_name;
    unsigned int mtime;
    unsigned long int inode;
    os_md5 hash_md5;
    os_sha1 hash_sha1;
    os_sha256 hash_sha256;

    // Options
    fim_event_mode mode;
    time_t last_event;
    unsigned int entry_type;
    unsigned long int dev;
    unsigned int scanned;
    int options;
    os_sha1 checksum;
} fim_entry_data;


typedef struct fim_entry {
    char *path;
    fim_entry_data *data;
} fim_entry;


typedef struct fim_inode_data {
    int items;
    char ** paths;
} fim_inode_data;

typedef struct fdb_transaction_t
{
    time_t last_commit;
    time_t interval;
} fdb_transaction_t;

typedef struct fdb_t
{
    sqlite3 *db;
    sqlite3_stmt *stmt[FIMDB_STMT_SIZE];
    fdb_transaction_t transaction;
} fdb_t;

typedef struct _config {
    int rootcheck;                  /* set to 0 when rootcheck is disabled */
    int disabled;                   /* is syscheck disabled? */
    int scan_on_start;
    int max_depth;                  /* max level of recursivity allowed */
    size_t file_max_size;           /* max file size for calculating hashes */

    fs_set skip_fs;
    int rt_delay;                   /* Delay before real-time dispatching (ms) */

    int time;                       /* frequency (secs) for syscheck to run */
    int queue;                      /* file descriptor of socket to write to queue */
    unsigned int restart_audit:1;   /* Allow Syscheck restart Auditd */
    unsigned int enable_whodata:1;  /* At less one directory configured with whodata */
    unsigned int enable_synchronization:1;    /* Enable database synchronization */

    int *opts;                      /* attributes set in the <directories> tag element */

    char *scan_day;                 /* run syscheck on this day */
    char *scan_time;                /* run syscheck at this time */

    char **ignore;                  /* list of files/dirs to ignore */
    OSMatch **ignore_regex;         /* regex of files/dirs to ignore */

    char **nodiff;                  /* list of files/dirs to never output diff */
    OSMatch **nodiff_regex;         /* regex of files/dirs to never output diff */

    char **dir;                     /* array of directories to be scanned */
    char **symbolic_links;         /* array of converted links directories */
    OSMatch **filerestrict;
    int *recursion_level;

    char **tag;                     /* array of tags for each directory */
    long max_sync_interval;         /* Maximum Synchronization interval (seconds) */
    long sync_interval;             /* Synchronization interval (seconds) */
    long sync_response_timeout;     /* Minimum time between receiving a sync response and starting a new sync session */
    long sync_queue_size;           /* Data synchronization message queue size */
    long sync_max_eps;              /* Maximum events per second for synchronization messages. */
    unsigned max_eps;               /* Maximum events per second. */

    /* Windows only registry checking */
#ifdef WIN32
    char realtime_change;                       // Variable to activate the change to realtime from a whodata monitoring
    registry *registry_ignore;                  /* list of registry entries to ignore */
    registry_regex *registry_ignore_regex;      /* regex of registry entries to ignore */
    registry *registry;                         /* array of registry entries to be scanned */
    int max_fd_win_rt;
    whodata wdata;
    whodata_event_list w_clist; // List of events cached from Whodata mode in the last seconds
#endif
    int max_audit_entries;          /* Maximum entries for Audit (whodata) */
    char **audit_key;               // Listen audit keys
    int audit_healthcheck;          // Startup health-check for whodata
    int sym_checker_interval;

    pthread_mutex_t fim_entry_mutex;
    pthread_mutex_t fim_scan_mutex;
    pthread_mutex_t fim_realtime_mutex;

    rtfim *realtime;
    fdb_t *database;
    int database_store;

    char *prefilter_cmd;
    int process_priority; // Adjusts the priority of the process (or threads in Windows)
    bool allow_remote_prefilter_cmd;
} syscheck_config;

/**
 * @brief Split a given string with all the directories delimited with selected char
 *
 * @param match Delimiter used to split
 * @param str String with directories to split
 * @param size Maximun number of directories to monitor in the same directories line.
 */
char **entries_split(char *match, const char *str, size_t size);

/**
 * @brief Adds (or overwrite if exists) an entry to the syscheck configuration structure
 *
 * @param syscheck Syscheck configuration structure
 * @param entry Entry to be dumped
 * @param vals Indicates the system arch for registries and the attributes for folders to be set
 * @param reg 1 if it's a registry, 0 if not
 * @param restrictfile The restrict regex to be set
 * @param recursion_level The recursion level to be set
 * @param tag The tag to be set
 * @param link If the added entry is pointed by a symbolic link
 */
void dump_syscheck_entry(syscheck_config *syscheck, char *entry, int vals, int reg, const char *restrictfile, int recursion_level, const char *tag, const char *link) __attribute__((nonnull(1, 2)));

/**
 * @brief Converts a bit mask with syscheck options to a human readable format
 *
 * @param [out] buf The buffer to write the check options in
 * @param [in] buflen The size of the buffer
 * @param [in] opts The bit mask of the options
 * @return A text version of the directory check option bits
 */
char *syscheck_opts2str(char *buf, int buflen, int opts);

/**
 * @brief Frees the memory of a syscheck configuration structure
 *
 * @param [out] config The syscheck configuration to free
 */
void Free_Syscheck(syscheck_config *config);

/**
 * @brief Transforms an ASCII text to HEX
 *
 * @param input The input text to transform
 * @return The HEX string on success, the original string on failure
 */
char *check_ascci_hex(char *input);

/**
 * @brief Logs the real time engine status
 *
 */
void log_realtime_status(int);

#endif /* SYSCHECKC_H */
