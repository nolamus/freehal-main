/*
 * This file is part of FreeHAL 2010.
 *
 * Copyright(c) 2006, 2007, 2008, 2009, 2010 Tobias Schulz and contributors.
 * http://freehal.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "hal2009-mysql.h"

int mysql_construct() {
    // initialize mysql
    if (0 == mysql_connection) {
        printf("%s%s\n", "Open MySQL connection to: ", mysql_filename);
        mysql_connection = calloc(sizeof(MYSQL), 0);
        mysql_init(mysql_connection);

        char* file = strdup(mysql_filename);
        char* user = strtok(line, ":@/");
        char* pass = strtok(NULL, ":@/");
        char* host = strtok(NULL, ":@/");
        char* db   = strtok(NULL, ":@/");

        if (!mysql_real_connect(mysql_connection, host, ‪user, pass, db, 0, NULL, 0)) {
            printf("%s%s\n", "Could not open MySQL connection to: ", sqlite_filename);
            sqlite_connection = 0;
            return NO_CONNECTION;
        }
        free(file);
    }
    return 0;
}

int mysql_begin() {
    // initialize semantic ram_net
    mysql_net = calloc(sizeof(void*)*(4+'z'-'a'), 1);
    int i;
    for (i = n('a'); i <= n('z'); ++i) {
        mysql_net[i] = calloc(sizeof(void*)*(4+'z'-'a'), 1);

        int k;
        for (k = n('a'); k <= n('z'); ++k) {
            mysql_net[i][k] = calloc(sizeof(struct list), 1);
            mysql_net[i][k]->size = 0;
            mysql_net[i][k]->list = 0;
        }

        k = WRONG;
        mysql_net[i][k] = calloc(sizeof(struct list), 1);
        mysql_net[i][k]->size = 0;
        mysql_net[i][k]->list = 0;
    }
    i = WRONG;
    mysql_net[i] = calloc(sizeof(void*)*(4+'z'-'a'), 1);
    int k;
    for (k = n('a'); k <= n('z'); ++k) {
        mysql_net[i][k] = calloc(sizeof(struct list), 1);
        mysql_net[i][k]->size = 0;
        mysql_net[i][k]->list = 0;
    }
    k = WRONG;
    mysql_net[i][k] = calloc(sizeof(struct list), 1);
    mysql_net[i][k]->size = 0;
    mysql_net[i][k]->list = 0;

    mysql_construct();

    char sql[99];
    strcpy(sql, "BEGIN;");
    mysql_real_query(mysql_connection, sql, (unsigned int)strlen(sql));
    return 0;
}

int mysql_free_wordlist(int i, int k) {
    int g;
    for (g = 0; g < mysql_net[i][k]->size; ++g) {
        free(mysql_net[i][k]->list[g]);
        mysql_net[i][k]->list[g] = 0;
    }
}

int mysql_end() {
    mysql_construct();

    char sql[99];
    strcpy(sql, "COMMIT;");
    mysql_real_query(mysql_connection, sql, (unsigned int)strlen(sql));
    mysql_close(mysql_connection);
    mysql_connection = 0;

    int i;
    for (i = n('a'); i <= n('z'); ++i) {
        int k;
        for (k = n('a'); k <= n('z'); ++k) {
            mysql_free_wordlist(i, k);
            free(mysql_net[i][k]);
            mysql_net[i][k] = 0;
        }

        k = WRONG;
        mysql_free_wordlist(i, k);
        free(mysql_net[i][k]);
        mysql_net[i][k] = 0;

        free(mysql_net[i]);
        mysql_net[i] = 0;
    }
    i = WRONG;
    int k;
    for (k = n('a'); k <= n('z'); ++k) {
        mysql_free_wordlist(i, k);
        free(mysql_net[i][k]);
        mysql_net[i][k] = 0;
    }
    k = WRONG;
    mysql_free_wordlist(i, k);
    free(mysql_net[i][k]);
    mysql_net[i][k] = 0;
    free(mysql_net[i]);
    mysql_net[i] = 0;
    free(mysql_net);
    mysql_net = 0;
}

int sql_mysql_set_filename(const char* filename) {
    if (mysql_filename)
        free(mysql_filename);
    mysql_filename = 0;
    mysql_filename = filename;
}

struct word* mysql_get_word(const char* name) {
    if (0 == strlen(name)) {
        return 0;
    }
    int i = WRONG;
    if (strlen(name) >= 1) {
        i = n(name[0]);
    }
    int k = WRONG;
    if (strlen(name) >= 2) {
        k = n(name[1]);
    }

    int length = strlen(name);

    struct word** list = (struct word**)(mysql_net[i][k]->list);

    if (0 == list) {
        //debugf("illegal list while searching %s.\n", name);
        return 0;
    }

    int g;
    for (g = 0; g < mysql_net[i][k]->size; ++g) {
        if (length == list[g]->length && 0 == strcmp(list[g]->name, name)) {
            //debugf("found: %s = %p.\n", name, list[g]);
            return list[g];
        }
    }

    return 0;
}

static int select_primary_key(void* key, int argc, char **argv, char **azColName) {
    strcpy((char*)key, argv[0]);
    return 0;
}

int get_last_pk(int rel) {
    // cache
    static int cache_clauses = INVALID_POINTER;
    static int cache_facts = INVALID_POINTER;
    if (rel) {
        if (cache_clauses > INVALID_POINTER) {
            ++cache_clauses;
            return cache_clauses;
        }
    }
    else {
        if (cache_facts > INVALID_POINTER) {
            ++cache_facts;
            return cache_facts;
        }
    }

    // no cache
    char sql[5120];
    *sql = 0;
    strcat(sql, "SELECT pk from ");
    strcat(sql, rel ? "clauses" : "facts");
    strcat(sql, " order by 1 desc limit 1;");

    char key[99];
    int error = sql_execute(sql, select_primary_key, key);

    if (rel) {
        cache_clauses = to_number(key);
    }
    else {
        cache_facts = to_number(key);
        printf("cache_facts: %d\n", cache_facts);
    }

    return (rel?cache_clauses:cache_facts);
}

int detect_words(int* num_of_words, char** words, const char* r_verbs, const char* r_subjects, const char* r_objects, const char* r_adverbs, const char* r_extra) {
    char* verbs = strdup(r_verbs    ? r_verbs    : "");
    char* subj  = strdup(r_subjects ? r_subjects : "");
    char* obj   = strdup(r_objects  ? r_objects  : "");
    char* advs  = strdup(r_adverbs  ? r_adverbs  : "");
    char* extra = strdup(r_extra    ? r_extra    : "");
    char* buffer;

    *num_of_words = 1;
    words[0] = strdup("0");

    if (strcmp(subj, "0")) {
        words[*num_of_words] = strdup(verbs);
        ++(*num_of_words);
    }

    if (strcmp(subj, "0")) {
        words[*num_of_words] = strdup(subj);
        ++(*num_of_words);
    }

    if (strcmp( obj, "0")) {
        words[*num_of_words] = strdup( obj);
        ++(*num_of_words);
    }

    if (strcmp(advs, "0")) {
        words[*num_of_words] = strdup(advs);
        ++(*num_of_words);
    }

    buffer = strtok(verbs, " ;.)(-,_");
    while (buffer && strlen(buffer) && strcmp(buffer, "0")) {
        if (is_a_trivial_word(buffer)) {
            buffer = strtok(NULL, " ;.)(-,_");
            continue;
        }
        words[*num_of_words] = strdup(buffer);
        buffer = strtok(NULL, " ;.)(-,_");
        ++(*num_of_words);
        if (*num_of_words >= 500) break;
    }


    buffer = strtok(subj, " ;.)(-,_");
    while (buffer && strlen(buffer) && strcmp(buffer, "0")) {
        if (is_a_trivial_word(buffer)) {
            buffer = strtok(NULL, " ;.)(-,_");
            continue;
        }
        words[*num_of_words] = strdup(buffer);
        buffer = strtok(NULL, " ;.)(-,_");
        ++(*num_of_words);
        if (*num_of_words >= 500) break;
    }

    buffer = strtok( obj, " ;.)(-,_");
    while (buffer && strlen(buffer) && strcmp(buffer, "0")) {
        if (is_a_trivial_word(buffer)) {
            buffer = strtok(NULL, " ;.)(-,_");
            continue;
        }
        words[*num_of_words] = strdup(buffer);
        buffer = strtok(NULL, " ;.)(-,_");
        ++(*num_of_words);
        if (*num_of_words >= 500) break;
    }

    buffer = strtok(advs, " ;.)(-,_");
    while (buffer && strlen(buffer) && strcmp(buffer, "0")) {
        if (is_a_trivial_word(buffer)) {
            buffer = strtok(NULL, " ;.)(-,_");
            continue;
        }
        words[*num_of_words] = strdup(buffer);
        buffer = strtok(NULL, " ;.)(-,_");
        ++(*num_of_words);
        if (*num_of_words >= 500) break;
    }

    if (extra) {
        buffer = strtok(extra, " ;.)(-,_");
        while (buffer && strlen(buffer) && strcmp(buffer, "0")) {
            if (is_a_trivial_word(buffer)) {
                buffer = strtok(NULL, " ;.)(-,_");
                continue;
            }
            words[*num_of_words] = strdup(buffer);
            buffer = strtok(NULL, " ;.)(-,_");
            ++(*num_of_words);
            if (*num_of_words >= 500) break;
        }
    }

    --(*num_of_words);

    free(verbs);
    free(subj);
    free(obj);
    free(advs);
    free(extra);
}

char* gen_sql_add_entry(char* sql, int pk, int rel, const char* subjects, const char* objects, const char* verbs, const char* adverbs, const char* extra, const char* questionword, const char* from, float truth, short verb_flag_want, short verb_flag_must, short verb_flag_can, short verb_flag_may, short verb_flag_should, short only_logic) {

    if (0 == sql) {
        sql = malloc(102400);
        *sql = 0;
    }
    strcat(sql, "INSERT INTO ");
    strcat(sql, rel ? "clauses" : "facts");
    strcat(sql, " (`pk`, `mix_1`, `verb`, `verbgroup`, `subjects`, `objects`, `adverbs`, `questionword`, `prio`, `from`, `rel`, `truth`, `only_logic`) VALUES (");
    char num_of_records_str[10];
    snprintf(num_of_records_str, 9, "%d", pk);
    strcat(sql, num_of_records_str);
    strcat(sql, ", \"");
    strcat(sql, " ");
    strcat(sql, subjects);
    strcat(sql, " ");
    strcat(sql, objects);
    strcat(sql, " ");
    strcat(sql, adverbs);
    strcat(sql, " ");
    strcat(sql, "\", \"");
    strcat(sql, verbs);
    strcat(sql, "\", \"");
    if (0==strcmp(verbs, "=") || 0==strcmp(verbs, "ist") || 0==strcmp(verbs, "bist") || 0==strcmp(verbs, "bin") || 0==strcmp(verbs, "sind") || 0==strcmp(verbs, "sein") || 0==strcmp(verbs, "heisst") || 0==strcmp(verbs, "heisse") || 0==strcmp(verbs, "heissen") || 0==strcmp(verbs, "war") || 0==strcmp(verbs, "is") || 0==strcmp(verbs, "are") || 0==strcmp(verbs, "am") || 0==strcmp(verbs, "was")) {
        strcat(sql, "be");
    }
    if (0==strcmp(verbs, "haben") || 0==strcmp(verbs, "habe") || 0==strcmp(verbs, "hat") || 0==strcmp(verbs, "hast") || 0==strcmp(verbs, "hab") || 0==strcmp(verbs, "have") || 0==strcmp(verbs, "has")) {
        strcat(sql, "have");
    }
    strcat(sql, "\", \"");
    if (subjects[0] != ' ')
        strcat(sql, subjects);
    strcat(sql, "\", \"");
    if (objects[0] != ' ')
        strcat(sql, objects);
    strcat(sql, "\", \"");
    if (adverbs[0] != ' ')
        strcat(sql, adverbs);
    strcat(sql, "\", \"");
    if (questionword)    strcat(sql, questionword);
    else                 strcat(sql, "NULL");
    strcat(sql, "\", 50, \"");
    strcat(sql, from);
    strcat(sql, "\", ");
    if (rel) strcat(sql, from_number(rel));
    else     strcat(sql, "-1");
    strcat(sql, ", ");
    char truth_str[10];
    snprintf(truth_str, 9, "%f", truth);
    strcat(sql, truth_str);
    strcat(sql, ", ");
    strcat(sql, only_logic?"1":"0");
    strcat(sql, ");\n");

    return sql;
}

char* gen_sql_add_verb_flags(char* sql, int pk, int rel, const char* subjects, const char* objects, const char* verbs, const char* adverbs, const char* extra, const char* questionword, const char* from, float truth, short verb_flag_want, short verb_flag_must, short verb_flag_can, short verb_flag_may, short verb_flag_should, short only_logic) {

    char key[101];
    snprintf(key, 100, "%d", pk);

    if (0 == sql) {
        sql = malloc(102400);
        *sql = 0;
    }
    strcat(sql, "INSERT INTO rel_");
    strcat(sql, rel ? "clause" : "fact");
    strcat(sql, "_flag (`fact`, `verb_flag_want`, `verb_flag_must`, `verb_flag_can`, `verb_flag_may`, `verb_flag_should`) VALUES (");
    strcat(sql, key);
    strcat(sql, ", ");
    strcat(sql, verb_flag_want?"1":"0");
    strcat(sql, ", ");
    strcat(sql, verb_flag_must?"1":"0");
    strcat(sql, ", ");
    strcat(sql, verb_flag_can?"1":"0");
    strcat(sql, ", ");
    strcat(sql, verb_flag_may?"1":"0");
    strcat(sql, ", ");
    strcat(sql, verb_flag_should?"1":"0");
    strcat(sql, ");\n");

    return sql;
}

char* gen_sql_add_word_fact_relations(char* sql, int pk, int rel, const char* subjects, const char* objects, const char* verbs, const char* adverbs, const char* extra, const char* questionword, const char* from, float truth, short verb_flag_want, short verb_flag_must, short verb_flag_can, short verb_flag_may, short verb_flag_should, short only_logic) {

    char key[101];
    snprintf(key, 100, "%d", pk);

    if (0 == sql) {
        sql = malloc(102400);
        *sql = 0;
    }

    int num_of_words = 0;
    char** words = calloc(501*sizeof(char*), 1);
    detect_words(&num_of_words, words, verbs, subjects, objects, adverbs, "");

    while (num_of_words >= 0) {
        if (words[num_of_words]) {
            if (words[num_of_words][0] != '0') {
                /*{
                    strcat(sql, "INSERT OR IGNORE INTO rel_word_fact__general (`word`, `fact`, `table`) VALUES (");
                    strcat(sql, "\n\"");
                    strcat(sql, words[num_of_words]);
                    strcat(sql, "\", \n");
                    strcat(sql, key);
                    strcat(sql, ", \n\"");
                    strcat(sql, rel ? "clauses" : "facts");
                    strcat(sql, "\"");
                    strcat(sql, ");");
                }*/

                char* smid = small_identifier(words[num_of_words]);
                {
                    strcat(sql, "INSERT OR IGNORE INTO rel_word_fact__");
                    strcat(sql, smid);
                    strcat(sql, " (`word`, `fact`, `table`) VALUES (");
                    strcat(sql, "\n\"");
                    strcat(sql, words[num_of_words]);
                    strcat(sql, "\", \n");
                    strcat(sql, key);
                    strcat(sql, ", \n\"");
                    strcat(sql, rel ? "c" : "f");
                    strcat(sql, "\"");
                    strcat(sql, ");");
                }
                free(smid);

                free(words[num_of_words]);
                words[num_of_words] = 0;
            }
        }
        --num_of_words;
    }
    free(words);

    strcat(sql, ";\n");

    return sql;
}

