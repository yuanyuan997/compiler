
#ifndef SCAN_H
#define SCAN_H


#define MAXTOKENLEN     64 /*maximum token length is 64*/

extern char tokenString[MAXTOKENLEN+1]; /*The lexeme beging scanned will be hold in tokenString*/

TokenType getToken(void);

#endif
