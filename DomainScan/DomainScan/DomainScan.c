#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <assert.h>

#include "sqlite3.h"

#ifndef NDEBUG
#define debug_printf printf
#else
#define debug_printf
#endif

enum {
        DOMAIN_LEN = 32,
        DATE_LEN = 16,
        REGISTRANT_LEN = 128,
        REGISTAR_LEN = 128,
        EMAIL_LEN = 32,
        TAGS_LEN = 64
};

#define DB_NAME "domain.db"
#define MSG_UPDATED = "续费了一次",




/** @see domain_t */
typedef struct domain_t domain_t;

/** 一个域名的信息结构体 */
struct domain_t {
        char url[DOMAIN_LEN + 1];               /**< 域名url */
        char creation_date[DATE_LEN + 1];       /**< 创建日期 */
        char expiration_date[DATE_LEN + 1];     /**< 过期日期 */
        char lastupdate_date[DATE_LEN + 1];     /**< 最近一次更新日期 */
        char query_date[DATE_LEN + 1] ;         /**< 最近查询日期 */
        char registrant[REGISTRANT_LEN + 1];    /**< 注册人 */
        char registrar[REGISTRANT_LEN + 1];     /**< 注册商 */
        char email[EMAIL_LEN + 1];              /**< 注册人的email */
        int pr;                                 /**< page rank */
        int dr ;                                /**< domain rank ，综合了各种因素，如PR, 域名长度等 */
        int status;                             /**< 域名状态 */
        char *change_history;                   /**< 变化历史 */
        char *original_whois_info;              /**< 原始 whois 信息 */
        char tags[TAGS_LEN];                    /**< 可以给每个域名添加多个标签，用逗号隔开，默认有一个标签为后缀 */
};



/**
 * @brief 返回 YYYY-MM-DD MM:SS格式的当前日期
 * @param[inout] 存放日期字符串的缓冲区
 * @return 日期字符串的首地址
 * @note 无
 * @remarks 无
 */
static const char* getdate(char date[DATE_LEN + 1])
{
        time_t current_time = {0};
        struct tm *local_time = NULL;

        assert(NULL != date);
        memset(date, 0, DATE_LEN + 1);

        time(&current_time);
        local_time = localtime(&current_time);
        strftime(&date[0], DATE_LEN, "%Y", local_time);
        date[4] = '-';
        strftime(&date[5], DATE_LEN, "%m", local_time);
        date[7] = '-';
        strftime(&date[8], DATE_LEN, "%d", local_time);
        date[10] = ' ';
        strftime(&date[11], DATE_LEN, "%H", local_time);
        date[13] = ':';
        strftime(&date[14], DATE_LEN, "%S", local_time);
        date[DATE_LEN] = '\0';

        return &date[0];
}


/**
 * @brief 从服务器返回的 whois 信息提取信息
 * @param[in] original_whois whois 服务器返回的原始信息
 * @param[out] domain 存放抽取出来的域名相关信息
 * @return 成功返回0，失败返回错误码
 * @note 无
 * @remarks 无
 */
int extract_data(const char *original_whois, domain_t *domain)
{
        int rc = 0;

        assert(NULL != original_whois);
        assert(NULL != domain);
        memset(domain, 0, sizeof(domain_t));

        return rc;
}



/**
 * @brief 插入域名及其信息到数据库
 * @param[in] db 数据库句柄
 * @param[in] domain 一个域名及其相关信息
 * @return 成功返回0，失败返回错误码
 * @note 无
 * @remarks 无
 */
