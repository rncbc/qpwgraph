#ifndef __CONFIG_H
#define __CONFIG_H


/* Define to the name of this package. */
#cmakedefine PROJECT_NAME "@PROJECT_NAME@"

/* Define to the version of this package. */
#cmakedefine PROJECT_VERSION "@PROJECT_VERSION@"

/* Define to the description of this package. */
#cmakedefine PROJECT_DESCRIPTION "@PROJECT_DESCRIPTION@"

/* Define to the homepage of this package. */
#cmakedefine PROJECT_HOMEPAGE_URL "@PROJECT_HOMEPAGE_URL@"


/* Define if debugging is enabled. */
#cmakedefine CONFIG_DEBUG @CONFIG_DEBUG@


/* Define if ALSA MIDI support is available. */
#cmakedefine CONFIG_ALSA_MIDI @CONFIG_ALSA_MIDI@

/* Define if system-tray icon support is available. */
#cmakedefine CONFIG_SYSTEM_TRAY @CONFIG_SYSTEM_TRAY@


/* Define if Wayland is supported */
#cmakedefine CONFIG_WAYLAND @CONFIG_WAYLAND@

#endif // __CONFIG_H

