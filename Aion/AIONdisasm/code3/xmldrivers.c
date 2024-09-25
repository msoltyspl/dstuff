#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "hashptr.h"
#include "xmldrivers.h"
#include "xmlblob.h"
#include "bfile.h"
#include "tags.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#define EXCEPT goto l_finalize
#define EXCEPT_LABEL l_finalize:
#define MAXINDENT 256

//drvtype g_curr_driver = TEXT;
//enum VALTYPE { t_null = -1, t_tinyint = 1, t_utinyint = 0, t_int = 3, t_uint = 2, t_llint = 5, t_ullint = 4, t_double = 6, t_str = 7, t_blob = 8};

#define T_NOTFOUND UINTPTR_MAX

#define T_NULL 0

#define T_UTINYINT 1
#define T_USHRTINT 2
#define T_UINT 3
#define T_ULLINT 4

#define T_TINYINT 5
#define T_SHRTINT 6
#define T_INT 7
#define T_LLINT 8

#define T_FLOAT 9

#define T_LONGTEXT 10

//base of varchar
#define T_VARCHAR 16

static struct { char *str; int siz;} gs_tnametab[17] = {
	[T_NULL] = { .str = "", .siz = 0 },

	[T_UTINYINT] = { .str = "TINYINT(3) UNSIGNED", .siz = 19 },
	[T_USHRTINT] = { .str = "SMALLINT(5) UNSIGNED", .siz = 20 },
	[T_UINT] = { .str =     "INT(10) UNSIGNED", .siz = 16 },
	[T_ULLINT] = { .str =   "BIGINT(20) UNSIGNED", .siz = 19 },

	[T_TINYINT] = { .str = "TINYINT(4)", .siz = 19-9 },
	[T_SHRTINT] = { .str = "SMALLINT(6)", .siz = 20-9 },
	[T_INT] = { .str =     "INT(11)", .siz = 16-9 },
	[T_LLINT] = { .str =   "BIGINT(20)", .siz = 19-9 },

	[T_FLOAT] = { .str =   "FLOAT(16)", .siz = 9 },

	[T_LONGTEXT] = { .str =   "LONGTEXT", .siz = 8 },

	[T_VARCHAR] = { .str =   "VARCHAR", .siz = 7 },

};

