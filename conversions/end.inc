<?php
/**
 * @file
 * Provides conversion routines applied to the directory after routines are applied to the files.
 *
 * These routines use the grammar parser.
 *
 * Copyright 2009-11 by Jim Berry ("solotandem", http://drupal.org/user/240748)
 */

/**
 * Implements hook_upgrade_end_alter().
 *
 * Processes the directory after upgrade routines are applied to the files.
 *
 * @param string $dirname
 *   The name of the directory with the converted files.
 */
function coder_upgrade_upgrade_end_alter($dirname) {
  global $_coder_upgrade_dirname, $_coder_upgrade_class_files, $_coder_upgrade_module_name;

  if (empty($_coder_upgrade_class_files)) {
    return;
  }

  $_coder_upgrade_dirname = $dirname;
  // Get list of files.
  $files = coder_upgrade_scan_directory_2($dirname, 'files');
  // In 6.x, add one hook_theme registry entry in each module.
  // In 7.x, add files entries to each .info file.
  coder_upgrade_add_file_entries($dirname, $files); // todo 

  $install_file = $dirname . '/' . $_coder_upgrade_module_name . '.install';
  if (file_exists($install_file)) {
    coder_upgrade_install_file($install_file);
  }
  coder_upgrade_build_config_files($dirname, $_coder_upgrade_module_name); // todo 
}

function coder_upgrade_build_config_files($dirname, $_coder_upgrade_module_name) {
  $config_names = config_get_names_with_prefix('coder_upgrade.' . $_coder_upgrade_module_name);
  if (!empty($config_names)) {
    $config = config($config_names[0]);

    $name = str_replace('coder_upgrade.', '', $config->getName());
    $data = array();
    $data['_config_name'] = $name;
    $data += $config->get();
    $contents = backdrop_json_encode($data, TRUE);
    // Create the config directory.
    $dir = $dirname . '/config';
    if (!file_prepare_directory($dir, FILE_CREATE_DIRECTORY)) {
      backdrop_set_message(st('The config directory at %directory could not be created.', array('%directory' => $dir)), 'error');
    }
    else {
      $result = file_put_contents($dir . '/' . $name . '.json', $contents);
    }
    $config->delete();
  }
}

/**
 * Applies grammar parser conversion routines to install file.
 *
 * @param string $install_file
 *   The name of the install file to convert.
 */
