/****************************************************************************
*                                                                          *
*     Loki - Programs for genetic analysis of complex traits using MCMC    *
*                                                                          *
*                   Simon Heath - CNG, Evry                                *
*                                                                          *
*                        January 2003                                      *
*                                                                          *
* read_xmlfile.c:                                                          *
*                                                                          *
* Read in the XML datafile produced by prep or other front-ends            *
*                                                                          *
* Copyright (C) Simon C. Heath 2003                                        *
* This is free software.  You can distribute it and/or modify it           *
* under the terms of the Modified BSD license, see the file COPYING        *
*                                                                          *
****************************************************************************/

#include <config.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <assert.h>

#include "utils.h"
#include "libhdr.h"
#include "loki.h"
#include "sparse.h"
#include "loki_peel.h"
#include "loki_utils.h"
#include "loki_compress.h"
#include "lk_malloc.h"
#include "xml.h"
#include "locus.h"

#define link my_link

#define MREC_FLAG 0x8000
#define GREC_FLAG 0x10000

static struct id_data *iddata,**iddata_p;
static int line,*recode,recode_size,n_id,current_comp=-1,current_population=0;

typedef struct vvar {
	struct vvar *next;
	char *name;
	char *levels;
	char *flags;
	char *parent;
	union {
		struct Variable *var;
		struct Marker *mark;
		struct TraitLocus *tlocus;
	} ref;
	int type;
	int idx;
} var;

typedef struct rec {
	struct rec *next;
	char *data;
	int id;
} record;

typedef struct {
	char *name;
	struct bin_node *var_list;
	var *var_list1;
	record *rec_list;
	int type;
	int count;
} grp;

typedef struct lnk_var {
	struct lnk_var *next;
	var *vv;
	char *rs_id;
	char *cng_id;
	char *mid;
	char *chrom_seg;
	double phys_pos;
	double pos[3];
	int pos_set[3];
	
} l_var;

typedef struct {
	char *name;
	l_var *v_list;
	int type;
} lnk;

typedef struct modterm {
	struct modterm *next;
	struct Model_Var *vars;
	int ran_flag;
	int n_var;
} model_term;

typedef struct mvar {
	struct mvar *next;
	struct Model_Var var;
} mod_var;

typedef struct mod {
	struct mod *next;
	model_term *terms;
	mod_var *vars;
} model;

static struct loki *loki;
static char *infile,**logfile;
static int unknown_depth,state,err_ct,c_grp,sorted_records,sorted_ped,element_stack_size=16,*element_stack,element_stack_ptr;
static int fam_array_size,comp_array_size,id_array_size,grp_array_size,n_grps,lnk_array_size;
static grp *grps;
static lnk *lnks;
static var *vlist;
static model *modlist;
static record *c_rec;
static char *var_types[]={"factor","real","integer",
	"microsat","alleles","ord_alleles","gen_locus","snp",
	"ind","father","mother","group","sex","affected","loki_qtl","trait_locus","super_locus",
	0};


static void comp_func(const args *);
static void data_func(const args *);
static void family_func(const args *);
static void population_func(const args *);
static void group_func(const args *);
static void ind_func(const args *);
static void link_func(const args *);
static void locus_func(const args *);
static void log_func(const args *);
static void loki_func(const args *);
static void model_func(const args *);
static void rec_func(const args *);
static void term_func(const args *);
static void trait_func(const args *);
static void var_func(const args *);
static void close_loki(void);
static void close_comp(void);

/* The following section generated automatically by readdtd.pl               */
/* The steps to create this are:                                             */
/*               dtdparse -o temp.xml loki.dtd                               */
/*               ./readdtd.pl temp.xml > temp.c                              */

#define CLOSE -1
#define CENS 0
#define COMP 1
#define DATA 2
#define FAMILY 3
#define GROUP 4
#define IND 5
#define LEVELS 6
#define LINK 7
#define LOC_ATTR 8
#define LOCUS 9
#define LOG 10
#define LOKI 11
#define MODEL 12
#define POPULATION 13
#define QC 14
#define REC 15
#define TERM 16
#define TRAIT 17
#define VAR 18

#define __END_STAGE 34
#define __ERR_STAGE 35

static char *elements[]={
	"cens","comp","data","family","group","ind","levels",
	"link","loc_attr","locus","log","loki","model","population","qc",
	"rec","term","trait","var",
};

static void (*start_element_funcs[])(const args *)={
	0, /* cens element */
	comp_func, /* comp element */
	data_func, /* data element */
	family_func, /* family element */
	group_func, /* group element */
	ind_func, /* ind element */
	0, /* levels element */
	link_func, /* link element */
	0, /* loc_attr element */
	locus_func, /* locus element */
	log_func, /* log element */
	loki_func, /* loki element */
	model_func, /* model element */
	population_func, /* population element */
	0, /* qc element */
	rec_func, /* rec element */
	term_func, /* term element */
	trait_func, /* trait element */
	var_func, /* var element */
};

static void (*end_element_funcs[])(void)={
	0, /* cens element */
	close_comp, /* comp element */
	0, /* data element */
	close_comp, /* family element */
	0, /* group element */
	0, /* ind element */
	0, /* levels element */
	0, /* link element */
	0, /* loc_attr element */
	0, /* locus element */
	0, /* log element */
	close_loki, /* loki element */
	0, /* model element */
	close_comp, /* population element */
	0, /* qc element */
	0, /* rec element */
	0, /* term element */
	0, /* trait element */
	0, /* var element */
};

/* For each state, give the starting offset into goto_tab1[] and goto_tab2[] */
static int goto_tab[]={
	0,1,9,10,17,21,24,26,27,28,30,31,32,35,37,38,39,41,42,44,
	45,49,51,53,54,56,57,60,61,63,64,65,68,69,71,71,71
};

/* List possible moves from each state */
static int goto_tab1[]={
	LOKI, /* 0: 0 */
	CLOSE,DATA,FAMILY,GROUP,LINK,LOG,MODEL,POPULATION, /* 1: 1 */
	CLOSE, /* 2: 9 */
	CLOSE,DATA,FAMILY,GROUP,LINK,MODEL,POPULATION, /* 3: 10 */
	CLOSE,COMP,FAMILY,IND, /* 4: 17 */
	CLOSE,COMP,IND, /* 5: 21 */
	CLOSE,IND, /* 6: 24 */
	CLOSE, /* 7: 26 */
	CLOSE, /* 8: 27 */
	CLOSE,IND, /* 9: 28 */
	CLOSE, /* 10: 30 */
	CLOSE, /* 11: 31 */
	CLOSE,COMP,IND, /* 12: 32 */
	CLOSE,IND, /* 13: 35 */
	CLOSE, /* 14: 37 */
	CLOSE, /* 15: 38 */
	CLOSE,VAR, /* 16: 39 */
	CLOSE, /* 17: 41 */
	CLOSE,LEVELS, /* 18: 42 */
	CLOSE, /* 19: 44 */
	CLOSE,DATA,LINK,MODEL, /* 20: 45 */
	CLOSE,LOCUS, /* 21: 49 */
	CLOSE,LOC_ATTR, /* 22: 51 */
	CLOSE, /* 23: 53 */
	CLOSE,REC, /* 24: 54 */
	CLOSE, /* 25: 56 */
	CLOSE,CENS,QC, /* 26: 57 */
	CLOSE, /* 27: 60 */
	CLOSE,MODEL, /* 28: 61 */
	TRAIT, /* 29: 63 */
	CLOSE, /* 30: 64 */
	CLOSE,TERM,TRAIT, /* 31: 65 */
	CLOSE, /* 32: 68 */
	CLOSE,TERM, /* 33: 69 */
	/* DUMMY STAGE  34: 71 */
	/* DUMMY STAGE  35: 71 */
};

/*Same format as goto_tab1[], but lists which states to move to */
static int goto_tab2[]={
	1,
	34,24,12,16,21,2,29,4,
	3,
	34,24,12,16,21,29,4,
	3,9,5,11,
	4,6,8,
	5,7,
	6,
	5,
	4,10,
	9,
	4,
	3,13,15,
	12,14,
	13,
	12,
	3,18,
	16,
	16,19,
	17,
	34,24,21,29,
	20,22,
	21,23,
	22,
	20,26,
	24,
	24,25,27,
	26,
	34,29,
	30,
	31,
	28,32,30,
	33,
	28,32,
};

/* End of automatically generated section                                    */

