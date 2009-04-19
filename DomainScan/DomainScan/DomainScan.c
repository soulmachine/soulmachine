#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <assert.h>

#include "sqlite3.h"

#define DS_OK 0

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
#define MSG_UPDATED = "������һ��",
#define MSG_REGISTERED "��ע��"
#define MSG_NOT_REGISTERED "δע��"



/** @see domain_t */
typedef struct domain_t domain_t;

/** һ����������Ϣ�ṹ�� */
struct domain_t {
        char url[DOMAIN_LEN + 1];               /**< ����url */
        char creation_date[DATE_LEN + 1];       /**< �������� */
        char expiration_date[DATE_LEN + 1];     /**< �������� */
        char lastupdate_date[DATE_LEN + 1];     /**< ���һ�θ������� */
        char query_date[DATE_LEN + 1] ;         /**< �����ѯ���� */
        char registrant[REGISTRANT_LEN + 1];    /**< ע���� */
        char registrar[REGISTRANT_LEN + 1];     /**< ע���� */
        char email[EMAIL_LEN + 1];              /**< ע���˵�email */
        int pr;                                 /**< page rank */
        int dr ;  /**< domain rank ���ۺ��˸������أ���PR, �������ȵ� */
        int status;                             /**< ����״̬ */
        char *change_history;                   /**< �仯��ʷ */
        char *original_whois_info;              /**< ԭʼ whois ��Ϣ */
        char tags[TAGS_LEN]; /**< ���Ը�������Ӷ����ǩ���ö��Ÿ��� */
};



/**
 * @brief ���� YYYY-MM-DD MM:SS��ʽ�ĵ�ǰ����
 * @param[inout] ��������ַ����Ļ�����
 * @return �����ַ������׵�ַ
 * @note ��
 * @remarks ��
 */
static const char* ds_getdate(char date[DATE_LEN + 1])
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
* @brief ��ʼ��
* @param[in] dbname ���ݿ��ļ���
* @return �ɹ����� DOMAIN_SCAN_OK��ʧ�ܷ��ش�����
* @note ��
* @remarks ��
*/
int ds_initialize(const char *dbname, sqlite3 **db)
{
        int rc = DS_OK;
        sqlite3_stmt *stmt = NULL;
        const char *tail = NULL;


        //��ָ�������ݿ��ļ�,��������ڽ�����һ��ͬ�������ݿ��ļ�
        rc = sqlite3_open(dbname, db); 
        if(SQLITE_OK == rc ){
                debug_printf("You have opened a sqlite3 database named %s "
                        "successfully! \n", dbname);
        }
        else {
                debug_printf("%d:\t%s\n", __LINE__, sqlite3_errmsg(*db));
                debug_printf("Can't open database: %s \n", sqlite3_errmsg(*db));
                goto sqlite3_open_failed;
        }


        //���� domain ��
        sqlite3_prepare_v2(*db, "CREATE TABLE Domain(URL VARCHAR(32) PRIMARY "
                "KEY, Status INTEGER, CreationDate VARCHAR(16), ExpirationDate "
				"VARCHAR(16), LastUpdateDate VARCHAR(16), QueryDate "
				"VARCHAR(16), Registrant VARCHAR(128), Registrar VARCHAR(128), "
				"Email VARCHAR(32), PR INTEGER, DR INTEGER, OriginalWhoisInfo "
				"TEXT, ChangeHistory TEXT, Tags VARCHAR(64));",
                -1, &stmt, &tail) ;
        if(SQLITE_OK != rc){
                debug_printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                debug_printf("%d:\t%s\n", __LINE__, sqlite3_errmsg(*db));
                goto sqlite3_prepare_v2_failed;
        }
        rc = sqlite3_step(stmt);
        if(SQLITE_DONE != rc){
                if(NULL == strstr(sqlite3_errmsg(*db), "already exists")) {
                        // table xxx already exists
                        debug_printf("%d: sqlite3_step() failed!\n", __LINE__);
                        debug_printf("%d: %s\n", __LINE__, sqlite3_errmsg(*db));
                        goto sqlite3_step_failed;
                }
        }

		sqlite3_finalize(stmt);

		//���� twitter ��
		sqlite3_prepare_v2(*db, "CREATE TABLE Twitter(Username VARCHAR(16) "
			"PRIMARY KEY, Name VARCHAR(16), Location VARCHAR(32), Web VARCHAR"
			"(128), Bio VARCHAR(160), Following INTEGER, Followers INTERGER, "
			"Updates INTEGER);",
			-1, &stmt, &tail) ;
		if(SQLITE_OK != rc){
			debug_printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
			debug_printf("%d:\t%s\n", __LINE__, sqlite3_errmsg(*db));
			goto sqlite3_prepare_v2_failed;
		}
		rc = sqlite3_step(stmt);
		if(SQLITE_DONE != rc){
			if(NULL == strstr(sqlite3_errmsg(*db), "already exists")) {
				// table xxx already exists
				debug_printf("%d: sqlite3_step() failed!\n", __LINE__);
				debug_printf("%d: %s\n", __LINE__, sqlite3_errmsg(*db));
				goto sqlite3_step_failed;
			}
		}

        rc = DS_OK;
sqlite3_step_failed:
        sqlite3_finalize(stmt);
sqlite3_prepare_v2_failed:
sqlite3_open_failed:
        return rc;
}



