/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <e/bytes.h>
#include <e/xmalloc.h>
#include "pcp_lib.h"
#include "pcp_options.h"

static int fanout_ok(int fanout)
{
    if (fanout < 1) {
        fprintf(stderr, "-F option (fanout) must be >= 1\n");
        return 0;
    }
    return 1;
}

static int frag_size_ok(int frag_size)
{
    if (frag_size < 1) {
        fprintf(stderr, "-n option (frag_size) must be >= 1\n");
        return 0;
    }
    return 1;
}

static int port_ok(int port)
{
    if (port < 1) {
        fprintf(stderr, "-P option (port) must be >= 1\n");
        return 0;
    }
    return 1;
}

static void set_cmd_type(pcp_options *opts, int cmd_type)
{
    if (opts->cmd_type != PCP_NULL_CMD) {
        printf("Only one cmd type allowed\n");
        exit(3);
    }
    opts->cmd_type = cmd_type;
}

static int file_exists(const char *filename)
{
#ifdef HAVE_ACCESS
    return access(filename, F_OK) >= 0;
#else
    struct stat buf;
    return stat(filename, &buf) >= 0;
#endif
}

static void print_help(const char *argv0, const struct option *longopts)
{
	const char *p;
	const struct option *q;

	if ((p = strchr(argv0, '/')) == NULL)
		p = argv0;
	else
		p++;
    printf("Usage: %s [OPTIONS] localfile remotefile host0 [host1] [host2] ..\n"
            "       Copy localfile to remotefile on listed hosts.\n\n"
            "Usage: %s --checksum [OPTIONS] remotefile host0 [host1] [host2] ..\n"
            "       Checksum remotefile on listed hosts.\n\n"
            "Usage: %s --delete [OPTIONS] remotefile host0 [host1] [host2] ..\n"
            "       Delete remotefile on listed hosts.\n\n"
            "       Note: remotefile must be a file pathname.\n\n", p, p, p);
	for (q = longopts; q->name; q++)
		if (q->has_arg == no_argument)
			printf("     -%c  --%s\n", q->val, q->name);
		else
			printf("     -%c  --%s=v\n", q->val, q->name);
}

static void print_version()
{
    printf("PCP %s\n\n", VERSION);
    printf("Copyright (C) 2002 Brent N. Chun\n");
    printf("This program is distributed in the hope that it will be useful,\n");
    printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n");
    printf("Library General Public License for more details.\n\n");
    printf("Written by Brent N. Chun <bnc@caltech.edu>\n");
}

void pcp_options_init(pcp_options *opts)
{
    opts->cmd_type     = PCP_NULL_CMD;
    opts->verbose      = 0;
    opts->port         = PCP_DEF_PORT;
    opts->fanout       = PCP_DEF_FANOUT;
    opts->frag_size    = PCP_DEF_FRAG_SIZE;
    opts->best_effort  = 0;
    opts->args_index   = 1;
}

void pcp_options_read_pcprc(pcp_options *opts)
{
    char  *pcprc, *home;
    char  attr[128];
    int   value;
    FILE  *f;

    if ((home = getenv("HOME")) == NULL)
        return;
    pcprc = (char *)xmalloc(strlen(home) + SLASHBYTE_SIZE + strlen(".pcprc") + 
                            NULLBYTE_SIZE);
    sprintf(pcprc, "%s/.pcprc", home);

    if (!file_exists(pcprc)) {
        xfree(pcprc);
        return;
    }
    if ((f = fopen(pcprc, "r")) == NULL) {
        xfree(pcprc);
        return;
    }
    while (fscanf(f, "%127s %d\n", attr, &value) != EOF) {
        if (strcmp(attr, "fanout") == 0) {
            opts->fanout = value;
            if (opts->fanout < 1) {
                fprintf(stderr, "Fanout %d must be >= 1\n", opts->fanout);
                exit(3);
            }
        }
        else if (strcmp(attr, "frag_size") == 0) {
            opts->frag_size = value;
            if (opts->frag_size < 1) {
                fprintf(stderr, "Fragment size %d must be >= 1\n",
                        opts->frag_size);
                exit(3);
            }
        }
        else {
            fprintf(stderr, "Unrecognized option %s in .pcprc\n", attr);
        }
    }
    fclose(f);
    xfree(pcprc);
}

void pcp_options_parse(pcp_options *opts, int argc, char **argv)
{
    int c;
    extern char *optarg;
    static struct option long_options[] =
    {
        { "best-effort", no_argument, NULL, 'b' },
        { "checksum", no_argument, NULL, 'c' },
        { "delete", no_argument, NULL, 'd' },
        { "help", no_argument, NULL, 'h' },
        { "verbose", no_argument, NULL, 'v' },
        { "version", no_argument, NULL, 'V' },
        { "fanout", required_argument, NULL, 'f' },
        { "frag-size", required_argument, NULL, 'n' },
        { "port", required_argument, NULL, 'P' },
        { 0, 0, 0, 0 }
    };

    while ((c = getopt_long (argc, argv, "bcdhvVf:n:P:", long_options, 
                             (int *)0)) != EOF) {
        switch (c) {

        case 'b':
            opts->best_effort = 1;
            opts->args_index++;
            break;

        case 'c':
            set_cmd_type(opts, PCP_CHECKSUM_CMD);
            opts->args_index++;
            break;

        case 'd':
            set_cmd_type(opts, PCP_DELETE_CMD);
            opts->args_index++;
            break;

        case 'h':
            print_help(argv[0], long_options);
            exit(0);

        case 'v':
            opts->verbose = 1;
            opts->args_index++;
            break;

        case 'V':
            print_version();
            exit(0);

        case 'f':
            opts->fanout = atoi(optarg);
            if (!fanout_ok(opts->fanout))
                exit(3);
            opts->args_index += 2;
            break;

        case 'n':
            opts->frag_size = atoi(optarg);
            if (!frag_size_ok(opts->fanout))
                exit(3);
            opts->args_index += 2;
            break;

        case 'P':
            opts->port = atoi(optarg);
            if (!port_ok(opts->port))
                exit(3);
            opts->args_index += 2;
            break;
        }

    }
    if (opts->cmd_type == PCP_NULL_CMD)
        set_cmd_type(opts, PCP_WRITE_CMD); /* default */
}