static int cmp_var(const void *p1,const void *p2)
{
	const var *v1,*v2;
	
	v1=p1;
	v2=p2;
	return strcmp(v1->name,v2->name);
}

static struct bin_node *new_var(const void *p)
{
	struct bin_node *nd;
	
	nd=lk_malloc(sizeof(struct bin_node));
	nd->left=nd->right=0;
	nd->balance=0;
	nd->data=(void *)p;
	return nd; 
}

static var *find_var(char *s,int *g)
{
	int i;
	var *vv=0,v;
	struct bin_node *nd;
	
	v.name=s;
	for(i=0;i<n_grps;i++) {
		nd=grps[i].var_list;
		if(nd) {
			nd=find_bin_node(nd,&v,cmp_var);
			if(nd) {
				vv=nd->data;
				if(g) *g=i;
				break;
			}
		}
	}
	return vv;
}

#define XFE(a,b) XMLFileError(__FILE__,__LINE__,a,b)
static void XMLFileError(const char *sfile,const int ln,const char *file,const char *s)
{
	(void)fprintf(stderr,"[%s:%d] Error reading from file '%s'\n",sfile,ln,file);
	if(s) (void)fprintf(stderr,"%s\n",s);
	if(errno) perror("loki");
	from_abt=1;
	exit(EXIT_FAILURE);
}

static void change_state(int i)
{
	int j,k,k1;
	
	k=goto_tab[state+1];
	k1=-2;
	for(j=goto_tab[state];j<k;j++) {
		k1=goto_tab1[j];
		if(k1>=i) break;
	}
	if(k1!=i) {
		if(state!=__ERR_STAGE) {
			if(i>=0)	fprintf(stderr,"Error at line %d: Starting element '%s' while in state %d\n",line+1,elements[i],state);
			else fprintf(stderr,"Error at line %d: Ending element while in state %d\n",line+1,state);
		}
		state=__ERR_STAGE;
	} else state=goto_tab2[j];
}

static var *link_vars(struct bin_node *nd,var *v)
{
	var *vv;
	
	if(nd->left) v=link_vars(nd->left,v);
	vv=nd->data;
	vv->next=v;
	v=vv;
	if(nd->right) v=link_vars(nd->right,v);
	return v;
}				

