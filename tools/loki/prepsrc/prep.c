/****************************************************************************
 *                                                                          *
 *     Loki - Programs for genetic analysis of complex traits using MCMC    *
 *                                                                          *
 *             Simon Heath - University of Washington                       *
 *                                                                          *
 *                       March 1997                                         *
 *                                                                          *
 * prep.c:                                                                  *
 *                                                                          *
 * Data preperation - reading control file, reading datafiles, recoding     *
 * factorial data, and writing out binary datafiles for loki                *
 *                                                                          *
 * Copyright (C) Simon C. Heath 1997, 2000, 2002                            *
 * This is free software.  You can distribute it and/or modify it           *
 * under the terms of the Modified BSD license, see the file COPYING        *
 *                                                                          *
 ****************************************************************************/

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif
#include <errno.h>
#include <ctype.h>
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "prep.h"
#include "version.h"
#include "ranlib.h"
#include "utils.h"
#include "libhdr.h"
#include "scan.h"
#include "y.tab.h"
#include "compat/compat.h"
#include "lkgetopt.h"
#include "write_data.h"

int strip_vars;
static int error_check=1;
static struct loki loki;
static char *LogFile;

void print_version_and_exit(void)
{
	(void)printf("%s\n",PREP_NAME);
	exit(EXIT_SUCCESS);
}

static int process_loki(int argc,char *argv[])
{
	FILE *fptr;
	int err,i,c,ec_flag=0;
	
	while((c=getopt(argc,argv,"evVX:d:p:"))!=-1) switch(c) {
	 case 'e':
		error_check=0;
		ec_flag=1;
		break;
	 case 'V':
		print_version_and_exit();
		break; /* Previous command never returns */
	 case 'v':
		loki.params.verbose_level=OUTPUT_VERBOSE;
		break;
	 case 'd':
		if((i=set_file_dir(optarg))) {
			fprintf(stderr,"Error setting default file directory: %s\n",i==UTL_BAD_STAT?strerror(errno):utl_error(i));
			exit(EXIT_FAILURE);
		}
		break;
	 case 'p':
		if((i=set_file_prefix(optarg))) {
			fprintf(stderr,"Error setting default file prefix: %s\n",utl_error(i));
			exit(EXIT_FAILURE);
		}
		break;
	 case 'X':
		fputs("-X option must occur as first argument\n",stderr);
		exit(EXIT_FAILURE);
	}
	if(optind>=argc) abt(__FILE__,__LINE__,"No control file specified\n");
	init_stuff(&LogFile);
	if((fptr=fopen(argv[optind],"r"))) {
/*		err=NewReadControl(fptr,argv[optind],&loki); */
/*		exit(0); */
		err=ReadControl(fptr,argv[optind],&LogFile); 
	} else {
		(void)printf("Couldn't open '%s' for input as control file\nAborting...\n",argv[optind]);
		exit(EXIT_FAILURE);
	}
	(void)fclose(fptr);
	if(!err) {
		if(!ec_flag) error_check=syst_var[ERROR_CHECK];
		print_start_time(PREP_NAME,"w",loki.names[LK_LOGFILE],&loki.sys.lktime);
		if(!scan_error_n) ReadData(LogFile);         /* Read in the datafile(s) and recode (where necessary) */
/*		if(!scan_error_n) ReadData1(loki.names[LK_LOGFILE],&loki);  */   /* Read in the datafile(s) and recode (where necessary) */
	}
	return err;
}

static void free_prep_struct(void)
{
	if(loki.prep) {
		if(loki.prep->pedigree) free(loki.prep->pedigree);
		if(loki.prep->data) free(loki.prep->data);
		free(loki.prep);
		FreeRemem(loki.sys.FirstRemBlock);
		loki.prep=0;
	}
	free_string_lib();
}

static void init_prep_struct(void) 
{
	struct lk_ped *ped;
	struct lk_data *dat;
	
	if(!(loki.prep=malloc(sizeof(struct lk_prep)))) ABT_FUNC(MMsg);
	if(!(loki.prep->pedigree=ped=malloc(sizeof(struct lk_ped)))) ABT_FUNC(MMsg);
	loki.prep->markers=0;
	loki.prep->models=loki.prep->assignments=0;
	loki.prep->links=0;
	ped->id_array=0;
	ped->family=0;
	ped->ped_recode=0;
	ped->family_recode=0;
	ped->sex_def=0;
	ped->ped_vars=0;
	ped->ped_size=0;
	if(!(loki.prep->data=dat=malloc(sizeof(struct lk_data)))) ABT_FUNC(MMsg);
	dat->missing=0;
	dat->infiles=0;
	/* Allocate memory handlers (for memory that is difficult to explicitly free) */
	if(!(loki.sys.FirstRemBlock=malloc(sizeof(struct remember)))) ABT_FUNC(MMsg);
	loki.sys.RemBlock=loki.sys.FirstRemBlock;
	loki.sys.RemBlock->pos=0;
	loki.sys.RemBlock->next=0;
	/* Find available compression routines */
	loki.compress=init_compress();
	(void)atexit(free_prep_struct);
}

int main(int argc,char *argv[])
{
	int err,type=LOKI_FORMAT;
	struct lk_compress *lkc;
	
	loki.sys.lktime.start_time=time(0);
	loki.sys.strcmpfunc=strcasecmp;
	init_lib_utils();
	init_prep_struct();
	/* Need on FreeBSD at least to prevent exceptions on arithmetic errors */
#ifdef HAVE_FPSETMASK
	fpsetmask(0);
#endif
	if(argc>1) {
		if(*argv[1]=='-' && argv[1][1]=='X') {
			type=check_format(argv[1]);
			optind=2;
			optreset=1;
		}
	}
	switch(type) {
	 case LOKI_FORMAT:
		err=process_loki(argc,argv);
		break;
	 case QTDT_FORMAT:
		err=process_qtdt(argc,argv,&LogFile,&error_check,&loki.sys.lktime);
		break;
	 default:
		fputs("Input type not yet handled\n",stderr);
		exit(EXIT_FAILURE);
	}
	if(err) {
		loki.names[LK_LOGFILE]=0;
		exit(EXIT_FAILURE);
	}
	if(!pruned_ped_size) {
		(void)printf("Zero size pedigree\nAborting...\n");
		exit(EXIT_FAILURE);
	}
	if(!scan_error_n)	{
		InitFamilies(loki.names[LK_LOGFILE]);
		count_loops(loki.names[LK_LOGFILE]);
		check_inbreeding(loki.names[LK_LOGFILE]);
		check_ymark();
	}
	if(!scan_error_n && (traitlocus || n_markers)) err=Genotype_Elimination(syst_var[CORRECT_ERRORS],loki.names[LK_LOGFILE],error_check);
	if(!scan_error_n && !err) Check_Inbr(loki.names[LK_LOGFILE]);
	if(family) free(family);
	lkc=loki.compress;
	if(!scan_error_n && !err) {
		WriteXMLData(loki.names[LK_LOGFILE],lkc);
		WriteReport(LogFile);
	}
	free_nodes();
	if(scan_error_n) (void)fprintf(stderr,"Errors: %d  ",scan_error_n);
	if(scan_warn_n) (void)fprintf(stderr,"Warnings: %d  ",scan_warn_n);
	if(scan_error_n || scan_warn_n) (void)fprintf(stderr,"\n");
	return sig_caught;
}
