#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>

// Simplifed xv6 shell.

#define MAXARGS 10
#define MAX_CMD_LEN 10
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
void echo(struct execcmd * cmd);
void grep(struct execcmd * cmd);
void ls(struct execcmd * cmd);
void rm(struct execcmd * cmd);
void sort(struct execcmd * cmd);
void uniq(struct execcmd * cmd);
void wc(struct execcmd * cmd);

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

struct ecmdfunc g_func_tbl[] = {
    {"cat", cat},
    {"echo", echo},
    {"grep", grep},
    {"ls", ls},
    {"rm", rm},
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

    /* read from stdin if there is no input file */
    if (!ecmd->argv[1])
    {
        while ((num = read(fileno(stdin), buf, BUF_LEN)) > 0)
        {
            write(fileno(stdout), buf, num);
        }

        return;
    }

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

            num = write(fileno(stdout), buf, num);
            if (num < 0)
            {
                fprintf(stderr, "failed to write %s!\r\n", ecmd->argv[pos]);
                break;
            }
        }

        close(fd);
    }
}

void echo(struct execcmd *cmd)
{
    int argc = 0;

    while (cmd->argv[++argc])
    {
        printf("%s ", cmd->argv[argc]);
    }

    printf("\n");
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

    if ((file) && (fd <= 0))
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
                fprintf(stderr, "failed to read %d!\r\n", fd);
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
    int num;
    int argc = 2; /* ecmd->argv[1] stands for pattern */
    char line[LINE_LEN];

    if (ecmd->argv[1] == NULL)
    {
        fprintf(stderr, "not found pattern string!\r\n");
        return;
    }

    do
    {
        while ((num = readline(ecmd->argv[argc], line)) > 0)
        {
            if (issubstr(line, ecmd->argv[1]))
            {
                if (write(fileno(stdout), line, num) < 0)
                {
                    fprintf(stderr, "failed to write %s!\r\n", line);
                }
            }
        }
    } while (ecmd->argv[argc++]);
}

void ls(struct execcmd *ecmd)
{
    int len;
    DIR *dp;
    struct dirent *ep;

    dp = opendir((ecmd->argv[1] ? ecmd->argv[1] : "./"));

    if (dp == NULL)
    {
        fprintf(stderr, "failed to open the directory!\r\n");
    }

    while (ep = readdir(dp))
    {
        len = strlen(ep->d_name);
        ep->d_name[len++] = '\n';
        ep->d_name[len] = 0;

        if (write(fileno(stdout), ep->d_name, len) < 0)
        {
            fprintf(stderr, "failed to write %s when ls\r\n", ep->d_name);
        }
    }

    closedir(dp);
}