static void sort_records(void)
{
	int i,j,k,k1,k2,k3,*nrec,mrec_flag=0,grec_flag,gflag,*mk_ix,id,rec=0,er;
	int n_markers,n_id_records,n_nonid_records,id1=0;
	record *rc,*rc1;
	var *vv;
	l_var *lv,*lv1;
	char *p1;
	tokens *tok=0;
	struct Variable *v;
	struct Locus *loc;
	struct id_data *data;
	struct Id_Record *id_array;
	struct Link *link;
	struct Marker *mark;
	struct bin_node *nd;
	
	if(err_ct) {
		state=__ERR_STAGE;
		return;
	}
	loki->models->qtl_name=loki->models->tl_name=0;
	id_array=loki->pedigree->id_array;
	sorted_records=1;
	/* Count records and variable types; check for multiple records */
	nrec=lk_malloc(sizeof(int)*loki->pedigree->ped_size);
	k2=k3=0;
	n_markers=n_id_records=n_nonid_records=0;
	for(i=0;i<loki->pedigree->ped_size;i++) id_array[i].flag=id_array[i].proband=id_array[i].n_gt_sets=0;
	for(i=0;i<n_grps;i++) {
		rc=grps[i].rec_list;
		if(rc) {
			/* Variables with data */
			k=0; 
			gflag=0;
			nd=grps[i].var_list;
			if(nd) {
				vv=link_vars(nd,0);
				grps[i].var_list1=vv;
				while(vv) {
					if(!vv->type) ABT_FUNC("Internal error - untyped variable\n");
					if(vv->type>8) abt(__FILE__,__LINE__,"%s(): Error - variable '%s' of type '%s' can not have data\n",__func__,vv->name,var_types[vv->type-1]);
					if(vv->type<4) k++;
					else {
						gflag=1;
						n_markers++;
					}
					vv=vv->next;
				}
			}
			memset(nrec,0,sizeof(int)*loki->pedigree->ped_size);
			k1=0;
			while(rc) {
				nrec[rc->id-1]++;
				k1++;
				if(k) id_array[rc->id-1].flag=1;
				rc=rc->next;
			}
			if(gflag) grps[i].type|=GREC_FLAG;
			for(j=0;j<loki->pedigree->ped_size;j++) if(nrec[j]>1) break;
			if(j<loki->pedigree->ped_size) {
				if(k) {
					if(mrec_flag&1) ABT_FUNC("Error - multiple variable groups with multiple records\n");
					mrec_flag|=1;
					grps[i].type|=MREC_FLAG;
					for(j=0;j<loki->pedigree->ped_size;j++) id_array[j].n_rec=nrec[j];
					n_nonid_records+=k;
					k3+=k1;
				}
				if(gflag) {
					if(mrec_flag&2) ABT_FUNC("Error - multiple genotype groups with multiple records\n");
					mrec_flag|=2;
					for(j=0;j<loki->pedigree->ped_size;j++) {
						if(id_array[j].n_gt_sets) {
							assert(id_array[j].n_gt_sets==1);
							if(nrec[j]) id_array[j].n_gt_sets=nrec[j];
						} else id_array[j].n_gt_sets=nrec[j];
					}
				}
			} else {
				n_id_records+=k;
				if(gflag) {
					for(j=0;j<loki->pedigree->ped_size;j++) {
						if(!id_array[j].n_gt_sets) id_array[j].n_gt_sets=nrec[j];
					}
				}
			}
			k2+=k1*k;
		} else {
			nd=grps[i].var_list;
			vv=nd?link_vars(nd,0):0;
			grps[i].var_list1=vv;
			while(vv) {
				switch(vv->type) {
					case 12:
						tok=tokenize(vv->levels,0,tok);
						j=tok?tok->n_tok:0;
						if(!j) {
							fputs("No levels found for group variable\n",stderr);
							ABT_FUNC(AbMsg);
						}
							loki->pedigree->n_genetic_groups=j;
						loki->pedigree->group_recode.flag=ST_STRING;
						loki->pedigree->group_recode.recode=lk_malloc(sizeof(union arg_type)*j);
						for(j=0;j<tok->n_tok;j++) loki->pedigree->group_recode.recode[j].string=tok->toks[j];
							break;
					case 15:
						if(loki->models->qtl_name) {
							fprintf(stderr,"Error - multiple QTLs specified\n");
							ABT_FUNC(AbMsg);
						}
						loki->models->qtl_name=vv->name;
						break;
					case 16:
						if(loki->models->tl_name) {
							fprintf(stderr,"Error - multiple TLs specified\n");
							ABT_FUNC(AbMsg);
						}
						loki->models->tl_name=vv->name;
						break;
					case 17:
						n_markers++;
						break;
				}
				vv=vv->next;
			}
		}
	}
	/* Set up storage for permanent data records */
	if(k2) {
		iddata=lk_malloc(sizeof(struct id_data)*k2);
		loki->sys.RemBlock=AddRemem(iddata,loki->sys.RemBlock);
		if(k3) {
			iddata_p=lk_malloc(sizeof(void *)*k3);
			loki->sys.RemBlock=AddRemem(iddata_p,loki->sys.RemBlock);
		}
	}
	if(n_markers) {
		loki->markers->marker=lk_calloc((size_t)n_markers,sizeof(struct Marker));
		for(i=0;i<n_markers;i++) {
			loki->markers->marker[i].parent=0;
			loki->markers->marker[i].children=0;
		}
		n_markers=0;
	}
	/* Storage for genotype data */
	for(k1=i=0;i<loki->pedigree->ped_size;i++) k1+=id_array[i].n_gt_sets;
	if(k1) {
		int *tmp_p;
		tmp_p=lk_malloc(sizeof(int)*k1);
		loki->sys.RemBlock=AddRemem(tmp_p,loki->sys.RemBlock);
		for(k1=i=0;i<loki->pedigree->ped_size;i++) {
			k=id_array[i].n_gt_sets;
			if(k) {
				id_array[i].gt_idx=tmp_p+k1;
				for(k2=0;k2<k;k2++,k1++) tmp_p[k1]=k1;
			} else id_array[i].gt_idx=0;
		}
	}
	loki->markers->n_id_recs=k1;
	/* Storage for phenotype variables */
	if(n_id_records+n_nonid_records) {
		loki->data->id_variable=lk_malloc(sizeof(struct Variable)*(n_id_records+n_nonid_records));
		if(n_nonid_records) loki->data->nonid_variable=loki->data->id_variable+n_id_records;
		if(!n_id_records) loki->data->id_variable=0;
		n_id_records=n_nonid_records=0;
	}
	/* Set up 'super-loci' */
	for(i=0;i<n_grps;i++) {
		vv=grps[i].var_list1;
		while(vv) {
			if(vv->type==17) {
				loki->markers->marker[n_markers].name=vv->name;
				vv->ref.mark=loki->markers->marker+n_markers;
				loc=&loki->markers->marker[n_markers++].locus;
				vv->ref.mark->recode=0;
				loc->n_alleles=0;
			}
			vv=vv->next;
		}
	}
	k1=n_markers;
	/* Set up recode information for factors */
	for(i=0;i<n_grps;i++) {
		if(!grps[i].rec_list) continue;
		vv=grps[i].var_list1;
		while(vv) {
			if(vv->type<4) {
				if(grps[i].type&MREC_FLAG) {
					v=loki->data->nonid_variable+n_nonid_records;
					v->index=n_nonid_records++;
				} else {
					v=loki->data->id_variable+n_id_records;
					v->index=n_id_records++;
				}
				vv->ref.var=v;
				v->name=vv->name;
				if(vv->type==1) {
					v->type=ST_FACTOR;
					tok=tokenize(vv->levels,0,tok);
					j=tok?tok->n_tok:0;
					v->n_levels=j;
					if(j) {
						v->recode=lk_malloc(sizeof(union arg_type)*j);
						v->rec_flag=ST_STRING;
						for(j=0;j<tok->n_tok;j++) v->recode[j].string=tok->toks[j];
					} else v->recode=0;
				} else {
					v->type=vv->type==2?ST_REAL:ST_INTEGER;
					v->recode=0;
					v->n_levels=v->rec_flag=0;
				}
				if(vv->flags) {
					tok=tokenize(vv->flags,',',tok);
					if(tok) {
						for(j=0;j<tok->n_tok;j++) if(!strcmp(tok->toks[j],"censored")) break;
						if(j<tok->n_tok) {
							v->type|=ST_CENSORED;
						}
					}
				}
			} else { /* Markers */
				if(vv->type!=4 && vv->type!=8) abt(__FILE__,__LINE__,"%s(): Marker type (%d) not currently handled\n",__func__,vv->type);
				loki->markers->marker[n_markers].name=vv->name;
				vv->ref.mark=loki->markers->marker+n_markers;
				loc=&loki->markers->marker[n_markers++].locus;
				tok=tokenize(vv->levels,0,tok);
				j=tok?tok->n_tok:0;
				vv->ref.mark->marker_type=vv->type==4?MARKER_MICROSAT:MARKER_SNP;
				if(j) {
					if(vv->ref.mark->marker_type==MARKER_MICROSAT) j++; /* Add extra allele to account for unobserved alleles */
					else {
						if(j>2) {
							fprintf(stderr,"Too many alleles (%d) found for SNP locus %s\n",j,vv->name);
							ABT_FUNC("Aborting\n");
						}
						j=2;
					}
					loc->n_alleles=j; 
					vv->ref.mark->recode=lk_malloc(sizeof(void *)*j);
					for(j=0;j<tok->n_tok;j++) vv->ref.mark->recode[j]=tok->toks[j];
					vv->ref.mark->recode[j]=0;
				} else {
					vv->ref.mark->recode=0;
					loc->n_alleles=0;
				}
				/* Check if marker is member of super-locus */
				if(vv->parent) {
					for(j=0;j<k1;j++) {
						if(!strcmp(loki->markers->marker[j].name,vv->parent)) {
							loki->markers->marker[j].n_children++;
							loki->markers->marker[n_markers-1].parent=loki->markers->marker+j;
							break;
						}
					}
					if(j==k1) {
						fprintf(stderr,"Super-locus %s not found for marker %s\n",vv->parent,vv->name);
						ABT_FUNC("Aborting\n");
					}
				}
			}
			vv=vv->next;
		}
	}
	/* More super-locus initialization */
	if(k1) {
		for(i=0;i<k1;i++) {
			if((j=loki->markers->marker[i].n_children)) {
				loki->markers->marker[i].children=lk_malloc(sizeof(void *)*j);
				loki->markers->marker[i].child_flag=lk_malloc(sizeof(int)*j);
				loki->markers->marker[i].n_children=0;
				loki->markers->marker[i].marker_type=MARKER_SUPER;
			} else {
				fprintf(stderr,"No child markers found for super-locus %s\n",loki->markers->marker[i].name);
				ABT_FUNC("Aborting\n");
			}
		}
		for(;i<n_markers;i++) if((mark=loki->markers->marker[i].parent))
			mark->children[mark->n_children++]=loki->markers->marker+i;
	}
	/* Initialize storage for markers */
	for(i=0;i<n_markers;i++) alloc_marker(i,loki);
	/* Set up linkage groups */
	if(loki->markers->n_links) {
		loki->markers->linkage=lk_malloc(sizeof(struct Link)*loki->markers->n_links);
		for(k=j=0;j<loki->markers->n_links;j++) {
			loki->markers->linkage[j].real_chr=0;
			lv=lnks[j].v_list;
			k1=0;
			while(lv) {
				k1++;
				lv=lv->next;
			}
			loki->markers->linkage[j].n_markers=k1;
			k+=k1;
		}
		if(k) {
			mk_ix=lk_malloc(sizeof(int)*k);
			loki->sys.RemBlock=AddRemem(mk_ix,loki->sys.RemBlock);
		} else mk_ix=0;
		er=0;
		for(j=0;j<loki->markers->n_links;j++) {
			link=loki->markers->linkage+j;
			lv=lnks[j].v_list;
			if(lv) {
				link->mk_index=mk_ix;
				while(lv) {
					mark=lv->vv->ref.mark;
					mark->locus.link_group=j;
					for(k1=0;k1<mark->n_children;k1++) mark->children[k1]->locus.link_group=j;
					k1=0;
					if(lv->pos_set[2]) {
						mark->pos_set=1;
						mark->locus.pos[0]=mark->locus.pos[1]=lv->pos[2];
						k1=3;
					} else {
						for(k1=k=0;k<2;k++) {
							if(lv->pos_set[k]) {
								mark->pos_set=1;
								mark->locus.pos[k]=lv->pos[k];
								k1|=(1<<k);
							}
						}
					}
					switch(link->type) {
						case LINK_AUTO:
							if(k1 && k1!=3) {
								fprintf(stderr,"Error: Only %s map position for marker %s is given\n",k1==1?"female":"male",mark->name);
								er=1;
							}
							break;
						case LINK_X:
							if(k1 && !(k1&1)) {
								fprintf(stderr,"Error: Female map position for marker %s is not given\n",mark->name);
								er=1;
							}
							break;
					}
					mark->rs_id=lv->rs_id;
					mark->cng_id=lv->cng_id;
					mark->mid=lv->mid;
					mark->locus.chrom_seg=lv->chrom_seg;
					mark->locus.phys_pos=lv->phys_pos;
					*mk_ix++=mark->locus.index;
					lv1=lv->next;
					free(lv);
					lv=lv1;
				}
			} else link->mk_index=0;
			link->range_set[0]=link->range_set[1]=0;
			link->name=lnks[j].name;
			link->ibd_list=0;
			link->ibd_est_type=0;
			link->type=lnks[j].type;
		}
		if(j) free(lnks);
		lnks=0;
		if(er) ABT_FUNC(AbMsg);
	}
	for(i=er=0;i<n_markers;i++) {
		if(loki->markers->marker[i].locus.link_group<0) {
			fprintf(stderr,"Error - marker %s has no linkage group\n",loki->markers->marker[i].name);
			er=1;
		}
	}
	/* We abort in this case, because this indicates an error in the XML file */
	if(er) ABT_FUNC(AbMsg);
	/* Copy data records into permanent locations */
	for(i=0;i<loki->pedigree->ped_size;i++) {
		j=id_array[i].n_rec;
		if(j) {
      id_array[i].data1=iddata_p;
			iddata_p+=j;
			id_array[i].n_rec=0;
		} else id_array[i].data1=0;
		if(n_id_records && id_array[i].flag) {
			id_array[i].data=iddata;
			iddata+=n_id_records;
		} else id_array[i].data=0;
	}
	for(i=0;i<n_grps;i++) {
		rc=grps[i].rec_list;
		mrec_flag=grps[i].type&MREC_FLAG;
		grec_flag=grps[i].type&GREC_FLAG;
		memset(nrec,0,sizeof(int)*loki->pedigree->ped_size);
		while(rc) {
			id=rc->id-1;
			if(mrec_flag) rec=id_array[id].n_rec++;
			if(grec_flag) id1=id_array[id].gt_idx[nrec[id]];
			tok=tokenize(rc->data,',',tok);
			if(tok) {
				vv=grps[i].var_list1;
				while(vv) {
					j=vv->idx;
					if(j>=tok->n_tok) k=0;
					else k=(*tok->toks[j])?1:0;
					switch(vv->type) {
						case 1: /* factor */
						case 2: /* real */
						case 3: /* integer */
							v=vv->ref.var;
							k1=v->index;
							if(mrec_flag) {
								data=id_array[id].data1[k1]+rec;
							} else data=id_array[id].data+k1;
							data->flag=0;
							data->data.value=0;
							if(k && *tok->toks[j]=='&') {
								if(*++(tok->toks[j])) data->flag=2;
								else k=0;
							}
								if(k) {
								if(vv->type==2) {
									data->data.rvalue=strtod(tok->toks[j],&p1);
									data->flag|=ST_REALTYPE;
								} else {
									data->data.value=(int)strtol(tok->toks[j],&p1,10);
									data->flag|=ST_INTTYPE;
								}
								if(*p1) {
									fprintf(stderr,"Bad numeric data '%s' for variable '%s'\n",tok->toks[j],v->name);
									ABT_FUNC(AbMsg);
								}
								data->flag|=1;
							}
							break;
					case 4: /* Microsatellite genotypes */
					case 8: /* SNPs */
						if(k) {
							k1=(int)strtol(tok->toks[j],&p1,10);
							if(*p1) {
								fprintf(stderr,"Bad numeric data '%s' for marker '%s'\n",tok->toks[j],vv->name);
								ABT_FUNC(AbMsg);
							}
							k2=vv->ref.mark->locus.n_alleles;
							k2=k2*(k2+1)/2;
							vv->ref.mark->orig_gt[id1]=k1;
							if(!nrec[id]) vv->ref.mark->haplo[id]=k1;
							else {
								k2=vv->ref.mark->haplo[id];
								if(k2) {
									if(k1 && k1!=k2) {
										if(k2>0) vv->ref.mark->haplo[id]=-1;
									}
								} else vv->ref.mark->haplo[id]=k1;
							}
						}
						break;
					default:
						ABT_FUNC("Unsupported var type\n");
						break;
				}
					vv=vv->next;
			}
      } 
      rc1=rc->next;
      if(rc->data) free(rc->data);
      free(rc);
      rc=rc1;
      if(grec_flag) nrec[id]++;
	}
}
free(nrec);
free(recode);
if(tok) free_tokens(tok);
for(j=0;j<n_markers;j++) {
	mark=loki->markers->marker+j;
	if(mark->haplo) {
		for(i=0;i<loki->pedigree->ped_size;i++) if(mark->haplo[i]<0) mark->haplo[i]=0;
	}
}
loki->markers->n_markers=n_markers;
loki->data->n_id_records=n_id_records;
loki->data->n_nonid_records=n_nonid_records;
}

