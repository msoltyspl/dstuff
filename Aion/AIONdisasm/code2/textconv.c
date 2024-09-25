#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:

#define BUF_RSV 256

static const char l_err_len[]="Textconv: input string too long.\n";
static const char l_err_mlf[]="Textconv: input string malformed or conversion impossible.\n";
static const char l_err_gen[]="Textconv: other error.\n";

static uint8_t *tmpbufC = NULL;//,*tmpbufT;

static int sl_lim, NONR_convs = 0;
/*
int utf8rem_bs(uint8_t* to, const uint8_t* from,int tocnt,int fromcnt)
{
	int i = 0, j = 0;

	while (j<tocnt && i<fromcnt) {
		//abort if malformed
		if(from[i] == '\\' && i == fromcnt-1)
			return -1;
		if(from[i] == '\\') switch(from[++i]) {
			case '0' : to[j] = '\0' ; break;
			case 't' : to[j] = '\t' ; break;
			case 'n' : to[j] = '\n' ; break;
			case 'r' : to[j] = '\r' ; break;
			case '\\': to[j] = '\\' ; break;
			default : to[j] = from[i];
		} else
			to[j] = from[i];
		i++;j++;
	}
	return (i < fromcnt) && (j >= tocnt) ? -1 : j;
}
*/

int utf8add_bs(uint8_t* to, const uint8_t* from,int tolim,int fromcnt,int bmode)
{
	int i = 0, j = 0, ret = -1;

	switch(bmode) { //0 - none, 1 - backslash, 2 - xml
		case 0:
			if (tolim < fromcnt)
				break;
			memcpy(to,from,fromcnt);
			ret = fromcnt;
			break;
		case 1:
		while (j+1<tolim && i<fromcnt) {
			switch(from[i]) {
				case '\0': to[j++] = '\\'; to[j] = '0' ; break;
				case '\t': to[j++] = '\\'; to[j] = 't' ; break;
				case '\n': to[j++] = '\\'; to[j] = 'n' ; break;
				case '\r': to[j++] = '\\'; to[j] = 'r' ; break;
				case '\\': to[j++] = '\\'; to[j] = '\\' ; break;
				default: to[j] = from[i];
			}
			i++;j++;
		}
		ret =  (i < fromcnt) && (j+1 >= tolim) ? -1 : j;
		break;

		case 2:
		while (j+7<tolim && i<fromcnt) {
			switch(from[i]) {
/*
0x39782326   0x0000003B 
0x41782326   0x0000003B
0x44782326   0x0000003B 
0x3B746C26           
0x3B746726           
0x6F706126   0x00003B73 
0x6F757126   0x00003B74
0x706D6126   0x0000003B

&#x9;000
&#xA;000
&#xD;000
&lt;
&gt;
&apos;00
&quot;00
&amp;000
*/
				case '\0': ((uint32_t*)(to+j))[0] = 0x30782326; ((uint32_t*)(to+j))[1] = 0x0000003B; j+=4; break; //formally not allowed
				case '\t': ((uint32_t*)(to+j))[0] = 0x39782326; ((uint32_t*)(to+j))[1] = 0x0000003B; j+=4; break;
				case '\n': ((uint32_t*)(to+j))[0] = 0x41782326; ((uint32_t*)(to+j))[1] = 0x0000003B; j+=4; break;
				case '\r': ((uint32_t*)(to+j))[0] = 0x44782326; ((uint32_t*)(to+j))[1] = 0x0000003B; j+=4; break;
				case '<':  ((uint32_t*)(to+j))[0] = 0x3B746C26; j+=3; break;     
				case '>':  ((uint32_t*)(to+j))[0] = 0x3B746726; j+=3; break;     
				case '\'': ((uint32_t*)(to+j))[0] = 0x6F706126; ((uint32_t*)(to+j))[1] = 0x00003B73; j+=5; break;
				case '"':  ((uint32_t*)(to+j))[0] = 0x6F757126; ((uint32_t*)(to+j))[1] = 0x00003B74; j+=5; break;
				case '&':  ((uint32_t*)(to+j))[0] = 0x706D6126; ((uint32_t*)(to+j))[1] = 0x0000003B; j+=4; break;
				default: to[j] = from[i];
			}
			i++;j++;
		}
		ret =  (i < fromcnt) && (j+7 >= tolim) ? -1 : j;
		break;
	}
	return ret;
}


