/*
 * Copyright (C) 2002 Brent N. Chun <bnc@caltech.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "gexec_options.h"

static int nprocs_ok(int nprocs)
{
    if (nprocs < 0) {
        fprintf(stderr, "-n option (nprocs) must be >= 0 (0 = all nodes)\n");
        return 0;
    }
    return 1;
}

static int prefix_type_ok(int prefix_type)
{
    switch (prefix_type) {
    case GEXEC_NO_PREFIX:
    case GEXEC_VNN_PREFIX:
    case GEXEC_HOST_PREFIX:
    case GEXEC_IP_PREFIX:
        return 1;
    default:
        fprintf(stderr, "-p option (prefix_type) invalid\n");
        return 0;
    }
}

static int port_ok(int port)
{
    if (port < 1) {
        fprintf(stderr, "-P option (port) must be >= 1\n");
        return 0;
    }
    return 1;
}

static int tree_fanout_ok(int tree_fanout)
{
    if (tree_fanout < 1) {
        fprintf(stderr, "-F option (tree_fanout) must be >= 1\n");
        return 0;
    }
    return 1;
}

static void check_options(gexec_options *opts)
{
    if (opts->nprocs == -1) {
        fprintf(stderr, "Must specify -n option (nprocs)\n");
        exit(3);
    }
}

static int parse_prefix_name(char *name)
{
    char   *names[] = { "none", "vnn",  "host", "ip", NULL };
    int    values[] = { GEXEC_NO_PREFIX, 
                        GEXEC_VNN_PREFIX,
                        GEXEC_HOST_PREFIX,
                        GEXEC_IP_PREFIX,
                        -1 };
    size_t l;
    int    i;

    l = strlen(name);
    for (i = 0; names[i] != NULL; i++)
        if (!strncasecmp(names[i], name, l))
            return values[i];
    return values[i];
}

static void print_help(const char *argv0, const struct option *longopts)
{
	const char *p;
	const struct option *q;

	if ((p = strchr(argv0, '/')) == NULL)
		p = argv0;
	else
		p++;
    printf("Usage: %s [OPTIONS] prog arg1 arg2 ...\n"
            "       Execute prog on hosts defined by $GEXEC_SVRS.\n\n"
            "       Note: Path of prog will be determined on this host.\n\n", p);
	for (q = longopts; q->name; q++)
		if (q->has_arg == no_argument)
			printf("     -%c  --%s\n", q->val, q->name);
		else
			printf("     -%c  --%s=v\n", q->val, q->name);
}

static void print_version()
{
    printf("GEXEC %s\n\n", VERSION);
    printf("Copyright (C) 2002 Brent N. Chun\n");
    printf("This program is distributed in the hope that it will be useful,\n");
    printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n");
    printf("Library General Public License for more details.\n\n");
    printf("Written by Brent N. Chun <bnc@caltech.edu>\n");
}

static void print_usage()
{
    printf("Usage: gexec -n nprocs prog arg1 arg2 ...\n");
}

void gexec_options_init(gexec_options *opts)
{
    opts->spawn_opts.verbose        = GEXEC_DEF_VERBOSE;
    opts->spawn_opts.detached       = GEXEC_DEF_DETACHED;
    opts->spawn_opts.port           = GEXEC_DEF_PORT;
    opts->spawn_opts.tree_fanout    = GEXEC_DEF_TREE_FANOUT;
    opts->spawn_opts.prefix_type    = GEXEC_DEF_PREFIX_TYPE;
    opts->nprocs                    = -1;
}

void gexec_options_parse(gexec_options *opts, int argc, char **argv,
                         int *args_index)
{
    int                  c;
    extern char          *optarg;
    static struct option long_options[] =
    {
        { "detached", no_argument, NULL, 'd' },
        { "help", no_argument, NULL, 'h' },
        { "verbose", no_argument, NULL, 'v' },
        { "version", no_argument, NULL, 'V' },
        { "fanout", required_argument, NULL, 'f' },
        { "nprocs", required_argument, NULL, 'n' },
        { "prefix-type", required_argument, NULL, 'p' },
        { "port", required_argument, NULL, 'P' },
        { 0, 0, 0, 0 }
    };


    *args_index = 1;
    while ((c = getopt_long (argc, argv, "+dhvVf:n:p:P:", long_options,
                             (int *)0)) != EOF) {
        switch (c) {

        case 'd':
            opts->spawn_opts.detached = 1;
            (*args_index)++;
            break;

        case 'h':
            print_help(argv[0], long_options);
            exit(0);

        case 'v':
            opts->spawn_opts.verbose = 1;
            (*args_index)++;
            break;

        case 'V':
            print_version();
            exit(0);

        case 'f':
            opts->spawn_opts.tree_fanout = atoi(optarg);
            if (!tree_fanout_ok(opts->spawn_opts.tree_fanout))
                exit(3);
            (*args_index) += 2;
            break;

        case 'n':
            opts->nprocs = atoi(optarg);
            if (!nprocs_ok(opts->nprocs))
                exit(3);
            (*args_index) += 2;
            break;

        case 'p':
            opts->spawn_opts.prefix_type = parse_prefix_name(optarg);
            if (!prefix_type_ok(opts->spawn_opts.prefix_type))
                exit(3);
            (*args_index) += 2;
            break;

        case 'P':
            opts->spawn_opts.port = atoi(optarg);
            if (!port_ok(opts->spawn_opts.port))
                exit(3);
            (*args_index) += 2;
            break;
        }

    }
    check_options(opts);
    if (*args_index == argc) {
        print_usage();
        exit(3);
    }
}

void gexec_options_print(gexec_options *opts)
{
    printf("gexec_options:  nprocs       = %d\n", opts->nprocs);
    printf("gexec_options:  verbose      = %d\n", opts->spawn_opts.verbose);
    printf("gexec_options:  detached     = %d\n", opts->spawn_opts.detached);
    printf("gexec_options:  port         = %d\n", opts->spawn_opts.port);
    printf("gexec_options:  tree_fanout  = %d\n", opts->spawn_opts.tree_fanout);
    printf("gexec_options:  prefix_type  = %d\n", opts->spawn_opts.prefix_type);
}
