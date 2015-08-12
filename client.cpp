#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <stdlib.h>
#include <locale.h>
#include <ctype.h>
#define WAITTIME 2

//todo: multilogin response, .h file, check c^u and f
pthread_mutex_t mutex;

int fd;
enum Ctrl{err, tag, para, ctag, mid, start};
enum Action{POST, WATER, MAIL};
char s[][15] = {"ID", "P", "W", "M", "TITLE", "PASS", "EXIT", "BOARD",  "CONTENT"};//total 9 tags
char newmail[] = {0xb7, 0x73, 0xab, 0x48, 0xa5, 0xf3, 0x00};
char mainc[] = {0xba, 0xeb, 0xb5, 0xd8, 0xa4, 0xbd, 0xa7, 0x47,0xc4, 0xe6, 0x00};
char friendlist[] = {0xA6, 0x6E, 0xA4, 0xCD, 0xA6, 0x43, 0xAA, 0xED, 0x00};
char cl[] = "\r";

void handle_error(const char s[]){
  perror(s);
}

void logout(){
  printf("logout\n");
  if(close(fd) == -1)//just for test
    handle_error("Logout");
}

FILE *f;
bool is_newmail = false;
bool is_readmail = false;
char mailnums[10] = "";
void read_mail(){
  puts("read mail");
  //return to main
  char back[] = "\EOD";
  for(int i = 0; i < 5; i++){
    write(fd, back, strlen(back));
  }
  //goto mail list
  char m[] = "m\EOC";
  write(fd, m, strlen(m));
  char r[] = "r\EOC";
  write(fd, r, strlen(r));
  sleep(WAITTIME);

  puts("first page");
  char home[] = "\E[1~";//not sure
  is_readmail = true;
  write(fd, home, strlen(home));
  sleep(WAITTIME);

  int page = 2;
  while(page <= 30){
    if(pthread_mutex_trylock(&mutex) == 0){
      printf("%d page", page++);
      char pgdown[] = "\E[6~";//not sur
      write(fd, pgdown, strlen(pgdown));
      sleep(WAITTIME);
      pthread_mutex_unlock(&mutex);
    }
  }
}

void read_a_mail(){
  write(fd, mailnums, strlen(mailnums));
  sleep(WAITTIME);
  char go[] = "\EOC";

  write(fd, cl, sizeof(cl));
  sleep(WAITTIME);
  write(fd, go, sizeof(go));
  char rs;// 1 or 2
  printf("read a mail\n");
  sleep(WAITTIME);
  char pre = 0x00;
  while(read(fd, &rs, sizeof(rs))){
    printf("%c", rs);
    if(pre == '-' && rs == '-'){//looked as the end of mail
      break;
    }
    pre = rs;
  }
  char back[] = "\EOD";
  write(fd, back, sizeof(back));
  sleep(WAITTIME);
  return;
}

int readt = 0;
bool is_friend = false;
void* readit(void* arg){
  char rs[30000];
  while(read(fd, rs, sizeof(rs)) > 0){//when readmail , break
    //printf("read something\n");
    //      printf("%s", rs);
    char *p;
    if(!is_newmail && (p = strstr(rs, newmail)) != NULL){
      //puts("find word [new mail]");
      is_newmail = true;
    }
    if((p = strstr(rs, friendlist)) != NULL){
      printf("is friend\n");
      is_friend = true;
    }
    
    if(is_readmail){
      while(pthread_mutex_trylock(&mutex) != 0){
	
      }
      p = rs;
      while((p = strstr(p, "+")) != NULL){
	if(*(p+1) != ' ' || *(p-1) != ' '){
	  p = p+1;
	  continue;
	}
	//puts("find +");
	int numlen = 1;
	while(*(p-numlen) == ' ' || isdigit(*(p-numlen))){
	  numlen++;
	}
	strncpy(mailnums, p-numlen+1, numlen-2);
	printf("mail num len=%d :%s\n", strlen(mailnums), mailnums);
	read_a_mail();
	read(fd, rs, sizeof(rs)); // read some gargabe
	p = p+1;
      }
      pthread_mutex_unlock(&mutex);
    }
  }
  return NULL;
}

