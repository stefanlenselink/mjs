#include "lastfm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <openssl/md5.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

typedef struct {
  int succes;
  int badsession;
  char * failure;
} LastFMSubmitResult;

static void *myrealloc(void *ptr, size_t size)
{
  /* There might be a realloc() out there that doesn't like reallocing
  NULL pointers, so we take care of it here */
  if(ptr)
    return realloc(ptr, size);
  else
    return malloc(size);
}

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)data;

  mem->memory = (char *)myrealloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory) {
    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
  }
  return realsize;
}

static struct MemoryStruct * curlSend(char * url, int post, char * postfields)
{
  fprintf(stderr,"1\n");
  CURL *curl_handle;
  fprintf(stderr,"2\n");
  struct MemoryStruct * chunk;
  fprintf(stderr,"3\n");
  chunk = malloc(sizeof(struct MemoryStruct));
  fprintf(stderr,"4\n");
  chunk->memory=NULL; /* we expect realloc(NULL, size) to work */
  fprintf(stderr,"5\n");
  chunk->size = 0;    /* no data at this point */
  fprintf(stderr,"6\n");

  curl_global_init(CURL_GLOBAL_ALL);
  fprintf(stderr,"7\n");

  /* init the curl session */
  curl_handle = curl_easy_init();
  fprintf(stderr,"8\n");

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  fprintf(stderr,"9\n");

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  fprintf(stderr,"10\n");

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
  fprintf(stderr,"11\n");

  curl_easy_setopt(curl_handle, CURLOPT_POST, post);
  fprintf(stderr,"11b\n");
  
  if(post){
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postfields);
    fprintf(stderr,"11c\n");
  }
  /* some servers don't like requests that are made without a user-agent
  field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  fprintf(stderr,"12\n");

  /* get it! */
  curl_easy_perform(curl_handle);
fprintf(stderr,"%s\n", chunk->memory);
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);
  fprintf(stderr,"13\n");
  
  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();
  fprintf(stderr,"14\n");
  
  return chunk;
}

static parseHandshake(LastFMHandshake * shake, struct MemoryStruct * data)
{
  shake->succes = (data != NULL && data->memory[0] == 'O');
  shake->baned = (data != NULL && data->memory[2] == 'N');
  shake->badauth = (data != NULL && data->memory[3] == 'A');
  shake->badtime = (data != NULL && data->memory[3] == 'T');
  
  shake->last_submit = malloc(sizeof(struct timeval));
  gettimeofday(shake->last_submit, NULL);
  //shake->username = NULL; //Must be set later!
  shake->session_id = NULL;
  shake->submit_url = NULL;
  shake->now_playing_url = NULL;
  shake->failure = NULL;
  
  if(shake->succes){
    int i, j = 0, start = 0;
    for(i = 0; j < 4; i++)
    {
      if(data->memory[i] && data->memory[i] == '\n')
      {
        char * buf = malloc(sizeof(char) * 1024);
        j++;
        strncpy(buf, data->memory+start, (i - start) + 1);
        buf[(i - start)] = '\0';
        data->memory-start;
        if(j == 2){
          shake->session_id = buf;
        }else if(j == 3){
          shake->now_playing_url = buf;
        }else if(j == 4){
          shake->submit_url = buf;
        }else{
          free(buf);
        }
        start = (i + 1);
      }
    }
  }else if(!shake->badauth && !shake->baned && !shake->badtime){
    //Only when failure and not baduser
    //TODO
  }
}

static void parseSubmit(LastFMSubmitResult * result, struct MemoryStruct * data)
{
  result->succes = (data != NULL && data->memory[0] == 'O');
  result->badsession = (data != NULL && data->memory[0] == 'B');
  if(!result->succes && !result->badsession){
    //TODO retrieve the reason
  }
}

void initLastFM(LastFMHandshake * handshake, char * username, char * passwd)
{
  struct MemoryStruct * chunk;
  char url[1024];
  char current_time[256];
  char authtok[256];
  char buf[128];
  char buf2[128];
  memset(buf, 0, 128);
  memset(buf2, 0, 128);
  time_t t;
  struct tm *tmp;
  t = time(NULL);
  tmp = localtime(&t);
  if (tmp == NULL) {
    perror("localtime");
    exit(EXIT_FAILURE);
  }
  if (strftime(current_time, sizeof(current_time), "%s", tmp) == 0) {
    fprintf(stderr, "strftime returned 0");
    exit(EXIT_FAILURE);
  }
    MD5_CTX    ctx;

    unsigned char final[MD5_DIGEST_LENGTH];
    MD5_Init(&ctx);
    MD5_Update(&ctx, passwd, strlen(passwd));
    MD5_Final(final, &ctx);
    int i;
    for(i =0; i<MD5_DIGEST_LENGTH; i++)
    {
      sprintf(buf, "%s%.2x", buf, final[i] );
    }
  //buf = MD5(passwd, strlen(passwd), buf);
  sprintf(authtok, "%s%s", buf, current_time);
  MD5_CTX    ctx2;
  unsigned char final2[MD5_DIGEST_LENGTH];
  MD5_Init(&ctx2);
  MD5_Update(&ctx2, authtok, strlen(authtok));
  MD5_Final(final2, &ctx2);
  for(i =0; i<MD5_DIGEST_LENGTH; i++)
  {
    sprintf(buf2, "%s%.2x", buf2, final2[i] );
  }
  sprintf(url, "http://post.audioscrobbler.com/?hs=true&p=1.2&c=tst&v=1.0&u=%s&t=%s&a=%s", username, current_time, buf2);
  chunk = curlSend(url, 0, NULL);
  parseHandshake(handshake, chunk);
  handshake->username = strdup(username);
  handshake->passwd = strdup(passwd);
  if(chunk->memory)
    free(chunk->memory);
  free(chunk);
}