static int gs_promotab[12][12] = {

//u->u
	[T_UTINYINT][T_UTINYINT] = 0,
	[T_UTINYINT][T_USHRTINT] = T_USHRTINT,
	[T_UTINYINT][T_UINT] = T_UINT,
	[T_UTINYINT][T_ULLINT] = T_ULLINT,
	[T_UTINYINT][T_FLOAT] = T_FLOAT,

	[T_USHRTINT][T_UTINYINT] = 0,
	[T_USHRTINT][T_USHRTINT] = 0,
	[T_USHRTINT][T_UINT] = T_UINT,
	[T_USHRTINT][T_ULLINT] = T_ULLINT,
	[T_USHRTINT][T_FLOAT] = T_FLOAT,

	[T_UINT][T_UTINYINT] = 0,
	[T_UINT][T_USHRTINT] = 0,
	[T_UINT][T_UINT] = 0,
	[T_UINT][T_ULLINT] = T_ULLINT,
	[T_UINT][T_FLOAT] = T_VARCHAR,

	[T_ULLINT][T_UTINYINT] = 0,
	[T_ULLINT][T_USHRTINT] = 0,
	[T_ULLINT][T_UINT] = 0,
	[T_ULLINT][T_ULLINT] = 0,
	[T_ULLINT][T_FLOAT] = T_VARCHAR,


//u->s
	[T_UTINYINT][T_TINYINT] = T_SHRTINT,
	[T_UTINYINT][T_SHRTINT] = T_SHRTINT,
	[T_UTINYINT][T_INT] = T_INT,
	[T_UTINYINT][T_LLINT] = T_LLINT,

	[T_USHRTINT][T_TINYINT] = T_INT,
	[T_USHRTINT][T_SHRTINT] = T_INT,
	[T_USHRTINT][T_INT] = T_INT,
	[T_USHRTINT][T_LLINT] = T_LLINT,

	[T_UINT][T_TINYINT] = T_LLINT,
	[T_UINT][T_SHRTINT] = T_LLINT,
	[T_UINT][T_INT] = T_LLINT,
	[T_UINT][T_LLINT] = T_LLINT,

	[T_ULLINT][T_TINYINT] = T_VARCHAR,
	[T_ULLINT][T_SHRTINT] = T_VARCHAR,
	[T_ULLINT][T_INT] = T_VARCHAR,
	[T_ULLINT][T_LLINT] = T_VARCHAR,


//s->u
	[T_TINYINT][T_UTINYINT] = T_SHRTINT,
	[T_TINYINT][T_USHRTINT] = T_INT,
	[T_TINYINT][T_UINT] = T_LLINT,
	[T_TINYINT][T_ULLINT] = T_VARCHAR,
	[T_TINYINT][T_FLOAT] = T_FLOAT,

	[T_SHRTINT][T_UTINYINT] = T_INT,
	[T_SHRTINT][T_USHRTINT] = T_INT,
	[T_SHRTINT][T_UINT] = T_LLINT,
	[T_SHRTINT][T_ULLINT] = T_VARCHAR,
	[T_SHRTINT][T_FLOAT] = T_FLOAT,

	[T_INT][T_UTINYINT] = T_LLINT,
	[T_INT][T_USHRTINT] = T_LLINT,
	[T_INT][T_UINT] = T_LLINT,
	[T_INT][T_ULLINT] = T_VARCHAR,
	[T_INT][T_FLOAT] = T_VARCHAR,

	[T_LLINT][T_UTINYINT] = T_VARCHAR,
	[T_LLINT][T_USHRTINT] = T_VARCHAR,
	[T_LLINT][T_UINT] = T_VARCHAR,
	[T_LLINT][T_ULLINT] = T_VARCHAR,
	[T_LLINT][T_FLOAT] = T_VARCHAR,


//s->s
	[T_TINYINT][T_UTINYINT] = 0,
	[T_TINYINT][T_USHRTINT] = T_SHRTINT,
	[T_TINYINT][T_UINT] = T_INT,
	[T_TINYINT][T_ULLINT] = T_LLINT,

	[T_SHRTINT][T_UTINYINT] = 0,
	[T_SHRTINT][T_USHRTINT] = 0,
	[T_SHRTINT][T_UINT] = T_INT,
	[T_SHRTINT][T_ULLINT] = T_LLINT,

	[T_INT][T_UTINYINT] = 0,
	[T_INT][T_USHRTINT] = 0,
	[T_INT][T_UINT] = 0,
	[T_INT][T_ULLINT] = T_LLINT,

	[T_LLINT][T_UTINYINT] = 0,
	[T_LLINT][T_USHRTINT] = 0,
	[T_LLINT][T_UINT] = 0,
	[T_LLINT][T_ULLINT] = 0,

//f->...

	[T_FLOAT][T_UTINYINT] = 0,
	[T_FLOAT][T_USHRTINT] = 0,
	[T_FLOAT][T_UINT] = T_VARCHAR,
	[T_FLOAT][T_ULLINT] = T_VARCHAR,

	[T_FLOAT][T_TINYINT] = 0,
	[T_FLOAT][T_SHRTINT] = 0,
	[T_FLOAT][T_INT] = T_VARCHAR,
	[T_FLOAT][T_LLINT] = T_VARCHAR,

//f->f
	[T_FLOAT][T_FLOAT] = 0,


};




static HTAB *gs_hashtype = NULL;
static FILE *gs_sqllog = NULL;

static char gs_desc_rename[11] = "desc_short";
static char gs_err_nattrs[] = "Warning - %d attributes on level %d.\n";
static char gs_err_lvl01val[] = "Warning - Lvl 0/1 has non-null value: \"%s\".\n";
static char gs_err_nosubs[] = "Warning - there should be children at lvl %d.\n";
static char gs_err_subs[] = "Warning - there should be no children at lvl %d.\n";
static char gs_err_nullcol[] = "Warning - couldn't determine the type of column: \"%s\".\n";
static char gs_err_lvl2subandval[] = "Warning - level 2 has both subs and a value.\n";
static char gs_err_lvl4[] = "Warning - 4 or more levels.\n";