int insert_data(sqlite3 *db, const domain_t *domain)
{
        int rc = 0;
        sqlite3_stmt *stmt = NULL;
        const char *tail = NULL;
        char **history = &((char*)domain->change_history);        // 故意去除 const 属性

        assert(NULL != db);
        assert(NULL != domain);
        
        // 先查询表中是否已经有了这条域名的记录
        rc = sqlite3_prepare_v2(db, "SELECT * FROM Domain WHERE url = ?", -1, &stmt, &tail);
        if(SQLITE_OK != rc){
                printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_prepare_v2_failed;
        }

        rc = sqlite3_bind_text(stmt, 1, domain->url, -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);

        sqlite3_finalize(stmt);

        if(rc == SQLITE_ROW) {  // 已经存在，则更新
                rc = sqlite3_prepare_v2(db, "UPDATE Domain SET CreationDate = ?, ExpirationDate = ?, LastUpdateDate = ?, QueryDate = ?, Registrant = ?, Registrar = ?, Email = ?, PR = ?, DR = ?, Status = ?, OriginalWhoisInfo = ?, ChangeHistory = ? WHERE URL =  ?;", -1, &stmt, &tail);
                if(SQLITE_OK != rc){
                        printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                        printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                        goto sqlite3_prepare_v2_failed;
                }
                sqlite3_bind_text(stmt, 1, domain->creation_date, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, domain->expiration_date, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, domain->lastupdate_date, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, domain->query_date, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 5, domain->registrant, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 6, domain->registrar, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 7, domain->email, -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 8, domain->pr);
                sqlite3_bind_int(stmt, 9, domain->dr);
                sqlite3_bind_int(stmt, 10, domain->status);
                sqlite3_bind_text(stmt, 11, domain->original_whois_info, -1, SQLITE_STATIC);
                // todo: 获取 change_history 相关信息
                *history = "";
                sqlite3_bind_text(stmt, 12, domain->change_history, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 13, domain->url, -1, SQLITE_STATIC);
                rc = sqlite3_step(stmt);
                if(SQLITE_DONE != rc){
                        printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                        goto sqlite3_step_failed;
                }
        }
        else {  // 不存在，则插入
                rc = sqlite3_prepare_v2(db, "INSERT INTO Domain VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 
                        -1, &stmt, &tail);
                if(SQLITE_OK != rc){
                        printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                        printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                        goto sqlite3_prepare_v2_failed;
                }
                sqlite3_bind_text(stmt, 1, domain->url, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, domain->creation_date, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, domain->expiration_date, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, domain->lastupdate_date, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 5, domain->query_date, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 6, domain->registrant, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 7, domain->registrar, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 8, domain->email, -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 9, domain->pr);
                sqlite3_bind_int(stmt, 10, domain->dr);
                sqlite3_bind_int(stmt, 11, domain->status);
                sqlite3_bind_text(stmt, 12, domain->original_whois_info, -1, SQLITE_STATIC);
                *history = "";    // 默认为空串
                sqlite3_bind_text(stmt, 13, domain->change_history, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 14, domain->tags, -1, SQLITE_STATIC);
                rc = sqlite3_step(stmt);
                if(SQLITE_DONE != rc){
                        printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                        goto sqlite3_step_failed;
                }
        }

        rc = 0;
sqlite3_step_failed:
        sqlite3_finalize(stmt);
sqlite3_prepare_v2_failed:
        return rc;
}


/**
 * @brief 将数据库导出为HTML文件
 * @param[inout] filename 文件名
 * @param[in] db 数据库句柄
 * @return 成功返回0，失败返回错误码
 * @note 无
 * @remarks 无
 */
int export_as_html(const char *filename, sqlite3 *db)
{
        int rc = 0;
        sqlite3_stmt *stmt = NULL;
        const char *tail = NULL;
        FILE *fp = fopen(filename, "w");
        if(NULL == fp) {
                debug_printf("can not create %s\n", filename);
                rc = -1;
                goto fopen_failed;
        }

        fprintf(fp, "<html><head><title>domain infos</title></head><body><table><tr><td><td></tr>\n");

        rc = sqlite3_prepare_v2(db, "SELECT * FROM Domain ORDER BY DR DESC, QueryDate DESC, LastUpdateDate DESC, Status DESC", -1, &stmt, &tail);
        if(SQLITE_OK != rc){
                printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_prepare_v2_failed;
        }
        rc = sqlite3_step(stmt);
        while(rc == SQLITE_ROW) {
                const unsigned char *domain = sqlite3_column_text(stmt, 0);
                const unsigned char *creation_date = sqlite3_column_text(stmt, 1);
                const unsigned char *expiration_date = sqlite3_column_text(stmt, 2);
                const unsigned char *lastupdate_date = sqlite3_column_text(stmt, 3);
                const unsigned char *query_date = sqlite3_column_text(stmt, 4);
                const unsigned char *registrant_name = sqlite3_column_text(stmt, 5);
                const unsigned char *registrar_name = sqlite3_column_text(stmt, 6);
                const unsigned char *email = sqlite3_column_text(stmt, 7);
                int pr = sqlite3_column_int(stmt, 8);
                int dr = sqlite3_column_int(stmt, 9);
                int status = sqlite3_column_int(stmt, 10);
                const unsigned char *original_whois_info = sqlite3_column_text(stmt, 11);
                const unsigned char *change_history = sqlite3_column_text(stmt, 12);
                const unsigned char *tags = sqlite3_column_text(stmt, 13);
                printf("Domain: %s\tCreationDate: %s\tExpirationDate: %s\tLastUpdateDate: %s\tQueryDate: %s\tReginstrant: %s\tRegistrar: "
                        "%s\tEmail: %s\tPR: %d\tDR: %d\tStatus: %d\tOriginalWhois: %s\tChangeHistory: %s\tTags: %s\n",
                        domain, creation_date, expiration_date, lastupdate_date, query_date, registrant_name, registrar_name, 
                        email, pr, dr, status, change_history, original_whois_info, tags);

                rc = sqlite3_step(stmt);
        }
        
        rc = 0;

        sqlite3_finalize(stmt);
sqlite3_prepare_v2_failed:
        fclose(fp);
fopen_failed:
        return rc;
}



