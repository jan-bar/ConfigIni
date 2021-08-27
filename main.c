#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define  MAX_VALUE  1024 /* ��ȡ�ļ���������󳤶� */
// printf("File = %s\nLine = %d\nFunc=%s\nDate=%s\nTime=%s\n", __FILE__, __LINE__, __FUNCTION__, __DATE__, __TIME__);
#define  PRINT_ERRMSG(STR) fprintf(stderr,"line:%d,msg:%s,eMsg:%s\n", __LINE__, STR, strerror(errno))

#define TRIM_LIFT  1 /* ȥ����߿հ��ַ� */
#define TRIM_RIGHT 2 /* ȥ���ұ߿հ��ַ� */
#define TRIM_SPACE 3 /* ȥ�����߿հ��ַ� */

typedef struct _option {
  char    *key;           /* ��Ӧ�� */
  char    *value;         /* ��Ӧֵ */
  struct  _option *next;  /* �������ӱ�ʶ */
}Option;

typedef struct _data {
  char    *section;    /* ����sectionֵ */
  Option  *option;     /* option����ͷ  */
  struct  _data *next; /* �������ӱ�ʶ  */
}Data;

typedef struct {
  char    comment;    /* ��ʾע�͵ķ���    */
  char    separator;  /* ��ʾ�ָ���        */
  char    *re_string; /* ����ֵ�ַ�����ֵ  */
  int     re_int;     /* ����int��ֵ       */
  bool    re_bool;    /* ����bool��ֵ      */
  double  re_double ; /* ����double����    */
  Data    *data;      /* �������ݵ�ͷ      */
}Config;

/**
* �ж��ַ����Ƿ�Ϊ��
* Ϊ�շ���true,��Ϊ�շ���false
**/
bool str_empty(const char *string)
{
  return NULL == string || '\0' == *string;
}

/**
* ���������section,key,value
* ������ʱ������section������һ��
* �����Ӧsection��key������������һ��
* ���section�Ѵ����򲻻��ظ�����
* �����Ӧsection��key�Ѵ�����ֻ�Ḳ��key��ֵ
**/
bool cnf_add_option(Config *cnf, const char *section, const char *key, const char *value)
{
  if (NULL == cnf || str_empty(section) || str_empty(key) || str_empty(value)) {
    return false; /* ��������ȷ,����false */
  }

  Data *p = cnf->data; /* �ñ���pѭ������data,�ҵ���Ӧsection */
  while (NULL != p && 0 != strcmp(p->section, section)) {
    p = p->next;
  }

  if (NULL == p) { /* ˵��û���ҵ�section,��Ҫ��һ�� */
    Data *ps = (Data*)malloc(sizeof(Data));
    if (NULL == ps) {
      exit(-1); /* �����ڴ���� */
    }
    /* ��̬����section�ڴ� */
    ps->section = (char*)malloc(sizeof(char) * (strlen(section)+1));
    strcpy(ps->section, section);
    ps->option = NULL;    /* ��ʼ��optionҪΪ�� */
    ps->next = cnf->data; /* cnf->data����ΪNULL */
    cnf->data = p = ps;   /* ͷ�巨�������� */
  }

  Option *q = p->option;
  while (NULL != q && 0 != strcmp(q->key, key)) {
    q = q->next; /* ����option,���key�Ƿ��Ѿ����� */
  }

  if (NULL == q) { /* ������option,���½�һ�� */
    q = (Option*)malloc(sizeof(Option));
    if (NULL == q) {
      exit(-1); /* �����ڴ���� */
    }
    /* ��̬����key�ڴ� */
    q->key = (char*)malloc(sizeof(char) * (strlen(key)+1));
    strcpy(q->key, key);
    q->next = p->option; /*����p->option����ΪNULL,����Ҳû��ϵ */
    p->option = q; /* ͷ�巨�������� */
    /* ��̬����value�ڴ� */
    q->value = (char*)malloc(sizeof(char) * (strlen(value)+1));
  } else if (strlen(q->value) < strlen(value)) {
    /* ����ֵ����С�ھ�ֵ,���������ڴ� */
    q->value = (char*)realloc(q->value, sizeof(char) * (strlen(value)+1));
  }
  strcpy(q->value, value); /* �������Ҫ��ֵ���� */
  return true;
}

