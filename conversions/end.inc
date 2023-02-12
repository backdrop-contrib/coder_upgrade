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

  $_coder_upgrade_dirname = $dirname;

  $modules = coder_upgrade_module_files($dirname);
  $_coder_upgrade_class_files_sorted = _coder_upgrade_sort_files_by_modules($_coder_upgrade_class_files, $modules);

  foreach ($modules as $module_name => $module_path) {
    // Create a hook_autoload_info() entry in the module file.
    $classlist = array();
    foreach ($_coder_upgrade_class_files_sorted as $class_file_path => $class_module_path) {
      if ($module_path == $class_module_path) {
        $relative_path = explode($module_path, $class_file_path);
        $classlist[$relative_path[1]] = coder_upgrade_get_file_classes($class_file_path);
      }
    }
    if (!empty($classlist)) {
      coder_upgrade_create_autoload($module_name, $module_path, $classlist);
      coder_upgrade_create_test_info($module_name, $module_path, $classlist);
    }

    $config = config('coder_upgrade_tmp.' . $module_name . '.settings');
    if (!empty($config->get())) {
      // Create an install file.
      $install_file = $module_path . $module_name . '.install';
      if (!file_exists($install_file)) {
        $result = file_put_contents($install_file, "<?php\n/**\n * @file\n * Install, update and uninstall functions for the $module_name module.\n *\n */\n\nfunction xxx() {}");
      }

      // Perform changes on install files.
      // Creates a hook_update_1000() implementation.
      // Creates an hook_update_last_removed() implementation.
      // Adds an entry to hook_install if there are dynamically created variables.
      coder_upgrade_install_file($module_name, $install_file, $config);

      // Creates a config directory and file if there were variable to config
      // conversions.
      coder_upgrade_build_config_files($module_path, $module_name, $config); // todo
    }
  }
}

function coder_upgrade_create_test_info($module_name, $module_path, $classlist) {
  $classes = array();
  foreach ($classlist as $path => $class_nodes) {
    $is_test_file = pathinfo($path, PATHINFO_EXTENSION) == 'test';
    if ($is_test_file) {
      foreach ($class_nodes['classes'] as $class) {
        $classes[$class->data->name]['path'] = $path;
      }
      foreach ($class_nodes['functions'] as $function) {
        if ($function->data->name == 'getInfo') {
          $body = $function->data->body;
          $editor = PGPEditor::getInstance();
          $nodes = $body->searchAll('PGPFunctionCall', 'name', 'value', 'return', TRUE);
          $return = reset($nodes);
          $string = $return->data->getParameter()->toString();
          $array = eval("return $string;");
          if (isset($function->container->parent->data->name)) {
            $classes[$function->container->parent->data->name]['info'] = $array;
          }
        }
      }
    }
  }
  $test_file = '';
  foreach ($classes as $classname => $info) {
    $test_file .= "[$classname]\n";
    $test_file .= 'name = ' . (isset($info['info']['name']) ? $info['info']['name'] : $classname) . "\n";;
    $test_file .= 'description = ' . (isset($info['info']['description']) ? $info['info']['description'] : $classname) . "\n";;
    $test_file .= 'group = ' . (isset($info['info']['group']) ? $info['info']['group'] : $module_name) . "\n";;
    $test_file .= 'file = ' . $info['path'] . "\n\n";;
  }
  $test_file_name = $module_path . $module_name . '.tests.info';
  if (file_put_contents($test_file_name, $test_file) === FALSE) {
    coder_upgrade_log_print('Test info file could not be written');
  }
}

function coder_upgrade_create_autoload($module_name, $module_path, $classes) {
  $config = config('coder_upgrade.settings');

  $module_file = $module_path . $module_name . '.module';
  if (file_exists($module_file)) {
    $cur = file_get_contents($module_file);
    $new = $cur;

    // Create reader object.
    $reader = PGPReader::getInstance();
    coder_upgrade_memory_print('create reader for file ' . $module_file);
    // Set array formatting preference.
    $reader->setPreserveArrayFormat($config->get('coder_upgrade_preserve_array_format'));
    // Set debug output preference.
    $reader->setDebug($config->get('coder_upgrade_enable_parser_debug_output'));
    $reader->setSnippet($new);
    coder_upgrade_memory_print('set snippet');
    // $reader->addTokenNames();
    coder_upgrade_memory_print('add token names');
    // Parse code file.
    $reader->buildGrammar();
    coder_upgrade_memory_print('build grammar');

    $body = new PGPBody();

    $strings[] = 'return array(';
    foreach ($classes as $path => $class_nodes) {
      $is_test_file = pathinfo($path, PATHINFO_EXTENSION) == 'test';
      if (!$is_test_file) {
        foreach ($class_nodes['classes'] as $class_node) {
          $strings[] = "'" . $class_node->data->name . "' => '" . $path . "',";
        }
      }
    }
    $strings[] = ');';

    // Use the editor to set the function parameters.
    $editor = PGPEditor::getInstance();
    //  $editor->setParameters($function, $parameters);
    $body_content = $editor->textToStatements(implode("\n", $strings))->getElement();

    $body->insertLast($body_content);

    // Set values for the new hook function.
    $comment = array(
      'type' => T_DOC_COMMENT,
      'value' => "/**\n * Implements hook_autoload_info().\n */",
    );
    $function_name = $module_name . '_autoload_info';

    $function = new PGPClass($function_name);
    $function->comment = $comment;
    $function->type = T_FUNCTION;
    $function->parameters = new PGPList();


    $function->body = $body;

    $nodes = &$reader->getFunctions();
    $node = end($nodes);
    $container = &$node->container;

    // Insert the new function before the old function.
    $container->insertAfter($node, $function, 'function');
    // Insert a blank line.
    $container->insertAfter($node, CODER_UPGRADE_WHITESPACE, 'whitespace');

    // Use writer to redo file.
    $writer = PGPWriter::getInstance();
    coder_upgrade_memory_print('create writer');
    $new = $writer->toString($reader->getStatements());
    coder_upgrade_memory_print('writer->toString');

    // Write the new file.
    if ($new != $cur) {
      if (file_put_contents($module_file, $new) === FALSE) {
        coder_upgrade_log_print('File could not be written');
      }
      coder_upgrade_log_print('Replaced the file');
    }

    // Free up memory.
    $reader->reset();
    coder_upgrade_memory_print('reset reader');
  }
}