static const args *get_single_attr(const args *attr,char *name,char *element)
{
	while(attr) {
		if(!strcmp(name,attr->name)) break;
		attr=attr->next;
	}
	if(!attr && element) {
		fprintf(stderr,"Error: Missing %s attributeute for %s element\n",name,element);
		err_ct++;
	}
	return attr;
}

static int get_int_attr(const args *attr,int *i,unsigned int mask,int low,int high,const char *name)
{
	int er=0,k;
	char *p;
	
	k=(int)strtol(attr->att,&p,10);
	if(*p) er=1;
	else {
		if((mask&1)&&(k<low)) er=1;
		else if((mask&2)&&(k>high)) er=1;
	}
	if(er) {
		fprintf(stderr,"Error: bad %s attribute '%s' for %s element\n",attr->name,attr->att,name);
		err_ct++;
		exit(0);
	} else *i=k;
	return er;
}

static int get_double_attr(const args *attr,double *x,unsigned int mask,double low,double high,const char *name)
{
	int er=0;
	double y;
	char *p;
	
	y=strtod(attr->att,&p);
	if(*p) er=1;
	else {
		if((mask&1)&&(y<low)) er=1;
		else if((mask&2)&&(y>high)) er=1;
	}
	if(er) {
		fprintf(stderr,"Error: bad %s attribute '%s' to %s element\n",attr->name,attr->att,name);
		err_ct++;
	} else *x=y;
	return er;
}

static void loki_func(const args *attr)
{
	attr=get_single_attr(attr,"program",0);
	if(attr) message(INFO_MSG,"File generated by %s\n",attr->att);
}

static void log_func(const args *attr)
{
	size_t i;
	
	attr=get_single_attr(attr,"file","log");
	if(attr) {
		assert(attr->att);
		i=strlen(attr->att);
		*logfile=lk_malloc(i+1);
		(void)strncpy(*logfile,attr->att,i+1);
	}
}

static void population_func(const args *attr)
{
	size_t i;
	char *p;
	struct population *pop;
	
	attr=get_single_attr(attr,"name",0);
	if(attr) {
		assert(attr->att);
		pop=loki->pedigree->populations;
		while(pop) {
			if(!strcmp(attr->att,pop->name)) break;
			pop=pop->next;
		}
		if(!pop) {
			pop=lk_malloc(sizeof(struct population));
			pop->next=loki->pedigree->populations;
			loki->pedigree->populations=pop;
			pop->name=strdup(attr->att);
			pop->idx=++loki->pedigree->n_pops;
			attr=get_single_attr(attr,"ethnicity",0);
			if(attr) {
				assert(attr->att);
				pop->ethnicity=strdup(attr->att);
			} else pop->ethnicity=0;
		}
		i=strlen(attr->att);
		p=lk_malloc(i+1);
		(void)strncpy(p,attr->att,i+1);
		current_population=pop->idx;
	} else current_population=0;
	comp_func(0);
}

static void family_func(const args *attr)
{
	size_t i;
	char *p;
	
	attr=get_single_attr(attr,"orig",0);
	if(attr) {
		if(loki->pedigree->n_orig_families && !loki->pedigree->family_id) {
			fputs("Error: Multiple families with no original id codes\n",stderr);
			err_ct++;
		}
		if(!loki->pedigree->fam_recode.recode) {
			fam_array_size=32;
			loki->pedigree->fam_recode.recode=lk_malloc(sizeof(union arg_type)*fam_array_size);
			loki->pedigree->fam_recode.flag=ST_STRING;
		} else if(loki->pedigree->n_orig_families==fam_array_size) {
			fam_array_size<<=1;
			loki->pedigree->fam_recode.recode=lk_realloc(loki->pedigree->fam_recode.recode,sizeof(union arg_type)*fam_array_size);
		}
		loki->pedigree->family_id=1;
		assert(attr->att);
		i=strlen(attr->att);
		p=lk_malloc(i+1);
		(void)strncpy(p,attr->att,i+1);
		loki->pedigree->fam_recode.recode[loki->pedigree->n_orig_families].string=p;
	} else {
		if(loki->pedigree->family_id || loki->pedigree->n_orig_families) {
			fputs("Error: Multiple families with no original id codes\n",stderr);
			err_ct++;
		}
	}
	loki->pedigree->n_orig_families++;
	comp_func(0);
}

static void comp_func(const args *attr _U_)
{
	if(current_comp<0) {
		if(!loki->pedigree->comp_size) {
			comp_array_size=32;
			loki->pedigree->comp_size=lk_malloc(sizeof(int)*comp_array_size);
		} else if(loki->pedigree->n_comp==comp_array_size) {
			comp_array_size<<=1;
			loki->pedigree->comp_size=lk_realloc(loki->pedigree->comp_size,sizeof(int)*comp_array_size);
		}
		current_comp=loki->pedigree->n_comp;
		loki->pedigree->comp_size[loki->pedigree->n_comp++]=0;
	}
}