char* gen_sql_get_clauses_for_rel(int rel, struct fact** facts, int limit, int* position) {

    char* sql = malloc(102400);
    *sql = 0;

    strcat(sql, "SELECT `nmain`.`pk`, `nmain`.`verb` || rff.verb_flag_want || rff.verb_flag_must || rff.verb_flag_can || rff.verb_flag_may || rff.verb_flag_should, `nmain`.`subjects`, `nmain`.`objects`, `nmain`.`adverbs`, `nmain`.`questionword`, `nmain`.`from`, `nmain`.`truth`, 0 ");
    strcat(sql, " FROM clauses AS nmain JOIN rel_clause_flag AS rff ON nmain.pk = rff.fact WHERE nmain.rel = ");
    char rel_str[10];
    snprintf(rel_str, 9, "%d", rel);
    strcat(sql, rel_str);
    strcat(sql, " UNION ALL ");
    strcat(sql, "SELECT `nmain`.`pk`, `nmain`.`verb` || rff.verb_flag_want || rff.verb_flag_must || rff.verb_flag_can || rff.verb_flag_may || rff.verb_flag_should, `nmain`.`subjects`, `nmain`.`objects`, `nmain`.`adverbs`, `nmain`.`questionword`, `nmain`.`from`, `nmain`.`truth`, 0 ");
    strcat(sql, " FROM facts AS nmain JOIN rel_fact_flag AS rff ON nmain.pk = rff.fact WHERE nmain.pk IN ");
    strcat(sql, " (SELECT f2 FROM `linking` WHERE f1 = ");
    strcat(sql, rel_str);
    strcat(sql, " );");

    return sql;
}

