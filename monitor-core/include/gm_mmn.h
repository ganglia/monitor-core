#ifndef GM_MMN_H
#define GM_MMN_H

/*
 * MODULE_MAGIC_NUMBER_MAJOR
 * Major API changes that could cause compatibility problems for older modules
 * such as structure size changes.  No binary compatibility is possible across
 * a change in the major version.
 *
 * MODULE_MAGIC_NUMBER_MINOR
 * Minor API changes that do not cause binary compatibility problems.
 * Should be reset to 0 when upgrading MODULE_MAGIC_NUMBER_MAJOR.
 *
 * See the MODULE_MAGIC_AT_LEAST macro below for an example.
 */

/*
 * 20070222.0 (3.1.0-dev) MODULE_MAGIC_COOKIE set to "GM31"
 * 20070918.0 (3.1.0-dev) mmodule_struct change. Pass parameter list 
 *                          C interface modules and python modules. Allow
 *                          configuration file access from a C module.
 * 20080913.0 (3.2-dev)   slurpfile ABI incompatible change in libganglia
 */

#define MMODULE_MAGIC_COOKIE 0x474D3332UL /* "GM32" */

#ifndef MMODULE_MAGIC_NUMBER_MAJOR
#define MMODULE_MAGIC_NUMBER_MAJOR 20080913
#endif
#define MMODULE_MAGIC_NUMBER_MINOR 0                     /* 0...n */

/**
 * Determine if the current MMODULE_MAGIC_NUMBER is at least a
 * specified value.
 * @param major The major module magic number
 * @param minor The minor module magic number
 * @deffunc GM_MODULE_MAGIC_AT_LEAST(int major, int minor)
 */
#define GM_MODULE_MAGIC_AT_LEAST(major,minor)		\
    ((major) < MMODULE_MAGIC_NUMBER_MAJOR 		\
	|| ((major) == MMODULE_MAGIC_NUMBER_MAJOR 	\
	    && (minor) <= MMODULE_MAGIC_NUMBER_MINOR))

#endif /* !GM_MMN_H */
/** @} */
