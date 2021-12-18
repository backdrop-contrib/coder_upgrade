<?php
/**
 * @file
 * Miscellaneous conversion routine file for the coder_upgrade module.
 * These routines upgrade text using simple regular expressions.
 *
 * Copyright 2008-11 by Jim Berry ("solotandem", http://drupal.org/user/240748)
 */

/**
 * Implements hook_upgrade_regex_alter().
 *
 * Upgrades text using simple regular expressions.
 */
function coder_upgrade_upgrade_regex_alter(&$file) {
  cdp("inside " . __FUNCTION__);
  $hook = 'regex_simple_changes';
  $cur = $file;
  $new = $cur;

  $from = array();
  $to = array();

  // taxonomy_get_parents() has been renamed.
  $from[] = "/taxonomy_get_parents/";
  $to[] = "taxonomy_term_load_parents";

  // taxonomy_get_parents_all() has been renamed.
  $from[] = "/taxonomy_get_parents_all/";
  $to[] = "taxonomy_term_load_parents_all";

  // taxonomy_get_children() has been renamed.
  $from[] = "/taxonomy_get_children/";
  $to[] = "taxonomy_term_load_children";

  // taxonomy_get_term_by_name() has been renamed.
  $from[] = "/taxonomy_get_term_by_name/";
  $to[] = "taxonomy_term_load_multiple_by_name";

  // format_username() has been renamed.
  $from[] = "/format_username/";
  $to[] = "user_format_name";

  // temporarily rename Drupal links so the other regexes dont create false 
  // links.
  $from[] = "/drupal\.org/";
  $to[] = "mothershiporg";

  // change Drupal to Backdrop throughtout.
  $from[] = "/drupal/";
  $to[] = "backdrop";

  // change Drupal to Backdrop throughtout.
  $from[] = "/Drupal/";
  $to[] = "Backdrop";

  // change Drupal to Backdrop throughtout.
  $from[] = "/DRUPAL/";
  $to[] = "BACKDROP";

  // rename Drupal links.
  $from[] = "/mothershiporg/";
  $to[] = "drupal.org";

  coder_upgrade_do_conversions($from, $to, $new);
  coder_upgrade_save_changes($cur, $new, $file, $hook);
}

/**
 * Implements hook_upgrade_regex_js_alter().
 *
 * Upgrades js files using simple regular expressions.
 */
function coder_upgrade_upgrade_regex_js_alter(&$file) {
  cdp("inside " . __FUNCTION__);
  $hook = 'regex_simple_changes';
  $cur = $file;
  $new = $cur;

  $from = array();
  $to = array();

  // temporarily rename Drupal links so the other regexes dont create false 
  // links.
  $from[] = "/drupal\.org/";
  $to[] = "mothershiporg";

  // change Drupal to Backdrop throughtout.
  $from[] = "/drupal/";
  $to[] = "backdrop";

  // change Drupal to Backdrop throughtout.
  $from[] = "/Drupal/";
  $to[] = "Backdrop";

  // change Drupal to Backdrop throughtout.
  $from[] = "/DRUPAL/";
  $to[] = "BACKDROP";

  // rename Drupal links.
  $from[] = "/mothershiporg/";
  $to[] = "drupal.org";

  coder_upgrade_do_conversions($from, $to, $new);
  coder_upgrade_save_changes($cur, $new, $file, $hook);
}

/**
 * Upgrades admin paths.
 *
 * http://drupal.org/node/224333#admin_path_changes
 * http://drupal.org/node/224333#clean_urls_search_paths
 */
function coder_upgrade_upgrade_admin_path_changes(&$from, &$to) {
  $path_conversions = array(
    'admin/reports/settings' => 'admin/config/system/statistics',
    'admin/settings/filters' => 'admin/config/content/filter',
    'admin/build/modules' => 'admin/modules',
    'admin/build/themes' => 'admin/appearance',
    'admin/build/path' => 'admin/config/search/path',
    'admin/content/types' => 'admin/structure/types',
    'admin/content/taxonomy' => 'admin/structure/taxonomy',
    'admin/content/forum' => 'admin/structure/forum',
    'admin/build/testing' => 'admin/config/development/testing',
    'admin/settings/site-maintenance' => 'admin/config/development/maintenance',
    'admin/settings/performance' => 'admin/config/development/performance',
    'admin/settings/filters' => 'admin/config/content/formats',
    'admin/settings/search' => 'admin/config/search/settings',
    'admin/settings/clean-urls' => 'admin/config/search/clean-urls',
    'admin/settings/image' => 'admin/config/media/image',
    'admin/settings' => 'admin/config',
    'admin/build' => 'admin/structure',
  );

  foreach ($path_conversions as $old_path => $new_path) {
    $from[] = "@(['\"])$old_path(/?.*?)(['\"])@";
    $to[] = "$1$new_path$2$1";
  }
}

/**
 * Implements hook_upgrade_regex_info_alter().
 *
 * Alters the content of a code file using regular expressions.
 *
 * @param string $file
 *   The content of the file to be converted.
 */
function coder_upgrade_upgrade_regex_info_alter(&$file) {
  cdp("inside " . __FUNCTION__);
  $hook = 'info_file';
  $cur = $file;
  $new = $cur;

  $from = array();
  $to = array();

  // Info file should specify core = 7.x.
  $from[] = '@^core\s?=\s?.*?$@m';
  // Replace the "core = 7.x" line with "backdrop = 1.x", and add a line for the
  // project type.
  // @todo: We are assuming .info file can only belong to a D7 module here, but
  // it can also be a theme. See: https://github.com/backdrop-contrib/coder_upgrade/issues/63
  $to[] = "backdrop = 1.x\ntype = module";

  // Remove packaging info.
  $from[] = '@[\n\r](;\sInformation\sadded)(.+?)(datestamp\s?=\s?"\d+")[\n\r]@s';
  $to[] = "";

  // Remove files[] entries - Backdrop uses hook_autoload_info() instead.
  $from[] = '@files\[\]\s?=?\s(.+)[\r\n]*@m';
  $to[] = "";

  coder_upgrade_do_conversions($from, $to, $new);
  coder_upgrade_save_changes($cur, $new, $file, $hook);
}
