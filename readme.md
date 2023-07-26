# libFFsh

A tiny set of shell like commands for FatFS

## Features

Included commands:

* `cat`
* `cd`
* `cp [-r|--recursive] [-n|--no-clobber]`
* `date`
* `echo [-o|--out:<file>] [-a|--append]`
* `exit`
* `false`
* `hexdump [-s|--skip:<offset>] [-n|--length:<bytes>]`
* `label drive: [<newlabel>]`
* `ls [-a|--all] [-l]]`
* `mkdir [-p|--parents]`
* `mv [-n|--no-clobber]`
* `pwd`
* `rm [-r|--recursive]`
* `rmdir`
* `sleep (with suffixes 'ms', 's', 'm', 'h', 'd')`
* `touch`
* `true`


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

Braces sequence expressions (eg: `{1..10}`) aren't supported.

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

### Multiple Commands

The operators `&&`, `||` && `;` can be used to group several commands into
a single command line.

eg:

```
cd /temp && rm -r *
```

The `()` operators can be used to run commands in a subshell:

```
(cd /subdir && ls -al)
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

* `FF_LFN_UNICODE=2` - libFFsh assumes all strings are utf8
* `FF_FS_LOCK=8` - either 0, or large enough to open all nest directories for recursive file
  operations + 2 for the source and target file being copied.
* `FF_FS_READONLY=0`



## API

### Invoking a Command

libFFsh doesn't provide an actual shell.  Rather it provides an API through which
a larger program can invoke commands.  Each command is associated with a `PROCESS`
which is similar in concept to an OS process in that it has a current working 
directory, a set of command arguments and two outputs - `stdout` and `stderr`

The following example shows how to setup a process and invoke a command

```
// Initialize FatFS
// Your code here

const char* pszCommand = "ls -al";

// Setup process
struct PROCESS proc;
process_init(&proc);
process_set_cwd(&proc, cwd);
process_set_stderr(&proc, NULL, my_stderr_handler);
process_set_stdout(&proc, NULL, my_stdout_handler);

// Invoke command
process_shell(&proc, pszCommand);

// Save cwd in case changed by `cd` command
strcpy(cwd, proc.cwd);

// Close process
process_close(&proc);
```



### stdio

libFFsh simulates `stdout` and `stderr` output by calling user-defined callback functions.
These functions are set using the `process_set_stderr` and `process_set_stdout` functions.

A trivial implementation that writes to C-runtime `stdout` and `stderr` might look like this:

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

The `user` parameter is the user value passed to `process_set_stdout` / `process_set_stderr`.



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


### ffsh_sleep

The sleep command implements parsing and totaling of the command line arguments but
requires an external function `ffsh_sleep(uint32_t millis)` to perform the actual
sleep operation.

eg: trivial implementation using Linux `usleep` function:

```
void ffsh_sleep(uint32_t millis)
{
    usleep(millis * 1000);
}
```



### Handling the `exit` command

Since `libFFsh` doesn't implement a command processing loop, it can't automatically
handle the `exit` command.  

Instead, the built-in `exit` command simply sets `PROCESS::did_exit` flag which
can be used by an external command loop to detect the user entering 'exit'.


