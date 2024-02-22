<?php
/**
 * @file
 * Provides conversion routines applied to install files.
 *
 * These routines use the grammar parser.
 *
 * Copyright 2009-11 by Jim Berry ("solotandem", http://drupal.org/user/240748)
 */

use PhpParser\Comment;
use PhpParser\Comment\Doc;
use PhpParser\NodeFinder;
use PhpParser\Node\Stmt\Nop;


/**
 * Implements hook_upgrade_parser_install_alter().
 *
 * Applies finishing touches to the install file.
 *
 * @param PGPReader $reader
 *   The object containing the grammar statements of the file to convert.
 */
function coder_upgrade_upgrade_parser_install_alter(&$ast) {
  coder_upgrade_convert_install_file_comment($ast);
}



/**
 * Adds a Doxygen style comment to install files.
 *
 * Install files must include a Doxygen style file comment.
 *
 * @param PGPReader $reader
 *   The object containing the grammar statements of the file to convert.
 */
function coder_upgrade_convert_install_file_comment(&$ast) {
  cdp("inside " . __FUNCTION__);


  if (empty($ast)) {
    return;
  }

  $nodeFinder = new NodeFinder;
  $comments = $ast[0]->getComments();

  if (!empty($comments[0])) {
    if (preg_match('#\*\s*@file#', $comments[0]->__toString())) {
      // The file already includes a file comment.
      return;
    }
  }

  /*
   * A standards-compliant Backdrop php file would begin with:
   * <?php
   * // $\Id: .. username Exp $ (slash added to prevent CVS from changing this line)
   *
   * ... rest of file
   *
   * Look for these types of statements in this order. Insert the file
   * comment after the Id line or the open tag (if no Id line).
   */
  coder_upgrade_add_install_file_comment($ast);
}

function coder_upgrade_add_install_file_comment(&$ast) {
  cdp("inside " . __FUNCTION__);
  global $_coder_upgrade_module_name;

  $nop = new Nop();
  // Insert file document comment.
  $comment = "/**\n * @file\n * Install, update and uninstall functions for the $_coder_upgrade_module_name module.\n *\n */";

  $doc = new Doc($comment);
  $nop->setDocComment($doc);
  array_unshift($ast, $nop);
}