char* gen_sql_get_double_facts() {

    char* sql = malloc(102400);
    *sql = 0;

    strcat(sql, "SELECT `nmain`.`pk`, `nmain`.`verb` || \"00000\", `nmain`.`subjects`, `nmain`.`objects`, `nmain`.`adverbs`, `nmain`.`questionword`, `nmain`.`from`, `nmain`.`truth`, `nmain`.`only_logic` ");
    strcat(sql, " FROM facts AS nmain WHERE mix_1||verb IN ( SELECT mix_1||verb AS a FROM facts GROUP BY a HAVING count(pk) >= 2) order by mix_1, verb;");

    return sql;
}

char* gen_sql_delete_everything_from(const char* filename) {

    char* sql = malloc(1024*30);
    *sql = 0;

    printf("Clean index...\n");

    strcat(sql, "delete from cache_facts ; INSERT OR IGNORE INTO cache_facts SELECT * FROM facts WHERE `from` GLOB '");
    strcat(sql, filename);
    strcat(sql, "*';\n");

    int error = sql_execute(sql, NULL, NULL);

    int i;
    for (i = 'a'; i <= 'z'; ++i) {
        printf("\r%fl\t\t\t", (100.0 / (float)(('z'-'a')*('z'-'a')) * (float)(('z'-'a')*(i-'a')) ));
        fflush(stdout);
        *sql = 0;

        int k;
        for (k = 'a'; k <= 'z'; ++k) {
            strcat(sql, "DELETE FROM rel_word_fact__");
            char z[3];
            z[0] = i;
            z[1] = k;
            z[2] = 0;
            strcat(sql, z);
            strcat(sql, " WHERE fact IN ( SELECT pk FROM cache_facts );\n");
        }

        int error = sql_execute(sql, NULL, NULL);
    }

    *sql = 0;
    strcat(sql, "DELETE FROM facts WHERE `from` GLOB '");
    strcat(sql, filename);
    strcat(sql, "*';");


            printf("\r%s\n", sql);
    /*
    static int first_run = 1;
    if (first_run) {
        first_run = 0;
        strcat(sql, "COMMIT; VACUUM; BEGIN;");
    }
    */

    return sql;
}

char* mysql_get_source(const char* key) {
    printf("mysql_get_source: %s\n", key);
    if (!key || !key[0])
        return 1;

    char* source = calloc(512, 1);
    {
        char* sql = malloc(1024);
        *sql = 0;
        if (0 == strcmp(key, "a")) {
            strcat(sql, "SELECT `from` FROM facts WHERE pk = (SELECT pk FROM facts ORDER BY pk DESC LIMIT 1);");
        }
        else {
            strcat(sql, "SELECT `from` FROM facts WHERE pk = ");
            strcat(sql, key);
            strcat(sql, " LIMIT 1;");
        }


        int error = sql_execute(sql, select_primary_key, source);
        free(sql);
    }
    printf("source: %s\n", source);

    return source;
}

char* mysql_del_record(const char* key) {
    if (!key || !key[0])
        return 1;

    char* source = mysql_get_source(key);

    char* sql = malloc(1024);
    *sql = 0;
    if (0 == strcmp(key, "a")) {
        strcat(sql, "DELETE FROM facts WHERE pk = (SELECT pk FROM facts ORDER BY pk DESC LIMIT 1);");
    }
    else {
        strcat(sql, "DELETE FROM facts WHERE pk = ");
        strcat(sql, key);
        strcat(sql, ";");
    }
    printf("pkey (in hal2009-disk.c): %s, SQL: %s\n", key, sql);

    int error = sql_execute(sql, NULL, NULL);

    int i;
    for (i = 'a'; i <= 'z'; ++i) {
        int k;
        for (k = 'a'; k <= 'z'; ++k) {
            *sql = 0;
            strcat(sql, "DELETE FROM rel_word_fact__");
            char z[3];
            z[0] = i;
            z[1] = k;
            z[2] = 0;
            strcat(sql, z);
            strcat(sql, " WHERE fact = ");
            strcat(sql, key);
            strcat(sql, ";");
            int error = sql_execute(sql, NULL, NULL);
        }
    }

    free(sql);

    return source;
}

char* gen_sql_get_facts_for_words(struct word*** words, struct fact** facts, int limit, int* position) {

    char* sql = malloc(512000);
    *sql = 0;
    int n, m, q;

    if (0 == can_be_a_pointer(words[0])) {
        return sql;
    }

    strcat(sql, "delete from cache_facts ; delete from cache_indices ");

    int in_bracket = 0;
    debugf("Generating SQL for searching facts for words (at %p).\n", words);
    char* last_smid = 0;
    for (n = 0; words[n]; ++n) {
        if (!can_be_a_pointer(words[n])) {
            continue;
        }

        int is_new = 1;
        if (can_be_a_pointer(words[n][0])) {
            for (q = 0; words[q] && q+1 < n; ++q) {
                if (can_be_a_pointer(words[q][0]) && words[n][0] == words[q][0])
                    is_new = 0;
            }
        }
        if (can_be_a_pointer(words[n]) && can_be_a_pointer(words[n][0])) {
            debugf("synonym no %d: %d, %p, %s\n",
                n,
                can_be_a_pointer(words[n]),
                can_be_a_pointer(words[n]) ? words[n][0] : 0,
                can_be_a_pointer(words[n]) && can_be_a_pointer(words[n][0]) ? words[n][0]->name : "(null)"
            );
        }
        if (!is_new) {
            debugf("not new.\n");
            continue;
        }

        for (m = 0; words[n][m]; ++m) {
            if (!(can_be_a_pointer(words[n][m]) && words[n][m]->name && words[n][m]->name[0])) {
                continue;
            }
            if (is_a_trivial_word(words[n][m]->name)) {
                continue;
            }
            if (is_bad(words[n][m]->name)) {
                continue;
            }

            char* smid = small_identifier(words[n][m]->name);
            if ((!last_smid || strcmp(last_smid, smid)) && smid) {
                if (in_bracket) {
                    strcat(sql, ")");
                    in_bracket = 0;
                }
                strcat(sql, " ; INSERT OR IGNORE INTO cache_indices SELECT fact FROM ");

                strcat(sql, " rel_word_fact__");
                strcat(sql, smid);
                strcat(sql, " AS rel_word_fact ");

                strcat(sql, " WHERE 0 ");
            }
            if (last_smid) {
                free(last_smid);
            }
            last_smid = strdup(smid);
            free(smid);


            if (words[n][m]->name[0] && words[n][m]->name[0] == '*') {
                if (strstr(words[n][m]->name+1, "*")) {
                    strcat(sql, "OR rel_word_fact.word GLOB \"");
                    strcat(sql, words[n][m]->name+1);
                    strcat(sql, "\" ");
                }
                else {
                    strcat(sql, "OR rel_word_fact.word = \"");
                    strcat(sql, words[n][m]->name+1);
                    strcat(sql, "\" ");
                }
            }
            else if (strstr(words[n][m]->name, "*")) {
                strcat(sql, "OR rel_word_fact.word GLOB \"");
                strcat(sql, words[n][m]->name);
                strcat(sql, "\" ");
            }
            else {
                strcat(sql, "OR rel_word_fact.word = \"");
                strcat(sql, words[n][m]->name);
                strcat(sql, "\" ");
            }
        }
    }
    if (last_smid)
        free(last_smid);
    if (in_bracket) {
        strcat(sql, ")");
    }
    strcat(sql, ";");
    strcat(sql, " ; INSERT OR IGNORE INTO cache_facts (pk, `from`, verb, verbgroup, subjects, objects, adverbs, mix_1, questionword, prio, rel, type, truth, hash_clauses) SELECT pk, `from`, verb, verbgroup, subjects, objects, adverbs, mix_1, questionword, prio, rel, type, truth, hash_clauses FROM facts WHERE pk in (SELECT i FROM cache_indices);");

    strcat(sql, "SELECT DISTINCT `nmain`.`pk`, `nmain`.`verb` || rff.verb_flag_want || rff.verb_flag_must || rff.verb_flag_can || rff.verb_flag_may || rff.verb_flag_should, `nmain`.`subjects`, `nmain`.`objects`, `nmain`.`adverbs`, `nmain`.`questionword`, `nmain`.`from`, `nmain`.`truth`, `nmain`.`only_logic` ");
    strcat(sql, " FROM cache_facts AS nmain LEFT JOIN rel_fact_flag AS rff ON nmain.pk = rff.fact");
    strcat(sql, ";");

    return sql;
}

