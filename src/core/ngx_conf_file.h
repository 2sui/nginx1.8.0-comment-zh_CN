
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONF_FILE_H_INCLUDED_
#define _NGX_CONF_FILE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 *        AAAA  number of arguments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */

/* 配置项参数属性 NGX_CONF_XXX */
#define NGX_CONF_NOARGS      0x00000001  /* 配置指令不接受参数 */
#define NGX_CONF_TAKE1       0x00000002  /* 接受一个参数 */
#define NGX_CONF_TAKE2       0x00000004  /* 接受两个参数 */
#define NGX_CONF_TAKE3       0x00000008  /* 接受三个参数 */
#define NGX_CONF_TAKE4       0x00000010  /* 接受四个参数 */
#define NGX_CONF_TAKE5       0x00000020  /* 接受五个参数 */
#define NGX_CONF_TAKE6       0x00000040  /* 接受六个参数 */
#define NGX_CONF_TAKE7       0x00000080  /* 接受七个参数 */

#define NGX_CONF_MAX_ARGS    8 /* 每条指令最大的指令数量 */

/* 参数组合 */
#define NGX_CONF_TAKE12      (NGX_CONF_TAKE1|NGX_CONF_TAKE2) /* 接受一个或两个参数 */
#define NGX_CONF_TAKE13      (NGX_CONF_TAKE1|NGX_CONF_TAKE3) /* 接受一个或三个参数 */

#define NGX_CONF_TAKE23      (NGX_CONF_TAKE2|NGX_CONF_TAKE3) /* 接受两个或三个参数 */

#define NGX_CONF_TAKE123     (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3) /* 接受一个或两个或三个或四个参数 */
#define NGX_CONF_TAKE1234    (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3   \
                              |NGX_CONF_TAKE4)

#define NGX_CONF_ARGS_NUMBER 0x000000ff
#define NGX_CONF_BLOCK       0x00000100  /* 指令接受块配置类型 */
#define NGX_CONF_FLAG        0x00000200   /* 指令接受标记类型参数，on off（会被转为bool） */
#define NGX_CONF_ANY         0x00000400  /* 指令接受任意参数，如on off等 */
#define NGX_CONF_1MORE       0x00000800 /* 指令接受至少一个参数 */
#define NGX_CONF_2MORE       0x00001000 /* 指令接受至少两个参数 */
#define NGX_CONF_MULTI       0x00000000  /* compatibility */

/* 配置项位置层级 NGX_XXX_CONF */
#define NGX_DIRECT_CONF      0x00010000 /* 配置参数位于根配置中，如 */

#define NGX_MAIN_CONF        0x01000000 /* 主要模块配置，如http mail error_log等 */
#define NGX_ANY_CONF         0x0F000000 /* 配置参数可出现在任意配置级别中 */



#define NGX_CONF_UNSET       -1
#define NGX_CONF_UNSET_UINT  (ngx_uint_t) -1
#define NGX_CONF_UNSET_PTR   (void *) -1
#define NGX_CONF_UNSET_SIZE  (size_t) -1
#define NGX_CONF_UNSET_MSEC  (ngx_msec_t) -1


#define NGX_CONF_OK          NULL
#define NGX_CONF_ERROR       (void *) -1

#define NGX_CONF_BLOCK_START 1
#define NGX_CONF_BLOCK_DONE  2
#define NGX_CONF_FILE_DONE   3

#define NGX_CORE_MODULE      0x45524F43  /* "CORE" */
#define NGX_CONF_MODULE      0x464E4F43  /* "CONF" */


#define NGX_MAX_CONF_ERRSTR  1024