static void close_comp(void)
{
	current_comp=-1;
}

static int lookup_id(int id) {
	int i,r;
	
	if(id>recode_size) {
		i=id*1.5;
		recode=lk_realloc(recode,sizeof(int)*i);
		memset(recode+recode_size,0,(i-recode_size)*sizeof(int));
		recode_size=i;
	}
	if(!id || !((r=recode[id-1]))) {
		r=++n_id;
		if(id) recode[id-1]=r;
		if(n_id==id_array_size) {
			i=id_array_size*1.5;
			loki->pedigree->id_array=lk_realloc(loki->pedigree->id_array,sizeof(struct Id_Record)*i);
			loki->pedigree->id_recode.recode=lk_realloc(loki->pedigree->id_recode.recode,sizeof(union arg_type)*i);
			memset(loki->pedigree->id_array+id_array_size,0,(i-id_array_size)*sizeof(struct Id_Record));
			for(;id_array_size<i;id_array_size++) loki->pedigree->id_recode.recode[id_array_size].string=0;
		}
		loki->pedigree->id_array[n_id-1].comp=current_comp;
		loki->pedigree->comp_size[current_comp]++;
		loki->pedigree->id_array[n_id-1].fam_code=loki->pedigree->n_orig_families;
	}
	return r-1;
}

static void ind_func(const args *attr)
{
	static int n_dummy;
	int i,j=0,k,id,fg;
	char *atts[]={"sex","aff","group","father","mother","orig","proband",0};
	char *p;
	const args *attr1;
	
	if(!loki->pedigree->id_array) {
		id_array_size=recode_size=32;
		n_id=0;
		loki->pedigree->id_array=lk_calloc((size_t)id_array_size,sizeof(struct Id_Record));
		loki->pedigree->id_recode.recode=lk_malloc(sizeof(union arg_type)*id_array_size);
		loki->pedigree->id_recode.flag=ST_STRING;
		/* Can't asssume that calloc clears pointers, so do it explicitly */
		for(i=0;i<id_array_size;i++) loki->pedigree->id_recode.recode[i].string=0;
		recode=lk_calloc((size_t)recode_size,sizeof(int));
	}
	attr1=get_single_attr(attr,"id","ind");
	if(!attr1) return;
	if(get_int_attr(attr1,&id,1,1,0,"ind")) return;
	id=lookup_id(id);
	fg=0;
	while(attr) {
		p=attr->name;
		i=-1;
		while(atts[++i]) if(!strcmp(atts[i],p)) break;
		switch(i) {
			case 0: /* sex */
				get_int_attr(attr,&loki->pedigree->id_array[id].sex,3,1,2,"ind");
				break;
			case 1: /* aff */
				get_int_attr(attr,&loki->pedigree->id_array[id].affected,1,0,0,"ind");
				break;
			case 2: /* group */
				get_int_attr(attr,&loki->pedigree->id_array[id].group,1,0,0,"ind");
				break;
			case 3: /* father */
				get_int_attr(attr,&j,1,0,0,"ind");
				k=lookup_id(j);
				loki->pedigree->id_array[id].sire=k+1;
				fg|=1;
				break;
			case 4: /* mother */
				get_int_attr(attr,&j,1,0,0,"ind");
				k=lookup_id(j);
				loki->pedigree->id_array[id].dam=k+1;
				fg|=2;
				break;
			case 5: /* orig */
				loki->pedigree->id_recode.recode[id].string=strdup(attr->att);
				break;
			case 6: /* proband */
				get_int_attr(attr,&loki->pedigree->id_array[id].proband,3,0,1,"ind");
				break;
		}
		attr=attr->next;
	}
	loki->pedigree->id_array[id].population=current_population;
	if(fg && fg!=3) {
		k=lookup_id(0);
		loki->pedigree->comp_size[current_comp]++;
		p=lk_malloc(16);
		snprintf(p,16,"<PAR_%d>",++n_dummy);
		loki->pedigree->id_recode.recode[k].string=p;
		if(fg==1) loki->pedigree->id_array[id].dam=k+1;
		else loki->pedigree->id_array[id].sire=k+1;
	}
}

static void group_func(const args *attr)
{
	int i;
	const args *attr1;
	
	vlist=0;
	attr1=get_single_attr(attr,"name","group");
	if(!attr1) return;
	if(!grps) {
		grp_array_size=32;
		grps=malloc(sizeof(grp)*grp_array_size);
	} else if(n_grps==grp_array_size) {
		grp_array_size<<=1;
		grps=lk_realloc(grps,sizeof(grp)*grp_array_size);
	}
	grps[n_grps].name=strdup(attr1->att);
	attr1=get_single_attr(attr,"type",0);
	if(attr1) {
		i=-1;
		while(var_types[++i]) if(!strcmp(attr1->att,var_types[i])) break;
		if(!var_types[i]) {
			fprintf(stderr,"Warning: unknown type attribute '%s' for group element '%s'\n",attr1->att,grps[n_grps-1].name);
			grps[n_grps].type=-1;
		} else grps[n_grps].type=i+1;
	} else grps[n_grps].type=0;
	grps[n_grps].rec_list=0;
	grps[n_grps].var_list1=0;
	grps[n_grps].count=0;
	grps[n_grps++].var_list=0;
}

static void var_func(const args *attr)
{
	int i;
	var *cvar;
	const args *attr1;
	
	attr1=get_single_attr(attr,"name","var");
	if(!attr1) return;
	cvar=lk_malloc(sizeof(var));
	cvar->next=0;
	cvar->flags=0;
	cvar->levels=0;
	cvar->parent=0;
	cvar->name=strdup(attr1->att);
	cvar->idx=grps[n_grps-1].count++;
	attr1=get_single_attr(attr,"type",grps[n_grps-1].type?0:"var");
	if(attr1) {
		i=-1;
		while(var_types[++i]) if(!strcmp(attr1->att,var_types[i])) break;
		if(!var_types[i]) {
			fprintf(stderr,"Error: unknown type attribute '%s' for var element '%s' in group element '%s'\n",attr1->att,cvar->name,grps[n_grps-1].name);
			err_ct++;
			cvar->type=-1;
		} else cvar->type=i+1;
	} else cvar->type=grps[n_grps-1].type;
	attr1=get_single_attr(attr,"flags",0);
	if(attr1) cvar->flags=strdup(attr1->att);
	attr1=get_single_attr(attr,"parent",0);
	if(attr1) cvar->parent=strdup(attr1->att);
	grps[n_grps-1].var_list=insert_bin_node(grps[n_grps-1].var_list,cvar,0,cmp_var,new_var);
	vlist=cvar;
}

static void link_func(const args *attr)
{
	int i;
	const args *attr1;
	static char *types[]={"autosomal","x","y","mt","z","w","unlinked",0};
	char *name;
	
	attr1=get_single_attr(attr,"name",0);
	name=attr1?strdup(attr1->att):0;
	for(i=0;i<loki->markers->n_links;i++) {
		if(!name) {
			if(!lnks[i].name) break;
		} else if(lnks[i].name && !strcasecmp(name,lnks[i].name)) break;
	}
	if(i<loki->markers->n_links) {
		if(!name) fputs("Error: Multiple un-named link groups\n",stderr);
		else fprintf(stderr,"Error: Multiple link groups with same name '%s'\n",name);
		err_ct++;
	}
	if(!lnks) {
		lnk_array_size=32;
		lnks=lk_malloc(sizeof(lnk)*lnk_array_size);
	} else if(loki->markers->n_links==lnk_array_size) {
		lnk_array_size<<=1;
		lnks=lk_realloc(lnks,sizeof(lnk)*lnk_array_size);
	}
	lnks[loki->markers->n_links].name=name;
	attr1=get_single_attr(attr,"type",0);
	if(attr1) {
		i=-1;
		while(types[++i]) if(!strcmp(attr1->att,types[i])) break;
		if(!types[i]) {
			fprintf(stderr,"Error: unknown type attribute '%s' for link element\n",attr1->att);
			err_ct++;
			i=-1;
		}
		lnks[loki->markers->n_links].type=i;
	} else lnks[loki->markers->n_links].type=0; /* autosomal default */
  lnks[loki->markers->n_links].v_list=0;
  loki->markers->n_links++;
}