int main(int argc, char* argv[])
{
        sqlite3 *db = NULL;
        sqlite3_stmt *stmt = NULL;
        const char *tail = NULL;

        domain_t domain = {0};

        int rc = 0;

        argc = argc;
        argv = argv;


        rc = sqlite3_open(DB_NAME, &db); //打开指定的数据库文件,如果不存在将创建一个同名的数据库文件
        if(SQLITE_OK == rc ){
                debug_printf("You have opened a sqlite3 database named %s successfully! \n", DB_NAME);
        }
        else {
                fprintf(stderr, "Can't open database: %s \n", sqlite3_errmsg(db));
                goto sqlite3_open_failed;
        }


        //创建一个表,如果该表存在，则不创建，并给出提示信息，存储在 err_msg 中
        sqlite3_prepare_v2(db, "CREATE TABLE Domain(URL VARCHAR(32) PRIMARY KEY, CreationDate VARCHAR(16), ExpirationDate VARCHAR(16), LastUpdateDate VARCHAR(16), QueryDate VARCHAR(16), Registrant VARCHAR(128), Registrar VARCHAR(128), Email VARCHAR(32), PR INTEGER, DR INTEGER, Status INTEGER, OriginalWhoisInfo TEXT, ChangeHistory TEXT, Tags VARCHAR(64));",
                -1, &stmt, &tail) ;
        if(SQLITE_OK != rc){
                printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_prepare_v2_failed;
        }
        rc = sqlite3_step(stmt);
        if(SQLITE_DONE != rc){
                if(NULL == strstr(sqlite3_errmsg(db), "already exists")){ // table xxx already exists
                        printf("%d: sqlite3_step() failed!\n", __LINE__);
                        printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                        goto sqlite3_step_failed;
                }
        }
        sqlite3_finalize(stmt);


        //插入数据
        strcpy(domain.url, "yanjiuyanjiu.com");
        strcpy(domain.creation_date, "2009-02-17");
        strcpy(domain.expiration_date, "2011-02-17");
        strcpy(domain.lastupdate_date, "2009-02-17");
        getdate(domain.query_date);
        strcpy(domain.registrant, "Jason Day");
        strcpy(domain.registrar, "GODADDY.COM, INC");
        strcpy(domain.email, "soulmachine@gmail.com");
        domain.pr = 0;
        domain.dr = 1;
        domain.status = 1;
        domain.original_whois_info = "";
        domain.change_history = "";
        strcpy(domain.tags, strrchr(domain.url, '.') + 1);

        insert_data(db, &domain);

        strcpy(domain.url, "yanjiuyanjiu.cn");
        strcpy(domain.creation_date, "2009-02-18");
        strcpy(domain.expiration_date, "2010-02-18");
        strcpy(domain.lastupdate_date, "2009-02-18");
        getdate(domain.query_date);
        strcpy(domain.registrant, "唐迪");
        strcpy(domain.registrar, "唐迪");
        strcpy(domain.email, "tandy1987115@sohu.com");
        domain.pr = 0;
        domain.dr = 0;
        domain.status = 2;
        domain.original_whois_info = "";
        domain.change_history = "";
        strcpy(domain.tags, strrchr(domain.url, '.') + 1);

        insert_data(db, &domain);
        
        export_as_html("result.html", db);


sqlite3_step_failed:
        sqlite3_finalize(stmt); 
sqlite3_prepare_v2_failed:
sqlite3_open_failed:
        sqlite3_close(db); //关闭数据库
        return rc;
}