int sql_execute(char* sql, int (*callback)(void*,int,char**,char**), void* arg) {
    char* err;
    int error_to_return = 0;
    while (TODO3_exec(TODO_connection, sql, callback, arg, &err)) {
        if (strstr(err, " not unique") && !strstr(err, "PRIMARY KEY must be unique")) {
            error_to_return = NOT_UNIQUE;
            break;
        }
        else if (strstr(err, "PRIMARY KEY must be unique")) {
            error_to_return = NOT_UNIQUE;
            break;
        }
        printf("SQL Error:\n------------\n%s\n------------\n%s\n------------\n\n", err, sql);
        if (strstr(err, "no such table")) {
            TODO3_free(err);

            if (TODO3_exec(TODO_connection, TODO_sql_create_table, NULL, NULL, &err)) {
                printf("SQL Error:\n------------\n%s\n------------\n%s\n------------\n\n", err, sql);
                error_to_return = TABLE_ERROR;
                break;
            }
            continue;
        }
        else {
            --num_facts_added_during_this_run;
            ++num_facts_not_added_during_this_run_because_other_error;
            printf("SQL Error:\n------------\n%s\n------------\n%s\n------------\n\n", err, sql);
            break;
        }
    }
    TODO3_free(err);

    return error_to_return;
}

struct fact* mysql_add_clause(int rel, const char* subjects, const char* objects, const char* verbs, const char* adverbs, const char* extra, const char* questionword, const char* from, float truth, short verb_flag_want, short verb_flag_must, short verb_flag_can, short verb_flag_may, short verb_flag_should) {
    if ((is_bad(subjects) && is_bad(objects) && is_bad(verbs) && !(verb_flag_want || verb_flag_must || verb_flag_can || verb_flag_may || verb_flag_should)) || (questionword && questionword[0] == ')')) {
        return 0;
    }

    int pk = get_last_pk(rel)+1;

    {
        char* sql = 0;
        sql = gen_sql_add_entry(sql, pk, rel, subjects, objects, verbs, adverbs, extra, questionword, from, truth, verb_flag_want, verb_flag_must, verb_flag_can, verb_flag_may, verb_flag_should, 0);
        sql = gen_sql_add_verb_flags(sql, pk, rel, subjects, objects, verbs, adverbs, extra, questionword, from, truth, verb_flag_want, verb_flag_must, verb_flag_can, verb_flag_may, verb_flag_should, 0);
        sql = gen_sql_add_word_fact_relations(sql, pk, rel, subjects, objects, verbs, adverbs, extra, questionword, from, truth, verb_flag_want, verb_flag_must, verb_flag_can, verb_flag_may, verb_flag_should, 0);
        int error = sql_execute(sql, NULL, NULL);
        free(sql);
    }

    return 0;
}

struct fact* mysql_add_fact(const char* subjects, const char* objects, const char* verbs, const char* adverbs, const char* extra, const char* questionword, const char* from, float truth, short verb_flag_want, short verb_flag_must, short verb_flag_can, short verb_flag_may, short verb_flag_should, short only_logic) {
    if ((is_bad(subjects) && is_bad(objects) && is_bad(verbs) && !(verb_flag_want || verb_flag_must || verb_flag_can || verb_flag_may || verb_flag_should)) || (questionword && questionword[0] == ')')) {
        return 0;
    }

    int pk = get_last_pk(0)+1;
    int error = 0;
    {
        char* sql = 0;
        sql = gen_sql_add_entry(sql, pk, 0, subjects, objects, verbs, adverbs, extra, questionword, from, truth, verb_flag_want, verb_flag_must, verb_flag_can, verb_flag_may, verb_flag_should, only_logic);
        sql = gen_sql_add_verb_flags(sql, pk, 0, subjects, objects, verbs, adverbs, extra, questionword, from, truth, verb_flag_want, verb_flag_must, verb_flag_can, verb_flag_may, verb_flag_should, only_logic);
        sql = gen_sql_add_word_fact_relations(sql, pk, 0, subjects, objects, verbs, adverbs, extra, questionword, from, truth, verb_flag_want, verb_flag_must, verb_flag_can, verb_flag_may, verb_flag_should, only_logic);
        //printf("%s\n", sql);
        error = sql_execute(sql, NULL, NULL);
        free(sql);
    }

    if (error) {
        printf("Error in mysql_add_fact.\n");
        return 0;
    }

    struct fact* fact = calloc(sizeof(struct fact), 1);
    fact->pk = pk;

    return fact;
}

struct word* mysql_set_word(const char* name) {
    if (!name || 0 == strlen(name)) {
        return 0;
    }
    struct word* already_there = get_word(name);
    if (already_there) {
        return already_there;
    }
    int i = WRONG;
    if (strlen(name) >= 1) {
        i = n(name[0]);
    }
    int k = WRONG;
    if (strlen(name) >= 2) {
        k = n(name[1]);
    }

    if (0 == mysql_net[i][k]->list) {
        // debugf("empty list wile inserting %s.\n", name);
    }
    else {
        0 && debugf("not empty list wile inserting %s: %p, %p entries, last entry = %s\n", name, mysql_net[i][k]->list, mysql_net[i][k]->size, ((struct word**)(mysql_net[i][k]->list))[mysql_net[i][k]->size-1]->name);
    }

    if (mysql_net[i][k]->size == 0) {
        mysql_net[i][k]->list = calloc(sizeof(struct word*), 11);
        mysql_net[i][k]->allocated_until = 10;
    }
    else if (mysql_net[i][k]->size >= mysql_net[i][k]->allocated_until) {
        mysql_net[i][k]->allocated_until += 10;
        mysql_net[i][k]->list = realloc(mysql_net[i][k]->list, sizeof(struct word*)*(mysql_net[i][k]->allocated_until+1));
    }

    mysql_net[i][k]->list[mysql_net[i][k]->size] = calloc(1, sizeof(struct word));
    ((struct word**)(mysql_net[i][k]->list))[mysql_net[i][k]->size]->name   = strdup(name);
    ((struct word**)(mysql_net[i][k]->list))[mysql_net[i][k]->size]->length = strlen(name);
    0 && debugf("inserted: %s = %p, %p.\n",
            ((struct word**)(mysql_net[i][k]->list))[mysql_net[i][k]->size]->name,
            mysql_net[i][k]->list[mysql_net[i][k]->size],
            mysql_net[i][k]->size);
    ++(mysql_net[i][k]->size);
    return mysql_net[i][k]->list[mysql_net[i][k]->size - 1];
}

