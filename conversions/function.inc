<?php
/**
 * @file
 * Provides conversion routines applied to functions (or hooks).
 *
 * These routines use the grammar parser.
 *
 * Copyright 2009-11 by Jim Berry ("solotandem", http://drupal.org/user/240748)
 */
use PhpParser\Comment\Doc;


/**
 * Implements hook_upgrade_hook_alter().
 */
function coder_upgrade_upgrade_hook_alter(&$node, &$reader, $hook) {
  cdp("inside " . __FUNCTION__);
  $item = &$node;

  // Changes: implementation_hook_comment
  if ($item->getDocComment()) {
    // Update document comment for hook implementations.
    $doc = new Doc('@\*\s+[iI]mplement.*?(hook_.*?)(\(\)|)(\.|)$@m', "* Implements $1().", $item->getDocComment());
    $item->setDocComment($doc);
  }
  else {
    // Add a document comment.
    $doc = new Doc("/**\n * @todo Please document this function.\n * @see http://drupal.org/node/1354\n */");
    $item->setDocComment($doc);
  }

  global $_coder_upgrade_theme_registry;
  cdp($node->data->name);
  // if (strpos($node->data->name, 'theme_') === 0) {
    // $theme_name = substr($node->data->name, strpos($node->data->name, "theme_") + 6);
    // if (!isset($_coder_upgrade_theme_registry[$theme_name])) {
      // $msg = 'TODO: Should this theme ' . $theme_name . ' be declared in hook_theme()?';
      // clp($msg);
      // $node->data->body->insertFirst($editor->commentToStatement($msg));
    // }
  // }
}

/**
 * Implements hook_upgrade_hook_update_N_alter().
 */
function coder_upgrade_upgrade_hook_update_N_alter(&$node, &$reader) {
  cdp("inside " . __FUNCTION__);
  $editor = PGPEditor::getInstance();

  $item = &$node;
  dpm($item);
  $doc = new Doc("// TODO The old hook_update_N functions cannot be applied to Backdrop.");
  $item->setDocComment($doc);
  $item->stmts = array();
}

