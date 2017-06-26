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

  // Create a hook_autoload_info() entry in the module file.
  $classlist = array();
  foreach ($_coder_upgrade_class_files as $class_file_path) {
    $relative_path = explode($dirname . '/', $class_file_path);
    $classlist[$relative_path[1]] = coder_upgrade_get_file_classes($class_file_path);
  }
  if (!empty($classlist)) {
    coder_upgrade_create_autoload($dirname, $classlist);
    coder_upgrade_create_test_info($dirname, $classlist);
  }

  // Perform changes on install files.
  // Creates a hook_update_1000() implementation.
  // Creates an hook_update_last_removed() implementation.
  // Adds an entry to hook_install if there are dynamically created variables.
  global $_coder_upgrade_module_name;
  $config_names = config_get_names_with_prefix('coder_upgrade_tmp.' . $_coder_upgrade_module_name);
  if (!empty($config_names)) {
    $install_file = $dirname . '/' . $_coder_upgrade_module_name . '.install';
    if (!file_exists($install_file)) {
      $result = file_put_contents($install_file, "<?php\n/**\n * @file\n * Install, update and uninstall functions for the $_coder_upgrade_module_name module.\n *\n */\n\nfunction xxx() {}");
    }
    coder_upgrade_install_file($install_file);

    // Creates a config directory and file if there were variable to config 
    // conversions.
    coder_upgrade_build_config_files($dirname, $_coder_upgrade_module_name); // todo 
  }
}

function coder_upgrade_create_test_info($dirname, $classlist) {
  global $_coder_upgrade_module_name;
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
    $test_file .= 'group = ' . (isset($info['info']['group']) ? $info['info']['group'] : $_coder_upgrade_module_name) . "\n";;
    $test_file .= 'file = ' . $info['path'] . "\n\n";;
  }
  $test_file_name = $dirname . '/' . $_coder_upgrade_module_name . '.tests.info';
  if (file_put_contents($test_file_name, $test_file) === FALSE) {
    coder_upgrade_log_print('Test info file could not be written');
  }
}

function coder_upgrade_create_autoload($dirname, $classes) {
  $config = config('coder_upgrade.settings');
  global $_coder_upgrade_module_name;

  $module_file = $dirname . '/' . $_coder_upgrade_module_name . '.module';
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
    $reader->addTokenNames();
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
    $modulename = $_coder_upgrade_module_name . '_autoload_info';

    $function = new PGPClass($modulename);
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
    $whitespace = array(
      'type' => T_WHITESPACE,
      'value' => 1,
    );
    $container->insertAfter($node, $whitespace, 'whitespace');

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

function coder_upgrade_build_config_files($dirname, $_coder_upgrade_module_name) {
  $config_names = config_get_names_with_prefix('coder_upgrade_tmp.' . $_coder_upgrade_module_name);
  if (!empty($config_names)) {
    $config = config($config_names[0]);

    $name = str_replace('coder_upgrade_tmp.', '', $config->getName());
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
  $config_names = config_get_names_with_prefix('coder_upgrade_tmp.' . $_coder_upgrade_module_name);
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

    if (!empty($dummy)) {
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
  $reader->addTokenNames();
  coder_upgrade_memory_print('add token names');
  // Parse code file.
  $reader->buildGrammar();
  coder_upgrade_memory_print('build grammar');

  $classes = &$reader->getClasses();
  $functions = &$reader->getFunctions();

  // $classes doesnt seem to hold any information about contained functions so
  // return both.
  if (!empty($classes)) {
    return array(
      'classes' => $classes,
      'functions' => $functions,
    );
  }
  return NULL;
}
