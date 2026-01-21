# UNIX-Shell
This is a homework assignment for class. In CSC331 Operating Systems we are using a lot of xv6 kernel. This assignment we had to create our own UNIX shell. All ran through grsh.c.

## Interactive mode
Use './grsh' in your console then you will see prompt 'grsh>'

## Batch Mode
Use './grsh batch.txt' Commands will be read form the file. No Prompt is shown in batch mode.

## Basic Command
Type 'ls -la /tmp' The shell searches for the executatble in the current path and runs it.

## Built in Commands
'exit' - End the shell
'cd' - changes the current working directory
'path' - set the list of directories the shell searches for executatables
