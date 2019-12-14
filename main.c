#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define  MAX_VALUE  64 /* 定义section,key,value字符串最大长度 */
// printf("File = %s\nLine = %d\nFunc=%s\nDate=%s\nTime=%s\n", __FILE__, __LINE__, __FUNCTION__, __DATE__, __TIME__);
#define  PRINT_ERRMSG(STR) fprintf(stderr,"line:%d,msg:%s,eMsg:%s\n", __LINE__, STR, strerror(errno))

#define TRIM_LIFT  1 /* 去除左边空白字符 */
#define TRIM_RIGHT 2 /* 去除右边空白字符 */
#define TRIM_SPACE 3 /* 去除两边空白字符 */

typedef struct _option {
  char    key[MAX_VALUE];   /* 对应键 */
  char    value[MAX_VALUE]; /* 对应值 */
  struct  _option *next;    /* 链表连接标识 */
}Option;

typedef struct _data {
  char    section[MAX_VALUE]; /* 保存section值          */
  Option  *option;            /* option链表头           */
  struct  _data *next;        /* 链表连接标识           */
}Data;

typedef struct {
  char    comment;              /* 表示注释的符号    */
  char    separator;            /* 表示分隔符        */
  char    re_string[MAX_VALUE]; /* 返回值字符串的值  */
  int     re_int;               /* 返回int的值       */
  bool    re_bool;              /* 返回bool的值      */
  double  re_double ;           /* 返回double类型    */
  Data    *data;                /* 保存数据的头      */
}Config;

/**
* 判断字符串是否为空
* 为空返回true,不为空返回false
**/
bool str_empty(const char *string)
{
  return NULL == string || '\0' == *string;
}

/**
* 向链表添加section,key,value
* 如果添加时不存在section则新增一个
* 如果对应section的key不存在则新增一个
* 如果section已存在则不会重复创建
* 如果对应section的key已存在则只会覆盖key的值
**/
bool cnf_add_option(Config *cnf, const char *section, const char *key, const char *value)
{
  if (NULL == cnf || str_empty(section) || str_empty(key) || str_empty(value)) {
    return false; /* 参数不正确,返回false */
  }
  
  Data *p = cnf->data; /* 让变量p循环遍历data,找到对应section */
  while (NULL != p && 0 != strcmp(p->section, section)) {
    p = p->next;
  }
  
  if (NULL == p) { /* 说明没有找到section,需要加一个 */
    Data *ps = (Data*)malloc(sizeof(Data));
    if (NULL == ps) {
      exit(-1); /* 申请内存错误 */
    }
    strcpy(ps->section, section);
    ps->option = NULL;    /* 初始的option要为空 */
    ps->next = cnf->data; /* cnf->data可能为NULL */
    cnf->data = p = ps;   /* 头插法插入链表 */
  }
  
  Option *q = p->option;
  while (NULL != q && 0 != strcmp(q->key, key)) {
    q = q->next; /* 遍历option,检查key是否已经存在 */
  }
  
  if (NULL == q) { /* 不存在option,则新建一个 */
    q = (Option*)malloc(sizeof(Option));
    if (NULL == q) {
      exit(-1); /* 申请内存错误 */
    }
    strcpy(q->key, key);
    q->next = p->option; /*这里p->option可能为NULL,不过也没关系 */
    p->option = q; /* 头插法插入链表 */
  }
  strcpy(q->value, value); /* 无论如何要把值改了 */
  
  return true;
}

