Coder Upgrade
=============

This module carries out many of the changes required to port a Drupal 7 module to Backdrop 1.x. 

In some cases, Coder Upgrade will make all changes required. In most cases, you will still need to do some minor touch-up here and there. And for more complex modules, you may have to do further manual edits to address issues that Coder Upgrade can't handle (at least, not yet). Even there, though, Coder Upgrade will handle many of the required changes, still saving some time.

This module utilizes the Grammar Parser library (v. 1.0) to modify the source code. The module also utilizes the familiar Backdrop hook
system to invoke upgrade routines, allowing other modules to enhance or modify
the upgrade process.

Installation
------------

Coder Upgrade is run from within a Backdrop installation.

1. Copy Coder Upgrade directory to your modules directory.
2. Enable the module at the module administration page.

Usage
-----

Go to `admin/config/development/coder-upgrade` to perform upgrades.

Coder Upgrade by default will create a `coder_upgrade` directory in your 
Backdrop `/files` directory. If your `/files` directory is not writable, there 
will be a system message on install warning you of this. If your base `coder_upgrade` directory was not created on install, or if you 
wish to change the default location, go to `admin/config/development/coder-upgrade/settings` 
and set an alternative.

Under the base directory Coder Upgrade creates three subdirectories:

 - `old`
 - `new`
 - `patch`

To convert modules:

1. Place the unpacked Drupal 7 module in the `old` directory.
2. Visit `admin/config/development/coder-upgrade`, select the Directories tab,
and check the box next to the module you wish to convert. Click the Convert
files button.
3. The Drupal 7 code will be converted, then copied to the `new` directory.
4. In addition to the converted module in the `new` directory, a patch that converts the old module will be placed in the `patch` directory.

Details
-------

For an overview of the process of porting a Drupal 7 module to Backdrop, please see [the official Backdrop porting guide](https://docs.backdropcms.org/converting-modules-from-drupal).

The changes made by Coder Upgrade fall into a few general categories.

### Replacement of Drupal functions with Backdrop equivalents

Some functions have been renamed in Backdrop (most obviously, those with "drupal" as part of their name). CU makes these changes automatically.

### .info File

CU will update the .info file per the guidelines. It doesn't touch the version, though; you'll still need to manually edit that value.

### Configuration Management

The most extensive and visible changes relate to Configuration Management: for values formerly stored in the `variables` table, replacing them with config files. In general, CU will detect these by the presence of calls to `variable_get()` and `variable_set()` and will make the necessary substitutions, including creation/alteration of functions in the .install file, creation of a config directory and file, and, if necessary, an implementation of `hook_update_N()` to perform the needed conversion of data values. A few things to watch out for, though:

- If the `variable_set()` or `variable_get()` expression contains an expression too complex for CU to handle, CU will insert a value `"dynamic variable in file..."` in the relevant place in the converted file. You'll need to go through and manually fix these. 
- CU assumes that any `variable_get()` variable name that does _not_ begin with the module name is a variable from some other module, and will insert a `"TODO..."` comment at that point. You'll need to go through and manually handle these.
- CU assumes that all `variable_set()` and `variable_get()` variables should be converted to config variables accessed through `config_set()` and `config_get()` and will make those conversions. However, persistent variables that are environment-specific should be _state_ variables, set via [`state_set()`](https://docs.backdropcms.org/api/backdrop/core%21includes%21bootstrap.inc/function/state_set/1) and [`state_get()`](https://docs.backdropcms.org/api/backdrop/core%21includes%21bootstrap.inc/function/state_get/1) and should not be in config files. You'll need to decide for yourself which variables are config and which are state. See the documentation of [`variable_set()`](https://docs.backdropcms.org/api/backdrop/core%21includes%21bootstrap.inc/function/variable_set/1) for further information about this distinction.

In places that CU knows need additional manual attention, it will insert a "dynamic..." value or "TODO..." comment into the converted file. It's a good idea to do a search for those two words (not including the ellipsis) to find all such instances. By comparing the (partially) converted code with the corresponding original code, it will often be clear what additional edits need to be made.

### Using `system_settings_form()`

Configuration pages often use `system_settings_form()` to create and edit configuration variables. CU will automatically handle the changes to functions that call `system_settings_form()`.

### Things Coder Upgrade Doesn't Do

* Backdrop has a specific recommended way of [organizing functions and files within a module](https://docs.backdropcms.org/converting-modules-from-drupal#file-organization). You'll need to do that reorganization (movement of functions into files and files into subdirectories) yourself.

* Classed entities. [See the "Classed Entities" section of the conversion guide for how to handle these.](https://docs.backdropcms.org/converting-modules-from-drupal)

* Removal of superfluous uninstall code from `hook_uninstall()` implementations. In Drupal modules, tables and variables were often uninstalled explicitly by code in `hook_uninstall()`. In Backdrop, this is often not necessary, and you can (and should) remove unnecessary uninstall code that will be left in place by CU. [More information in the "Install and Uninstall hooks" section.](https://docs.backdropcms.org/converting-modules-from-drupal)

* Interactions with core information that now resides in config. Changes in core have moved some information out of the database and into config files (for example, values related to roles and taxonomy vocabularies). You'll need to make corresponding changes yourself.

Dependencies
------------
There are no dependencies. Unlike the Drupal version, the Grammar Parser Library is bundled with Coder Upgrade. The bundled library has some CU-specific changes relative to the Drupal version.

Developers
----------
In the event of issues with the upgrade routines, debug output may be enabled on
the settings page of this module. This information is very verbose; it is recommended to enable this only with
smaller files that include the code causing an issue.

Current Maintainers
-------------------

- [Docwilmot](https://github.com/docwilmot)
- [Robert J. Lang (bugfolder)]([bugfolder](http://github.com/bugfolder))

Credits
-------

- Grammar Parser Library and Drupal 7 version of Coder Upgrade by [Jim Berry (solotandem)](http://drupal.org/user/240748).

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.


