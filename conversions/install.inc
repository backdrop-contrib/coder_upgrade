<?php
/**
 * @file
 * Provides conversion routines applied to install files.
 *
 * These routines use the grammar parser.
 *
 * Copyright 2009-11 by Jim Berry ("solotandem", http://drupal.org/user/240748)
 */

/**
 * The upgrades to these functions are documented at the following urls.
 *
 * Module Info / Install
 * http://drupal.org/node/224333#afile
 * http://drupal.org/node/224333#info_core_7 (this anchor does not exist on the chronological page)
 */

/**
 * Implements hook_upgrade_parser_install_alter().
 *
 * Applies finishing touches to the install file.
 *
 * @param PGPReader $reader
 *   The object containing the grammar statements of the file to convert.
 */
function coder_upgrade_upgrade_parser_install_alter(&$reader) {
  global $_coder_upgrade_module_name;
  $config_names = config_get_names_with_prefix('coder_upgrade.' . $_coder_upgrade_module_name);
  if (!empty($config_names)) {
    $config = config($config_names[0]);
    $config_data = $config->get();

    $nodes = &$reader->getFunctions();
    if (!empty($nodes)) {
      foreach ($nodes as &$node) {
        $item = &$node->data;
        $name = &$item->name;
        if ($name == 'xxx') {
          $dummy = $node;
        }
      }
    }

    // Create a hook_update_1000() implementation.
    coder_upgrade_create_update_1000($reader, $nodes, $config_data);

    // Create an hook_update_last_removed() implementation.
    coder_upgrade_create_update_last_removed($reader, $nodes);

    if ($dummy) {
      $dummy->container->delete($dummy);
    }
  }
  coder_upgrade_convert_install_file_comment($reader);
}

function coder_upgrade_create_update_1000(&$reader, &$nodes, $config_data) {
  global $_coder_upgrade_module_name;
  // Use the editor to set the function parameters.
  $editor = PGPEditor::getInstance();
  $strings = array();
  $delstrings = array();

  $body = new PGPBody();

  $string = "\$config = config('$_coder_upgrade_module_name.settings');";
  $body_content = $editor->textToStatements($string)->getElement(0);
  $body->insertLast($body_content);

  foreach ($config_data as $key => $line) {
    $string = "  \$config->set('$key', update_variable_get('$key', '$line'));";
    $body_content = $editor->textToStatements($string)->getElement(0);
    $body->insertLast($body_content);
  }

  foreach ($config_data as $key => $line) {
    $string = '  update_variable_del(\'' . $key . '\');';
    $body_content = $editor->textToStatements($string)->getElement(0);
    $body->insertLast($body_content);
  }

  // Set values for the new hook function.
  $comment = array(
    'type' => T_DOC_COMMENT,
    'value' => "/**\n * Implements hook_update_N().\n */",
  );
  $modulename = $_coder_upgrade_module_name . '_update_1000';

  $function = new PGPClass($modulename);
  $function->comment = $comment;
  $function->type = T_FUNCTION;
  $function->parameters = new PGPList();


  $function->body = $body;

  $node = end($nodes);
  $container = &$node->container;

  // Insert the new function before the old function.
  $container->insertAfter($node, $function, 'function');
  // Insert a blank line.
  $whitespace = array(
    'type' => T_WHITESPACE,
    'value' => 1,
  );
  $container->insertAfter($node, $whitespace, 'whitespace');
}


function coder_upgrade_create_update_last_removed(&$reader, &$nodes) {
  global $_coder_upgrade_module_name;
  $dummy_node = NULL;
  $ns = array();
  $last_removed = '';
  if (!empty($nodes)) {
    foreach ($nodes as &$node) {
      $item = &$node->data;
      $name = &$item->name;
      if (preg_match('@[a-zA-Z_]+(update_)(\d+)$@', $name, $matches)) {
        $ns[$matches[2]] = $node;
      }
      if ($name == 'xxx') {
        $dummy = $node;
      }
    }
    $last_removed = max(array_keys($ns));
  }

  // Set values for the new hook function.
  $comment = array(
    'type' => T_DOC_COMMENT,
    'value' => "/**\n * Implements hook_update_last_removed().\n */",
  );
  $modulename2 = $_coder_upgrade_module_name . '_update_last_removed';

  // Create the new hook function.
  $function = new PGPClass($modulename2);
  $function->comment = $comment;
  $function->type = T_FUNCTION;
  $function->parameters = new PGPList();

  // Use the editor to set the function parameters.
  $editor = PGPEditor::getInstance();
  //  $editor->setParameters($function, $parameters);

  $string = "return $last_removed;\n";
  cdp(print_r($string, 1));

  // Copy the case (or if) block as the body of the function.
  $function->body = $editor->textToStatements($string);
  cdp(get_class($function->body));
  cdp($editor->statementsToText($function->body));

  $node = end($nodes);
  $container = &$node->container;

  // Insert the new function before the old function.
  $container->insertAfter($node, $function, 'function');
  // Insert a blank line.
  $whitespace = array(
    'type' => T_WHITESPACE,
    'value' => 1,
  );
  $container->insertAfter($node, $whitespace, 'whitespace');
}



/**
 * Adds a Doxygen style comment to install files.
 *
 * Install files must include a Doxygen style file comment.
 *
 * @param PGPReader $reader
 *   The object containing the grammar statements of the file to convert.
 */
function coder_upgrade_convert_install_file_comment(&$reader) {
  cdp("inside " . __FUNCTION__);

  $comments = &$reader->getComments();
  foreach ($comments as &$comment) {
    if (is_array($comment) && !empty($comment)) {
      if (preg_match('#\*\s*@file#', $comment['value'])) {
        // The file already includes a file comment.
        return;
      }
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

  $statements = &$reader->getStatements();
  if (is_null($statements)) {
    // Currently, the file must begin with an open tag.
    return;
  }
/*
  $statement = $statements->getElement(0);
  if (!is_array($statement) || empty($statement) || $statement['type'] != T_OPEN_TAG) {
    clp('ERROR: file does not begin with an open tag');
    return;
  }
*/
  $statement = $statements->getElement(1);
  if (!is_array($statement) || empty($statement) || $statement['type'] != T_COMMENT || strpos($statement['value'], '// $Id:') !== 0) {
    $position = 1;
    coder_upgrade_add_install_file_comment($reader, $position);
    return;
  }

  $statement = $statements->getElement(2);
  if (!is_array($statement) || empty($statement) || $statement['type'] != T_WHITESPACE) {
    $position = 2;
    coder_upgrade_add_install_file_comment($reader, $position);
    return;
  }

  $position = 3;
  coder_upgrade_add_install_file_comment($reader, $position);
}

function coder_upgrade_add_install_file_comment(&$reader, $position) {
  cdp("inside " . __FUNCTION__);
  global $_coder_upgrade_module_name;

  $statements = &$reader->getStatements();

  // Insert a blank line.
  $whitespace = array(
    'type' => T_WHITESPACE,
    'value' => 1,
  );
  $statements->insertElement($position, $whitespace);

  // Insert file document comment.
  $comment = array(
    'type' => T_DOC_COMMENT,
    'value' => "/**\n * @file\n * Install, update and uninstall functions for the $_coder_upgrade_module_name module.\n *\n */",
  );
  $statements->insertElement($position, $comment);
}
