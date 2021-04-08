Coder Upgrade
=============

This module provides upgrade routines to modify a source code file for changes
to the Backdrop core APIs. Currently, the module provides routines for an 
upgrade from Drupal to Backdrop 1.0

This module utilizes the Grammar Parser library to modify source code in a
precise and programmatic fashion. The module utilizes the familiar Backdrop hook
system to invoke upgrade routines, allowing other modules to enhance or modify
a routine.

Contributed modules that define an API can develop upgrade routines that would
enable other contributed modules relying on that API to upgrade their code.

Installation and usage
------------

1. Copy Coder Upgrade directory to your modules directory
2. Enable the module at the module administration page
3. Coder Upgrade by default will create a `coder_upgrade` directory in your 
Backdrop `/files` directory. If your `/files` directory is not writable, there 
will be a system message on install warning you of this.
4. If your base `coder_upgrade` directory was not created on install, or if you 
wish to change the default location, go to `admin/config/development/coder-upgrade/settings` 
and set an alternative.
5. Under the base directory Coder Upgrade creates three subdirectories:
 - old
 - new
 - patch
6. To convert modules, place the unpacked Drupal 7 module in the 'old' directory
7. Visit `admin/config/development/coder-upgrade`, select the 'Directories' tab,
and check the box next to the module you wish to convert. Click the 'Convert
files' button.
8. The Drupal 7 code will be converted, then copied to the 'new' directory.

Dependencies
------------
There are no dependencies. Unlike the Drupal version, the Grammar Parser 
Library is bundled with Coder Upgrade.

Developers
----------
In the event of issues with the upgrade routines, debug output may be enabled on
the settings page of this module. It is recommended to enable this only with
smaller files that include the code causing an issue.

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.

Current Maintainers
-------------------

- Docwilmot (https://github.com/docwilmot
- Robert J. Lang ([bugfolder](http://github.com/bugfolder))

Credits
-------

Jim Berry ("solotandem", http://drupal.org/user/240748)