static char gs_htabcp[] = "HACK HTAB CREATE PANIC !\n";
static char gs_htabsp[] = "HACK HTAB SET PANIC !\n";

static void (*gs_vout)(XML_NODE*) = NULL;

static BFdesc *gs_bfout = NULL;

static char gs_tt[MAXINDENT];
//static const char gs_xml16tag[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";

static void XMLDriverXMLoutR(XML_NODE *ptr, int tidx)
{
	int i, valno = 1;

	//name

	bfwrite(gs_bfout, "\n", 1);
	bfwrite(gs_bfout, gs_tt, tidx);
	bfwrite(gs_bfout, "<", 1);
	bfwrite(gs_bfout, g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz);

	//attrs

	if(ptr->nattr) {
		for(i=0;i<ptr->nattr;i++) {
			bfwrite(gs_bfout, " ", 1);
			bfwrite(gs_bfout, g_xb_strptr[ptr->attr[i].key].str, g_xb_strptr[ptr->attr[i].key].siz);
			bfwrite(gs_bfout, "=\"", 2);
			bfwrite(gs_bfout, g_xb_strptr[ptr->attr[i].val].str, g_xb_strptr[ptr->attr[i].val].siz);
			bfwrite(gs_bfout, "\"", 1);
		}
	}
	bfwrite(gs_bfout, ">", 1);

	//val

	if(g_xb_strptr[ptr->val].siz) {
		valno = 0;
//		bfwrite(gs_bfout, "\n", 1);
//		bfwrite(gs_bfout, gs_tt, tidx+1);
//		bfwrite(gs_bfout, "\"", 1);
		bfwrite(gs_bfout, g_xb_strptr[ptr->val].str, g_xb_strptr[ptr->val].siz);
//		bfwrite(gs_bfout, "\"", 1);
	}

	//children

	if(ptr->nsubs) {
		valno = 1;
		for(i=0;i<ptr->nsubs;i++) {
//			fputc('\n',gs_fout);
			XMLDriverXMLoutR(ptr->subs[i],tidx+1);
		}
	}
	
	//close name
	if(valno) {
		bfwrite(gs_bfout, "\n", 1);
		bfwrite(gs_bfout, gs_tt, tidx);
	}
	bfwrite(gs_bfout, "</", 2);
	bfwrite(gs_bfout, g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz);
	bfwrite(gs_bfout, ">", 1);
}

static void XMLDriverXMLout(XML_NODE *root)
{
	bfwrite(gs_bfout, g_xml8tag, g_xml8taglen);
	XMLDriverXMLoutR(root,0);
	bfwrite(gs_bfout, "\n", 1);
}

static void XMLDriverTEXToutR(XML_NODE *ptr, int tidx)
{
	int i;

	//name

//	fputc('\n',gs_fout);
	bfwrite(gs_bfout, gs_tt, tidx);           
	bfwrite(gs_bfout, "N: ", 3);
	bfwrite(gs_bfout, g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz);

	//val

	if(g_xb_strptr[ptr->val].siz) {
		bfwrite(gs_bfout, "\n", 1);
		bfwrite(gs_bfout, gs_tt, tidx);
		bfwrite(gs_bfout, "+- v: ", 6);
		bfwrite(gs_bfout, g_xb_strptr[ptr->val].str, g_xb_strptr[ptr->val].siz);
	}

	//attrs

	if(ptr->nattr) {
		bfwrite(gs_bfout, "\n", 1);
		bfwrite(gs_bfout, gs_tt, tidx);
		bfwrite(gs_bfout, "+- a:", 6);
		for(i=0;i<ptr->nattr;i++) {
			bfwrite(gs_bfout, " ", 1);
			bfwrite(gs_bfout, g_xb_strptr[ptr->attr[i].key].str, g_xb_strptr[ptr->attr[i].key].siz);
			bfwrite(gs_bfout, "=\"", 2);
			bfwrite(gs_bfout, g_xb_strptr[ptr->attr[i].val].str, g_xb_strptr[ptr->attr[i].val].siz);
			bfwrite(gs_bfout, "\"", 1);
		}
	}

	//children

	if(ptr->nsubs) {
		for(i=0;i<ptr->nsubs;i++) {
			bfwrite(gs_bfout, "\n", 1);
			XMLDriverTEXToutR(ptr->subs[i],tidx+1);
		}
	}
}