/**
* 按照参数去除字符串左右空白
**/
char *trim_string(char *string,int mode)
{
  char *left = string;
  if ((mode & 1) != 0) { // 去除左边空白字符
    for (;*left != '\0'; left++) {
      if (0 == isspace(*left)) {
        break;
      }
    }
  }
  if ((mode & 2) != 0) { // 去除右边空白字符
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
* 传递配置文件路径
* 参数有文件路径,注释字符,分隔符
* 返回Config结构体
**/
Config *cnf_read_config(const char *filename, char comment, char separator)
{
  Config *cnf = (Config*)malloc(sizeof(Config));
  if (NULL == cnf) {
    exit(-1); /* 申请内存错误 */
  }
  cnf->comment   = comment; /* 每一行该字符及以后的字符将丢弃 */
  cnf->separator = separator; /* 用来分隔Section 和 数据 */
  cnf->data      = NULL; /* 初始数据为空 */

  if (str_empty(filename)) {
    return cnf; /* 空字符串则直接返回对象 */
  }

  FILE *fp = fopen(filename, "r");
  if(NULL == fp) {
    PRINT_ERRMSG("fopen");
    exit(errno); /* 读文件错误直接按照错误码退出 */
  }

  char *s, *e, *pLine, sLine[MAX_VALUE];    /* 保存一行数据到字符串 */
  char section[MAX_VALUE] = {'\0'}, key[MAX_VALUE], value[MAX_VALUE]; /* 缓存section,key,value */
  while (NULL != fgets(sLine, MAX_VALUE, fp)) {
    pLine = trim_string(sLine, TRIM_SPACE); /* 去掉一行两边的空白字符 */
    if (*pLine == '\0' || *pLine == comment) {
      continue; /* 空行或注释行跳过 */
    }
    s = strchr(pLine, comment);
    if (s != NULL) {
      *s = '\0'; /* 忽略本行注释后的字符 */
    }

    s = strchr(pLine, '[');
    if (s != NULL) {
      e = strchr(++s, ']');
      if (e != NULL) {
        *e = '\0'; /* 找到section */
        strcpy(section, s);
      }
    } else {
      s = strchr(pLine, separator);
      if (s != NULL && *section != '\0') { /* 找到包含separator的行,且前面行已经找到section */
        *s = '\0'; /* 将分隔符前后分成2个字符串 */
        strcpy(key, trim_string(pLine, TRIM_RIGHT)); /* 赋值key */
        strcpy(value, trim_string(s+1, TRIM_LIFT));  /* 赋值value */
        cnf_add_option(cnf, section, key, value);    /* 添加section,key,value */
      }
    }
  } /* end while */
  fclose(fp);
  return cnf;
}

/**
* 获取指定类型的值
* 根据不同类型会赋值给对应值
* 本方法需要注意,int和double的转换,不满足就是0
*     需要自己写代码时判断好
**/
bool cnf_get_value(Config *cnf, const char *section, const char *key)
{
  Data *p = cnf->data; /* 让变量p循环遍历data,找到对应section */
  while (NULL != p && 0 != strcmp(p->section, section)) {
    p = p->next;
  }
  
  if (NULL == p) {
    PRINT_ERRMSG("section not find!");
    return false;
  }
  
  Option *q = p->option;
  while (NULL != q && 0 != strcmp(q->key, key)) {
    q = q->next; /* 遍历option,检查key是否已经存在 */
  }
  
  if (NULL == q) {
    PRINT_ERRMSG("key not find!");
    return false;
  }
  
  strcpy(cnf->re_string, q->value);       /* 将结果字符串赋值 */
  cnf->re_int    = atoi(cnf->re_string);  /* 转换为整形 */
  cnf->re_bool   = 0 == strcmp ("true", cnf->re_string); /* 转换为bool型 */
  cnf->re_double = atof(cnf->re_string);  /* 转换为double型 */
  
  return true;
}

/**
* 判断section是否存在
* 不存在返回空指针
* 存在则返回包含那个section的Data指针
**/
Data *cnf_has_section(Config *cnf, const char *section)
{
  Data *p = cnf->data; /* 让变量p循环遍历data,找到对应section */
  while (NULL != p && 0 != strcmp(p->section, section)) {
    p = p->next;
  }
  
  if (NULL == p) { /* 没找到则不存在 */
    return NULL;
  }
  
  return p;
}

/**
* 判断指定option是否存在
* 不存在返回空指针
* 存在则返回包含那个section下key的Option指针
**/
Option *cnf_has_option(Config *cnf, const char *section, const char *key)
{
  Data *p = cnf_has_section(cnf, section);
  if (NULL == p) { /* 没找到则不存在 */
    return NULL;
  }
  
  Option *q = p->option;
  while (NULL != q && 0 != strcmp(q->key, key)) {
    q = q->next; /* 遍历option,检查key是否已经存在 */
  }
  if (NULL == q) { /* 没找到则不存在 */
    return NULL;
  }
  
  return q;
}

/**
* 将Config对象写入指定文件中
* header表示在文件开头加一句注释
* 写入成功则返回true
**/
bool cnf_write_file(Config *cnf, const char *filename, const char *header)
{
  FILE *fp = fopen(filename, "w");
  if(NULL == fp) {
    PRINT_ERRMSG("fopen");
    exit(errno); /* 读文件错误直接按照错误码退出 */
  }
  
  if (!str_empty(header)) { /* 文件注释不为空,则写注释到文件 */
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
* 删除option
**/
bool cnf_remove_option(Config *cnf, const char *section, const char *key)
{
  Data *ps = cnf_has_section(cnf, section);
  if (NULL == ps) { /* 没找到则不存在 */
    return false;
  }
  
  Option *p, *q;
  q = p = ps->option;
  while (NULL != p && 0 != strcmp(p->key, key)) {
    if (p != q) { q = q->next; } /* 始终让q处于p的上一个节点 */
    p = p->next;
  }
  
  if (NULL == p) { /* 没找到则不存在 */
    return false;
  }
  
  if (p == q) { /* 第一个option就匹配了 */
    ps->option = p->next;
  } else {
    q->next = p->next;
  }
  
  free(p);
  q = p = NULL; // 避免野指针
  
  return true;
}

/**
* 删除section
**/
bool cnf_remove_section(Config *cnf, const char *section)
{
  if (str_empty(section)) {
    return false;
  }
  
  Data *p, *q;
  q = p = cnf->data; /* 让变量p循环遍历data,找到对应section */
  while (NULL != p && 0 != strcmp(p->section, section)) {
    if (p != q) { q = q->next; } /* 始终让q处于p的上一个节点 */
    p = p->next;
  }
  
  if (NULL == p) { /* 没有找到section */
    return false;
  }
  
  if (p == q) { /* 这里表示第一个section,因此链表头位置改变 */
    cnf->data = p->next;
  } else { /* 此时是中间或尾部节点 */
    q->next = p->next;
  }
  
  Option *ot,*o = p->option;
  while (NULL != o) { /* 循环释放所有option */
    ot = o;
    o = o->next;
    free(ot);
  }
  p->option = NULL; // 避免野指针
  free(p); /* 释放删除的section */
  q = p = NULL;  // 避免野指针
  
  return true;
}

/**
* 销毁Config对象
* 删除所有数据
**/
void destroy_config(Config **cnf)
{
  if (NULL != *cnf)
  {
    if (NULL != (*cnf)->data)
    {
      Data *pt,*p = (*cnf)->data;
      Option *qt,*q;
      while (NULL != p) {
        q = p->option;
        while (NULL != q) {
          qt = q;
          q = q->next;
          free(qt);
        }
        pt = p;
        p = p->next;
        free(pt);
      }
    }
    free(*cnf);
    *cnf = NULL;
  }
}

/**
* 打印当前Config对象
**/
void print_config(Config *cnf)
{
  Data *p = cnf->data; // 循环打印结果
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
* 主程序,放在最底下
* 避免重复声明其他函数
**/
int main(int argc, char *argv[])
{
  // 读取配置文件cnf.ini,注释字符为#,分隔键值字符为=
  Config *cnf = cnf_read_config("cnf.ini", '#', '=');
  if (NULL == cnf) {
    return -1; /* 创建对象失败 */
  }

  printf("-------------- After Read File --------------\n");
  print_config(cnf); // 打印cnf对象
  cnf_remove_section(cnf,"AAA"); // 删除AAA的section
  cnf_remove_option(cnf, "CC","df");  // 删除CC下的df
  printf("-------------- After remove --------------\n");
  print_config(cnf); // 打印cnf对象  
  cnf_add_option(cnf, "NEW", "new_1", "true"); // 新增NEW下的new_1的值
  cnf_add_option(cnf, "NEW", "new_2", "asdas"); // 新增NEW下的new_2的值
  cnf_add_option(cnf, "NEW1", "new_2", "true");
  printf("-------------- After add --------------\n");
  print_config(cnf); // 打印cnf对象  

  cnf_get_value(cnf, "NEW1", "new_2"); // 获取NEW1下的new_2值
  printf("cnf_get_value:%s,%d,%d,%f\n",cnf->re_string,cnf->re_int,cnf->re_bool,cnf->re_double);

  cnf->separator = ':'; // 将分隔符改成 : ,冒号
  cnf_write_file(cnf, "cnf_new.ini", "write a new ini file!"); // 将对象写入cnf_new.ini文件
  destroy_config(&cnf); // 销毁Config对象
  return 0;
}