static int callback_get_facts(void* arg, int argc, char **argv, char **azColName) {
    struct request_get_facts_for_words* req = arg;

    if (*req->position >= req->limit) {
        return 1;
    }

    if (argc <= 5 || (!argv[1] || !strlen(argv[1])) || ((!argv[2] || !strlen(argv[2])) && (!argv[3] || !strlen(argv[3])))) {
        return 0;
    }

    struct fact* fact  = calloc(sizeof(struct fact), 1);
    fact->pk           = to_number(argv[0] ? argv[0] : "0");
    fact->verbs        = divide_words(argv[1] ? argv[1] : "");
    fact->subjects     = divide_words(argv[2] ? argv[2] : "");
    fact->objects      = divide_words(argv[3] ? argv[3] : "");
    fact->adverbs      = divide_words(argv[4] ? argv[4] : "");
    fact->extra        = divide_words("");
    fact->questionword = strdup(argv[5] ? argv[5] : "");
    fact->from         = strdup(argv[6] ? argv[6] : "");
    fact->truth        = (argv[7] && argv[7][0] && argv[7][0] == '1') ? 1.0 : ((argv[7] && argv[7][0] && argv[7][0] && argv[7][1] && argv[7][2] != '0') ? 0.5 : 0.0);
    fact->only_logic   = argv[8] && argv[8][0] && argv[8][0] == '1' ? 1 : 0;

    req->facts[*req->position] = fact;
    debugf("Added fact no %d at %p (%s, %s, %s, %s).\n", *req->position, req->facts[*req->position], argv[1], argv[2], argv[3], argv[4]);
    ++(*req->position);

    return 0;
}

int mysql_search_facts_for_words_in_net(struct word*** words, struct fact** facts, int limit, int* position) {
    struct request_get_facts_for_words req;
    req.words    = words;
    req.facts    = facts;
    req.limit    = limit;
    req.position = position;
    req.rel      = 0;

    {
        char* sql = gen_sql_get_facts_for_words(words, facts, limit, position);
        printf("%s\n", sql);
        int error = sql_execute(sql, callback_get_facts, &req);
        free(sql);
    }

    if (*req.position > limit - 10) {
        return TOOMUCH;
    }

    return 0;
}

int mysql_search_double_facts(struct word*** words, struct fact** facts, int limit, int* position) {
    struct request_get_facts_for_words req;
    req.words    = words;
    req.facts    = facts;
    req.limit    = limit;
    req.position = position;
    req.rel      = 0;

    {
        char* sql = gen_sql_get_double_facts();
        printf("%s\n", sql);
        int error = sql_execute(sql, callback_get_facts, &req);
        free(sql);
    }

    if (*req.position > limit - 10) {
        return TOOMUCH;
    }

    return 0;
}

int re_index_size_sql = 0;
int re_index_pos_sql = 0;
int re_index_in_facts = 1;

static int callback_re_index(void* arg, int argc, char **argv, char **azColName) {
    char* re_index_sql = *((void**)(arg));

    char** verbs       = divide_string(argv[1] ? argv[1] : "");
    char** subjects    = divide_string(argv[2] ? argv[2] : "");
    char** objects     = divide_string(argv[3] ? argv[3] : "");
    char** adverbs     = divide_string(argv[4] ? argv[4] : "");

    int stage;
    for (stage = 1; stage <= 4; ++stage) {

        char** words;
        if (stage == 1) words = verbs;
        if (stage == 2) words = subjects;
        if (stage == 3) words = objects;
        if (stage == 4) words = adverbs;

        int i;
        for (i = 0; words[i]; ++i) {
            char* smid = small_identifier(words[i]);
            {
                char sql[1000];
                strcat(sql, "INSERT OR IGNORE INTO rel_word_fact__");
                strcat(sql, smid);
                strcat(sql, " (`word`, `fact`, `table`) VALUES (");
                strcat(sql, "\n\"");
                strcat(sql, words[i]);
                strcat(sql, "\", \n");
                strcat(sql, argv[0]);
                strcat(sql, ", \n\"");
                strcat(sql, re_index_in_facts ? "f" : "c");
                strcat(sql, "\"");
                strcat(sql, ");");

                int size_sql = strlen(sql);
                if (!(re_index_pos_sql + size_sql < re_index_size_sql - 10)) {
                    re_index_size_sql += 10000;
                    *((void**)(arg)) = realloc(*((void**)(arg)), re_index_size_sql);
                    re_index_sql = *((void**)(arg));
                }
                re_index_pos_sql += size_sql;
                strcat(re_index_sql, sql);
            }
            free(smid);
            free(words[i]);
        }

        free(words);
    }

    return 0;
}

