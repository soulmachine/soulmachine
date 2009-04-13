// name： test.cpp
// This prog is used to test C/C++ API for sqlite3 .It is very simple,ha !
// Author : zieckey
// data : 2006/11/28

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>

#include "config.h"
#include "sqlite3.h" 


int main(int argc, char* argv[])
{
        sqlite3 *db = NULL;
        sqlite3_stmt *stmt = NULL;
        const char *tail = NULL;

        char domain[32] = {0};
        char creation_date[11] = {0};
        char expiration_date[11] = {0};
        //char query_date[11] = {0};
        char registrant_name[128] = {0};
        char registrar_name[128] = {0};
        char email[32] = {0};
        int pr = 0;
        int dr = 0;
        int status = 0;
        char *change_history = NULL;
        char *original_whois_info = NULL;

        int rc;

        argc = argc;
        argv = argv;

        rc = sqlite3_open("domain.db", &db); //打开指定的数据库文件,如果不存在将创建一个同名的数据库文件
        if(SQLITE_OK == rc ){
                printf("You have opened a sqlite3 database named domain.db successfully! Congratulations! Have fun ! ^-^ \n");
        }
        else {
                fprintf(stderr, "Can't open database: %s \n", sqlite3_errmsg(db));
                goto sqlite3_open_failed;
        }


        //创建一个表,如果该表存在，则不创建，并给出提示信息，存储在 err_msg 中
        sqlite3_prepare_v2(db, " CREATE TABLE domain(Domain VARCHAR(32) PRIMARY KEY, CreationDate CHAR(10), ExpirationDate CHAR(10), "
                "QueryDate CHAR(10) DEFAULT GETDATE(), RegistrantName VARCHAR(128), RegistrarName VARCHAR(128), "
                "Email VARCHAR(32), PR INTEGER DEFAULT 0, DR INTEGER DEFAULT 0, Status INTEGER DEFAULT 0, ChangeHistory TEXT, OriginalWhoisInfo TEXT);",
                -1, &stmt, &tail) ;
        if(SQLITE_OK != rc){
                printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_prepare_v2_failed;
        }
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);


        //插入数据
        rc = sqlite3_prepare_v2(db, "INSERT INTO domain(Domain, CreationDate, ExpirationDate, RegistrantName, "
                "RegistrarName, Email, PR, DR, Status, ChangeHistory, OriginalWhoisInfo) "
                "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", -1, &stmt, &tail);

        if(SQLITE_OK != rc){
                printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_prepare_v2_failed;
        }

        strcpy(domain, "yanjiuyanjiu.com");
        strcpy(creation_date, "2009-02-17");
        strcpy(expiration_date, "2011-02-17");
        strcpy(registrant_name, "Jason Day");
        strcpy(registrar_name, "GODADDY.COM, INC");
        strcpy(email, "soulmachine@gmail.com");
        pr = 0;
        dr = 0;
        status = 1;
        change_history = NULL;
        original_whois_info = NULL;
        
        sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, creation_date, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, expiration_date, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, registrant_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, registrar_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 7, email, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 8, pr);
        sqlite3_bind_int(stmt, 9, dr);
        sqlite3_bind_int(stmt, 10, status);
        sqlite3_bind_text(stmt, 11, change_history, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 12, original_whois_info, -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        if(SQLITE_DONE != rc){
                printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_step_failed;
        }
        rc = sqlite3_reset(stmt);
        if(SQLITE_OK != rc){
                printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_reset_failed;
        }

        strcpy(domain, "yanjiuyanjiu.cn");
        strcpy(creation_date, "2009-02-18");
        strcpy(expiration_date, "2010-02-18");
        strcpy(registrant_name, "唐迪");
        strcpy(registrar_name, "唐迪");
        strcpy(email, "tandy1987115@sohu.com");
        pr = 0;
        dr = 0;
        status = 2;
        change_history = NULL;
        original_whois_info = NULL;

        sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, creation_date, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, expiration_date, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, registrant_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, registrar_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 7, email, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 8, pr);
        sqlite3_bind_int(stmt, 9, dr);
        sqlite3_bind_int(stmt, 10, status);
        sqlite3_bind_text(stmt, 11, change_history, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 12, original_whois_info, -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        if(SQLITE_DONE != rc){
                printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_step_failed;
        }

        sqlite3_finalize(stmt);

        rc = sqlite3_prepare_v2(db, "SELECT * FROM Domain ORDER BY Domain", -1, &stmt, &tail);
        if(SQLITE_OK != rc){
                printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                goto sqlite3_prepare_v2_failed;
        }
        rc = sqlite3_step(stmt);
        while(rc == SQLITE_ROW) {
                const unsigned char *domain = sqlite3_column_text(stmt, 0);
                const unsigned char *creation_date = sqlite3_column_text(stmt, 1);
                const unsigned char *expiration_date = sqlite3_column_text(stmt, 2);
                const unsigned char *query_date = sqlite3_column_text(stmt, 3);
                const unsigned char *registrant_name = sqlite3_column_text(stmt, 4);
                const unsigned char *registrar_name = sqlite3_column_text(stmt, 5);
                const unsigned char *email = sqlite3_column_text(stmt, 6);
                int pr = sqlite3_column_int(stmt, 7);
                int dr = sqlite3_column_int(stmt, 8);
                int status = sqlite3_column_int(stmt, 9);
                const unsigned char *change_history = sqlite3_column_text(stmt, 10);
                const unsigned char *original_whois_info = sqlite3_column_text(stmt, 11);
                printf("Domain: %s\tCreationDate: %s\tExpirationDate: %s\tQueryDate: %s\tReginstrant: %s\tRegistra: "
                        "%s\tEmail: %s\tPR: %d\tDR: %d\tStatus: %d\tChangeHistory: %s\tOriginalWhois: %s\n",
                        domain, creation_date, expiration_date, query_date, registrant_name, registrar_name, 
                        email, pr, dr, status, change_history, original_whois_info);

                rc = sqlite3_step(stmt);
        } 

sqlite3_reset_failed:
sqlite3_step_failed:
        sqlite3_finalize(stmt); 
sqlite3_prepare_v2_failed:
sqlite3_open_failed:
        sqlite3_close(db); //关闭数据库
        return rc;
}