function coder_upgrade_build_config_files($module_path, $module_name, $config) {
  $tmp_name = $config->getName();
  $name = str_replace('coder_upgrade_tmp.', '', $tmp_name);
  $data = array();
  $data['_config_name'] = $name;
  $data += $config->get();
  $contents = backdrop_json_encode($data, TRUE);
  // Create the config directory.
  $dir = $module_path . 'config';
  if (!file_prepare_directory($dir, FILE_CREATE_DIRECTORY)) {
    backdrop_set_message(st('The config directory at %directory could not be created.', array('%directory' => $dir)), 'error');
  }
  else {
    $result = file_put_contents($dir . '/' . $name . '.json', $contents);
  }
  $config = config($tmp_name);
  $config->delete();
}

/**
 * Applies grammar parser conversion routines to install file.
 *
 * @param string $install_file
 *   The name of the install file to convert.
 */
function coder_upgrade_install_file($module_name, $install_file, $config) {
  $cur = file_get_contents($install_file);
  $new = $cur;

  $settings = config('coder_upgrade.settings');

  // Create reader object.
  $reader = PGPReader::getInstance();
  coder_upgrade_memory_print('create reader for file ' . $install_file);
  // Set array formatting preference.
  $reader->setPreserveArrayFormat($settings->get('coder_upgrade_preserve_array_format'));
  // Set debug output preference.
  $reader->setDebug($settings->get('coder_upgrade_enable_parser_debug_output'));
  $reader->setSnippet($new);
  coder_upgrade_memory_print('set snippet');
  // $reader->addTokenNames();
  coder_upgrade_memory_print('add token names');
  // Parse code file.
  $reader->buildGrammar();
  coder_upgrade_memory_print('build grammar');

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

  $config_data = $config->get();

  // If dynamic variables, create or update hook_install() implementation.
  coder_upgrade_create_update_install($module_name, $reader, $nodes, $config_data);

  // Create a hook_update_1000() implementation.
  coder_upgrade_create_update_1000($module_name, $reader, $nodes, $config_data);

  // Create an hook_update_last_removed() implementation.
  coder_upgrade_create_update_last_removed($module_name, $reader, $nodes);

  if (!empty($dummy)) {
    $dummy->container->delete($dummy);
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


function coder_upgrade_create_update_install($module_name, &$reader, &$nodes, $config_data) {
  // Use the editor to set the function parameters.
  $editor = PGPEditor::getInstance();
  $strings = array();
  $delstrings = array();

  $line_matches = array();
  foreach ($config_data as $key => $line) {
    if ($line == array()) {
      // Catch initialization with empty array.
      continue;
    }
    if (preg_match('/(dynamic variable in file )(.*)/', $line, $matches) || preg_match('/(dynamic value in file )(.*)/', $line, $matches)) {
      $line_matches[] = $matches[2];
    }
  }

  $install_function = NULL;
  foreach ($nodes as &$node) {
    $item = &$node->data;
    $name = &$item->name;
    if ($name == $module_name . '_install') {
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
    $function_name = $module_name . '_install';

    $function = new PGPClass($function_name);
    $function->comment = $comment;
    $function->type = T_FUNCTION;
    $function->parameters = new PGPList();


    $function->body = $body;

    $node = end($nodes);
    $container = &$node->container;

    // Insert the new function before the old function.
    $container->insertAfter($node, $function, 'function');
    // Insert a blank line.
    $container->insertAfter($node, CODER_UPGRADE_WHITESPACE, 'whitespace');
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

function coder_upgrade_create_update_1000($module_name, &$reader, &$nodes, $config_data) {
  // Use the editor to set the function parameters.
  $editor = PGPEditor::getInstance();
  $strings = array();
  $delstrings = array();

  $body = new PGPBody();

  $string = "\$config = config('$module_name.settings');";
  $body_content = $editor->textToStatements($string)->getElement(0);
  $body->insertLast($body_content);

  foreach ($config_data as $key => $line) {
    if ($line == array()) {
      // Check initialization with empty array.
      $string = "  \$config->set('$key', update_variable_get('$key', array()));";
    }
    elseif (is_bool($line)) {
      // Check initialization with a Boolean variable
      $line = $line ? 'TRUE' : 'FALSE';
      $string = "  \$config->set('$key', update_variable_get('$key', $line));";
    }
    else {
      $string = "  \$config->set('$key', update_variable_get('$key', '$line'));";
    }
    $body_content = $editor->textToStatements($string)->getElement(0);
    $body->insertLast($body_content);
  }

  $string = "\$config->save();";
  $body_content = $editor->textToStatements($string)->getElement(0);
  $body->insertLast($body_content);

  $body->insertLast(CODER_UPGRADE_WHITESPACE, 'whitespace');

  foreach ($config_data as $key => $line) {
    $string = '  update_variable_del(\'' . $key . '\');';
    $body_content = $editor->textToStatements($string)->getElement(0);
    $body->insertLast($body_content);
  }

  // Set values for the new hook function.
  $comment = array(
    'type' => T_DOC_COMMENT,
    'value' => "/**\n * Migrate {$module_name} variables to config.\n */",
  );
  $function_name = $module_name . '_update_1000';

  $function = new PGPClass($function_name);
  $function->comment = $comment;
  $function->type = T_FUNCTION;
  $function->parameters = new PGPList();


  $function->body = $body;

  $node = end($nodes);
  $container = &$node->container;

  // Insert the new function before the old function.
  $container->insertAfter($node, $function, 'function');
  // Insert a blank line.
  $container->insertAfter($node, CODER_UPGRADE_WHITESPACE, 'whitespace');
}

function coder_upgrade_create_update_last_removed($module_name, &$reader, &$nodes) {
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
    if (!empty($ns)) {
      $last_removed = max(array_keys($ns));

      // Set values for the new hook function.
      $comment = array(
        'type' => T_DOC_COMMENT,
        'value' => "/**\n * Implements hook_update_last_removed().\n */",
      );
      $function_name = $module_name . '_update_last_removed';

      // Create the new hook function.
      $function = new PGPClass($function_name);
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
      $container->insertAfter($node, CODER_UPGRADE_WHITESPACE, 'whitespace');
    }
  }
}

/**
 * Returns declared classes in a file.
 *
 * @param string $file
 *   The name of the file to scan.
 */
function coder_upgrade_get_file_classes($file) {
  $contents = file_get_contents($file);

  $config = config('coder_upgrade.settings');

  // Create reader object.
  $reader = PGPReader::getInstance();
  coder_upgrade_memory_print('create reader for file ' . $file);
  // Set array formatting preference.
  $reader->setPreserveArrayFormat($config->get('coder_upgrade_preserve_array_format'));
  // Set debug output preference.
  $reader->setDebug($config->get('coder_upgrade_enable_parser_debug_output'));
  $reader->setSnippet($contents);
  coder_upgrade_memory_print('set snippet');
  // $reader->addTokenNames();
  coder_upgrade_memory_print('add token names');
  // Parse code file.
  $reader->buildGrammar();
  coder_upgrade_memory_print('build grammar');

  $classes = &$reader->getClasses();
  $functions = &$reader->getFunctions();

  // $classes doesn't seem to hold any information about contained functions so
  // return both.
  if (!empty($classes)) {
    return array(
      'classes' => $classes,
      'functions' => $functions,
    );
  }
  return NULL;
}


/**
 * Sorts an array of files by path.
 *
 * @param string $files
 *   The array of the file to sort.
 * @param string $modules
 *   An array of paths to each module or submodule in a directory.
 */
function _coder_upgrade_sort_files_by_modules($files, $modules) {
  // We have here an array of files with classes in them, and an array of paths
  // to directories with a ".module' file. We assume the latter are likely to be
  // sub-modules of the currently converting project item. We can't do a simple
  // strpos() to find which module directory a class file belongs to because
  // for a file "dira/dirb/file.file", strpos() would match both dira/ and
  // dira/dirb/.
  // So we sort the module directories by depth, check each file to see if it
  // belongs there, then remove the file from the search so it won't match a
  // shallower directory.
  uasort ($modules , function ($a, $b) {
    $deptha = substr_count($a, '/');
    $depthb = substr_count($b, '/');
    return ($deptha < $depthb) ? 1 : 0;
    }
  );
  $sorted = array();
  foreach ($modules as $module) {
    foreach ($files as $index => $file) {
      if (strpos($file, $module) !== FALSE) {
        $sorted[$file] = $module;
        unset($files[$index]);
      }
    }
  }
  return $sorted;
}
