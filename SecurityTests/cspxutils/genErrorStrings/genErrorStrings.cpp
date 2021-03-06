/*
 * genErrorStrings.cpp - parse supplied files, generate error table from
 *		them of the following form:
 *
 * typedef struct {
 *		CSSM_RETURN		errCode;
 *		const char		*errStr;
 * } ErrString;
 *
 * ErrString errStrings[] = {
 *		{ CSSMERR_CSSM_INTERNAL_ERROR, "CSSMERR_CSSM_INTERNAL_ERROR" },
 *		...
 *		{ CSSMERR_CSP_FUNCTION_FAILED, "CSSMERR_CSP_FUNCTION_FAILED" }
 * };
 *
 * The error table is written to stdout. 
 */
 
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include "fileIo.h"

#define MAX_LINE_LEN	256

static void usage(char **argv)
{
	printf("usage: %s inFile [inFile...]\n", argv[0]);
	exit(1);
}

static void writePreamble(
	FILE *f)
{
	fprintf(f, "/*\n");
	fprintf(f, " * This file autogenerated by genErrorStrings. Do not edit. \n");
	fprintf(f, " */\n\n");
	fprintf(f, "#include <Security/Security.h>\n\n");
	fprintf(f, "typedef struct {\n");
	fprintf(f, "\tCSSM_RETURN errCode;\n");
	fprintf(f, "\tconst char *errStr;\n");
	fprintf(f, "} ErrString;\n\n");
	fprintf(f, "static const ErrString errStrings[] = {\n");
}

static void writePostamble(
	FILE *f)
{
	/* generate a null entry as terminator */
	fprintf(f, "\t{0, NULL}\n");
	fprintf(f, "};\n");
}

static void writeToken(
	const char *token,
	FILE *f)
{
	printf("\t{ %s,\"%s\"},\n", token, token);
}

/* skip whitespace (but not line terminators) */
static void skipWhite(
	const char *&cp,
	unsigned &bytesLeft)
{
	while(bytesLeft != 0) {
		switch(*cp) {
			case ' ':
			case '\t':
				cp++;
				bytesLeft--;
				break;
			default:
				return;
		}
	}
}

static void getLine(
	const char	*&cp,			// IN/OUT
	unsigned	&bytesLeft,		// IN/OUT bytes left
	char		*lineBuf)
{
	char *outp = lineBuf;
	char *endOfOut = outp + MAX_LINE_LEN - 2;
	while(bytesLeft != 0) {
		switch(*cp) {
			case '\n':
			case '\r':
				cp++;
				bytesLeft--;
				*outp = 0;
				return;
			default:
				*outp++ = *cp++;
				bytesLeft--;
				break;
		}
		if(outp == endOfOut) {
			printf("***getLine: line length exceeded!\n");
			break;
		}
	}
	/* end of file */
	*outp = 0;
}

/* incoming line is NULL terminated even if it's empty */
static bool isLineEmpty(
	const char *cp)
{
	for( ; *cp; cp++) {
		switch(*cp) {
			case ' ':
			case '\t':
				break;
			default:
				return false;
		}
	}
	return true;
}

/* process one file */
static void processFile(
	const char *fileName,
	const char *in,
	unsigned inLen,
	FILE *f)
{
	char lineBuf[MAX_LINE_LEN];
	unsigned lineLen;
	const char *cp;
	const char *endOfToken;
	char tokenBuf[MAX_LINE_LEN];
	unsigned tokenLen;
	const char *lastSlash = fileName;
	const char *nextSlash;
	
	while((nextSlash = strchr(lastSlash, '/')) != NULL) {
		lastSlash = nextSlash + 1;
	}
	fprintf(f, "\t/* Error codes from %s */\n", lastSlash);
	
	while(inLen != 0) {
		/* get one line, NULL terminated */
		getLine(in, inLen, lineBuf);
		if(isLineEmpty(lineBuf)) {
			continue;
		}
		
		/* skip leading whitespace */
		lineLen = strlen(lineBuf);
		cp = lineBuf;
		skipWhite(cp, lineLen);
		
		/* interesting? */
		if(strncmp((char *)cp, "CSSMERR_", 8)) {
			continue;
		}
		
		/*
		 * cp is the start of the CSSMERR_ token
		 * find end of token 
		 */
		endOfToken = cp + 8;
		for(;;) {
			if(isalnum(*endOfToken) || (*endOfToken == '_')) {
				endOfToken++;
				continue;
			}
			else {
				break;
			}
		}
		
		/* endOfToken is one past the end of the CSSMERR_ token */
		tokenLen = endOfToken - cp;
		memmove(tokenBuf, cp, tokenLen);
		tokenBuf[tokenLen] = '\0';
		
		/* write the stuff */
		writeToken(tokenBuf, f);
	}
}

int main(int argc, char **argv)
{
	unsigned char *inFile;
	unsigned inFileLen;
	int dex;
	
	if(argc < 2) {
		usage(argv);
	}
	
	writePreamble(stdout);
	writeToken("CSSM_OK", stdout);
	for(dex=1; dex<argc; dex++) {
		if(readFile(argv[dex], &inFile, &inFileLen)) {
			printf("***Error reading %s. Aborting.\n", argv[dex]);
			exit(1);
		}
		processFile(argv[dex], (const char *)inFile, inFileLen, stdout);
		free(inFile);
	}
	writePostamble(stdout);
	return 0;
}