static void locus_func(const args *attr)
{
	int i;
	char *p;
	double x;
	const args *attr1;
	var *vv;
	l_var *lv;
	lnk *lk;
	static char *atts[]={"female","male","avg","phys_pos","rs_id","cng_id","mid","chrom_seg",0};
	static int ct;
	
	attr1=get_single_attr(attr,"name","locus");
	if(!attr1) return;
	ct++;
	if(!(ct%1000)) printf("Reading in marker %s (%d)\n",attr1->att,ct);
	vv=find_var(attr1->att,0);
	if(!vv) {
		fprintf(stderr,"Variable '%s' in link list not found\n",attr1->att);
		err_ct++;
	} else {
		lk=lnks+loki->markers->n_links-1;
		lv=lk_malloc(sizeof(l_var));
		lv->rs_id=lv->cng_id=lv->mid=0;
		lv->phys_pos=-1.0;
		lv->next=lk->v_list;
		lk->v_list=lv;
		for(i=0;i<3;i++) lv->pos_set[i]=0;
		while(attr) {
			p=attr->name;
			i=-1;
			while(atts[++i]) if(!strcmp(atts[i],p)) break;
			if(i<3) {
				if(!(get_double_attr(attr,&x,0,0.0,0.0,"locus"))) {
					lv->pos_set[i]=1;
					lv->pos[i]=x;
				}
			} else switch(i) {
				case 3:
					if(!(get_double_attr(attr,&x,0,0.0,0.0,"locus"))) lv->phys_pos=x;
					break;
				case 4:
					lv->rs_id=strdup(attr->att);
					break;
				case 5:
					lv->cng_id=strdup(attr->att);
					break;
				case 6:
					lv->mid=strdup(attr->att);
					break;
				case 7:
					lv->chrom_seg=strdup(attr->att);
					break;
			}
				attr=attr->next;
		}
		lv->vv=vv;
	}
}

static void model_func(const args *attr _U_)
{
	model *mod;
	
	if(!sorted_records) sort_records();
	mod=lk_malloc(sizeof(model));
	mod->next=modlist;
	mod->vars=0;
	mod->terms=0;
	modlist=mod;
}

static void trait_func(const args *attr _U_)
{
	mod_var *mvar;
	
	assert(modlist);
	mvar=lk_malloc(sizeof(mod_var));
	mvar->next=modlist->vars;
	modlist->vars=mvar;
}

static void term_func(const args *attr)
{
	model_term *term;
	
	assert(modlist);
	term=lk_malloc(sizeof(model_term));
	term->next=modlist->terms;
	modlist->terms=term;
	attr=get_single_attr(attr,"random",0);
	if(attr) {
		if(!strcmp(attr->att,"no")) term->ran_flag=0;
		else if(!strcmp(attr->att,"yes")) term->ran_flag=1;
		else {
			fprintf(stderr,"Error at line %d: unknown random attribute '%s' for term element\n",line+1,attr->att);
			err_ct++;
		}
	} else term->ran_flag=0;
}

static int recode_ped(int *perm,int i,int *id)
{
	int par,er=0;
	
	if(loki->pedigree->id_array[i].flag) {
		print_orig_id(stderr,i+1);
		fputs(" is own ancestor\nLinking individuals follow:\n",stderr);
		return -1;
	}
	loki->pedigree->id_array[i].flag=1;
	if((par=loki->pedigree->id_array[i].sire)) if(!perm[par-1]) er=recode_ped(perm,par-1,id);
	if(!er && (par=loki->pedigree->id_array[i].dam)) if(!perm[par-1]) er=recode_ped(perm,par-1,id);
	if(er) {
		fputc(' ',stderr);
		print_orig_id(stderr,i+1);
		fputc('\n',stderr);
	} else perm[i]=++(*id);
	return er;
}

static void sort_ped(void) 
{
	int *perm,i,j,k,k1,par;
	struct Id_Record *idr;
	union arg_type *rr;
	
	loki->pedigree->ped_size=n_id;
	if(n_id) {
		perm=lk_calloc((size_t)n_id,sizeof(int));
		for(i=0;i<n_id;i++) loki->pedigree->id_array[i].flag=0;
		for(j=i=0;i<n_id;i++) if(!perm[i]) {
			k=recode_ped(perm,i,&j);
			if(k) ABT_FUNC(AbMsg);
		}
			if(j!=n_id) ABT_FUNC("Internal error - ped size mismatch\n");
		idr=lk_malloc(sizeof(struct Id_Record)*n_id);
		rr=lk_malloc(sizeof(union arg_type)*id_array_size);
		for(i=0;i<n_id;i++) {
			j=perm[i]-1;
			memcpy(idr+j,loki->pedigree->id_array+i,sizeof(struct Id_Record));
			par=idr[j].sire;
			if(par) idr[j].sire=perm[par-1];
			par=idr[j].dam;
			if(par) idr[j].dam=perm[par-1];
			rr[j].string=loki->pedigree->id_recode.recode[i].string;
			idr[j].idx=j;
		}
		free(loki->pedigree->id_array);
		free(loki->pedigree->id_recode.recode);
		loki->pedigree->id_array=idr;
		loki->pedigree->id_recode.recode=rr;
		for(i=0;i<recode_size;i++) if((j=recode[i])) recode[i]=perm[j-1];
		free(perm);
		loki->pedigree->comp_ngenes=lk_malloc(sizeof(int)*loki->pedigree->n_comp*2);
		loki->pedigree->comp_start=loki->pedigree->comp_ngenes+loki->pedigree->n_comp;
		for(j=i=0;i<loki->pedigree->n_comp;i++) {
			loki->pedigree->comp_ngenes[i]=0;
			loki->pedigree->comp_start[i]=j;
			j+=loki->pedigree->comp_size[i];
		}
		loki->pedigree->singleton_flag=lk_calloc((size_t)loki->pedigree->n_comp,sizeof(int));
		for(i=0,j=0;j<loki->pedigree->n_comp;j++)	{
			for(k1=k=0;k<loki->pedigree->comp_size[j];k++,i++) {
				if(!idr[i].sire) loki->pedigree->comp_ngenes[j]++;
				else k1++;
				if(!idr[i].dam) loki->pedigree->comp_ngenes[j]++;
				else k1++;
			}
			if(!k1) {
				loki->pedigree->singleton_flag[j]=1;
				message(DEBUG_MSG,"Component %d consists of singletons\n",j+1);
			}
		}
	}
	sorted_ped=1;
}

static void data_func(const args *attr)
{
	int i;
	
	if(!sorted_ped) sort_ped();
	attr=get_single_attr(attr,"group","data");
	c_grp=-1;
	if(!attr) return;
	for(i=0;i<n_grps;i++) if(!strcmp(attr->att,grps[i].name)) break;
	if(i==n_grps) {
		fprintf(stderr,"Unknown group attribute '%s' in data element\n",attr->att);
		err_ct++;
	} else c_grp=i;
}

static void rec_func(const args *attr)
{
	int id;
	
	attr=get_single_attr(attr,"id","rec");
	if(c_grp<0) return;
	c_rec=0;
	if(!attr) return;
	id=0;
	if(!(get_int_attr(attr,&id,3,1,recode_size-1,"rec"))) {
		id=recode[id-1];
		if(!id) {
			fprintf(stderr,"Error: bad id attribute '%s' to rec element\n",attr->att);
			exit(0);
			err_ct++;
		} else {
			c_rec=lk_malloc(sizeof(record));
			c_rec->id=id;
			c_rec->data=0;
			c_rec->next=grps[c_grp].rec_list;
			grps[c_grp].rec_list=c_rec;
		}
	}
}

static void start_element(const string *name,const args *attr)
{
	int i;
	
	i=-1;
	while(elements[++i]) {
		if(!(strcmp(elements[i],get_cstring(name)))) break;
	}
	if(!elements[i]) {
		fprintf(stderr,"Warning: unknown element %s\n",get_cstring(name));
		unknown_depth++;
	} else {
		if(unknown_depth) unknown_depth++;
		else {
			if(element_stack_ptr==element_stack_size) {
				element_stack_size*=1.5;
				element_stack=lk_realloc(element_stack,sizeof(int)*element_stack_size);
			}
			element_stack[element_stack_ptr++]=i;
			change_state(i);
			if(state!=__ERR_STAGE && start_element_funcs[i]) start_element_funcs[i](attr);
		}
	}
}