/* 保存配置指令对应的处理方法 */
struct ngx_command_s {
    /* 配置指令名称 */
    ngx_str_t             name;
    /* 该配置类型（NGX_CONF_XXX） */
    ngx_uint_t            type;
    /* 指令处理回调函数 */
    char               *(*set)(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
    /* 该字段被NGX_HTTP_MODULE类型模块所用，该字段指定当前配置项存储的内存位置。
     * 实际上是使用哪个内存池的问题。因为http模块对所有http模块所要保存的配置信息，
     * 划分了main, server和location三个地方进行存储，每个地方都有一个内存池用来分配存储这些信息的内存。
     * 这里可能的值为 NGX_HTTP_MAIN_CONF_OFFSET、NGX_HTTP_SRV_CONF_OFFSET或NGX_HTTP_LOC_CONF_OFFSET。
     * 当然也可以直接置为0，就是NGX_HTTP_MAIN_CONF_OFFSET。
     */
    ngx_uint_t            conf;
    ngx_uint_t            offset;
    void                 *post;
};

#define ngx_null_command  { ngx_null_string, 0, NULL, 0, 0, NULL }


struct ngx_open_file_s {
    ngx_fd_t              fd;
    ngx_str_t             name;

    void                (*flush)(ngx_open_file_t *file, ngx_log_t *log);
    void                 *data;
};

/*
 * 初始化ngx_module_t的前7个字段（ctx_index,index,spare0~spare3,version）
 */
#define NGX_MODULE_V1          0, 0, 0, 0, 0, 0, 1
/* 初始化ngx_module_t的最后8个字段（spare_hook0～spare_hook7） */
#define NGX_MODULE_V1_PADDING  0, 0, 0, 0, 0, 0, 0, 0

/*
 * 每个模块都由一个ngx_modules_t维护，其中会保存该模块索引、该类模块索引、模块类型、模块命令集、模块上下文等。
 * 其中对于ctx字段（模块上下文）来讲，不同类型的模块上下文不同，如当type字段为NGX_CORE_MODULE时，这类模块有
 * ngx_core_module（nginx主模块）、ngx_conf_module（nginx配置模块）、ngx_events_module（event类管理模块）、
 * ngx_http_module（http类管理模块）、ngx_mail_module（mail类管理模块），即所有各类模块的管理模块或主模块的
 * 上下文都是ngx_core_module_t类型（ngx_conf_module除外，它没有模块上下文），然后各类模块的具体子模块的上下文又指
 * 向各自定义的模块类型（如ngx_http_core_module子模块的上下文为ngx_http_module_t类型而不是ngx_core_module_t类型）。
 * （ngx_conf_module是个特例，它是没有模块上下文ctx的）
 */
struct ngx_module_s {
    /*
     * 对于一类模块（由下面的type成员决定类别）而言，ctx_index表示当前模块在这类模块中的索引号。
     * 这个成员常常是由管理这类模块的一个nginx核心模块设置的，对于所有的HTTP模块而言，ctx_index
     * 是由核心模块ngx_http_module设置的。该字段在各类模块的ngx_xxx_block()函数中初始化。
     */
    ngx_uint_t            ctx_index;

    /* 表示这一类模块在各类模块中的索引号 */
    ngx_uint_t            index;

    ngx_uint_t            spare0;
    ngx_uint_t            spare1;
    ngx_uint_t            spare2;
    ngx_uint_t            spare3;

    /* 模块版本 */
    ngx_uint_t            version;

    /*
     * 模块上下文，每个模块有不同模块上下文,每个模块都有自己的特性，而ctx会指向特定类型模块的公共接口。
     * 比如，在HTTP模块中，ctx需要指向ngx_http_module_t结构体。
     */
    void                 *ctx;

    /* 模块命令集，将处理nginx.conf中的配置项 */
    ngx_command_t        *commands;

    /* 标示该模块(上下文结构体ctx)的类型，和ctx是紧密相关的。它的取值范围是以下几种:
     * NGX_CORE_MODULE,
     * NGX_CONF_MODULE,
     * NGX_EVENT_MODULE,
     * NGX_HTTP_MODULE,
     * NGX_MAIL_MODULE
     */
    ngx_uint_t            type;

    /* nginx运行流程中对应的各过程 */
    ngx_int_t           (*init_master)(ngx_log_t *log); /* 初始化master */

