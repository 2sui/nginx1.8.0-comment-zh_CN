
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#ifndef NGX_CYCLE_POOL_SIZE
#define NGX_CYCLE_POOL_SIZE     NGX_DEFAULT_POOL_SIZE
#endif


#define NGX_DEBUG_POINTS_STOP   1
#define NGX_DEBUG_POINTS_ABORT  2


typedef struct ngx_shm_zone_s  ngx_shm_zone_t;

typedef ngx_int_t (*ngx_shm_zone_init_pt) (ngx_shm_zone_t *zone, void *data);

struct ngx_shm_zone_s {
    void                     *data;
    ngx_shm_t                 shm;
    ngx_shm_zone_init_pt      init;
    void                     *tag;
};


struct ngx_cycle_s {
    /*
     保存着所有模块存储配置项的结构体指针，
     它首先是一个数组，数组大小为ngx_max_module，正好与Nginx的module个数一样；
     每个数组成员又是一个指针，指向另一个存储着指针的数组，因此会看到void ****

    请见陶辉所著《深入理解Nginx-模块开发与架构解析》一书302页插图。
    另外，这个图也不错：http://img.my.csdn.net/uploads/201202/9/0_1328799724GTUk.gif

    解引用后：
        *(
          *(
            *(
              *conf_ctx // 具体配置
             )//指向具体模块中的具体配置项的指针
           )//指向某类模块的数组指针，每个元素指向这类模块里一个具体模块
          )//指向所有配置模块(指针数组)的指针,每个元素指向一类模块
    */
    void                  ****conf_ctx;
    /* 内存池 */
    ngx_pool_t               *pool;

    /*
     * 日志模块中提供了生成基本ngx_log_t日志对象的功能，这里的log实际上是在还没有执行ngx_init_cycle方法前，
     * 在ngx_init_cycle方法执行后，将会根据nginx.conf配置文件中的配置项，构造出正确的日志文件，此时会对log重新赋值。
    */
    ngx_log_t                *log;
    ngx_log_t                 new_log;

    ngx_uint_t                log_use_stderr;  /* unsigned  log_use_stderr:1; */

    ngx_connection_t        **files; /* 连接文件 */
    ngx_connection_t         *free_connections; /* 空闲连接 */
    ngx_uint_t                free_connection_n; /* 空闲连接数量 */
    /* 双向链表，元素是ngx_connection_t结构，表示可重复使用连接队列 */
    ngx_queue_t               reusable_connections_queue;

    /* 动态数组，每个数组元素储存着ngx_listening_t成员，表示监听端口及相关的参数 */
    ngx_array_t               listening; /* ngx_listening_t */
    /* 路径数组 */
    ngx_array_t               paths; /* ngx_path_t */
    /* 打开文件链表 */
    ngx_list_t                open_files;  /* ngx_open_file_t */
    /* 共享内存链表 */
    ngx_list_t                shared_memory; /* ngx_shm_zone_t */

    /* 链接数 */
    ngx_uint_t                connection_n;
    /* 打开文件数 */
    ngx_uint_t                files_n;

    /* 连接 */
    ngx_connection_t         *connections;
    /* 读事件 */
    ngx_event_t              *read_events;
    /* 写事件 */
    ngx_event_t              *write_events;

    /*
     * 继承来的ngx_cycle_t。
     *
     * 旧的ngx_cycle_t 对象用于引用上一个ngx_cycle_t 对象中的成员。例如ngx_init_cycle 方法，在启动初期，
     * 需要建立一个临时的ngx_cycle_t对象保存一些变量，再调用ngx_init_cycle 方法时就可以把旧的ngx_cycle_t 对象传进去，
     * 而这时old_cycle对象就会保存这个前期的ngx_cycle_t对象。
     */
    ngx_cycle_t              *old_cycle;

    ngx_str_t                 conf_file; /* 配置文件路径 */
    ngx_str_t                 conf_param; /* 配置参数 */
    ngx_str_t                 conf_prefix; /* 配置文件前缀 */
    ngx_str_t                 prefix; /* 程序路径前缀 */
    ngx_str_t                 lock_file; /* 文件锁 */
    ngx_str_t                 hostname; /* 主机名 */
};


typedef struct {
     ngx_flag_t               daemon;
     ngx_flag_t               master;

     ngx_msec_t               timer_resolution;

     ngx_int_t                worker_processes;
     ngx_int_t                debug_points;

     ngx_int_t                rlimit_nofile;
     ngx_int_t                rlimit_sigpending;
     off_t                    rlimit_core;

     int                      priority;

     ngx_uint_t               cpu_affinity_n;
     uint64_t                *cpu_affinity;

     char                    *username;
     ngx_uid_t                user;
     ngx_gid_t                group;

     ngx_str_t                working_directory;
     ngx_str_t                lock_file;

     ngx_str_t                pid;
     ngx_str_t                oldpid;

     ngx_array_t              env;
     char                   **environment;

#if (NGX_OLD_THREADS)
     ngx_int_t                worker_threads;
     size_t                   thread_stack_size;
#endif

} ngx_core_conf_t;


#if (NGX_OLD_THREADS)

typedef struct {
     ngx_pool_t              *pool;   /* pcre's malloc() pool */
} ngx_core_tls_t;

#endif


#define ngx_is_init_cycle(cycle)  (cycle->conf_ctx == NULL)


ngx_cycle_t *ngx_init_cycle(ngx_cycle_t *old_cycle);
ngx_int_t ngx_create_pidfile(ngx_str_t *name, ngx_log_t *log);
void ngx_delete_pidfile(ngx_cycle_t *cycle);
ngx_int_t ngx_signal_process(ngx_cycle_t *cycle, char *sig);
void ngx_reopen_files(ngx_cycle_t *cycle, ngx_uid_t user);
char **ngx_set_environment(ngx_cycle_t *cycle, ngx_uint_t *last);
ngx_pid_t ngx_exec_new_binary(ngx_cycle_t *cycle, char *const *argv);
uint64_t ngx_get_cpu_affinity(ngx_uint_t n);
ngx_shm_zone_t *ngx_shared_memory_add(ngx_conf_t *cf, ngx_str_t *name,
    size_t size, void *tag);


extern volatile ngx_cycle_t  *ngx_cycle;
extern ngx_array_t            ngx_old_cycles;
extern ngx_module_t           ngx_core_module;
extern ngx_uint_t             ngx_test_config;
extern ngx_uint_t             ngx_quiet_mode;
#if (NGX_OLD_THREADS)
extern ngx_tls_key_t          ngx_core_tls_key;
#endif


#endif /* _NGX_CYCLE_H_INCLUDED_ */