int mysql_re_index() {
    {
        int error1 = sql_execute("COMMIT;", NULL, NULL);
        printf("Delete old index...\n");
        int error2 = sql_execute("delete from rel_word_fact__aa; delete from rel_word_fact__ab; delete from rel_word_fact__ac; delete from rel_word_fact__ad; delete from rel_word_fact__ae; delete from rel_word_fact__af; delete from rel_word_fact__ag; delete from rel_word_fact__ah; delete from rel_word_fact__ai; delete from rel_word_fact__aj; delete from rel_word_fact__ak; delete from rel_word_fact__al; delete from rel_word_fact__am; delete from rel_word_fact__an; delete from rel_word_fact__ao; delete from rel_word_fact__ap; delete from rel_word_fact__aq; delete from rel_word_fact__ar; delete from rel_word_fact__as; delete from rel_word_fact__at; delete from rel_word_fact__au; delete from rel_word_fact__av; delete from rel_word_fact__aw; delete from rel_word_fact__ax; delete from rel_word_fact__ay; delete from rel_word_fact__az; delete from rel_word_fact__a_; delete from rel_word_fact__ba; delete from rel_word_fact__bb; delete from rel_word_fact__bc; delete from rel_word_fact__bd; delete from rel_word_fact__be; delete from rel_word_fact__bf; delete from rel_word_fact__bg; delete from rel_word_fact__bh; delete from rel_word_fact__bi; delete from rel_word_fact__bj; delete from rel_word_fact__bk; delete from rel_word_fact__bl; delete from rel_word_fact__bm; delete from rel_word_fact__bn; delete from rel_word_fact__bo; delete from rel_word_fact__bp; delete from rel_word_fact__bq; delete from rel_word_fact__br; delete from rel_word_fact__bs; delete from rel_word_fact__bt; delete from rel_word_fact__bu; delete from rel_word_fact__bv; delete from rel_word_fact__bw; delete from rel_word_fact__bx; delete from rel_word_fact__by; delete from rel_word_fact__bz; delete from rel_word_fact__b_; delete from rel_word_fact__ca; delete from rel_word_fact__cb; delete from rel_word_fact__cc; delete from rel_word_fact__cd; delete from rel_word_fact__ce; delete from rel_word_fact__cf; delete from rel_word_fact__cg; delete from rel_word_fact__ch; delete from rel_word_fact__ci; delete from rel_word_fact__cj; delete from rel_word_fact__ck; delete from rel_word_fact__cl; delete from rel_word_fact__cm; delete from rel_word_fact__cn; delete from rel_word_fact__co; delete from rel_word_fact__cp; delete from rel_word_fact__cq; delete from rel_word_fact__cr; delete from rel_word_fact__cs; delete from rel_word_fact__ct; delete from rel_word_fact__cu; delete from rel_word_fact__cv; delete from rel_word_fact__cw; delete from rel_word_fact__cx; delete from rel_word_fact__cy; delete from rel_word_fact__cz; delete from rel_word_fact__c_; delete from rel_word_fact__da; delete from rel_word_fact__db; delete from rel_word_fact__dc; delete from rel_word_fact__dd; delete from rel_word_fact__de; delete from rel_word_fact__df; delete from rel_word_fact__dg; delete from rel_word_fact__dh; delete from rel_word_fact__di; delete from rel_word_fact__dj; delete from rel_word_fact__dk; delete from rel_word_fact__dl; delete from rel_word_fact__dm; delete from rel_word_fact__dn; delete from rel_word_fact__do; delete from rel_word_fact__dp; delete from rel_word_fact__dq; delete from rel_word_fact__dr; delete from rel_word_fact__ds; delete from rel_word_fact__dt; delete from rel_word_fact__du; delete from rel_word_fact__dv; delete from rel_word_fact__dw; delete from rel_word_fact__dx; delete from rel_word_fact__dy; delete from rel_word_fact__dz; delete from rel_word_fact__d_; delete from rel_word_fact__ea; delete from rel_word_fact__eb; delete from rel_word_fact__ec; delete from rel_word_fact__ed; delete from rel_word_fact__ee; delete from rel_word_fact__ef; delete from rel_word_fact__eg; delete from rel_word_fact__eh; delete from rel_word_fact__ei; delete from rel_word_fact__ej; delete from rel_word_fact__ek; delete from rel_word_fact__el; delete from rel_word_fact__em; delete from rel_word_fact__en; delete from rel_word_fact__eo; delete from rel_word_fact__ep; delete from rel_word_fact__eq; delete from rel_word_fact__er; delete from rel_word_fact__es; delete from rel_word_fact__et; delete from rel_word_fact__eu; delete from rel_word_fact__ev; delete from rel_word_fact__ew; delete from rel_word_fact__ex; delete from rel_word_fact__ey; delete from rel_word_fact__ez; delete from rel_word_fact__e_; delete from rel_word_fact__fa; delete from rel_word_fact__fb; delete from rel_word_fact__fc; delete from rel_word_fact__fd; delete from rel_word_fact__fe; delete from rel_word_fact__ff; delete from rel_word_fact__fg; delete from rel_word_fact__fh; delete from rel_word_fact__fi; delete from rel_word_fact__fj; delete from rel_word_fact__fk; delete from rel_word_fact__fl; delete from rel_word_fact__fm; delete from rel_word_fact__fn; delete from rel_word_fact__fo; delete from rel_word_fact__fp; delete from rel_word_fact__fq; delete from rel_word_fact__fr; delete from rel_word_fact__fs; delete from rel_word_fact__ft; delete from rel_word_fact__fu; delete from rel_word_fact__fv; delete from rel_word_fact__fw; delete from rel_word_fact__fx; delete from rel_word_fact__fy; delete from rel_word_fact__fz; delete from rel_word_fact__f_; delete from rel_word_fact__ga; delete from rel_word_fact__gb; delete from rel_word_fact__gc; delete from rel_word_fact__gd; delete from rel_word_fact__ge; delete from rel_word_fact__gf; delete from rel_word_fact__gg; delete from rel_word_fact__gh; delete from rel_word_fact__gi; delete from rel_word_fact__gj; delete from rel_word_fact__gk; delete from rel_word_fact__gl; delete from rel_word_fact__gm; delete from rel_word_fact__gn; delete from rel_word_fact__go; delete from rel_word_fact__gp; delete from rel_word_fact__gq; delete from rel_word_fact__gr; delete from rel_word_fact__gs; delete from rel_word_fact__gt; delete from rel_word_fact__gu; delete from rel_word_fact__gv; delete from rel_word_fact__gw; delete from rel_word_fact__gx; delete from rel_word_fact__gy; delete from rel_word_fact__gz; delete from rel_word_fact__g_; delete from rel_word_fact__ha; delete from rel_word_fact__hb; delete from rel_word_fact__hc; delete from rel_word_fact__hd; delete from rel_word_fact__he; delete from rel_word_fact__hf; delete from rel_word_fact__hg; delete from rel_word_fact__hh; delete from rel_word_fact__hi; delete from rel_word_fact__hj; delete from rel_word_fact__hk; delete from rel_word_fact__hl; delete from rel_word_fact__hm; delete from rel_word_fact__hn; delete from rel_word_fact__ho; delete from rel_word_fact__hp; delete from rel_word_fact__hq; delete from rel_word_fact__hr; delete from rel_word_fact__hs; delete from rel_word_fact__ht; delete from rel_word_fact__hu; delete from rel_word_fact__hv; delete from rel_word_fact__hw; delete from rel_word_fact__hx; delete from rel_word_fact__hy; delete from rel_word_fact__hz; delete from rel_word_fact__h_; delete from rel_word_fact__ia; delete from rel_word_fact__ib; delete from rel_word_fact__ic; delete from rel_word_fact__id; delete from rel_word_fact__ie; delete from rel_word_fact__if; delete from rel_word_fact__ig; delete from rel_word_fact__ih; delete from rel_word_fact__ii; delete from rel_word_fact__ij; delete from rel_word_fact__ik; delete from rel_word_fact__il; delete from rel_word_fact__im; delete from rel_word_fact__in; delete from rel_word_fact__io; delete from rel_word_fact__ip; delete from rel_word_fact__iq; delete from rel_word_fact__ir; delete from rel_word_fact__is; delete from rel_word_fact__it; delete from rel_word_fact__iu; delete from rel_word_fact__iv; delete from rel_word_fact__iw; delete from rel_word_fact__ix; delete from rel_word_fact__iy; delete from rel_word_fact__iz; delete from rel_word_fact__i_; delete from rel_word_fact__ja; delete from rel_word_fact__jb; delete from rel_word_fact__jc; delete from rel_word_fact__jd; delete from rel_word_fact__je; delete from rel_word_fact__jf; delete from rel_word_fact__jg; delete from rel_word_fact__jh; delete from rel_word_fact__ji; delete from rel_word_fact__jj; delete from rel_word_fact__jk; delete from rel_word_fact__jl; delete from rel_word_fact__jm; delete from rel_word_fact__jn; delete from rel_word_fact__jo; delete from rel_word_fact__jp; delete from rel_word_fact__jq; delete from rel_word_fact__jr; delete from rel_word_fact__js; delete from rel_word_fact__jt; delete from rel_word_fact__ju; delete from rel_word_fact__jv; delete from rel_word_fact__jw; delete from rel_word_fact__jx; delete from rel_word_fact__jy; delete from rel_word_fact__jz; delete from rel_word_fact__j_; delete from rel_word_fact__ka; delete from rel_word_fact__kb; delete from rel_word_fact__kc; delete from rel_word_fact__kd; delete from rel_word_fact__ke; delete from rel_word_fact__kf; delete from rel_word_fact__kg; delete from rel_word_fact__kh; delete from rel_word_fact__ki; delete from rel_word_fact__kj; delete from rel_word_fact__kk; delete from rel_word_fact__kl; delete from rel_word_fact__km; delete from rel_word_fact__kn; delete from rel_word_fact__ko; delete from rel_word_fact__kp; delete from rel_word_fact__kq; delete from rel_word_fact__kr; delete from rel_word_fact__ks; delete from rel_word_fact__kt; delete from rel_word_fact__ku; delete from rel_word_fact__kv; delete from rel_word_fact__kw; delete from rel_word_fact__kx; delete from rel_word_fact__ky; delete from rel_word_fact__kz; delete from rel_word_fact__k_; delete from rel_word_fact__la; delete from rel_word_fact__lb; delete from rel_word_fact__lc; delete from rel_word_fact__ld; delete from rel_word_fact__le; delete from rel_word_fact__lf; delete from rel_word_fact__lg; delete from rel_word_fact__lh; delete from rel_word_fact__li; delete from rel_word_fact__lj; delete from rel_word_fact__lk; delete from rel_word_fact__ll; delete from rel_word_fact__lm; delete from rel_word_fact__ln; delete from rel_word_fact__lo; delete from rel_word_fact__lp; delete from rel_word_fact__lq; delete from rel_word_fact__lr; delete from rel_word_fact__ls; delete from rel_word_fact__lt; delete from rel_word_fact__lu; delete from rel_word_fact__lv; delete from rel_word_fact__lw; delete from rel_word_fact__lx; delete from rel_word_fact__ly; delete from rel_word_fact__lz; delete from rel_word_fact__l_; delete from rel_word_fact__ma; delete from rel_word_fact__mb; delete from rel_word_fact__mc; delete from rel_word_fact__md; delete from rel_word_fact__me; delete from rel_word_fact__mf; delete from rel_word_fact__mg; delete from rel_word_fact__mh; delete from rel_word_fact__mi; delete from rel_word_fact__mj; delete from rel_word_fact__mk; delete from rel_word_fact__ml; delete from rel_word_fact__mm; delete from rel_word_fact__mn; delete from rel_word_fact__mo; delete from rel_word_fact__mp; delete from rel_word_fact__mq; delete from rel_word_fact__mr; delete from rel_word_fact__ms; delete from rel_word_fact__mt; delete from rel_word_fact__mu; delete from rel_word_fact__mv; delete from rel_word_fact__mw; delete from rel_word_fact__mx; delete from rel_word_fact__my; delete from rel_word_fact__mz; delete from rel_word_fact__m_; delete from rel_word_fact__na; delete from rel_word_fact__nb; delete from rel_word_fact__nc; delete from rel_word_fact__nd; delete from rel_word_fact__ne; delete from rel_word_fact__nf; delete from rel_word_fact__ng; delete from rel_word_fact__nh; delete from rel_word_fact__ni; delete from rel_word_fact__nj; delete from rel_word_fact__nk; delete from rel_word_fact__nl; delete from rel_word_fact__nm; delete from rel_word_fact__nn; delete from rel_word_fact__no; delete from rel_word_fact__np; delete from rel_word_fact__nq; delete from rel_word_fact__nr; delete from rel_word_fact__ns; delete from rel_word_fact__nt; delete from rel_word_fact__nu; delete from rel_word_fact__nv; delete from rel_word_fact__nw; delete from rel_word_fact__nx; delete from rel_word_fact__ny; delete from rel_word_fact__nz; delete from rel_word_fact__n_; delete from rel_word_fact__oa; delete from rel_word_fact__ob; delete from rel_word_fact__oc; delete from rel_word_fact__od; delete from rel_word_fact__oe; delete from rel_word_fact__of; delete from rel_word_fact__og; delete from rel_word_fact__oh; delete from rel_word_fact__oi; delete from rel_word_fact__oj; delete from rel_word_fact__ok; delete from rel_word_fact__ol; delete from rel_word_fact__om; delete from rel_word_fact__on; delete from rel_word_fact__oo; delete from rel_word_fact__op; delete from rel_word_fact__oq; delete from rel_word_fact__or; delete from rel_word_fact__os; delete from rel_word_fact__ot; delete from rel_word_fact__ou; delete from rel_word_fact__ov; delete from rel_word_fact__ow; delete from rel_word_fact__ox; delete from rel_word_fact__oy; delete from rel_word_fact__oz; delete from rel_word_fact__o_; delete from rel_word_fact__pa; delete from rel_word_fact__pb; delete from rel_word_fact__pc; delete from rel_word_fact__pd; delete from rel_word_fact__pe; delete from rel_word_fact__pf; delete from rel_word_fact__pg; delete from rel_word_fact__ph; delete from rel_word_fact__pi; delete from rel_word_fact__pj; delete from rel_word_fact__pk; delete from rel_word_fact__pl; delete from rel_word_fact__pm; delete from rel_word_fact__pn; delete from rel_word_fact__po; delete from rel_word_fact__pp; delete from rel_word_fact__pq; delete from rel_word_fact__pr; delete from rel_word_fact__ps; delete from rel_word_fact__pt; delete from rel_word_fact__pu; delete from rel_word_fact__pv; delete from rel_word_fact__pw; delete from rel_word_fact__px; delete from rel_word_fact__py; delete from rel_word_fact__pz; delete from rel_word_fact__p_; delete from rel_word_fact__qa; delete from rel_word_fact__qb; delete from rel_word_fact__qc; delete from rel_word_fact__qd; delete from rel_word_fact__qe; delete from rel_word_fact__qf; delete from rel_word_fact__qg; delete from rel_word_fact__qh; delete from rel_word_fact__qi; delete from rel_word_fact__qj; delete from rel_word_fact__qk; delete from rel_word_fact__ql; delete from rel_word_fact__qm; delete from rel_word_fact__qn; delete from rel_word_fact__qo; delete from rel_word_fact__qp; delete from rel_word_fact__qq; delete from rel_word_fact__qr; delete from rel_word_fact__qs; delete from rel_word_fact__qt; delete from rel_word_fact__qu; delete from rel_word_fact__qv; delete from rel_word_fact__qw; delete from rel_word_fact__qx; delete from rel_word_fact__qy; delete from rel_word_fact__qz; delete from rel_word_fact__q_; delete from rel_word_fact__ra; delete from rel_word_fact__rb; delete from rel_word_fact__rc; delete from rel_word_fact__rd; delete from rel_word_fact__re; delete from rel_word_fact__rf; delete from rel_word_fact__rg; delete from rel_word_fact__rh; delete from rel_word_fact__ri; delete from rel_word_fact__rj; delete from rel_word_fact__rk; delete from rel_word_fact__rl; delete from rel_word_fact__rm; delete from rel_word_fact__rn; delete from rel_word_fact__ro; delete from rel_word_fact__rp; delete from rel_word_fact__rq; delete from rel_word_fact__rr; delete from rel_word_fact__rs; delete from rel_word_fact__rt; delete from rel_word_fact__ru; delete from rel_word_fact__rv; delete from rel_word_fact__rw; delete from rel_word_fact__rx; delete from rel_word_fact__ry; delete from rel_word_fact__rz; delete from rel_word_fact__r_; delete from rel_word_fact__sa; delete from rel_word_fact__sb; delete from rel_word_fact__sc; delete from rel_word_fact__sd; delete from rel_word_fact__se; delete from rel_word_fact__sf; delete from rel_word_fact__sg; delete from rel_word_fact__sh; delete from rel_word_fact__si; delete from rel_word_fact__sj; delete from rel_word_fact__sk; delete from rel_word_fact__sl; delete from rel_word_fact__sm; delete from rel_word_fact__sn; delete from rel_word_fact__so; delete from rel_word_fact__sp; delete from rel_word_fact__sq; delete from rel_word_fact__sr; delete from rel_word_fact__ss; delete from rel_word_fact__st; delete from rel_word_fact__su; delete from rel_word_fact__sv; delete from rel_word_fact__sw; delete from rel_word_fact__sx; delete from rel_word_fact__sy; delete from rel_word_fact__sz; delete from rel_word_fact__s_; delete from rel_word_fact__ta; delete from rel_word_fact__tb; delete from rel_word_fact__tc; delete from rel_word_fact__td; delete from rel_word_fact__te; delete from rel_word_fact__tf; delete from rel_word_fact__tg; delete from rel_word_fact__th; delete from rel_word_fact__ti; delete from rel_word_fact__tj; delete from rel_word_fact__tk; delete from rel_word_fact__tl; delete from rel_word_fact__tm; delete from rel_word_fact__tn; delete from rel_word_fact__to; delete from rel_word_fact__tp; delete from rel_word_fact__tq; delete from rel_word_fact__tr; delete from rel_word_fact__ts; delete from rel_word_fact__tt; delete from rel_word_fact__tu; delete from rel_word_fact__tv; delete from rel_word_fact__tw; delete from rel_word_fact__tx; delete from rel_word_fact__ty; delete from rel_word_fact__tz; delete from rel_word_fact__t_; delete from rel_word_fact__ua; delete from rel_word_fact__ub; delete from rel_word_fact__uc; delete from rel_word_fact__ud; delete from rel_word_fact__ue; delete from rel_word_fact__uf; delete from rel_word_fact__ug; delete from rel_word_fact__uh; delete from rel_word_fact__ui; delete from rel_word_fact__uj; delete from rel_word_fact__uk; delete from rel_word_fact__ul; delete from rel_word_fact__um; delete from rel_word_fact__un; delete from rel_word_fact__uo; delete from rel_word_fact__up; delete from rel_word_fact__uq; delete from rel_word_fact__ur; delete from rel_word_fact__us; delete from rel_word_fact__ut; delete from rel_word_fact__uu; delete from rel_word_fact__uv; delete from rel_word_fact__uw; delete from rel_word_fact__ux; delete from rel_word_fact__uy; delete from rel_word_fact__uz; delete from rel_word_fact__u_; delete from rel_word_fact__va; delete from rel_word_fact__vb; delete from rel_word_fact__vc; delete from rel_word_fact__vd; delete from rel_word_fact__ve; delete from rel_word_fact__vf; delete from rel_word_fact__vg; delete from rel_word_fact__vh; delete from rel_word_fact__vi; delete from rel_word_fact__vj; delete from rel_word_fact__vk; delete from rel_word_fact__vl; delete from rel_word_fact__vm; delete from rel_word_fact__vn; delete from rel_word_fact__vo; delete from rel_word_fact__vp; delete from rel_word_fact__vq; delete from rel_word_fact__vr; delete from rel_word_fact__vs; delete from rel_word_fact__vt; delete from rel_word_fact__vu; delete from rel_word_fact__vv; delete from rel_word_fact__vw; delete from rel_word_fact__vx; delete from rel_word_fact__vy; delete from rel_word_fact__vz; delete from rel_word_fact__v_; delete from rel_word_fact__wa; delete from rel_word_fact__wb; delete from rel_word_fact__wc; delete from rel_word_fact__wd; delete from rel_word_fact__we; delete from rel_word_fact__wf; delete from rel_word_fact__wg; delete from rel_word_fact__wh; delete from rel_word_fact__wi; delete from rel_word_fact__wj; delete from rel_word_fact__wk; delete from rel_word_fact__wl; delete from rel_word_fact__wm; delete from rel_word_fact__wn; delete from rel_word_fact__wo; delete from rel_word_fact__wp; delete from rel_word_fact__wq; delete from rel_word_fact__wr; delete from rel_word_fact__ws; delete from rel_word_fact__wt; delete from rel_word_fact__wu; delete from rel_word_fact__wv; delete from rel_word_fact__ww; delete from rel_word_fact__wx; delete from rel_word_fact__wy; delete from rel_word_fact__wz; delete from rel_word_fact__w_; delete from rel_word_fact__xa; delete from rel_word_fact__xb; delete from rel_word_fact__xc; delete from rel_word_fact__xd; delete from rel_word_fact__xe; delete from rel_word_fact__xf; delete from rel_word_fact__xg; delete from rel_word_fact__xh; delete from rel_word_fact__xi; delete from rel_word_fact__xj; delete from rel_word_fact__xk; delete from rel_word_fact__xl; delete from rel_word_fact__xm; delete from rel_word_fact__xn; delete from rel_word_fact__xo; delete from rel_word_fact__xp; delete from rel_word_fact__xq; delete from rel_word_fact__xr; delete from rel_word_fact__xs; delete from rel_word_fact__xt; delete from rel_word_fact__xu; delete from rel_word_fact__xv; delete from rel_word_fact__xw; delete from rel_word_fact__xx; delete from rel_word_fact__xy; delete from rel_word_fact__xz; delete from rel_word_fact__x_; delete from rel_word_fact__ya; delete from rel_word_fact__yb; delete from rel_word_fact__yc; delete from rel_word_fact__yd; delete from rel_word_fact__ye; delete from rel_word_fact__yf; delete from rel_word_fact__yg; delete from rel_word_fact__yh; delete from rel_word_fact__yi; delete from rel_word_fact__yj; delete from rel_word_fact__yk; delete from rel_word_fact__yl; delete from rel_word_fact__ym; delete from rel_word_fact__yn; delete from rel_word_fact__yo; delete from rel_word_fact__yp; delete from rel_word_fact__yq; delete from rel_word_fact__yr; delete from rel_word_fact__ys; delete from rel_word_fact__yt; delete from rel_word_fact__yu; delete from rel_word_fact__yv; delete from rel_word_fact__yw; delete from rel_word_fact__yx; delete from rel_word_fact__yy; delete from rel_word_fact__yz; delete from rel_word_fact__y_; delete from rel_word_fact__za; delete from rel_word_fact__zb; delete from rel_word_fact__zc; delete from rel_word_fact__zd; delete from rel_word_fact__ze; delete from rel_word_fact__zf; delete from rel_word_fact__zg; delete from rel_word_fact__zh; delete from rel_word_fact__zi; delete from rel_word_fact__zj; delete from rel_word_fact__zk; delete from rel_word_fact__zl; delete from rel_word_fact__zm; delete from rel_word_fact__zn; delete from rel_word_fact__zo; delete from rel_word_fact__zp; delete from rel_word_fact__zq; delete from rel_word_fact__zr; delete from rel_word_fact__zs; delete from rel_word_fact__zt; delete from rel_word_fact__zu; delete from rel_word_fact__zv; delete from rel_word_fact__zw; delete from rel_word_fact__zx; delete from rel_word_fact__zy; delete from rel_word_fact__zz;", NULL, NULL);
        printf("Done.\n");
        printf("Run vacuum...\n");
        int error3 = sql_execute("VACUUM;", NULL, NULL);
        printf("Done.\n");
        printf("Create new index...\n");

        re_index_size_sql = 10001;
        re_index_pos_sql = 10001;
        char* sql = calloc(re_index_size_sql, 1);

        re_index_in_facts = 1;
        {
            char sql[5120];
            *sql = 0;
            strcat(sql, "SELECT count(pk) from facts;");
            printf("%s\n", sql);
            char key[99];
            int error = sql_execute(sql, select_primary_key, key);
            int count = to_number(key ? key : "0");

            int k = 0;
            while (k < count) {
                *sql = 0;
                strcat(sql, "SELECT `nmain`.`pk`, `nmain`.`verb` || \"00000\", `nmain`.`subjects`, `nmain`.`objects`, `nmain`.`adverbs`, `nmain`.`questionword`, `nmain`.`from`, `nmain`.`truth`");
                strcat(sql, " FROM facts AS nmain LIMIT ");
                char _k_1[40];
                snprintf(_k_1, 39, "%d", k);
                char _k_2[40];
                snprintf(_k_2, 39, "%d", k+20000);
                strcat(sql, _k_1);
                strcat(sql, ", ");
                strcat(sql, _k_2);
                strcat(sql, ";");
                printf("%s\n", sql);

                int error = sql_execute(sql, callback_re_index, key);
            }
        }

        printf("Done.\n");
        int error4 = sql_execute("BEGIN;", NULL, NULL);
    }
    return 0;
}

