# Purpose
This is to run cp command and backup files with log message at the same time. This is like `tee` command which shows an output on stdout and saves it to a file together. The goal of this `cpee` is to be alias for cp command.

`cpee` has git-log-like feature to read the past logs you copied by giving subcommand `--log` or `--show`. You can get only head of the log by adding `--head`. You can also checkout file by hash with `--checkout` option. Several functions help you not to forget leaving logs, etc.

The root directories containing copied files have the pseudo hash value calcurated by summing all the hash values of files under it. Thus, the sudo hash value of directories should be same as long as the files under it are not changed.

You can see the subcommands supported by cpee with --help subcommand.

# Environment
Linux

```
$ find a
a
a/b
a/b/c
a/b/c/file2
a/file1
$ find x
x
x/y
x/y/z
x/y/z/file4
x/file3
$ ./cpee a x to
MD5 = 0cc175b9c0f1b6a831c399e269772661
$ find to/
to/
to/b
to/b/c
to/b/c/file2
to/y
to/y/z
to/y/z/file4
to/file1
to/file3
```