static void XMLDriverTEXTout(XML_NODE *root)
{
	XMLDriverTEXToutR(root,0);
	bfwrite(gs_bfout, "\n", 1);
}

static uintptr_t DetermineType(const STR_NODE *ptr)
{
	char *tailptr;
	unsigned long long utmp;
	long long stmp;
//	double dtmp;

	if(!ptr->siz)
		return T_NULL;

	if(ptr->siz > 254)	//255 including null
		return T_LONGTEXT;

	if(ptr->siz > 31) //with good safe margin
		return T_VARCHAR + ptr->siz + 1;

	//unsigned first

	errno = 0; utmp = strtoull(ptr->str,&tailptr,10);
	if(tailptr != ptr->str && !errno && *tailptr == '\0') {
		if(utmp <= UCHAR_MAX)
			return T_UTINYINT;
		if(utmp <= USHRT_MAX)
			return T_USHRTINT;
		if(utmp <= UINT_MAX)
			return T_UINT;
		return T_ULLINT;
	}

	//signed ?

	errno = 0; stmp = strtoll(ptr->str,&tailptr,10);
	if(tailptr != ptr->str && !errno && *tailptr == '\0') {
		if(CHAR_MIN <= stmp && stmp <= CHAR_MAX)
			return T_TINYINT;
		if(SHRT_MIN <= stmp && stmp <= SHRT_MAX)
			return T_SHRTINT;
		if(INT_MIN <= stmp && stmp <= INT_MAX)
			return T_INT;
		return T_LLINT;
	}

	//float ?

	errno = 0; strtof(ptr->str,&tailptr);
	if(tailptr != ptr->str && !errno && *tailptr == '\0') {
			return T_FLOAT;
	}

	//all failed, return as varchar

	return T_VARCHAR + ptr->siz + 1;
}

/*
	convert to proper varchar
*/
static uintptr_t conv2vc(uintptr_t dat)
{
	uintptr_t val;
	if(dat >= T_VARCHAR)
		return dat;
	switch(dat) {
		case T_TINYINT:
			val = 4; break;
		case T_SHRTINT:
			val = 6; break;
		case T_INT:
			val = 11; break;
		case T_LLINT:
			val = 20; break;
		case T_UTINYINT:
			val = 3; break;
		case T_USHRTINT:
			val = 5; break;
		case T_UINT:
			val = 10; break;
		case T_ULLINT:
			val = 20; break;
		case T_FLOAT:
			val = 17; break;
		case T_NULL:
		default:
			val = 0;
	}

	return T_VARCHAR + 1 + val; //implicitly include \0 in strings (see switch with +1s above)
}