struct fact** mysql_search_clauses(int rel) {
    int limit = 8000;
    if (strcmp("1", check_config("limit-amount-of-answers", "1"))) {
        limit = 100000;
    }
    struct fact** clauses = calloc(1, sizeof(struct fact*)*(limit+1));
    int position = 0;

    struct request_get_facts_for_words req;
    req.words    = 0;
    req.facts    = clauses;
    req.limit    = limit;
    req.position = &position;
    req.rel      = rel;

    {
        char* sql = gen_sql_get_clauses_for_rel(rel, clauses, limit, &position);
        //printf("%s\n", sql);
        int error = sql_execute(sql, callback_get_facts, &req);
        free(sql);
    }

    return clauses;
}

int mysql_delete_everything_from(const char* filename) {
    {
        char* sql = gen_sql_delete_everything_from(filename);
        int error = sql_execute(sql, NULL, NULL);
        free(sql);
        return error;
    }
    return INVALID_POINTER;
}

int mysql_set_to_invalid_value(void** p) {
    if (!p) return 1;
    if (*p && can_be_a_pointer(*p)) free(*p);
    *p = INVALID_POINTER;
    return 0;
}

int mysql_add_link (const char* link, int key_1, int key_2) {
    if (!link) {
        return INVALID;
    }

    char sql[5120];
    *sql = 0;
    strcat(sql, "INSERT INTO linking (`link`, `f1`, `f2`) VALUES (\"");

    char str_fact_1[40];
    snprintf(str_fact_1, 39, "%d", key_1);
    char str_fact_2[40];
    snprintf(str_fact_2, 39, "%d", key_2);

    strcat(sql, link);
    strcat(sql, "\", ");
    strcat(sql, str_fact_1);
    strcat(sql, ", ");
    strcat(sql, str_fact_2);
    strcat(sql, ");");

    int error = sql_execute(sql, NULL, NULL);
    return error;
}


