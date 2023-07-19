# libFFsh

A tiny set of shell like commands for FatFS

## Features

Included commands:

* `cat`
* `cd`
* `cp [-r|--recursive] [-n|--no-clobber]`
* `echo [-o|--out:<file>] [-a|--append]`
* `exit`
* `hexdump [-s|--skip:<offset>] [-n|--length:<bytes>]`
* `ls [-a|--all] [-l]]`
* `mkdir [-p|--parents]`
* `mv [-n|--no-clobber]`
* `pwd`
* `rm [-r|--recursive]`
* `rmdir`
* `touch`


## Command Processor

For the functionality it supports, libFFsh tries to mimmick bash and linux type commands.

### Wildcard Expansion

All commands support '*' and '?' glob patterns which are conceptually expanded before the 
command is invoked.

eg:

```
>cp *.cpp target/
```

### Brace Expansion

All commands also support brace expansion:

```
> ls *.{png,jpg,gif}
```

### Argument Quoting

Any arguments passed inside single or double quotes are treated literally and no 
wildcard or brace expansion is performed.

```
> echo "*.{png.jpg}"
*.{png.jpg}
```

Backslashes can also be used to escape special characters and spaces:

```
> ls path\ with\ spaces
```

### Other Notes

* There is no support for piping or redirection.
* Unlike bash/linux libFFsh is uses case insensitive file names.
* libFFsh prefers slashes over backslashes for directory separators
  but will accept either.




## Building

To build libFFsh:

* compile all the files in the `src` directory
* link with a suitably configured build of FatFS

FatFS should be configured with:

* `FF_LFN_UNICODE` - libFFsh assumes all strings are utf8
* `FF_FS_LOCK` - either 0, or large enough to open all nest directories for recursive file
  operations + 2 for the source and target file being copied.
* `FF_FS_READONLY` - 0



## API

### Invoking a Command

libFFsh doesn't provide an actual shell.  Rather it provides an API through which
a larger program can invoke commands.  Each command is associated with a `FFSH_CONTEXT`
which is similar in concept to a process - it has a current working directory, a set of
command arguments and two outputs - `stdout` and `stderr`

To invoke a command simply setup a `FFSH_CONTEXT` and call `ffsh_exec`.

```
// Initialize FatFS
// Your code here

// Setup current working directory 
// (NB: cd command will modify this)
char cwd[FF_MAX_LFN];
strcpy(cwd, "/");

// Setup command context
FFSH_CONTEXT ctx;
ctx.cwd = cwd;
ctx.user = NULL;
ctx.pfn_stdout = my_stdout_handler;
ctx.pfn_stderr = my_stderr_handler;

// Invoke a command
char szCommand[] = "ls -al";
int exitcode = ffsh_exec(szCommand);
```

NB: `ffsh_exec` modifies the passed command string buffer (so don't pass a const 
or literal string).



### stdio

libFFsh simulates `stdout` and `stderr` output by calling `FFSH_CONTEXT::pfn_stdout`
and `FFSH_CONTEXT::pfn_stderr` for each character to be output.

A trivial implementation that writes to C `stdout` and `stderr` might look like this:

```
void my_stdout_handler(void* user, char ch)
{
    fputc(ch, stdout);
}

void my_stderr_handler(void* user, char ch)
{
    fputc(ch, stderr);
}
```

The `user` parameter is the user defined value passed to `FFSH_CONTEXT::user`.



### ffsh_printf

For formatting output, libFFsh requires an externally linked function `ffsh_printf`:

```
void ffsh_printf(void (*write)(void*, char), void* arg, const char* format, ...);
```

This function should format a printf style string and send it to the passed `write`
function (which will be one of the stdout/stderr functions used for stdio).

A trivial implementation using the C `vsnprintf` function might look like this:

```
void fsh_printf(void (*write)(void*, char), void* arg, const char* format, ...)
{
    char szTemp[FF_MAX_LFN + 100];

	va_list args;
	va_start(args, format);
    vsnprintf(szTemp, sizeof(szTemp), format, args);
    va_end(args);

    const char* p = szTemp;
    while (*p)
    {
        write(arg, *p++);
    }
}
```

If you're providing your own printf formatting core, it should support the following
format specifiers: `%c`, `%s`, `%i`, `%02i`, `%12i`, `%08x`, `%02x`.


### Handling the `exit` command

Since `libFFsh` doesn't implement a command processing loop, it can't automatically
handle the `exit` command.  

Instead, the built-in `exit` command simply sets `FFSH_CONTEXT::did_exit` flag which
can be used by an external command loop to detect the user entering 'exit'.