static void XMLDriverSQLinitR(XML_NODE *ptr, int lvl)
{
//	int ret = -1, fallback = 0, i;
	int i;
	uintptr_t cur, tmp, val;
//	static int ls_prefix = -1;

	if(ptr->nattr) //no attributes allowed
		fprintf(gs_sqllog, gs_err_nattrs , ptr->nattr, lvl);

	if(lvl == 3)
		return;	//currently deeper data is ignored

	//lvl 2 may have either subs, or values

	if(lvl<2) {
		if(g_xb_strptr[ptr->val].siz) //no value allowed at lvl 0 or 1
			fprintf(gs_sqllog, gs_err_lvl01val, g_xb_strptr[ptr->val].str);
		if(!ptr->nsubs)
			fprintf(gs_sqllog, gs_err_nosubs, lvl);
	} else if (lvl == 2) {
		if(ptr->nsubs && g_xb_strptr[ptr->val].siz)
			fprintf(gs_sqllog, gs_err_lvl2subandval);
	} else if (lvl == 3) {
		if(ptr->nsubs)
			fprintf(gs_sqllog, gs_err_subs, lvl);
	} else if (lvl >= 4) {
		fprintf(gs_sqllog, gs_err_lvl4, lvl);
	}

	switch(lvl) {
		case 0: //first level of assumed scheme
		case 1: //second level of assumed scheme
			if(ptr->nsubs) {
				for(i=0;i<ptr->nsubs;i++) {
					XMLDriverSQLinitR(ptr->subs[i], lvl+1);
				}
			}
			break;
		case 2:
			if(ptr->nsubs) {
				for(i=0;i<ptr->nsubs;i++) {
					XMLDriverSQLinitR(ptr->subs[i], lvl+1);
				}
			}
		case 3:
			cur = HashTest(gs_hashtype, ptr->name);
			if(cur == T_NOTFOUND) { //first insert
				HashSet(gs_hashtype, ptr->name, T_NULL);
				cur = T_NULL;
				//change name if desc
				if(g_xb_strptr[ptr->name].siz == 4 && *(uint32_t*)(g_xb_strptr[ptr->name].str) == 0x63736564) {
//				if(!(strcmp(g_xb_strptr[ptr->name].str,"desc"))) {
					g_xb_strptr[ptr->name].str = gs_desc_rename;
					g_xb_strptr[ptr->name].siz = sizeof(gs_desc_rename) - 1;
				}
			}

			//current is longtext => nothing to do
			if(cur == T_LONGTEXT) break;

			//determine new
			tmp = DetermineType(g_xb_strptr + ptr->val);

			//new is null => nothing to do
			if(tmp == T_NULL) break;

			//cur is null OR new is longtext => update to new
			if(cur == T_NULL || tmp == T_LONGTEXT) {
				HashSet(gs_hashtype, ptr->name, tmp);
				break;
			}

			//new OR cur is varchar => adjust length
			if(tmp >= T_VARCHAR || cur >= T_VARCHAR) {
				HashSet(gs_hashtype, ptr->name, MAX(conv2vc(tmp), conv2vc(cur)));
				break;
			}
/*
at this point ruled out:
	T_NULL (cur, tmp)
	T_LONGTEXT (cur, tmp)
	T_VARCHAR (cur,tmp)

*/
			val = gs_promotab[cur][tmp];
			if(!val)
				break;
			else if(val == T_VARCHAR)
				HashSet(gs_hashtype, ptr->name, MAX(conv2vc(tmp), conv2vc(cur)));
			else
				HashSet(gs_hashtype, ptr->name, val);
	}


//	ret = 0;
//EXCEPT_LABEL;
//	return ret;
}

static void XMLDriverSQLoutCTR(void)
{
	int ttype, i, len, comma = 0;
	char lenbuf[32];


	for(i=0;i<g_xb_strcnt;i++) {

		ttype = HashTest(gs_hashtype, i);

		if(ttype == T_NOTFOUND)
			continue;
		if (ttype == T_NULL) {
			fprintf(gs_sqllog, gs_err_nullcol, g_xb_strptr[i].str);
			continue;
		}

		if(comma)
			bfwrite(gs_bfout, ",\n", 2);

		bfwrite(gs_bfout, g_xb_strptr[i].str, g_xb_strptr[i].siz);
		bfwrite(gs_bfout, " ", 1);

		if(ttype >= T_VARCHAR) {
			len = snprintf(lenbuf,32,"%d",ttype - T_VARCHAR);
			bfwrite(gs_bfout, gs_tnametab[T_VARCHAR].str,gs_tnametab[T_VARCHAR].siz);
			bfwrite(gs_bfout, "(", 1);
			bfwrite(gs_bfout, lenbuf, len);
			bfwrite(gs_bfout, ")", 1);
		} else {
			bfwrite(gs_bfout, gs_tnametab[ttype].str,gs_tnametab[ttype].siz);
		}
		bfwrite(gs_bfout, " DEFAULT NULL", 13);
		comma = 1;
	}
	bfwrite(gs_bfout, "\n", 1);
}

