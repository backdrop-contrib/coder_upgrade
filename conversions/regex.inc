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
  global $_coder_upgrade_module_name;
  $name = $_coder_upgrade_module_name;
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

  // theme_html_tag() has been renamed.
  $from[] = "/theme_html_tag/";
  $to[] = "theme_head_tag";

  // hook_page_build() has been replaced: https://api.backdropcms.org/change-records/new-layout-module-implements-drag-drop-model-building-layouts
  $from[] = "/" . $name . "_page_build/";
  $to[] = $name . "_preprocess_page";

  // hook_library() has been renamed.
  $from[] = "/" . $name . "_library/";
  $to[] = $name . "_library_info";

  // hook_library_alter() has been renamed.
  $from[] = "/" . $name . "_library_alter/";
  $to[] = $name . "_library_info_alter";

  // replace special sorting function: https://api.backdropcms.org/change-records/generic-sorting-function-replace-implementations-uasort
  $from[] = '/uasort\(\$items, *' . "'element_sort'\)/";
  $to[] = 'backdrop_sort($items, array("#weight"))';

  // replace special sorting function 2
  $from[] = '/uasort\(\$items, *' . "'element_sort_title'\)/";
  $to[] = 'backdrop_sort($items, array("#title" => SORT_STRING))';

  // replace special sorting function 3
  $from[] = '/uasort\(\$items, *' . "'drupal_sort'\)/";
  $to[] = 'backdrop_sort($items)';

  // replace special sorting function 4
  $from[] = '/uasort\(\$items, *' . "'drupal_sort_title'\)/";
  $to[] = 'backdrop_sort($items, array("title" => SORT_STRING))';

  // replace special sorting function 5
  $from[] = '/uasort\(\$items, *' . "'admin_bar_element_sort'\)/";
  $to[] = 'backdrop_sort($items, array("#weight" => SORT_NUMERIC, "#title" => SORT_STRING))';

  // temporarily rename Drupal links so the other regexes don't create false 
  // links.
  $from[] = "/drupal\.org/";
  $to[] = "mothershiporg";

  // change Drupal to Backdrop throughout.
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
 * Alters the text of a code file using regular expressions.
 *
 * Module Info / Install
 * http://drupal.org/node/224333#info_core_7 (this anchor does not exist on the chronological page)
 *
 * @param string $file
 *   The text of the file to convert.
 */
function coder_upgrade_upgrade_regex_info_alter(&$file) {
  cdp("inside " . __FUNCTION__);
  $hook = 'info_file';
  $cur = $file;
  $new = $cur;

  $from = array();
  $to = array();

  // Info file should specify core = 7.x.
  $from[] = '@^core\s+=\s+.*?$@m';
  $to[] = "backdrop = 1.x\ntype = module";

  // Remove packaging info.
  $from[] = '@[\n\r](;\sInformation\sadded)(.+?)(datestamp\s=\s"\d+")[\n\r]@s';
  $to[] = "";

  // Remove files[] entries.
  $from[] = '@files\[\]\s=\s(.+)[\r\n]*@m';
  $to[] = "";

  coder_upgrade_do_conversions($from, $to, $new);
  coder_upgrade_save_changes($cur, $new, $file, $hook);
}