void rm(struct execcmd *ecmd)
{
    int argc = 0;
    int result;

    while (ecmd->argv[++argc])
    {
        if (result = remove(ecmd->argv[argc]))
        {
            fprintf(stderr, "failed to remove %s! err=%d\r\n", ecmd->argv[argc], result);
        }
    }
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
    char lines[LINE_NUM][LINE_LEN] = {0};
    char *plines[LINE_NUM];
    int num = 0;
    int argc = 1;
    int pos = 0;

    /* read at most LINE_num lines */
    do
    {
        while ((num < LINE_NUM) && (readline(ecmd->argv[argc], lines[num]) > 0))
        {
            plines[num] = lines[num];
            num++;
        }
    } while (ecmd->argv[argc++]);

    quick_sort(plines, 0, num-1);

    for (pos = 0; pos < num; pos++)
    {
        if (write(fileno(stdout), plines[pos], strlen(plines[pos])) < 0)
        {
            fprintf(stderr, "failed to write %s!\r\n", plines[pos]);
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
    int argc = 1;
    int pos;
    int fd = 0;
    int num;
    int start;
    char last;
    char buf[BUF_LEN];
    char result[BUF_LEN];

    memset(&total, 0, sizeof(total));

    memset(&result, 0, BUF_LEN);

    do
    {
        if (ecmd->argv[argc])
        {
            fd = open(ecmd->argv[argc], O_RDONLY);
            if (fd < 0)
            {
                fprintf(stderr, "failed to open %s!\r\n", ecmd->argv[argc]);
                break;
            }
        }

        memset(&cur, 0, sizeof(cur));
        last = 0;

        while (1)
        {
            memset(buf, 0, BUF_LEN);

            num = read(fd, buf, BUF_LEN);

            if (num == 0)
            {
                sprintf(result + strlen(result), "%5d %5d %5d %s\r\n", cur.nline, cur.nword, cur.nbyte, (ecmd->argv[argc] ? ecmd->argv[argc] : ""));
                break;
            }
            else if (num < 0)
            {
                printf("failed to read %d!\r\n", fd);
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

        fd = -1;

        total.nline += cur.nline;
        total.nword += cur.nword;
        total.nbyte += cur.nbyte;
    } while (ecmd->argv[argc++]);
    
    if (argc > 2)
    {
        sprintf(result + strlen(result), "%5d %5d %5d total\r\n", total.nline, total.nword, total.nbyte);
    }

    if (write(fileno(stdout), result, strlen(result)) < 0)
    {
        fprintf(stderr, "failed to write %s in wc!\r\n", result);
    }
}

void
runecmd(struct execcmd *ecmd)
{
    int pos;
    int ncmd = sizeof(g_func_tbl) / sizeof(struct ecmdfunc);

    for (pos = 0; pos < ncmd; pos++)
    {
        if (0 == strcmp(ecmd->argv[0], g_func_tbl[pos].cmd))
        {
            g_func_tbl[pos].func(ecmd);
            break;
        }
    }

    if (pos == ncmd)
    {
        fprintf(stderr, "unknown execcmd\n");
    }
}

void runrcmd(struct redircmd *rcmd)
{
    int fd = -1;

    fd = open(rcmd->file, rcmd->flags , 0777);
    if (fd < 0)
    {
        fprintf(stderr, "failed to open %s!\r\n", rcmd->file);
        return;
    }

    dup2(fd, rcmd->fd);

    close(fd);

    runecmd((struct execcmd *)rcmd->cmd);
}

void runpcmd(struct pipecmd *pcmd)
{
    int num;
    int pos;
    int id;
    int *fds;
    struct execcmd **cmds;
    struct pipecmd *cur = pcmd;

    for (num = 1; (cur) && (cur->type == '|'); cur = (struct pipecmd *)cur->right, num++);

    /* notice: fds[0], fds[1], fds[2n] and fds[2n+1] act as sentinels */
    fds = malloc(sizeof(int) * num * 2 + 2);
    cmds = malloc(sizeof(struct execcmd *) * num);

    if ((!fds) || (!cmds))
    {
        fprintf(stderr, "failed to malloc memory for fds or cmds!\n");
        return;
    }

    fds[0] = fds[1] = -1;
    fds[2 * num + 1] = fds[2 * num] = -1;

    cmds[0] = (struct execcmd *)pcmd->left;
    cur = (struct pipecmd *)pcmd->right;

    for (pos = 1; pos < num; pos++)
    {
        pipe(fds + 2 * pos);

        cmds[pos] = ((cur->type == '|')? (struct execcmd *)cur->left : (struct execcmd *)cur);
        cur = (struct pipecmd *)cur->right;
    }

    for (pos = 0; pos < num; pos++)
    {
        if (fork() == 0)
        {
            dup2(fds[2 * pos], fileno(stdin));
            dup2(fds[2 * pos + 3], fileno(stdout));

            for (id = 0; id <= 2 * pos + 2; id++)
            {
                close(fds[id]);
            }

            if (cmds[pos]->type == ' ')
            {
                runecmd(cmds[pos]);
            }
            else
            {
                runrcmd((struct redircmd *)cmds[pos]);
            }

            close(fds[2 * pos + 3]);

            exit(0);
        }
        else
        {
            wait(0);

            for (id = 0; id < 2 * pos + 2; id++)
            {
                close(fds[id]);
            }

            close(fds[2 * pos + 3]);
        }
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
    runrcmd(rcmd);
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    runpcmd(pcmd);
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