static void XMLDriverSQLoutR(XML_NODE *ptr, int lvl)
{
	int ttype, i, comma, val;
	HTAB *htab;

	if(lvl == 0 && ptr->nsubs) {
		for(i=0;i<ptr->nsubs;i++) {
			XMLDriverSQLoutR(ptr->subs[i], lvl+1);
		}
	} else if(lvl == 1 && ptr->nsubs) {
		bfwrite(gs_bfout, "INSERT INTO ", 12);
		bfwrite(gs_bfout,g_xb_strptr[g_xb_root->name].str, g_xb_strptr[g_xb_root->name].siz);
		bfwrite(gs_bfout, " (", 2);
		comma = 0;
		if(!(htab = HashInit(2048))) {
			fputs(gs_htabcp,stderr);
			return;
		}
		for(i=0;i<ptr->nsubs;i++) {
			ttype = HashTest(gs_hashtype, ptr->subs[i]->name);
			if(ttype == T_NOTFOUND || ttype == T_NULL)
				continue;
			val = HashTest(htab, ptr->subs[i]->name);
			if(val == 1)
				continue;
			if(comma)
				bfwrite(gs_bfout, ", ", 2);
			bfwrite(gs_bfout,g_xb_strptr[ptr->subs[i]->name].str, g_xb_strptr[ptr->subs[i]->name].siz);
			if(HashSet(htab,ptr->subs[i]->name,1)) {
				fputs(gs_htabsp,stderr);
				return;
			}
			comma = 1;
		}
		bfwrite(gs_bfout, ") VALUES (", 10);

		comma = 0;
		if(!(htab = HashReinit(htab,2048))) {
			fputs(gs_htabcp,stderr);
			return;
		}

		for(i=0;i<ptr->nsubs;i++) {
			ttype = HashTest(gs_hashtype, ptr->subs[i]->name);
			if(ttype == T_NOTFOUND || ttype == T_NULL)
				continue;
			val = HashTest(htab, ptr->subs[i]->name);
			if(val == 1)
				continue;
			if(comma)
				bfwrite(gs_bfout, ", ", 2);

			if(!g_xb_strptr[ptr->subs[i]->val].siz)
				bfwrite(gs_bfout, "NULL", 4);
			else {
//				ttype = HashTest(gs_hashtype, ptr->subs[i]->name);
				if(ttype >= T_VARCHAR || ttype == T_LONGTEXT)
					bfwrite(gs_bfout, "'", 1);
				bfwrite(gs_bfout,g_xb_strptr[ptr->subs[i]->val].str, g_xb_strptr[ptr->subs[i]->val].siz);
				if(ttype >= T_VARCHAR || ttype == T_LONGTEXT)
					bfwrite(gs_bfout, "'", 1);
			}
			if(HashSet(htab,ptr->subs[i]->name,1)) {
				fputs(gs_htabsp,stderr);
				return;
			}
			comma = 1;
		}
		
		bfwrite(gs_bfout, ");\n", 3);
		HashDestroy(htab);
	}
}

static void XMLDriverSQLout(XML_NODE *root)
{

	XMLDriverSQLinitR(root, 0);

	bfwrite(gs_bfout,"CREATE TABLE ",13);
	bfwrite(gs_bfout,g_xb_strptr[root->name].str, g_xb_strptr[root->name].siz);
	bfwrite(gs_bfout, " (\n", 3);

	XMLDriverSQLoutCTR();

	bfwrite(gs_bfout, ") TYPE=MyISAM;\n\n", 16);

	XMLDriverSQLoutR(root,0);
//	bfwrite(gs_bfout, "\n", 2);
}

void XMLDriverDestroy(void)
{
	if(gs_hashtype)
		HashDestroy(gs_hashtype);
	if(gs_sqllog)
		fclose(gs_sqllog);
//	gs_hashtype = NULL;
//	gs_sqllog = NULL;
}

int XMLDriverInit(drvtype idx)
{
	int ret = -1;

	switch(idx) {
		case TEXT:
			gs_vout = &XMLDriverTEXTout; break;
		case XML:
			gs_vout = &XMLDriverXMLout; break;
		case SQL:
			if(!(gs_hashtype = HashInit(32768)))
				EXCEPT;
			if(!(gs_sqllog = fopen("sqllog.txt","wb")))
				EXCEPT;
			gs_vout = &XMLDriverSQLout; break;
		default:
			EXCEPT;
	}

	memset(gs_tt,'\t',MAXINDENT);

	ret = 0;
EXCEPT_LABEL;
	if(ret<0) {
		XMLDriverDestroy();
	}
	return ret;
}