/**
* @brief �ͷ���Դ
* @param[in] db ���ݿ���
* @return �ɹ����� DOMAIN_SCAN_OK��ʧ�ܷ��ش�����
* @note ��
* @remarks ��
*/
int ds_finalize(sqlite3 *db)
{
        if(NULL != db) {
                return sqlite3_close(db); //�ر����ݿ�
        }
        return DS_OK;
}



/**
 * @brief �ӷ��������ص� whois ��Ϣ��ȡ��Ϣ
 * @param[in] original_whois whois ���������ص�ԭʼ��Ϣ
 * @param[out] domain ��ų�ȡ���������������Ϣ
 * @return �ɹ����� DOMAIN_SCAN_OK��ʧ�ܷ��ش�����
 * @note ��
 * @remarks ��
 */
int ds_extract_data(const char *original_whois, domain_t *domain)
{
        int rc = DS_OK;

        assert(NULL != original_whois);
        assert(NULL != domain);
        memset(domain, 0, sizeof(domain_t));

        return rc;
}



/**
 * @brief ��������������Ϣ�����ݿ�
 * @param[in] db ���ݿ���
 * @param[in] domain һ���������������Ϣ
 * @return �ɹ����� DOMAIN_SCAN_OK��ʧ�ܷ��ش�����
 * @note ��
 * @remarks ��
 */