    ngx_int_t           (*init_module)(ngx_cycle_t *cycle); /* 初始化模块 */

    ngx_int_t           (*init_process)(ngx_cycle_t *cycle); /* 初始化进程 */
    ngx_int_t           (*init_thread)(ngx_cycle_t *cycle); /* 初始化线程 */
    void                (*exit_thread)(ngx_cycle_t *cycle); /* 退出线程 */
    void                (*exit_process)(ngx_cycle_t *cycle); /* 退出进程 */

    void                (*exit_master)(ngx_cycle_t *cycle); /* 退出master */

    uintptr_t             spare_hook0;
    uintptr_t             spare_hook1;
    uintptr_t             spare_hook2;
    uintptr_t             spare_hook3;
    uintptr_t             spare_hook4;
    uintptr_t             spare_hook5;
    uintptr_t             spare_hook6;
    uintptr_t             spare_hook7;
};

/* 维护各类模块的核心模块 */
typedef struct {
    ngx_str_t             name;  /* core module名 */
    void               *(*create_conf)(ngx_cycle_t *cycle); /* 创建配置项，解析配置 */
    char               *(*init_conf)(ngx_cycle_t *cycle, void *conf); /* 初始化配置 */
} ngx_core_module_t;


typedef struct {
    ngx_file_t            file; /* 文件句柄 */
    ngx_buf_t            *buffer; /* 缓冲 */
    ngx_uint_t            line;
} ngx_conf_file_t;

/* 指向函数指针的指针 */
typedef char *(*ngx_conf_handler_pt)(ngx_conf_t *cf,
    ngx_command_t *dummy, void *conf);


/*
 * 保存从配置文件中读取的配置指令。
 * 注意ngx_conf_module模块只负责处理 include 一条指令。
*/
struct ngx_conf_s {
    char                 *name; /* 配置名 */
    ngx_array_t          *args; /* 参数表,第一项为解析的配置指令本身，第二项为该配置指令的第一个参数，以此类推 */

    ngx_cycle_t          *cycle; /* 对应cycle */
    ngx_pool_t           *pool; /* 内存池 */
    ngx_pool_t           *temp_pool; /* 临时内存池 */
    ngx_conf_file_t      *conf_file; /* 配置文件 */
    ngx_log_t            *log;

    void                 *ctx; /* 每个模块具体配置的模块上下文 */
    ngx_uint_t            module_type; /* 模块类型 */
    ngx_uint_t            cmd_type; /* 命令类型 */

    ngx_conf_handler_pt   handler; /* 自定义配置处理回调句柄 */
    char                 *handler_conf;
};


typedef char *(*ngx_conf_post_handler_pt) (ngx_conf_t *cf,
    void *data, void *conf);

typedef struct {
    ngx_conf_post_handler_pt  post_handler;
} ngx_conf_post_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    char                     *old_name;
    char                     *new_name;
} ngx_conf_deprecated_t;


typedef struct {
    ngx_conf_post_handler_pt  post_handler;
    ngx_int_t                 low;
    ngx_int_t                 high;
} ngx_conf_num_bounds_t;


typedef struct {
    ngx_str_t                 name;
    ngx_uint_t                value;
} ngx_conf_enum_t;


#define NGX_CONF_BITMASK_SET  1

typedef struct {
    ngx_str_t                 name;
    ngx_uint_t                mask;
} ngx_conf_bitmask_t;



char * ngx_conf_deprecated(ngx_conf_t *cf, void *post, void *data);
char *ngx_conf_check_num_bounds(ngx_conf_t *cf, void *post, void *data);


#define ngx_get_conf(conf_ctx, module)  conf_ctx[module.index]



#define ngx_conf_init_value(conf, default)                                   \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = default;                                                      \
    }

#define ngx_conf_init_ptr_value(conf, default)                               \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = default;                                                      \
    }