//iconv_t d_ucs2user,d_user2ucs,d_ascf2user,d_user2ascf;

/*
	return values:
		-1 for generic conversion error
		-2 conversion produces too long string
*/

//int textconv_conv(const char *icode, const char *ocode, const char* ibuf, const int isiz, char **obuf, int *osiz, int dotrans, const int loud)
int textconv_conv(const char *ocode, uint8_t **obuf, int *osiz, const char *icode, const uint8_t* ibuf, const int isiz, int dotrans, const int loud)
{
	int ret = -1;
	iconv_t cd = (iconv_t)(-1);
	size_t nconv,inrem,outrem;
	const char *inptr;
	char ocode2[128],*outptr;

	if(isiz > sl_lim-BUF_RSV) {	//input buffer may not be that long
		if(loud) fputs(l_err_len,stderr);
		EXCEPT;
	}
	if(!strcmp(icode,ocode)) {
		memcpy(tmpbufC,ibuf,isiz);
		*obuf = tmpbufC;
		*osiz = isiz;
		return 0;
	}

	strncpy(ocode2,ocode,99);
	ocode2[99]=0;

	if(dotrans)
		strcat(ocode2,"//TRANSLIT");

	cd = iconv_open(ocode2, icode);
	if(cd ==  (iconv_t)(-1)) {
		if(loud) fprintf(stderr,"Conversion from '%s' to '%s' is not supported",icode,ocode);
		EXCEPT;
	}

	inptr = (const char*)ibuf;
	inrem = isiz;

	outptr = (char*)tmpbufC;
	outrem = sl_lim-BUF_RSV;	//reserve for specific conditions
	
//	fwrite(ibuf,1,isiz,stdout);

	errno = 0;
	while(inrem > 0) {
//		if((nconv = iconv(cd, &inptr, &inrem, &outptr, &outrem)) == (size_t)(-1)) {
		if((nconv = iconv(cd, (char **)(&inptr), &inrem, &outptr, &outrem)) == (size_t)(-1)) {
			switch(errno) {
				case EILSEQ:
					if(loud) fputs(l_err_mlf,stderr); break;
				case E2BIG:
					if(loud) fputs(l_err_len,stderr); break;
				default:
					if(loud) fputs(l_err_gen,stderr);
			}
			EXCEPT;
		}
		NONR_convs += nconv;
	}
//for stateful encodings
	outrem+=BUF_RSV;
	iconv(cd, NULL, NULL, &outptr, &outrem);

	*obuf = tmpbufC;
	*osiz = sl_lim-outrem;

	ret = 0;
EXCEPT_LABEL;
	if(cd != (iconv_t)(-1))
		iconv_close(cd);

//	fwrite(*obuf,1,*osiz,stdout);

	return ret;
}

void textconv_summary(void)
{
	puts("Textconv summary:\n");
	printf("NONR convs. : %d\n\n",NONR_convs);
}

int textconv_init(int siz, int bmode)
{
	int ret = -1;
	sl_lim = siz;

	if(!(tmpbufC = (uint8_t*)malloc(sl_lim)))
		EXCEPT;
	ret = 0;
EXCEPT_LABEL;

	return ret;
}

void textconv_destroy(void)
{
	if(tmpbufC)
		free(tmpbufC);
	tmpbufC = NULL;
}






#if 0
				default:
					if(extras) {
						switch(from[i]) {
							case '>': to[j++] = '\\'; to[j] = '>' ; break;
							case '<': to[j++] = '\\'; to[j] = '<' ; break;
							default: to[j] = from[i];
						}
					} else
						to[j] = from[i];
#endif
