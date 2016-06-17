# Purpose
This is to run cp command and backup files with log message at the same time. This is like `tee` command which shows an output on stdout and saves it to a file together. The goal of this `cpee` is to be alias for cp command.

`cpee` has git-log-like feature to read the past logs you copied by giving subcommand `read` or `show`. You can get only head of the log by `head`. Several functions help you not to forget leaving logs, etc.

The root directories containing copied files have the sudo hash value calcurated by summing all the hash values of files under it. Thus, the sudo hash value of directories should be same as long as the files under it are not changed.

You can see the subcommands supported by cpee with help subcommand or --help option.

# Environment
Linux