#define ngx_conf_init_uint_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_size_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_init_msec_value(conf, default)                              \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = default;                                                      \
    }

#define ngx_conf_merge_value(conf, prev, default)                            \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_ptr_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET_PTR) {                                        \
        conf = (prev == NGX_CONF_UNSET_PTR) ? default : prev;                \
    }

#define ngx_conf_merge_uint_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_UINT) {                                       \
        conf = (prev == NGX_CONF_UNSET_UINT) ? default : prev;               \
    }

#define ngx_conf_merge_msec_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_MSEC) {                                       \
        conf = (prev == NGX_CONF_UNSET_MSEC) ? default : prev;               \
    }

#define ngx_conf_merge_sec_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_size_value(conf, prev, default)                       \
    if (conf == NGX_CONF_UNSET_SIZE) {                                       \
        conf = (prev == NGX_CONF_UNSET_SIZE) ? default : prev;               \
    }

#define ngx_conf_merge_off_value(conf, prev, default)                        \
    if (conf == NGX_CONF_UNSET) {                                            \
        conf = (prev == NGX_CONF_UNSET) ? default : prev;                    \
    }

#define ngx_conf_merge_str_value(conf, prev, default)                        \
    if (conf.data == NULL) {                                                 \
        if (prev.data) {                                                     \
            conf.len = prev.len;                                             \
            conf.data = prev.data;                                           \
        } else {                                                             \
            conf.len = sizeof(default) - 1;                                  \
            conf.data = (u_char *) default;                                  \
        }                                                                    \
    }

#define ngx_conf_merge_bufs_value(conf, prev, default_num, default_size)     \
    if (conf.num == 0) {                                                     \
        if (prev.num) {                                                      \
            conf.num = prev.num;                                             \
            conf.size = prev.size;                                           \
        } else {                                                             \
            conf.num = default_num;                                          \
            conf.size = default_size;                                        \
        }                                                                    \
    }

#define ngx_conf_merge_bitmask_value(conf, prev, default)                    \
    if (conf == 0) {                                                         \
        conf = (prev == 0) ? default : prev;                                 \
    }


char *ngx_conf_param(ngx_conf_t *cf);
char *ngx_conf_parse(ngx_conf_t *cf, ngx_str_t *filename);
char *ngx_conf_include(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


ngx_int_t ngx_conf_full_name(ngx_cycle_t *cycle, ngx_str_t *name,
    ngx_uint_t conf_prefix);
ngx_open_file_t *ngx_conf_open_file(ngx_cycle_t *cycle, ngx_str_t *name);
void ngx_cdecl ngx_conf_log_error(ngx_uint_t level, ngx_conf_t *cf,
    ngx_err_t err, const char *fmt, ...);

/* 内置的指令处理回调函数 */

/* 读取 NGX_CONF_FLAG 类型的参数 */
char *ngx_conf_set_flag_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取字符串类型的参数 */
char *ngx_conf_set_str_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取字符串数组类型的参数 */
char *ngx_conf_set_str_array_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
/* 读取 键值对类型的参数 */
char *ngx_conf_set_keyval_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取数值（整型）类型的参数 */
char *ngx_conf_set_num_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取 size_t 类型的参数 */
char *ngx_conf_set_size_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取 off_t 类型的参数 */
char *ngx_conf_set_off_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取毫秒值类型参数 */
char *ngx_conf_set_msec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取秒值类型的参数 */
char *ngx_conf_set_sec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取 buf 类型参数，该类型氛围两部分，buf 的数量和 buf 的大小，如： output_buffers 1 128k; */
char *ngx_conf_set_bufs_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取枚举类型参数，转成 ngx_uint_t类型 */
char *ngx_conf_set_enum_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/* 读取 bit 类型参数，值将以bit类型存储 */
char *ngx_conf_set_bitmask_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


extern ngx_uint_t     ngx_max_module;
extern ngx_module_t  *ngx_modules[];


#endif /* _NGX_CONF_FILE_H_INCLUDED_ */