int XMLDriverOut(BFdesc *fout)
{
	int ret = -1;

	//sanity check

	if(!g_xb_root || !g_xb_strptr) {
		fputs("XML tree not prepared ? o_O.\n",stderr);
		EXCEPT;
	}

	if(g_xb_indent  >= MAXINDENT) {
		fputs("Indentation too big.\n",stderr);
		EXCEPT;
	}

	gs_bfout = fout;
	gs_vout(g_xb_root);

	ret = 0;

EXCEPT_LABEL;

	return ret;
}





#if 0


			//update integers within respected domains, incl. T_FLOAT (float) if no accuracy is lost
			switch(cur) {
				case T_TINYINT:
				case T_SHRTINT:
				case T_INT:
					if( (tmp > cur && tmp <= T_LLINT) || (tmp == T_FLOAT && cur <= T_SHRTINT) ) {
						HashSet(gs_hashtype, ptr->name, tmp);
						goto done;
					}
					break;

				case T_UTINYINT:
				case T_USHRTINT:
				case T_UINT:
					if( (tmp > cur && tmp <= T_ULLINT) || (tmp == T_FLOAT && cur <= T_USHRTINT)) {
						HashSet(gs_hashtype, ptr->name, tmp);
						goto done;
					}
//					break;
			}
			//cross
			switch(cur) {
				case T_TINYINT:
				case T_SHRTINT:
				case T_INT:
				case T_LLINT:
					if( T_UTINYINT <= tmp && tmp <= T_UINT && tmp+4+1 > cur) {
						HashSet(gs_hashtype, ptr->name, tmp+4+1);
						goto done;
					}
					break;

				case T_UTINYINT:
				case T_USHRTINT:
				case T_UINT:
					if(tmp <= T_LLINT && cur-4+1 > tmp) // ???
						HashSet(gs_hashtype, ptr->name, cur-4+1);
						goto done;
					}
//					break;
			}
			//the rest goes to varchar
			HashSet(gs_hashtype, ptr->name, MAX(conv2vc(tmp), conv2vc(cur)));
	}
	done:;
#endif