void build_connection(){
  char sitename[] = "ptt2.cc";
  struct hostent *hp;
  struct sockaddr_in srv;
  srv.sin_family = AF_INET;
  srv.sin_port = htons(23); // bind socketfd to port 23
  if((hp = gethostbyname(sitename)) != NULL){
    srv.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr))->s_addr;
    printf("Translate %s => %s\n", sitename, inet_ntoa(srv.sin_addr));
  }
  if((fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    handle_error("Error: ");//handle error()
  if(connect(fd, (struct sockaddr*)&srv, sizeof(srv)) < 0)
    handle_error("connect");
}

void open_read_thread(){
  pthread_t thread;
  //second argument attr can be emitted
  if(pthread_create(&thread, NULL, readit, NULL) == -1)
    handle_error("thread");
}

bool login(char in[][1000]){
  build_connection();
  open_read_thread();

  char id[1000], pass[1000];
  strcpy(id, in[0]);
  strcpy(pass, in[1]);
  char skipad[] = "y";
  char arrow[] = "a\EOC";

  printf("id = %s pass = %s\n", id, pass);
  write(fd, id, strlen(id));
  write(fd, cl, sizeof(cl));
  sleep(WAITTIME);
  write(fd, pass, strlen(pass));
  write(fd, cl, sizeof(cl));
  sleep(WAITTIME);
  write(fd, skipad, sizeof(skipad));
  sleep(WAITTIME);

  return true;
}

bool board(char in[][1000]){
  printf("board %s\n", in[0]);
  char board[] = "s";
  write(fd, board, sizeof(board));
  sleep(WAITTIME);
  write(fd, in[0], strlen(in[0]));
  write(fd, cl, sizeof(cl));
  sleep(WAITTIME);
  return true;
}

bool post(char in[][1000]){
  puts("post!");
  char post[] = "\020";
  write(fd, post, sizeof(post));
  write(fd, cl, sizeof(cl));
  sleep(WAITTIME);
  write(fd, in[0], strlen(in[0]));//title
  write(fd, cl, sizeof(cl));
  //content
  write(fd, in[1], strlen(in[1]));//does content include \n? seems like it include \r
  char afterpost[] = "\030s\r\r";
  write(fd, afterpost, sizeof(afterpost));
  return true;
}

//https://www.ptt.cc/bbs/B96310XXX/M.1187186138.A.15C.html
bool water(char in[][1000]){
  is_friend = false;
  char userlist[] = "\025";
  write(fd, userlist, sizeof(userlist));
  sleep(WAITTIME);

  //if there is friendlist, change it
  if(is_friend){
    char f[] = "f";
    write(fd, f, sizeof(f));
  }

  char fs[] = "s";//should f?? check f or not f
  write(fd, fs, sizeof(fs));
  sleep(WAITTIME);
  write(fd, in[0], strlen(in[0]));
  write(fd, cl, sizeof(cl));
  char w[] = "w";
  write(fd, w, sizeof(w));
  sleep(WAITTIME);
  write(fd, in[1], strlen(in[1]));
  write(fd, cl, sizeof(cl));
  char y[] = "y";
  write(fd, y, sizeof(y));
  write(fd, cl, sizeof(cl));
  return true;
}

bool mail(char in[][1000]){
  char go[] = "\EOD";
  for(int i = 0; i < 10; i++)//back to main
    write(fd, go, sizeof(go));
  
  char m[] = "m\EOC";
  write(fd, m, sizeof(m));
  sleep(WAITTIME);
  char s[] = "s\EOC";
  write(fd, s, sizeof(s));
  write(fd, in[0], strlen(in[0]));
  write(fd, cl, sizeof(cl));
  write(fd, in[1], strlen(in[1]));
  write(fd, cl, sizeof(cl));
  write(fd, in[2], strlen(in[2]));
  write(fd, cl, sizeof(cl));
  char afterpost[] = "\030s\r\r";
  write(fd, afterpost, sizeof(afterpost));
  return true;
}

Action actnum;

bool content(char in[][1000]){
  puts("content");
	
  bool ret;
  switch(actnum){
  case POST:
    ret = post(in);
    break;
  case WATER:
    ret = water(in);
    break;
  case MAIL:
    ret = mail(in);
    break;
  }
  return ret;
}

/*
  possible combination
  id - pass
  p - content
  w -content
  m - title - content
 */
char* t[3];
char tags[3][1000];
bool (*funcarr[10])(char[][1000]);
//http://stackoverflow.com/questions/252748/how-can-i-use-an-array-of-function-pointers
int saved = 0;
bool check_tag(){
  if(strcmp(t[0], t[2])){
    fprintf(stderr, "parse arg failed\n");
    return false;
  }
  for(int i = 0; i < 9; i++){
    if(strcmp(s[i], t[0]) == 0){// should not include logout
      if(i <= 4){//that means, input set is not yet complete
	strcpy(tags[saved++], t[1]);
	switch(i){
	case 1:
	  actnum = POST;
	  break;
	case 2:
	  actnum = WATER;
	  break;
	case 3:
	  actnum = MAIL;
	  break;
	}
      } else {
	strcpy(tags[saved++], t[1]);
	saved = 0;
	if(!funcarr[i](tags))
	  fprintf(stderr, "%s failed\n", s[i]);	
      }
    }
  }
  return true;
}

int main(){
  pthread_mutex_init(&mutex, NULL);
  //read all
  char ins[30000];
  /*FILE* infile = fopen("input.txt", "w");
  while(gets(ins) != NULL){
    fprintf(infile, "%s", ins);
  }
  fclose(infile);*/
  
  system("iconv -f UTF-8 -t BIG5 input.txt > inbig5.txt");
  //take inbig5 as input
  freopen("inbig5.txt", "r", stdin);
  t[0] = (char*)malloc(1000);
  t[1] = (char*)malloc(1000);
  t[2] = (char*)malloc(1000);

  f = fopen("readin", "w");
  char in;
  funcarr[5] = login;
  funcarr[7] = board;
  funcarr[8] = content;//not so good for p, m, w
  Ctrl cnum = start;
  std::string s;
  while((in = getchar()) != EOF){// start -> tag -> para -> mid -> ctag -> start // with error state
    if(in == '\n'){
      if(cnum == tag || cnum == para || cnum == ctag)
	s.push_back('\r');
      continue;
    }
    switch(cnum){
    case start:
      cnum = (in == '<')?tag:err;
      break;
    case tag:
      if(in == '>'){
	cnum = para;
	strcpy(t[0], s.c_str());
	if(strcmp(t[0], "EXIT") == 0){
	  printf("exit!\n");
	  if(is_newmail)
	    read_mail();
	  logout();
	  cnum = start;
	  //before exit, read mail
	}
	s.clear();
      } else if(in == '<' || in == '/')
	cnum = err;
      else
	s.push_back(in);
      break;
    case para:
      if(in == '<'){
	cnum = mid;
	strcpy(t[1], s.c_str());
	s.clear();
      } else if(in == '>' || in == '/')
	cnum = err;
      else
	s.push_back(in);      
      break;
    case mid:
      cnum = (in == '/')?ctag:err;
      break;
    case ctag:
      if(in == '>'){
	cnum = start;
	strcpy(t[2], s.c_str());
	s.clear();
	for(int i = 0; i < 3; i++){
	  printf("in[%d] = %s\n", i, t[i]);
	}
	if(!check_tag()){
	  fprintf(stderr, "check tag error\n");
	}
      } else if(in == '<' || in == '/')
	cnum = err;
      else
	s.push_back(in);            
      break;
    case err:
      fprintf(stderr, "state error\n");
      break;
    }
  }
  //thread cancel

  return 0;
}