/**
* ���ղ���ȥ���ַ������ҿհ�
**/
char *trim_string(char *string,int mode)
{
  char *left = string;
  if ((mode & TRIM_LIFT) != 0) { // ȥ����߿հ��ַ�
    for (;*left != '\0'; left++) {
      if (0 == isspace(*left)) {
        break;
      }
    }
  }
  if ((mode & TRIM_RIGHT) != 0) { // ȥ���ұ߿հ��ַ�
    char *right = string - 1 + strlen(string);
    for (;right >= left; right--) {
      if (0 == isspace(*right)) {
        *(right+1) = '\0';
        break;
      }
    }
  }
  return left;
}

/**
* ���������ļ�·��
* �������ļ�·��,ע���ַ�,�ָ���
* ����Config�ṹ��
**/
Config *cnf_read_config(const char *filename, char comment, char separator)
{
  Config *cnf = (Config*)malloc(sizeof(Config));
  if (NULL == cnf) {
    exit(-1); /* �����ڴ���� */
  }
  cnf->comment   = comment; /* ÿһ�и��ַ����Ժ���ַ������� */
  cnf->separator = separator; /* �����ָ�Section �� ���� */
  cnf->data      = NULL; /* ��ʼ����Ϊ�� */

  if (str_empty(filename)) {
    return cnf; /* ���ַ�����ֱ�ӷ��ض��� */
  }

  FILE *fp = fopen(filename, "r");
  if(NULL == fp) {
    PRINT_ERRMSG("fopen");
    exit(errno); /* ���ļ�����ֱ�Ӱ��մ������˳� */
  }

  char *s, *e, *pLine, sLine[MAX_VALUE];    /* ����һ�����ݵ��ַ��� */
  char section[MAX_VALUE] = {'\0'}, key[MAX_VALUE], value[MAX_VALUE]; /* ����section,key,value */
  while (NULL != fgets(sLine, MAX_VALUE, fp)) {
    pLine = trim_string(sLine, TRIM_SPACE); /* ȥ��һ�����ߵĿհ��ַ� */
    if (*pLine == '\0' || *pLine == comment) {
      continue; /* ���л�ע�������� */
    }
    s = strchr(pLine, comment);
    if (s != NULL) {
      *s = '\0'; /* ���Ա���ע�ͺ���ַ� */
    }

    s = strchr(pLine, '[');
    if (s != NULL) {
      e = strchr(++s, ']');
      if (e != NULL) {
        *e = '\0'; /* �ҵ�section */
        strcpy(section, s);
      }
    } else {
      s = strchr(pLine, separator);
      if (s != NULL && *section != '\0') { /* �ҵ�����separator����,��ǰ�����Ѿ��ҵ�section */
        *s = '\0'; /* ���ָ���ǰ��ֳ�2���ַ��� */
        strcpy(key, trim_string(pLine, TRIM_RIGHT)); /* ��ֵkey */
        strcpy(value, trim_string(s+1, TRIM_LIFT));  /* ��ֵvalue */
        cnf_add_option(cnf, section, key, value);    /* ���section,key,value */
      }
    }
  } /* end while */
  fclose(fp);
  return cnf;
}