#if 0
			switch(cur) {
				case T_TINYINT:
					switch(tmp) {
						case T_UTINYINT:
							HashSet(gs_hashtype, ptr->name, T_SHRTINT); break;
						case T_USHRTINT:
							HashSet(gs_hashtype, ptr->name, T_INT); break;
						case T_UINT:
							HashSet(gs_hashtype, ptr->name, T_LLINT); break;
						case T_ULLINT:
							HashSet(gs_hashtype, ptr->name, MAX(conv2vc(cur),conv2vc(tmp))); break;
						case T_TINYINT:
							break;
						default:
							HashSet(gs_hashtype, ptr->name, tmp); break;
					}; break;
				case T_SHRTINT:
					switch(tmp) {
						case T_USHRTINT:
							HashSet(gs_hashtype, ptr->name, T_INT); break;
						case T_UINT:
							HashSet(gs_hashtype, ptr->name, T_LLINT); break;
						case T_ULLINT:
							HashSet(gs_hashtype, ptr->name, MAX(conv2vc(cur),conv2vc(tmp))); break;
						case T_TINYINT:
						case T_UTINYINT:
						case T_SHRTINT:
							break;
						default:
							HashSet(gs_hashtype, ptr->name, tmp); break;
					}; break;
				case T_INT:
					switch(tmp) {
						case T_UINT:
							HashSet(gs_hashtype, ptr->name, T_LLINT); break;
						case T_ULLINT:
						case T_FLOAT:
							HashSet(gs_hashtype, ptr->name, MAX(conv2vc(cur),conv2vc(tmp))); break;
						case T_TINYINT:
						case T_UTINYINT:
						case T_SHRTINT:
						case T_USHRTINT:
						case T_INT:
							break;
						default: //T_LLINT
							HashSet(gs_hashtype, ptr->name, tmp); break;
					}; break;
				case T_LLINT:
					switch(tmp) {
						case T_TINYINT:
						case T_UTINYINT:
						case T_SHRTINT:
						case T_USHRTINT:
						case T_INT:
						case T_UINT:
						case T_LLINT:
							break;
						case T_ULLINT:
						case T_FLOAT:
						default:
							HashSet(gs_hashtype, ptr->name, MAX(conv2vc(cur),conv2vc(tmp))); break;
					}; break;
					
					
				case T_UTINYINT:
					switch(tmp) {
						case T_TINYINT:
							HashSet(gs_hashtype, ptr->name, T_SHRTINT); break;
						case T_UTINYINT:
							break;
						default:
							HashSet(gs_hashtype, ptr->name, tmp); break;
					}; break;
				case T_USHRTINT:
					switch(tmp) {
						case T_TINYINT:
						case T_SHRTINT:
							HashSet(gs_hashtype, ptr->name, T_INT); break;
						case T_UTINYINT:
						case T_USHRTINT:
							break;
						default:
							HashSet(gs_hashtype, ptr->name, tmp); break;
					}; break;
				case T_UINT:
					switch(tmp) {
						case T_TINYINT:
						case T_SHRTINT:
						case T_INT:
							HashSet(gs_hashtype, ptr->name, T_LLINT); break;
						case T_UTINYINT:
						case T_USHRTINT:
						case T_UINT:
							break;
						case T_FLOAT:
							HashSet(gs_hashtype, ptr->name, MAX(conv2vc(cur),conv2vc(tmp))); break;
						default: //T_ULLINT, T_LLINT
							HashSet(gs_hashtype, ptr->name, tmp); break;
					}; break;
				case T_ULLINT:
					switch(tmp) {
						case T_TINYINT:
						case T_SHRTINT:
						case T_INT:
						case T_LLINT:
							HashSet(gs_hashtype, ptr->name, MAX(conv2vc(cur),conv2vc(tmp))); break;
						case T_UTINYINT:
						case T_USHRTINT:
						case T_UINT:
						case T_ULLINT:
							break;
						case T_FLOAT:
						default:
							HashSet(gs_hashtype, ptr->name, MAX(conv2vc(cur),conv2vc(tmp))); break;
					}; break;
				case T_FLOAT:
					switch(tmp) {
						case T_TINYINT:
						case T_SHRTINT:
						case T_UTINYINT:
						case T_USHRTINT:
						case T_FLOAT:
							break;
						default:
							HashSet(gs_hashtype, ptr->name, MAX(conv2vc(cur),conv2vc(tmp))); break;
					}; break;
				case T_VARCHAR:
					HashSet(gs_hashtype, ptr->name, MAX(cur,conv2vc(tmp))); break;
			}
#endif

#if 0
static void XMLDriverSQLoutCTR(XML_NODE *ptr, int lvl)
{
	int ttype, i, len;
	char lenbuf[32];

	if(lvl == 0 && ptr->nsubs) {
		for(i=0;i<ptr->nsubs;i++) { //yes walking all over is needed
			XMLDriverSQLoutCTR(ptr->subs[0], lvl+1);
		}
	} else if(lvl == 1 && ptr->nsubs) {
		for(i=0;i<ptr->nsubs;i++) {
			if(i)
				bfwrite(gs_bfout, ",\n", 2);
			XMLDriverSQLoutCTR(ptr->subs[i], lvl+1);
		}
		bfwrite(gs_bfout, "\n", 1);
	} else {

		bfwrite(gs_bfout, g_xb_strptr[ptr->name].str, g_xb_strptr[ptr->name].siz);
		bfwrite(gs_bfout, " ", 1);

		ttype = HashTest(gs_hashtype, ptr->name);

		if(ttype >= T_VARCHAR) {
			len = snprintf(lenbuf,32,"%d",ttype - T_VARCHAR);
			bfwrite(gs_bfout, gs_tnametab[T_VARCHAR].str,gs_tnametab[T_VARCHAR].siz);
			bfwrite(gs_bfout, "(", 1);
			bfwrite(gs_bfout, lenbuf, len);
			bfwrite(gs_bfout, ")", 1);
		} else if (ttype == T_NULL) {
			fprintf(gs_sqllog, gs_err_nullcol, g_xb_strptr[ptr->name].str);
		} else {
			bfwrite(gs_bfout, gs_tnametab[ttype].str,gs_tnametab[ttype].siz);
		}

	}
}

#endif