static void content(const string *s)
{
	int i,j,k;
	var *vv;
	tokens *tok=0;
	
	i=element_stack[element_stack_ptr-1];
	switch(i) {
		case LEVELS: /* Levels content */
			vlist->levels=lk_malloc((size_t)s->len+1);
			strncpy(vlist->levels,get_cstring(s),s->len+1);
			break;
		case TRAIT: /* Trait content */
			assert(modlist && modlist->vars);
			qstrip(get_cstring(s));
			vv=find_var(get_cstring(s),&i);
			if(!vv) {
				fprintf(stderr,"Line %d: Trait variable '%s' not found\n",line+1,get_cstring(s));
				err_ct++;
			} else {
				if(grps[i].rec_list) {
					if(vv->type<4) {
						modlist->vars->var.var_index=vv->ref.var->index;
						modlist->vars->var.type=(grps[i].type&MREC_FLAG)?ST_MULTIPLE:ST_CONSTANT;
					} else {
						fprintf(stderr,"Line %d: Inappropriate type for trait variable '%s'\n",line+1,get_cstring(s));
						err_ct++;
					}
				} else {
					fprintf(stderr,"Line %d: Trait variable '%s' has no data\n",line+1,get_cstring(s));
					err_ct++;
				}
			}
				break;
		case TERM: /* Term content */
			assert(modlist && modlist->terms);
			tok=tokenize(get_cstring(s),'*',tok);
			if(tok) {
				j=tok->n_tok;
				if(j) {
					modlist->terms->n_var=j;
					modlist->terms->vars=lk_malloc(sizeof(struct Model_Var)*j);
					for(j=0;j<tok->n_tok;j++) {
						vv=find_var(tok->toks[j],&i);
						if(!vv) {
							fprintf(stderr,"Line %d: Term variable '%s' not found\n",line+1,tok->toks[j]);
							err_ct++;
						} else {
							if(grps[i].rec_list) {
								if(vv->type<4) {
									modlist->terms->vars[j].var_index=vv->ref.var->index;
									modlist->terms->vars[j].type=(grps[i].type&MREC_FLAG)?ST_MULTIPLE:ST_CONSTANT;
								} else if(vv->type==4) {
									modlist->terms->vars[j].var_index=vv->ref.mark->locus.index;
									modlist->terms->vars[j].type=ST_MARKER;
								} else {
									fprintf(stderr,"Line %d: Inappropriate type for term variable '%s'\n",line+1,tok->toks[j]);
									err_ct++;
								}
							} else {
								modlist->terms->vars[j].var_index=-1;
								switch(vv->type) {
									case 9:
										k=ST_ID;
										break;
									case 10:
										k=ST_SIRE;
										break;
									case 11:
										k=ST_DAM;
										break;
									case 15:
										k=ST_TRAITLOCUS;
										break;
									default:
										fprintf(stderr,"Line %d: Unsupported type (%d) for term variable '%s'\n",line+1,vv->type,tok->toks[j]);
										k=0;
										err_ct++;
								}
								modlist->terms->vars[j].type=k;
							}
						}
					}
				}
			}
				if(!modlist->terms->vars) {
					fprintf(stderr,"Line %d: Null term content\n",line+1);
					err_ct++;
				}
				break;
		case REC: /* Rec content */
			if(c_rec) {
				if(c_rec->data) {
					i=(int)strlen(c_rec->data);
					c_rec->data=lk_realloc(c_rec->data,(size_t)i+s->len+1);
					strncpy(c_rec->data+i,get_cstring(s),s->len+1);
				} else {
					c_rec->data=lk_malloc((size_t)s->len+1);
					strncpy(c_rec->data,get_cstring(s),s->len+1);
				}
			}
			break;
		case CENS: /* Cens content */
			if(c_rec) {
				if(c_rec->data) {
					i=(int)strlen(c_rec->data);
					c_rec->data=lk_realloc(c_rec->data,(size_t)i+s->len+2);
					c_rec->data[i]='&';
					strncpy(c_rec->data+i+1,get_cstring(s),s->len+1);
				} else {
					c_rec->data=lk_malloc((size_t)s->len+2);
					c_rec->data[0]='&';
					strncpy(c_rec->data+1,get_cstring(s),s->len+1);
				}
			}
			break;
	}
	if(tok) free_tokens(tok);
}

static void close_loki(void)
{
	int i,j,n_models=0,n_terms,poly_flag,cens_flag,m,k,k1,use_student_t,type;
	model *mod,*mod1;
	mod_var *mvar,*mvar1;
	model_term *mterm,*mterm1;
	double **tpp,*td;
	struct Model *mm;
	struct Model_Term *terms;
	struct id_data *data;
	struct Id_Record *id_array;
	struct Variable *v;
	
	if(!err_ct) {
		/* Count models */
		n_terms=0;
		mod=modlist;
		while(mod) {
			mvar=mod->vars;
			while(mvar) {
				n_models++;
				mvar=mvar->next;
			}
			mterm=mod->terms;
			while(mterm) {
				n_terms++;
				mterm=mterm->next;
			}
			mod=mod->next;
		}
		/* Allocate storage for models */
		if(n_models) {
			loki->models->models=malloc(sizeof(struct Model)*n_models);
			terms=lk_malloc(sizeof(struct Model_Term)*n_terms);
			loki->sys.RemBlock=AddRemem(terms,loki->sys.RemBlock);
			n_models=n_terms=0;
			mod=modlist;
			while(mod) {
				mterm=mod->terms;
				i=0;
				poly_flag=0;
				while(mterm) {
					terms[n_terms+i].eff=0;
					terms[n_terms+i].out_flag=terms[n_terms+i].df=0;
					terms[n_terms+i].vars=mterm->vars;
					terms[n_terms+i].n_vars=mterm->n_var;
					for(j=0;j<mterm->n_var;j++) {
						if(mterm->ran_flag) {
							if(mterm->vars[j].type&ST_ID) poly_flag=1;
							mterm->vars[j].type|=ST_RANDOM;
						}
					}
					i++;
					mterm1=mterm->next;
					free(mterm);
					mterm=mterm1;
				}
				mvar=mod->vars;
				while(mvar) {
					j=mvar->var.var_index;
					v=(mvar->var.type&ST_CONSTANT)?loki->data->id_variable+j:loki->data->nonid_variable+j;
					mvar->var.type|=(v->type&(ST_FACTOR|ST_CENSORED));
					loki->models->models[n_models].var=mvar->var;
					loki->models->models[n_models].n_terms=i;
					loki->models->models[n_models].polygenic_flag=poly_flag;
					loki->models->models[n_models++].term=terms+n_terms;
					mvar1=mvar->next;
					free(mvar);
					mvar=mvar1;
				}
				n_terms+=i;
				mod1=mod->next;
				free(mod);
				mod=mod1;
			}
			/* Allocate storage for model associated parameters */
			for(i=0;i<loki->markers->n_markers;i++) {
				loki->markers->marker[i].mterm=lk_malloc(sizeof(void *)*n_models);
				for(j=0;j<n_models;j++) loki->markers->marker[i].mterm[j]=0;
			}
			loki->models->n_models=n_models;
			j=n_models*(n_models+1)/2;
			for(cens_flag=poly_flag=i=0;i<n_models;i++) {
				poly_flag+=loki->models->models[i].polygenic_flag;
				if(loki->models->models[i].var.type&ST_CENSORED) cens_flag=1;
			}
			i=poly_flag?j*2+n_models*5:j+n_models*4;
			loki->models->residual_var=lk_malloc(sizeof(double)*i);
			for(k=0;k<i;k++) loki->models->residual_var[k]=0.0;
			i=n_models*(poly_flag?3:2);
			loki->models->res_var_set=lk_calloc((size_t)i,sizeof(int));
			loki->models->residual_var_limit=loki->models->residual_var+j;
			loki->models->tau=loki->models->residual_var_limit+n_models;
			loki->models->tau_beta=loki->models->tau+n_models;
			loki->models->grand_mean=loki->models->tau_beta+n_models;
			loki->models->grand_mean_set=loki->models->res_var_set+n_models;
			if(poly_flag) {
				loki->models->additive_var=loki->models->grand_mean+n_models;
				loki->models->additive_var_limit=loki->models->additive_var+j;
				loki->models->add_var_set=loki->models->grand_mean_set+n_models;
			}
			use_student_t=loki->models->use_student_t;
			i=1+(use_student_t?1:0)+(cens_flag?1:0);
			i*=n_models*loki->pedigree->ped_size;
			tpp=lk_malloc(sizeof(void *)*i);
			for(j=0;j<i;j++) tpp[j]=0;
			loki->sys.RemBlock=AddRemem(tpp,loki->sys.RemBlock);
			if(poly_flag) {
				td=lk_malloc(sizeof(double)*loki->pedigree->ped_size*n_models*3);
				loki->sys.RemBlock=AddRemem(td,loki->sys.RemBlock);
			} else td=0;
			id_array=loki->pedigree->id_array;
			for(j=0;j<loki->pedigree->ped_size;j++) {
				id_array[j].res=tpp;
				tpp+=n_models;
				if(cens_flag) {
					id_array[j].pseudo_qt=tpp;
					tpp+=n_models;
				}
				if(use_student_t) {
					id_array[j].vv=tpp;
					tpp+=n_models;
				}
				if(poly_flag) {
					id_array[j].bv=td;
					id_array[j].bvsum=td+n_models;
					id_array[j].bvsum2=td+2*n_models;
					td+=3*n_models;
				}
			}
			/* Count storage required for residuals etc. */
			for(m=0;m<n_models;m++) {
				j=0;
				mm=loki->models->models+m;
				type=mm->var.type;
				k=mm->var.var_index;
				k1=loki->models->use_student_t+(type&ST_CENSORED)?2:1;
				for(i=0;i<loki->pedigree->ped_size;i++) {
					if(type&ST_CONSTANT) {
						data=id_array[i].data;
						if(data && data[k].flag) j+=k1; /* Storage for residual + nu + censored value*/
					} else j+=id_array[i].n_rec*k1;
				}
				if(j) {
					td=lk_malloc(sizeof(double)*j);
					loki->sys.RemBlock=AddRemem(td,loki->sys.RemBlock);
					for(i=0;i<loki->pedigree->ped_size;i++) {
						if(type&ST_CONSTANT) {
							data=id_array[i].data;
							if(data && data[k].flag) {
								id_array[i].res[m]=td++;
								if(use_student_t) id_array[i].vv[m]=td++;
								if(type&ST_CENSORED) {
									id_array[i].pseudo_qt[m]=td++;
								}
							}
						} else if((k1=id_array[i].n_rec)) {
							id_array[i].res[m]=td;
							td+=k1;
							if(use_student_t) {
								id_array[i].vv[m]=td;
								td+=k1;
							}
							if(type&ST_CENSORED) {
								id_array[i].pseudo_qt[m]=td;
								td+=k1;
							}
						}
					}
				}
			}
		}
	}
	if(!n_models) {
		loki->models->n_models=0;
		loki->models->models=0;
		loki->models->residual_var=0;
		loki->models->res_var_set=0;
	}
}