/**
* ��ȡָ�����͵�ֵ
* ���ݲ�ͬ���ͻḳֵ����Ӧֵ
* ��������Ҫע��,int��double��ת��,���������0
*     ��Ҫ�Լ�д����ʱ�жϺ�
**/
bool cnf_get_value(Config *cnf, const char *section, const char *key)
{
  Data *p = cnf->data; /* �ñ���pѭ������data,�ҵ���Ӧsection */
  while (NULL != p && 0 != strcmp(p->section, section)) {
    p = p->next;
  }

  if (NULL == p) {
    PRINT_ERRMSG("section not find!");
    return false;
  }

  Option *q = p->option;
  while (NULL != q && 0 != strcmp(q->key, key)) {
    q = q->next; /* ����option,���key�Ƿ��Ѿ����� */
  }

  if (NULL == q) {
    PRINT_ERRMSG("key not find!");
    return false;
  }

  cnf->re_string = q->value;              /* ��ֵ��� */
  cnf->re_int    = atoi(cnf->re_string);  /* ת��Ϊ���� */
  cnf->re_bool   = 0 == strcmp ("true", cnf->re_string); /* ת��Ϊbool�� */
  cnf->re_double = atof(cnf->re_string);  /* ת��Ϊdouble�� */
  return true;
}

/**
* �ж�section�Ƿ����
* �����ڷ��ؿ�ָ��
* �����򷵻ذ����Ǹ�section��Dataָ��
**/
Data *cnf_has_section(Config *cnf, const char *section)
{
  Data *p = cnf->data; /* �ñ���pѭ������data,�ҵ���Ӧsection */
  while (NULL != p && 0 != strcmp(p->section, section)) {
    p = p->next;
  }

  if (NULL == p) { /* û�ҵ��򲻴��� */
    return NULL;
  }

  return p;
}

/**
* �ж�ָ��option�Ƿ����
* �����ڷ��ؿ�ָ��
* �����򷵻ذ����Ǹ�section��key��Optionָ��
**/
Option *cnf_has_option(Config *cnf, const char *section, const char *key)
{
  Data *p = cnf_has_section(cnf, section);
  if (NULL == p) { /* û�ҵ��򲻴��� */
    return NULL;
  }

  Option *q = p->option;
  while (NULL != q && 0 != strcmp(q->key, key)) {
    q = q->next; /* ����option,���key�Ƿ��Ѿ����� */
  }
  if (NULL == q) { /* û�ҵ��򲻴��� */
    return NULL;
  }

  return q;
}

/**
* ��Config����д��ָ���ļ���
* header��ʾ���ļ���ͷ��һ��ע��
* д��ɹ��򷵻�true
**/
bool cnf_write_file(Config *cnf, const char *filename, const char *header)
{
  FILE *fp = fopen(filename, "w");
  if(NULL == fp) {
    PRINT_ERRMSG("fopen");
    exit(errno); /* ���ļ�����ֱ�Ӱ��մ������˳� */
  }

  if (!str_empty(header)) { /* �ļ�ע�Ͳ�Ϊ��,��дע�͵��ļ� */
    fprintf(fp, "%c %s\n\n", cnf->comment, header);
  }

  Option *q;
  Data   *p = cnf->data;
  while (NULL != p) {
    fprintf(fp, "[%s]\n", p->section);
    q = p->option;
    while (NULL != q) {
      fprintf(fp, "%s %c %s\n", q->key, cnf->separator, q->value);
      q = q->next;
    }
    p = p->next;
  }

  fclose(fp);
  return true;
}

/**
* ɾ��option
**/
bool cnf_remove_option(Config *cnf, const char *section, const char *key)
{
  Data *ps = cnf_has_section(cnf, section);
  if (NULL == ps) { /* û�ҵ��򲻴��� */
    return false;
  }

  Option *p, *q;
  q = p = ps->option;
  while (NULL != p && 0 != strcmp(p->key, key)) {
    if (p != q) { q = q->next; } /* ʼ����q����p����һ���ڵ� */
    p = p->next;
  }

  if (NULL == p) { /* û�ҵ��򲻴��� */
    return false;
  }

  if (p == q) { /* ��һ��option��ƥ���� */
    ps->option = p->next;
  } else {
    q->next = p->next;
  }

  free(p->key);
  free(p->value);
  free(p);
  q = p = NULL; // ����Ұָ��

  return true;
}

