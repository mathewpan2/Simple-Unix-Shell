# Simple-Unix-Shell

A basic Unix shell written in c able to excute most commands by calling execv, but pipes, input redirection, process management, and error 
checking are implemented manually using system calls. To see process management in action, jobs can be suspended by pressing ctrl + z. Susepnded 
jobs can be seen with the command "jobs", and suspended jobs can be resumed with "fg <id>"