static void goLastFM(LastFMHandshake * handshake, char * url, char * postfields)
{
  if(!fork()){
    struct MemoryStruct * chunk;
    LastFMSubmitResult * result;
    
    fprintf(stderr,"%s\n", url);
    
    chunk = curlSend(url, 1, postfields);
    fprintf(stderr,"a\n");
    result = malloc(sizeof(LastFMSubmitResult));
    fprintf(stderr,"b\n");
    parseSubmit(result, chunk);
    fprintf(stderr,"c\n");
    if(chunk->memory)
      free(chunk->memory);
    fprintf(stderr,"d\n");
    free(chunk);
    fprintf(stderr,"e\n");
    if(result->badsession)
    {
      fprintf(stderr,"f\n");
      fprintf(stderr,"Badsession\n");
      exit(1);
      char * cp_username = strdup(handshake->username);
      char * cp_passwd = strdup(handshake->passwd);
      deleteLastFMHandshake(handshake, 0);
      initLastFM(handshake, cp_username, cp_passwd);
      free(cp_username);
      free(cp_passwd);
    }else{
      fprintf(stderr,"g\n");
      if(handshake->last_submit){
        free(handshake->last_submit);
      }
      fprintf(stderr,"h\n");
      handshake->last_submit = malloc(sizeof(struct timeval));
      gettimeofday(handshake->last_submit, NULL);
      fprintf(stderr,"i\n");
    }
    fprintf(stderr,"j\n");
    fprintf(stderr,"k\n");
    free(result);
    fprintf(stderr,"l\n");
  }//The Parent thread immediatly finishes
}

static void urlencode(unsigned char *s, unsigned char *t) {
  unsigned char *p, *tp;
  if(t == NULL) {
    fprintf(stderr, "Serious memory error...\n");
    exit(1);
  }
  tp=t;
  for(p=s; *p; p++) {
    if((*p > 0x00 && *p < ',') ||
         (*p > '9' && *p < 'A') ||
         (*p > 'Z' && *p < '_') ||
         (*p > '_' && *p < 'a') ||
         (*p > 'z' && *p < 0xA1)) {
      sprintf(tp, "%%%02X", *p);
      tp += 3;
         } else {
           *tp = *p;
           tp++;
         }
  }
  *tp='\0';
  return;
}

void currentPlayingLastFM(LastFMHandshake * handshake, char * name, char * title, char * album, int length, int trackid)
{
  if(handshake == NULL)
  {
    return;
  }
  char postfields[4096];
  char postfields2[4096];
  sprintf(postfields, "s=%s&a=%s&t=%s&b=%s&l=%d&n=%d&m=", handshake->session_id, name, title, album, length, trackid);
  fprintf(stderr,"%s\n", postfields);
  //urlencode(postfields, postfields2);
  //printf("%s\n", postfields2);
  goLastFM(handshake, handshake->now_playing_url, postfields);
}

void submitLastFM(LastFMHandshake * handshake, char * name, char * title, char * album, int length, int trackid)
{
  if(handshake == NULL)
  {
    return;
  }
  char url[4096];
  char current_time[200];
  time_t t;
  struct tm *tmp;
  struct timeval now;
  gettimeofday(&now, NULL);
    
  t = time(NULL);
  tmp = localtime(&t);
  if (tmp == NULL) {
    perror("localtime");
    exit(EXIT_FAILURE);
  }
  if (strftime(current_time, sizeof(current_time), "%s", tmp) == 0) {
    fprintf(stderr, "strftime returned 0");
    exit(EXIT_FAILURE);
  }
  
  sprintf(url,  "s=%s&a[0]=%s&t[0]=%s&i[0]=%d&o[0]=P&r[0]=&l[0]=%d&b[0]=%s&n[0]=%d&m[0]=", handshake->session_id, name, title, atoi(current_time) - length,  length, album, trackid);
  goLastFM(handshake, handshake->submit_url, url);
}



void deleteLastFMHandshake(LastFMHandshake * ptr, int full_delete)
{
  if(ptr->username){
    free(ptr->username);
  }
  if(ptr->passwd){
    free(ptr->passwd);
  }
  if(ptr->session_id){
    free(ptr->session_id);
  }
  if(ptr->submit_url){
    free(ptr->submit_url);
  }
  if(ptr->failure){
    free(ptr->failure);
  }
  if(ptr->last_submit)
  {
    free(ptr->last_submit);
  }
  if(full_delete){
    free(ptr);
  }
}
