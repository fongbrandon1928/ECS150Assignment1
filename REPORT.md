# Report

## Check for >>
The redirection logic is altered to check for another '>' symbol after 
the first one to see if the file will be overwritten or appended to.
Variable append is set to 1 if the second '>' symbol is present.