int ds_insert_data(sqlite3 *db, const domain_t *domain)
{
        int rc = DS_OK;
        sqlite3_stmt *stmt = NULL;
        const char *tail = NULL;
        // ����ȥ�� const ���ԣ����������Ҫ���� change_history
        char **history = &((char*)domain->change_history);

        assert(NULL != db);
        assert(NULL != domain);
        
        // �Ȳ�ѯ�����Ƿ��Ѿ��������������ļ�¼
        rc = sqlite3_prepare_v2(db, "SELECT * FROM Domain WHERE url = ?", -1, 
                &stmt, &tail);
        if(SQLITE_OK != rc){
                printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_prepare_v2_failed;
        }

        rc = sqlite3_bind_text(stmt, 1, domain->url, -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);

        sqlite3_finalize(stmt);

        if(rc == SQLITE_ROW) {  // �Ѿ����ڣ������
                rc = sqlite3_prepare_v2(db, "UPDATE Domain SET CreationDate = "
                        "?, Status = ?, ExpirationDate = ?, LastUpdateDate = "
						"?, QueryDate = ?, Registrant = ?, Registrar = ?, "
						"Email = ?, PR = ?, DR = ?, OriginalWhoisInfo = ?, "
                        "ChangeHistory = ? WHERE URL =  ?;", -1, &stmt, &tail);
                if(SQLITE_OK != rc){
                        printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                        printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                        goto sqlite3_prepare_v2_failed;
                }
                sqlite3_bind_text(stmt, 1, domain->creation_date, -1, 
                        SQLITE_STATIC);
				sqlite3_bind_int(stmt, 2, domain->status);
                sqlite3_bind_text(stmt, 3, domain->expiration_date, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, domain->lastupdate_date, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 5, domain->query_date, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 6, domain->registrant, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 7, domain->registrar, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 8, domain->email, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_int(stmt, 9, domain->pr);
                sqlite3_bind_int(stmt, 10, domain->dr);
                sqlite3_bind_text(stmt, 11, domain->original_whois_info, -1, 
                        SQLITE_STATIC);
                // todo: ��ȡ change_history �����Ϣ
                *history = "";
                sqlite3_bind_text(stmt, 12, domain->change_history, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 13, domain->url, -1, SQLITE_STATIC);
                rc = sqlite3_step(stmt);
                if(SQLITE_DONE != rc){
                        printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                        goto sqlite3_step_failed;
                }
        }
        else {  // �����ڣ������
                rc = sqlite3_prepare_v2(db, "INSERT INTO Domain VALUES(?, ?, "
                        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 
                        -1, &stmt, &tail);
                if(SQLITE_OK != rc){
                        printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                        printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                        goto sqlite3_prepare_v2_failed;
                }
                sqlite3_bind_text(stmt, 1, domain->url, -1, SQLITE_STATIC);
				sqlite3_bind_int(stmt, 2, domain->status);
                sqlite3_bind_text(stmt, 3, domain->creation_date, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, domain->expiration_date, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 5, domain->lastupdate_date, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 6, domain->query_date, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 7, domain->registrant, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 8, domain->registrar, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 9, domain->email, -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 10, domain->pr);
                sqlite3_bind_int(stmt, 11, domain->dr);
                sqlite3_bind_text(stmt, 12, domain->original_whois_info, -1, 
                        SQLITE_STATIC);
                *history = "";    // Ĭ��Ϊ�մ�
                sqlite3_bind_text(stmt, 13, domain->change_history, -1, 
                        SQLITE_STATIC);
                sqlite3_bind_text(stmt, 14, domain->tags, -1, SQLITE_STATIC);
                rc = sqlite3_step(stmt);
                if(SQLITE_DONE != rc){
                        printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                        goto sqlite3_step_failed;
                }
        }

        rc = DS_OK;
sqlite3_step_failed:
        sqlite3_finalize(stmt);
sqlite3_prepare_v2_failed:
        return rc;
}



/**
 * @brief �����ݿ⵼��ΪHTML�ļ�
 * @param[inout] filename �ļ���
 * @param[in] db ���ݿ���
 * @return �ɹ����� DOMAIN_SCAN_OK��ʧ�ܷ��ش�����
 * @note ��
 * @remarks ��
 */
int ds_export_as_html(const char *filename, sqlite3 *db)
{
        int rc = DS_OK;
        sqlite3_stmt *stmt = NULL;
        const char *tail = NULL;
        FILE *fp = fopen(filename, "w");
        if(NULL == fp) {
                debug_printf("can not create %s\n", filename);
                rc = -1;
                goto fopen_failed;
        }

		fprintf(fp, "<HTML>\n<HEAD><TITLE>%s</TITLE>\n"
			"<style type=\"text/css\">\n.Tab{width:100%%;border-collapse:"
			"collapse;border:1px solid #000000}\n.Tab td{text-align:center;"
			"border:1px solid #000000;}\n</style></HEAD>\n<BODY>\n<TABLE class="
			"\"Tab\">\n", filename);

		fprintf(fp, "<TR><TD>��ַ</TD><TD>����״̬</TD><TD>��������</TD><TD>"
			"��������</TD><TD>ע����</TD><TD>ע����</TD><TD>Email</TD></TR>\n");

        rc = sqlite3_prepare_v2(db, "SELECT * FROM Domain ORDER BY DR DESC, "
			"QueryDate DESC, LastUpdateDate DESC, Status DESC", -1, &stmt, 
			&tail);
        if(SQLITE_OK != rc){
                debug_printf("%d: sqlite3_prepare_v2() failed!\n", __LINE__);
                debug_printf("%d: %s\n", __LINE__, sqlite3_errmsg(db));
                goto sqlite3_prepare_v2_failed;
        }
        rc = sqlite3_step(stmt);
        while(rc == SQLITE_ROW) {
                const unsigned char *domain;
				int status;
				const char *str_status;
				const unsigned char *creation_date;
				const unsigned char *expiration_date;
				const unsigned char *lastupdate_date;
				const unsigned char *query_date;
				const unsigned char *registrant_name;
				const unsigned char *registrar_name;
				const unsigned char *email;
				int pr;
				int dr;
				const unsigned char *original_whois_info;
				const unsigned char *change_history;
				const unsigned char *tags;

				domain = sqlite3_column_text(stmt, 0);
				status = sqlite3_column_int(stmt, 1);
				if(0 == status) {
					str_status = MSG_NOT_REGISTERED;
				}
				else {
					str_status = MSG_REGISTERED;
				}
                creation_date = sqlite3_column_text(stmt, 2);
                expiration_date = sqlite3_column_text(stmt, 3);
                lastupdate_date = sqlite3_column_text(stmt,4);
                query_date = sqlite3_column_text(stmt, 5);
                registrant_name = sqlite3_column_text(stmt,6);
                registrar_name = sqlite3_column_text(stmt, 7);
                email = sqlite3_column_text(stmt, 8);
                pr = sqlite3_column_int(stmt, 9);
                dr = sqlite3_column_int(stmt, 10);
                original_whois_info = sqlite3_column_text(stmt, 11);
                change_history = sqlite3_column_text(stmt, 12);
                tags = sqlite3_column_text(stmt, 13);

				fprintf(fp, "<TR><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD>"
					"<TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n", domain, 
					str_status, creation_date, expiration_date, registrant_name,
					registrar_name, email);

                rc = sqlite3_step(stmt);
        }

		fprintf(fp, "</TABLE>\n</BODY>\n</HTML>\n");
        
        rc = DS_OK;

        sqlite3_finalize(stmt);
sqlite3_prepare_v2_failed:
        fclose(fp);
fopen_failed:
        return rc;
}



int main(int argc, char* argv[])
{
        sqlite3 *db = NULL;

        domain_t domain = {0};

        int rc = 0;

        argc = argc;
        argv = argv;


        ds_initialize(DB_NAME, &db);


        //��������
        strcpy(domain.url, "yanjiuyanjiu.com");
		domain.status = 1;
        strcpy(domain.creation_date, "2009-02-17");
        strcpy(domain.expiration_date, "2011-02-17");
        strcpy(domain.lastupdate_date, "2009-02-17");
        ds_getdate(domain.query_date);
        strcpy(domain.registrant, "Jason Day");
        strcpy(domain.registrar, "GODADDY.COM, INC");
        strcpy(domain.email, "soulmachine@gmail.com");
        domain.pr = 0;
        domain.dr = 1;
        domain.original_whois_info = "";
        domain.change_history = "";
        strcpy(domain.tags, strrchr(domain.url, '.') + 1);

        ds_insert_data(db, &domain);

        strcpy(domain.url, "yanjiuyanjiu.cn");
		domain.status = 2;
        strcpy(domain.creation_date, "2009-02-18");
        strcpy(domain.expiration_date, "2010-02-18");
        strcpy(domain.lastupdate_date, "2009-02-18");
        ds_getdate(domain.query_date);
        strcpy(domain.registrant, "�Ƶ�");
        strcpy(domain.registrar, "�Ƶ�");
        strcpy(domain.email, "tandy1987115@sohu.com");
        domain.pr = 0;
        domain.dr = 0;
        domain.original_whois_info = "";
        domain.change_history = "";
        strcpy(domain.tags, strrchr(domain.url, '.') + 1);

        ds_insert_data(db, &domain);
        
        ds_export_as_html("result.html", db);

        ds_finalize(db);

        return rc;
}