/**
* ɾ��section
**/
bool cnf_remove_section(Config *cnf, const char *section)
{
  if (str_empty(section)) {
    return false;
  }

  Data *p, *q;
  q = p = cnf->data; /* �ñ���pѭ������data,�ҵ���Ӧsection */
  while (NULL != p && 0 != strcmp(p->section, section)) {
    if (p != q) { q = q->next; } /* ʼ����q����p����һ���ڵ� */
    p = p->next;
  }

  if (NULL == p) { /* û���ҵ�section */
    return false;
  }

  if (p == q) { /* �����ʾ��һ��section,�������ͷλ�øı� */
    cnf->data = p->next;
  } else { /* ��ʱ���м��β���ڵ� */
    q->next = p->next;
  }

  Option *ot,*o = p->option;
  while (NULL != o) { /* ѭ���ͷ�����option */
    free(o->key);
    free(o->value);
    ot = o;
    o = o->next;
    free(ot);
  }
  free(p->option); /* �ͷ��ڴ� */
  free(p);         /* �ͷ�ɾ����section */
  q = p = NULL;  // ����Ұָ��

  return true;
}

/**
* ����Config����
* ɾ����������
**/
void destroy_config(Config **cnf)
{
  if (NULL == *cnf) return;

  if (NULL != (*cnf)->data)
  {
    Data *pt, *p = (*cnf)->data;
    Option *qt, *q;
    while (NULL != p) {
      q = p->option;
      while (NULL != q) {
        free(q->key);
        free(q->value); /* �ͷ��ڴ� */

        qt = q;
        q = q->next;
        free(qt);
      }
      free(p->section); /* �ͷ��ڴ� */

      pt = p;
      p = p->next;
      free(pt);
    }
  }
  free(*cnf);
  *cnf = NULL;
}

/**
* ��ӡ��ǰConfig����
**/
void print_config(Config *cnf)
{
  Data *p = cnf->data; // ѭ����ӡ���
  while (NULL != p) {
    printf("[%s]\n",p->section);

    Option *q = p->option;
    while (NULL != q) {
      printf("%s%c%s\n", q->key, cnf->separator, q->value);
      q = q->next;
    }
    p = p->next;
  }
}

/**
* ������,���������
* �����ظ�������������
**/
int main(int argc, char *argv[])
{
  // ��ȡ�����ļ�cnf.ini,ע���ַ�Ϊ#,�ָ���ֵ�ַ�Ϊ=
  Config *cnf = cnf_read_config("cnf.ini", '#', '=');
  if (NULL == cnf) {
    return -1; /* ��������ʧ�� */
  }

  printf("-------------- After Read File --------------\n");
  print_config(cnf); // ��ӡcnf����
  cnf_remove_section(cnf,"AAA"); // ɾ��AAA��section
  cnf_remove_option(cnf, "CC","df");  // ɾ��CC�µ�df
  cnf_remove_option(cnf, "CC","zxc");  // ɾ��CC�µ�df
  printf("-------------- After remove --------------\n");
  print_config(cnf); // ��ӡcnf����
  cnf_add_option(cnf, "NEW", "new_1", "true");  // ����NEW�µ�new_1��ֵ
  cnf_add_option(cnf, "NEW", "new_2", "abc"); // ����NEW�µ�new_2��ֵ
  cnf_add_option(cnf, "NEW", "new_2", "123456789"); // ����֮ǰ��ֵ
  cnf_add_option(cnf, "NEW1", "new_2", "true");
  printf("-------------- After add --------------\n");
  print_config(cnf); // ��ӡcnf����

  cnf_get_value(cnf, "NEW1", "new_2"); // ��ȡNEW1�µ�new_2ֵ
  printf("cnf_get_value:'%s','%d','%d','%f'\n",cnf->re_string,cnf->re_int,cnf->re_bool,cnf->re_double);

  cnf->separator = ':'; // ���ָ����ĳ� : ,ð��
  cnf_write_file(cnf, "cnf_new.ini", "write a new ini file!"); // ������д��cnf_new.ini�ļ�
  destroy_config(&cnf); // ����Config����
  return 0;
}