function coder_upgrade_install_file($install_file) {
  $cur = file_get_contents($install_file);
  $new = $cur;

  $config = config('coder_upgrade.settings');

  // Create reader object.
  $reader = PGPReader::getInstance();
  coder_upgrade_memory_print('create reader for file ' . $install_file);
  // Set array formatting preference.
  $reader->setPreserveArrayFormat($config->get('coder_upgrade_preserve_array_format'));
  // Set debug output preference.
  $reader->setDebug($config->get('coder_upgrade_enable_parser_debug_output'));
  $reader->setSnippet($new);
  coder_upgrade_memory_print('set snippet');
  $reader->addTokenNames();
  coder_upgrade_memory_print('add token names');
  // Parse code file.
  $reader->buildGrammar();
  coder_upgrade_memory_print('build grammar');

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

    // If dynamic variables, create or update hook_install() implementation.
    coder_upgrade_create_update_install($reader, $nodes, $config_data);

    // Create a hook_update_1000() implementation.
    coder_upgrade_create_update_1000($reader, $nodes, $config_data);

    // Create an hook_update_last_removed() implementation.
    coder_upgrade_create_update_last_removed($reader, $nodes);

    if ($dummy) {
      $dummy->container->delete($dummy);
    }
  }

  // Use writer to redo file.
  $writer = PGPWriter::getInstance();
  coder_upgrade_memory_print('create writer');
  $new = $writer->toString($reader->getStatements());
  coder_upgrade_memory_print('writer->toString');

  // Write the new file.
  if ($new != $cur) {
    if (file_put_contents($install_file, $new) === FALSE) {
      coder_upgrade_log_print('File could not be written');
    }
    coder_upgrade_log_print('Replaced the file');
  }

  // Free up memory.
  $reader->reset();
  coder_upgrade_memory_print('reset reader');
}

 
function coder_upgrade_create_update_install(&$reader, &$nodes, $config_data) {
  global $_coder_upgrade_module_name;
  // Use the editor to set the function parameters.
  $editor = PGPEditor::getInstance();
  $strings = array();
  $delstrings = array();

  $line_matches = array();
  foreach ($config_data as $key => $line) {
    if (preg_match('/(dynamic variable in file )(.*)/', $line, $matches) || preg_match('/(dynamic value in file )(.*)/', $line, $matches)) {
      $line_matches[] = $matches[2];
    }
  }

  $install_function = NULL;
  foreach ($nodes as &$node) {
    $item = &$node->data;
    $name = &$item->name;
    if ($name == $_coder_upgrade_module_name . '_install') {
      $install_function = &$node;
      break;
    }
  }

  if (!$install_function) {
    $body = new PGPBody();

    $string = "// Dynamically generated variable data was detected.";
    $body_content = $editor->commentToStatement($string);
    $body->insertLast($body_content);

    foreach ($line_matches as $match) {
      $body_content = $editor->commentToStatement($match);
      $body->insertLast($body_content);
    }

    // Set values for the new hook function.
    $comment = array(
      'type' => T_DOC_COMMENT,
      'value' => "/**\n * Implements hook_install().\n */",
    );
    $modulename = $_coder_upgrade_module_name . '_install';

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
  else {
    $item = &$install_function->data;
    $body = &$item->body;

    $string = "// Dynamically generated variable data was detected on the following lines.";
    $body_content = $editor->commentToStatement($string);
    $body->insertLast($body_content);

    foreach ($line_matches as $match) {
      $body_content = $editor->commentToStatement($match);
      $body->insertLast($body_content);
    }
  }
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
  $ns = array();
  $last_removed = '';
  if (!empty($nodes)) {
    foreach ($nodes as &$node) {
      $item = &$node->data;
      $name = &$item->name;
      if (preg_match('@[a-zA-Z_]+(update_)(\d+)$@', $name, $matches)) {
        $ns[$matches[2]] = $node;
      }
    }
    $last_removed = max(array_keys($ns));
  }

  // Set values for the new hook function.
  $comment = array(
    'type' => T_DOC_COMMENT,
    'value' => "/**\n * Implements hook_update_last_removed().\n */",
  );
  $modulename = $_coder_upgrade_module_name . '_update_last_removed';

  // Create the new hook function.
  $function = new PGPClass($modulename);
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
 * Scans a directory and finds all files and/or directories beneath it.
 *
 * @todo Move to .inc file in parent directory.
 *
 * @param string $dirname
 *   Directory path to scan.
 *
 * @return array
 *   Array of file and/or directory names.
 */
function coder_upgrade_scan_directory_2($dirname, $retrieve = 'dirs') {
  $dirs = array();
  if (!is_dir($dirname)) {
    return $dirs;
  }

  static $ignore = array(/*'.', '..', '.bzr', '.git', '.svn',*/ 'CVS');
  $files = scandir($dirname);
  $dirname .= '/';

  foreach ($files as $key => $file) {
    $file_path = $dirname . $file;
    if (substr(basename($file_path), 0, 1) == '.' || in_array(basename($file_path), $ignore)) {
      // Ignore all hidden directories and CVS directory.
      unset($files[$key]);
    }
    elseif (is_dir($file_path)) {
      unset($files[$key]);
      $files[$key]['dir'] = $file;
      $files[$key]['entries'] = coder_upgrade_scan_directory_2($file_path, $retrieve);
      $dirs[] = $file;
    }
  }
  return $retrieve == 'dirs' ? $dirs : $files;
}

/**
 * Adds file entries to .info files.
 *
 * Module .info files must now specify all loadable code files explicitly.
 * http://drupal.org/node/224333#registry
 *
 * @param string $dirname
 *   Module directory path.
 * @param array $files
 *   Contents of directory $dirname.
 * @param array $items
 *   List of registry files.
 * @param string $path
 *   Subdirectory whose parent directory contains a module.
 */
function coder_upgrade_add_file_entries($dirname = '', $files = array(), &$items = array(), $path = '') {
  if (empty($files)) {
    return;
  }

  global $_coder_upgrade_dirname, $_coder_upgrade_class_files;

  // Extensions that indicate a module is present.
  $extensions = array('info', 'module');
  $info_file = '';

  if ($path != 'new' && ($path != '' || $dirname != $_coder_upgrade_dirname)) {
    // Scan subdirectory for .info file indicating presence of another module.
    foreach ($files as $file) {
      if (!is_array($file)) {
        $extension = pathinfo($file, PATHINFO_EXTENSION);
        if (in_array($extension, $extensions)) {
          // This subdirectory contains a module - so recurse.
          $dirname .= $path != '' ? '/' . $path : '';
          $path = 'new';
          $items2 = array();
          coder_upgrade_add_file_entries($dirname, $files, $items2, $path);
          return;
        }
      }
    }
  }
  // Clear path if we are starting another module.
  $path = $path == 'new' ? '' : $path;

  // Create list of files for registry.
  // TODO Allow for custom extensions!!!
  $extensions = array('inc', 'install', 'module', 'php', 'profile', 'test', 'theme', 'upgrade');
  $dirname .= $path != '' ? '/' . $path : '';
  $path = $path != '' ? $path . '/' : $path;
  foreach ($files as $file) {
    if (is_array($file)) {
      coder_upgrade_add_file_entries($dirname, $file['entries'], $items, $file['dir']);
    }
    else {
      $extension = pathinfo($file, PATHINFO_EXTENSION);
      if ($extension == 'php' && substr($file, -8) == '.tpl.php') {
        // Exclude template files.
      }
      elseif (in_array($extension, $extensions) && in_array($dirname . '/' . $file, $_coder_upgrade_class_files)) {
        $items[] = $dirname . '/' . $file;
      }
      elseif ($extension == 'info') {
        // Save the name of the .info file.
        $info_file = $dirname . '/' . $file;
      }
    }
  }

  if ($info_file != '') {
    $new = file_get_contents($info_file);
    // Remove any file entries from .info file.
    $from = "/^\s*files\[\]\s*=.*?$/m";
    $to = '';
    $new = preg_replace($from, $to, $new);
    // Add sorted entries to .info file.
    $new .= "\n";
    foreach ($items as $item) {
      $item = str_replace($dirname . '/', '', $item);
      $new .= "files[] = $item\n";
    }
    if (file_put_contents($info_file, $new) === FALSE) {
      clp(t('File could not be written: @file', array('@file' => $info_file)));
    }
  }
}