static void end_element(const string *name)
{
	int i;
	
	if(unknown_depth) unknown_depth--;
	else {
		assert(element_stack_ptr);
		element_stack_ptr--;
		i=-1;
		while(elements[++i]) {
			if(!(strcmp(elements[i],get_cstring(name)))) break;
		}
		assert(elements[i]);
		change_state(-1);
		if(state==__END_STAGE && !sorted_records) sort_records();
		if(state!=__ERR_STAGE && end_element_funcs[i]) end_element_funcs[i]();
	}
}

static void handle_pi(const string *name _U_,const string *ct _U_)
{
	/*	printf("PI - %s: ->%s<-\n",get_cstring(name),get_cstring(content)); */
}

static void handle_comment(const string *ct _U_)
{
	/*	printf("Comment ->%s<-\n",get_cstring(content)); */
}

static void xml_error(const int ln, const char *fmt, ...)
{
	va_list a;
	
	err_ct++;
	if(stdout) (void)fflush(stdout);
	(void)fprintf(stderr,"\n[%s:%d] ",infile,ln);
	va_start(a,fmt);
	(void)vfprintf(stderr,fmt,a);
	va_end(a);
}

/* Free structures allocated by ReadXMLFile */
static void FreeXMLFile(void)
{
	int i,j;
	var *vv,*vv1;
	
	if(loki->markers->marker) {
		for(i=0;i<loki->markers->n_markers;i++) {
			free_marker(loki->markers->marker+i,loki->pedigree->ped_size);
		}
		free(loki->markers->marker);
	}
	if(loki->pedigree->id_array) free(loki->pedigree->id_array);
	if(loki->pedigree->fam_recode.recode) {
		for(i=0;i<loki->pedigree->n_orig_families;i++) free(loki->pedigree->fam_recode.recode[i].string);
		free(loki->pedigree->fam_recode.recode);
	}
	if(loki->pedigree->id_recode.recode) {
		for(i=0;i<loki->pedigree->ped_size;i++) 
			if(loki->pedigree->id_recode.recode[i].string) free(loki->pedigree->id_recode.recode[i].string);
		free(loki->pedigree->id_recode.recode);
	}
	if(loki->markers->linkage) {
		for(i=0;i<loki->markers->n_links;i++) {
			if(!(loki->markers->linkage[i].type&LINK_PSEUDO)) {
				if(loki->markers->linkage[i].ibd_list) {
					free(loki->markers->linkage[i].ibd_list->pos);
					free(loki->markers->linkage[i].ibd_list);
				}
			}
			if(loki->markers->linkage[i].name) free(loki->markers->linkage[i].name);
		}
		free(loki->markers->linkage);
	}
	if(loki->pedigree->comp_size) free(loki->pedigree->comp_size);
	if(loki->pedigree->comp_ngenes) free(loki->pedigree->comp_ngenes);
	if(loki->pedigree->singleton_flag) free(loki->pedigree->singleton_flag);
	for(i=0;i<loki->data->n_id_records;i++) 
		if(loki->data->id_variable[i].recode) free(loki->data->id_variable[i].recode);
	for(i=0;i<loki->data->n_nonid_records;i++) 
		if(loki->data->nonid_variable[i].recode) free(loki->data->nonid_variable[i].recode);
	if(loki->data->id_variable) free(loki->data->id_variable);
	else if(loki->data->nonid_variable) free(loki->data->nonid_variable);
	for(i=0;i<n_grps;i++) {
		vv=grps[i].var_list1;
		while(vv) {
			vv1=vv->next;
			if(vv->name) free(vv->name);
			if(vv->levels) free(vv->levels);
			if(vv->flags) free(vv->flags);
			free(vv);
			vv=vv1;
		}
		free(grps[i].name);
	}
	if(grps) free(grps); 
	if(loki->pedigree->group_recode.recode) free(loki->pedigree->group_recode.recode);
	if(loki->models->models) {
		for(i=0;i<loki->models->n_models;i++) for(j=0;j<loki->models->models[i].n_terms;j++) 
			if(loki->models->models[i].term[j].vars) free(loki->models->models[i].term[j].vars);
		free(loki->models->models);
	}
	if(loki->models->tlocus) {
		delete_all_traitloci(loki);
		free(loki->models->tlocus);
	}
	if(loki->models->pruned_flag) free(loki->models->pruned_flag);
	if(loki->models->residual_var) free(loki->models->residual_var);
	if(loki->models->res_var_set) free(loki->models->res_var_set);
}

int ReadXMLFile(struct loki *lk)
{
	int i,fflag;
	FILE *fptr;
	XML_handler handle;
	
	loki=lk;
	infile=make_file_name(".xml");
	logfile=&lk->names[LK_LOGFILE];
	lk->pedigree->n_genetic_groups=1;
	lk->pedigree->group_recode.recode=0;
	fptr=open_readfile_and_check(infile,&fflag,lk->compress);
	element_stack_size=16;
	element_stack=lk_malloc(sizeof(int)*element_stack_size);
	element_stack_ptr=0;
	err_ct=0;
	message(INFO_MSG,"Reading from %s\n",infile);
	handle.start_element=start_element;
	handle.end_element=end_element;
	handle.content=content;
	handle.comment=handle_comment;
	handle.declaration=0;
	handle.pi=handle_pi;
	handle.error=xml_error;
	handle.warning=xml_error;
	handle.line=&line;
	unknown_depth=state=0;
	err_ct+=Read_XML(fptr,&handle);
	if(fclose(fptr)) XFE(infile,0);
	free(infile);
	free(element_stack);
	if(atexit(FreeXMLFile)) message(WARN_MSG,"Unable to register exit function FreeXMLFile()\n");
	if(fflag) {
		signal(SIGCHLD,SIG_DFL);
		while(waitpid(-1,&i,WNOHANG)>0);
	}
	return err_ct;
}
