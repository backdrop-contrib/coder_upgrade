<?php

/**
 * @file
 * This include file implements coder functionality to check for Internationalization issues.
 */

/**
 * Implements hook_reviews().
 */
function coder_review_i18n_reviews() {
  $argex = '(((\$?)[a-zA-Z_]+((\([^)]*\))|\[[^\]]*\])?)|[0-9]+(\.[0-9]*)?|\'\'|"")';

  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\s\(]l\s*\(\s*["\']',
    '#filename-not' => array('install'),
    '#warning' => array(
      '#text' => 'The $text argument to !l() should be enclosed within !t() so that it is translatable.',
      '#args' => array(
        '!l' => _backdropapi('l'),
        '!t' => _backdropapi('t'),
      ),
    ),
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\s\(]l\s*\(\s*[\'"]',
    '#function' => '_install$',
    '#warning' => array(
      '#text' => 'The $text argument to !l() should be enclosed within !st() so that it is translatable from within the install.',
      '#args' => array(
        '!l' => _backdropapi('l'),
        '!st' => _backdropapi('st'),
      ),
    ),
    '#filename' => array('install'),
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\s\(]t\s*\(\s*[\'"]',
    '#function' => '_install$',
    '#warning' => array(
      '#text' => 'Use !st() instead of !t() in !hook_install()',
      '#args' => array(
        '!st' => _backdropapi('st'),
        '!t' => _backdropapi('t'),
        '!hook_install' => _backdropapi('hook_install'),
      ),
    ),
    '#filename' => array('install'),
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\s\(]alert\s*\(\s*[\'"]',
    '#filename' => array('js'),
    '#warning' => 'Javascript strings should be passed through Backdrop.t().',
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '#title\s*=>\s*[\'"][\'"]',
    '#warning' => array(
      '#text' => 'The FAPI #title should be enclosed within !t() so that it is translatable.',
      '#args' => array(
        '!l' => _backdropapi('l'),
        '!t' => _backdropapi('t'),
      ),
    ),
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\s\(]form_error\s*\(\s*' . $argex . '\s*,\s*[\'"]',
    '#warning' => array(
      '#text' => 'The $message argument to !form_error() should be enclosed within !t() so that it is translatable.',
      '#args' => array(
        '!form_error' => _backdropapi('form_error'),
        '!t' => _backdropapi('t'),
      ),
    ),
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\'"]title[\'"]\s*=>\s*[\'"][^<]',
    '#warning' => array(
      '#text' => "The 'title' option should be enclosed within !t() so that it is translatable.",
      '#args' => array(
        '!t' => _backdropapi('t'),
      ),
    ),
    '#source' => 'allphp',
    '#function' => '_link$',
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\s\(]backdrop_set_title\s*\(\s*[\'"]',
    '#warning' => array(
      '#text' => 'The $title argument to !backdrop_set_title() should be enclosed within !t() so that it is translatable.',
      '#args' => array(
        '!backdrop_set_title' => _backdropapi('backdrop_set_title'),
        '!t' => _backdropapi('t'),
      ),
    ),
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\s\(]backdrop_set_message\s*\(\s*[\'"]',
    '#warning' => array(
      '#text' => 'The $message argument to !backdrop_set_message() should be enclosed within !t() so that it is translatable.',
      '#args' => array(
        '!backdrop_set_message' => _backdropapi('backdrop_set_message'),
        '!t' => _backdropapi('t'),
      ),
    ),
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\s\(]watchdog\s*\(\s*' . $argex . '\s*,\s*(t|st)\(',
    '#warning' => array(
      '#text' => 'The $message argument to !watchdog() should NOT be enclosed within !t(), so that it can be properly translated at display time.',
      '#args' => array(
        '!watchdog' => _backdropapi('watchdog'),
        '!t' => _backdropapi('t'),
      ),
    ),
  );
  // @NOTE: Add duplicate of the 6.x upgrade rule.
  $rules[] = array(
    '#type' => 'regex',
    '#function' => '_menu$',
    '#source' => 'allphp',
    '#value' => '\'title\'\s*=>\s*t\(|\'description\'\s*=>\s*t\(',
    '#warning' => array(
      '#text' => 'Menu item titles and descriptions should NOT be enclosed within !t().',
      '#args' => array(
        '!t' => _backdropapi('t'),
      ),
    ),
  );
  $rules[] = array(
    '#type' => 'regex',
    '#value' => '[\s\(](t|st)\s*\(\s*[\'"](\s+|[^\)]*?\s+[\'"]\s*[,\)])',
    '#not' => '[\s\(](t|st)\s*\(\s*[\'"][^\s].*?([\'"]\s+[^,\)])*.*[^\s][\'"][,\)]',
    '#source' => 'allphp',
    '#warning' => array(
      '#text' => 'The $string argument to !t() should not begin or end with a space.',
      '#args' => array(
            '!t' => _backdropapi('t'),
      ),
      '#link' => _backdropnode(304150),
    ),
  );

  $review = array(
    '#title' => 'Internationalization',
    '#rules' => $rules,
    '#version' => 2,
    '#image' => 'images/i18n.png',
  );
  return array('i18n' => $review);
}
