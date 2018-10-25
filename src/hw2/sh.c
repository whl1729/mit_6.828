#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

// Simplifed xv6 shell.

#define MAXARGS 10
#define MAX_CMD_LEN 10
#define CMD_NUM 6
#define BUF_LEN 512
#define LINE_NUM 1000
#define LINE_LEN 100
#define TRUE   1
#define FALSE  0
#define ERROR    (-1)

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

typedef void (*efunc)(struct execcmd*);

struct redircmd {
  int type;          // < or > 
  struct cmd *cmd;   // the command to be run (e.g., an execcmd)
  char *file;        // the input/output file
  int flags;         // flags for open() indicating read or write
  int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

struct ecmdfunc {
  char cmd[MAX_CMD_LEN];
  efunc func;
};

struct cntinfo {
    int nline;
    int nword;
    int nbyte;
};

int fork1(void);  // Fork but exits on failure.
struct cmd *parsecmd(char*);
void cat(struct execcmd * cmd);
void grep(struct execcmd * cmd);
void ls(struct execcmd * cmd);
void sort(struct execcmd * cmd);
void uniq(struct execcmd * cmd);
void wc(struct execcmd * cmd);

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

struct ecmdfunc g_func_tbl[CMD_NUM] = {
    {"cat", cat},
    {"grep", grep},
    {"ls", ls},
    {"sort", sort},
    {"uniq", uniq},
    {"wc", wc}
};

/* todo: support reading from stdin */
void cat(struct execcmd *ecmd)
{
    int fd;
    int pos = 0;
    int num;
    char buf[BUF_LEN];

    while (ecmd->argv[++pos])
    {
        fd = open(ecmd->argv[pos], O_RDONLY);
        if (fd < 0)
        {
            fprintf(stderr, "cannot open %s!\r\n", ecmd->argv[pos]);
            continue;
        }

        while (1)
        {
            memset(buf, 0, BUF_LEN);

            num = read(fd, buf, BUF_LEN);
            if (num == 0)
            {
                break;
            }
            else if (num < 0)
            {
                fprintf(stderr, "failed to read %s!\r\n", ecmd->argv[pos]);
                break;
            }

            num = write(fileno(stdin), buf, num);
            if (num < 0)
            {
                fprintf(stderr, "failed to write %s!\r\n", ecmd->argv[pos]);
                break;
            }
        }

        close(fd);
    }
}

int issubstr(char *str, char *key)
{
    int slen = strlen(str);
    int klen = strlen(key);
    int start;

    for (start = 0; start < slen - klen; start++)
    {
        if (strncmp(str + start, key, klen) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

int readline(char *file, char *line)
{
    int pos = 0;
    int offset = 0;
    static int fd = 0;
    static int start = 0;
    static int num = BUF_LEN;
    static char buf[BUF_LEN];

    memset(line, 0, LINE_LEN);

    if (fd <= 0)
    {
        fd = open(file, O_RDONLY);
        if (fd < 0)
        {
            fprintf(stderr, "failed to open %s!\r\n", file);
            return ERROR;
        }
    }

    while (1)
    {
        if (start >= num)
        {
            memset(buf, 0, BUF_LEN);

            num = read(fd, buf, BUF_LEN);
            if (num == 0)
            {
                close(fd);
                return strlen(line);
            }
            else if (num < 0)
            {
                fprintf(stderr, "failed to read %s!\r\n", file);
                return ERROR;
            }

            start = 0;
        }

        for (pos = start; (pos < num) && (buf[pos] != '\n'); pos++) ;

        /* check line's length */
        if (offset + pos - start + 1 >= LINE_LEN)
        {
            start = pos + 1;
            continue;
        }

        memcpy(line + offset, buf + start, pos - start + 1);
        
        offset += (pos - start);
        start = pos + 1;

        if (pos < num)
        {
            break;
        }
    }

    return strlen(line);
}

/* todo: support more options and patterns */
void grep(struct execcmd *ecmd)
{
    int argc = 1; /* ecmd->argv[1] stands for pattern */
    int num;
    char line[LINE_LEN];

    while (ecmd->argv[++argc])
    {
        while ((num = readline(ecmd->argv[argc], line)) > 0)
        {
            if (issubstr(line, ecmd->argv[1]))
            {
                if (write(fileno(stdin), line, num) < 0)
                {
                    fprintf(stderr, "failed to write %s!\r\n", line);
                }
            }
        }
    }
}

void ls(struct execcmd *ecmd)
{
    printf("welcome to use %s!\r\n", ecmd->argv[0]);
}

/* algorithm for quick sort:
 * choose a pivot: p
 * exchange a[0] and a[p]
 * i = 1, j = n-1
 * while i <= j
 *     if a[i] < a[0], i++
 *     else exchange a[i] and a[j], j--
 * exchange a[0] and a[i-1]
 * qsort(a, 0, i-2) and qsort(a, i, n-1)
 * */
void quick_sort(char **strs, int left, int right)
{
    int start = left + 1;
    int end = right;
    char *tmp;

    if (left >= right)
    {
        return;
    }

    while (start <= end)
    {
        if (strcmp(strs[start], strs[left]) < 0)
        {
            start++;
        }
        else
        {
            tmp = strs[start];
            strs[start] = strs[end];
            strs[end] = tmp;
            end--;
        }
    }

    tmp = strs[left];
    strs[left] = strs[start - 1];
    strs[start - 1] = tmp;

    quick_sort(strs, left, start - 2);

    quick_sort(strs, start, right);
}

/* todo: support more options */
void sort(struct execcmd *ecmd)
{
    char lines[LINE_NUM][LINE_LEN];
    char *plines[LINE_NUM];
    int num = 0;
    int argc = 0;
    int pos;

    /* read at most LINE_NUM lines */
    while (ecmd->argv[++argc])
    {
        while ((num < LINE_NUM) && (readline(ecmd->argv[argc], lines[num])))
        {
            plines[num] = lines[num];
            num++;
        }
    }

    quick_sort(plines, 0, num-1);

    for (pos = 0; pos < num; pos++)
    {
        if (write(fileno(stdin), plines[pos], LINE_LEN) < 0)
        {
            fprintf(stderr, "failed to write %s!\r\n", lines[pos]);
        }
    }
}

/* todo: support more options */
void uniq(struct execcmd *ecmd)
{
    int num;
    int fd;
    int argc = 0;
    char cur[LINE_LEN] = {0};
    char prev[LINE_LEN] = {0};

    if ((!ecmd->argv[2]) || ((fd = open(ecmd->argv[2], O_WRONLY | O_CREAT | O_TRUNC)) < 0))
    {
        fd = fileno(stdout);
    }

    while ((num = readline(ecmd->argv[1], cur)) > 0)
    {
        if (memcmp(cur, prev, num))
        {
            write(fd, cur, num);
        }

        memcpy(prev, cur, num);

        prev[num] = 0;
    }
}

/* todo: support reading from stdin */
void wc(struct execcmd *ecmd)
{
    struct cntinfo cur;
    struct cntinfo total;
    int argc = 0;
    int pos;
    int fd;
    int num;
    int start;
    char last;
    char buf[BUF_LEN];

    memset(&total, 0, sizeof(total));

    while (ecmd->argv[++argc])
    {
        fd = open(ecmd->argv[argc], O_RDONLY);
        if (fd < 0)
        {
            fprintf(stderr, "failed to open %s!\r\n", ecmd->argv[argc]);
            continue;
        }

        memset(&cur, 0, sizeof(cur));
        last = 0;

        while (1)
        {
            memset(buf, 0, BUF_LEN);

            num = read(fd, buf, BUF_LEN);
            if (num == 0)
            {
                printf("%5d %5d %5d %s\r\n", cur.nline, cur.nword, cur.nbyte, ecmd->argv[argc]);
                break;
            }
            else if (num < 0)
            {
                printf("failed to read %s!\r\n", ecmd->argv[argc]);
                break;
            }

            cur.nbyte += num;
            
            pos = 0;

            while (pos < num)
            {
                while ((pos < num) && strchr(whitespace, buf[pos]))
                {
                    if (buf[pos] == '\n')
                    {
                        cur.nline++;
                    }
                    
                    pos++;
                }
                
                start = pos;

                while ((pos < num) && (!strchr(whitespace, buf[pos])))
                {
                    pos++;
                }

                if ((pos > start) && 
                    ((start != 0) || (strchr(whitespace, last))))
                {
                    cur.nword++;
                }
            }

            last = buf[num-1];
        }

        close(fd);

        total.nline += cur.nline;
        total.nword += cur.nword;
        total.nbyte += cur.nbyte;
    }
    
    printf("%5d %5d %5d total\r\n", total.nline, total.nword, total.nbyte);
}

void
runecmd(struct execcmd *ecmd)
{
    int pos;

    for (pos = 0; pos < CMD_NUM; pos++)
    {
        if (0 == strcmp(ecmd->argv[0], g_func_tbl[pos].cmd))
        {
            g_func_tbl[pos].func(ecmd);
            break;
        }
    }

    if (pos == CMD_NUM)
    {
        fprintf(stderr, "unknown execcmd\n");
    }
}

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    _exit(0);
  
  switch(cmd->type){
  default:
    fprintf(stderr, "unknown runcmd\n");
    _exit(-1);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      _exit(0);

    runecmd(ecmd);
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    fprintf(stderr, "redir not implemented\n");
    // Your code here ...
    runcmd(rcmd->cmd);
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    fprintf(stderr, "pipe not implemented\n");
    // Your code here ...
    break;
  }    
  _exit(0);
}

int
getcmd(char *buf, int nbuf)
{
  if (isatty(fileno(stdin)))
    fprintf(stdout, "6.828$ ");
  memset(buf, 0, nbuf);
  if(fgets(buf, nbuf, stdin) == 0)
    return -1; // EOF
  return 0;
}

int
main(void)
{
  static char buf[100];
  int fd, r;

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf+3);
      continue;
    }
    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait(&r);
  }
  exit(0);
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->flags = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

// Parsing

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
