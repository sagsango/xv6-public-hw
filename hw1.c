#include"user.h"
char*
fdgets(int fd, char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(fd, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}


int test(char* num, char *denom, char *dividend) {
//    printf(1,"Trying: div %s %s\n",num,denom);
    int fds[2];
    pipe(fds);
    int pid=fork();
    if(pid==0) {
        dup2(fds[1],1);
        char *argv[]={"divide",num,denom,0};
        close(fds[0]);
        exec("divide",argv);
    }
    else {
        close(fds[1]);
        char result[100] = {0};
        fdgets(fds[0],result,100);
        wait();
        if(strlen(result)>0)
            result[strlen(result)-1]=0;
        if(strcmp(result,dividend)) {
            printf(1,"Got: %s\n",result);
            printf(1,"Expected: %s\n", dividend);
            exit();
        }
    }
    return 0;
}

int main() {
    test("20","10","2");
    test("-20","10","-2");
    test("-2","10","-0.2");
    test("-22","100","-0.22");
    test("6663","100","66.63");
    test("10","0","NaN");
    test("a","b","NaN");
    test("0","20","0");
    printf(1,"All tests passed.\n");
    exit();
}