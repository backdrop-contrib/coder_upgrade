<?php
/**
 * @file
 * Provides conversion routines applied to function calls.
 *
 * These routines use the grammar parser.
 *
 * Copyright 2009-11 by Jim Berry ("solotandem", http://drupal.org/user/240748)
 */

/**
 * The upgrades to these functions are documented at the following urls.
 */

/**
 * Implements hook_upgrade_call_alter().
 */
function coder_upgrade_upgrade_call_alter(&$node, &$reader, $name) { // NEEDS WORK
  // Create helper objects.
  $editor = PGPEditor::getInstance();

  // Get the function call object.
  $item = &$node->data;

  // Process function call.
//  $name = &$item->name;

}

/**
 * Implements hook_upgrade_call_variable_get_alter().
 */
function coder_upgrade_upgrade_call_variable_get_alter(&$node, &$reader) { // DONE
  // Create helper objects.
  $editor = PGPEditor::getInstance();

  // Get the function call object.
  $item = &$node->data;

  // Process function call.
  $name = &$item->name;
  global $_coder_upgrade_module_name, $_coder_upgrade_filename;
  $variable = str_replace("'", '', $item->printParameter(0));

  // Build a list of variables to later parse and build a config file.
  // We also store a warning message if variable or value are not strings, since
  // these will need to be manually corrected by the developer.

  // Only store the variable in the module's config if it starts with the module
  // name. This is necessary since we cannot know if otherwise named variables
  // are core variables or other contrib variables.
  if (strpos($variable, $_coder_upgrade_module_name) !== FALSE) { // todo change this to a check for core variables.
    $config = config('coder_upgrade_tmp.' . $_coder_upgrade_module_name . '.settings');
    $value = '';
    // Find the path to the current file.
    $old_dir = config_get('coder_upgrade.settings', 'coder_upgrade_dir_old');
    $split = explode($old_dir, $_coder_upgrade_filename);
    $path = $split[1];

    if (strpos($variable, '$')) {
      // Account for dynamically generated variables that include a $variable
      // rather than a string, such as ('variable_' . $node->type).
      $variable = preg_replace('@[^a-zA-Z_]@', '', $variable);
      $value = 'dynamic variable in file ' . $path . ' line ' . $node->line;
    }
    else {
      $raw_value = trim($item->printParameter(1));  // Remove extra whitespace
      $value_uc = strtoupper($raw_value);

      // Catch initialization with empty array.
      if ($raw_value == 'array()') {
        $value = array();
      }

      // Catch Booleans, which should be stored as Booleans, not strings.
      elseif ($value_uc == 'TRUE') {
        $value = TRUE;
      }
      elseif ($value_uc == 'FALSE') {
        $value = FALSE;
      }

      // Any special characters in the variable, it's beyond our handling, so
      // just insert the warning.
      elseif (preg_match('@[\.\s\$\[\]\#\>\(]@', $value)) {
        $value = 'dynamic value in file ' . $path . ' line ' . $node->line;
      }

      // If what's left starts and ends with either single or double quotes then
      // it's an ordinary string.
      else {
        $len = strlen($raw_value);
        $first = substr($raw_value, 0, 1);
        $last = substr($raw_value, -1);
        $sq_delimited = $first == "'" && $last == "'";
        $dq_delimited = $first == '"' && $last == '"';
        if ($len >= 2 && ($sq_delimited || $dq_delimited)) {
          $value = substr($raw_value, 1, $len - 2);
        }
        else {
          // It's a numeric expression, so use it directly.
          $value = $raw_value;

          // something is amiss! Insert the warning.
          //$value = 'dynamic value in file ' . $path . ' line ' . $node->line;
        }
      }
    }

    $config->set($variable, $value);
    $config->save();

    // Now replace the variable_get with config.
    // Process function call.
    $name['value'] = 'config_get';

    $p1 = $item->printParameter(0);
    $temp = $_coder_upgrade_module_name . '.settings';
    $editor->setParameter($item, 0, "'" . $temp . "'");
    $editor->setParameter($item, 1, $p1);
  }
  else {
    module_load_include('inc', 'coder_upgrade', 'includes/variables');
    $variable_data = coder_upgrade_drupal_variables($variable);
    if ($variable_data) {
      if ($variable_data['storage'] == 'to_config') {
        $storage = coder_upgrade_backdrop_configs($variable_data['variable']);
        // Now replace the variable_get with config.
        // Process function call.
        $name['value'] = 'config_get';

        $editor->setParameter($item, 0, "'" . $storage . "'");
        $editor->setParameter($item, 1, "'" . $variable_data['variable'] . "'");
      }
      elseif ($variable_data['storage'] == 'to_settings') {
        $name['value'] = 'settings_get';
        $editor->setParameter($item, 0, "'" . $variable_data['variable'] . "'");
      }
      elseif ($variable_data['storage'] == 'to_states') {
        $name['value'] = 'state_get';
        $editor->setParameter($item, 0, "'" . $variable_data['variable'] . "'");
      }
      elseif ($variable_data['storage'] == 'removed') {
        // This variable was removed in Backdrop without replacement.
        $statement = $editor->commentToStatement("// TODO This variable was removed in Backdrop without replacement.");
        $item->insertStatementBefore($statement);
      }
      elseif ($variable_data['storage'] == 'unknown') {
        // This variable is a known Drupal variable but we're not sure about it.
        $statement = $editor->commentToStatement("// TODO This variable was probably removed in Backdrop without replacement.");
        $item->insertStatementBefore($statement);
      }
    }
    else {
      // This variable is not a known Drupal core variable.
      $statement = $editor->commentToStatement("// TODO This variable was probably removed in Backdrop without replacement.");
      $item->insertStatementBefore($statement);
    }
  }
}

/**
 * Implements hook_upgrade_call_variable_set_alter().
 */
function coder_upgrade_upgrade_call_variable_set_alter(&$node, &$reader) { // DONE
  // Create helper objects.
  $editor = PGPEditor::getInstance();

  // Get the function call object.
  $item = &$node->data;
  $p0 = $item->printParameter(0);
  $p1 = $item->printParameter(1);

  // Process function call.
  $name = &$item->name;

  global $_coder_upgrade_module_name, $_coder_upgrade_filename;
  $variable = str_replace("'", '', $item->printParameter(0));

  // Only store the variable in the module's config if it starts with the module
  // name. This is necessary since we cannot know if otherwise named variables
  // are core variables or other contrib variables.
  if (strpos($variable, $_coder_upgrade_module_name) !== FALSE) { // todo change this to a check for core variables.
    $name['value'] = 'config_set';

    $temp = $_coder_upgrade_module_name . '.settings';
    $editor->setParameter($item, 0, "'" . $temp . "'");
    $editor->setParameter($item, 1, $p0);
    $editor->setParameter($item, 2, $p1);
  }
  else {
    module_load_include('inc', 'coder_upgrade', 'includes/variables');
    $variable_data = coder_upgrade_drupal_variables($variable);
    if ($variable_data) {
      if ($variable_data['storage'] == 'to_config') {
        $storage = coder_upgrade_backdrop_configs($variable_data['variable']);
        // Now replace the variable_del with config.
        // Process function call.
        $name['value'] = 'config_set';

        $editor->setParameter($item, 0, "'" . $storage . "'");
        $editor->setParameter($item, 1, "'" . $variable_data['variable'] . "'");
        $editor->setParameter($item, 2, $p1);
      }
      elseif ($variable_data['storage'] == 'to_states') {
        $name['value'] = 'state_set';
        $editor->setParameter($item, 0, "'" . $variable_data['variable'] . "'");
      }
      elseif ($variable_data['storage'] == 'to_settings') {
        // This variable is a settings variable and cannot be set.
        $statement = $editor->commentToStatement("// TODO In Backdrop this variable is a settings variable and cannot be set, only retrieved.");
        $item->insertStatementBefore($statement);
      }
      elseif ($variable_data['storage'] == 'removed') {
        // This variable was removed in Backdrop without replacement.
        $statement = $editor->commentToStatement("// TODO This variable was removed in Backdrop without replacement.");
        $item->insertStatementBefore($statement);
      }
      elseif ($variable_data['storage'] == 'unknown') {
        // This variable is a known Drupal variable but we're not sure about it.
        $statement = $editor->commentToStatement("// TODO This variable was probably removed in Backdrop without replacement.");
        $item->insertStatementBefore($statement);
      }
    }
    else {
      // This variable is not a known Drupal core variable.
      $statement = $editor->commentToStatement("// TODO This variable was probably removed in Backdrop without replacement.");
      $item->insertStatementBefore($statement);
    }
  }
}

/**
 * Implements hook_upgrade_call_variable_del_alter().
 */
function coder_upgrade_upgrade_call_variable_del_alter(&$node, &$reader) { // DONE
  // Create helper objects.
  $editor = PGPEditor::getInstance();

  // Get the function call object.
  $item = &$node->data;

  // Process function call.
  $name = &$item->name;
  global $_coder_upgrade_module_name, $_coder_upgrade_filename;
  $variable = str_replace("'", '', $item->printParameter(0));

  // Only store the variable in the module's config if it starts with the module
  // name. This is necessary since we cannot know if otherwise named variables
  // are core variables or other contrib variables.
  if (strpos($variable, $_coder_upgrade_module_name) !== FALSE) { // todo change this to a check for core variables.
    $name['value'] = 'config_clear';

    $p0 = $item->printParameter(0);
    $temp = $_coder_upgrade_module_name . '.settings';
    $editor->setParameter($item, 0, "'" . $temp . "'");
    $editor->setParameter($item, 1, $p0);
  }
  else {
    module_load_include('inc', 'coder_upgrade', 'includes/variables');
    $variable_data = coder_upgrade_drupal_variables($variable);
    if ($variable_data) {
      if ($variable_data['storage'] == 'to_config') {
        $storage = coder_upgrade_backdrop_configs($variable_data['variable']);
        // Now replace the variable_del with config.
        // Process function call.
        $name['value'] = 'config_clear';

        $editor->setParameter($item, 0, "'" . $storage . "'");
        $editor->setParameter($item, 1, "'" . $variable_data['variable'] . "'");
      }
      elseif ($variable_data['storage'] == 'to_states') {
        $name['value'] = 'state_del';
        $editor->setParameter($item, 0, "'" . $variable_data['variable'] . "'");
      }
      elseif ($variable_data['storage'] == 'to_settings') {
        // This variable is a settings variable and cannot be deleted.
        $statement = $editor->commentToStatement("// TODO In Backdrop this variable is a settings variable and cannot be deleted, only retrieved.");
        $item->insertStatementBefore($statement);
      }
      elseif ($variable_data['storage'] == 'removed') {
        // This variable was removed in Backdrop without replacement.
        $statement = $editor->commentToStatement("// TODO This variable was removed in Backdrop without replacement.");
        $item->insertStatementBefore($statement);
      }
      elseif ($variable_data['storage'] == 'unknown') {
        // This variable is a known Drupal variable but we're not sure about it.
        $statement = $editor->commentToStatement("// TODO This variable was probably removed in Backdrop without replacement.");
        $item->insertStatementBefore($statement);
      }
    }
    else {
      // This variable is not a known Drupal core variable.
      $statement = $editor->commentToStatement("// TODO This variable was probably removed in Backdrop without replacement.");
      $item->insertStatementBefore($statement);
    }
  }
}

/**
 * Implements hook_upgrade_call_system_settings_form_alter().
 */
function coder_upgrade_upgrade_call_system_settings_form_alter(&$node, &$reader) { // DONE
  // Create helper objects.
  $editor = PGPEditor::getInstance();
  global $_coder_upgrade_module_name;

  $item = &$node->data;
  // Get the string used to represent the form (usually "$form") by checking the
  // parameter in system_settings_form return call.
  $form = $item->printParameter();

  // Now search for the first occurence of this string.
  $parent = &$item->parent;
  $statements = $parent->container->searchAll('PGPAssignment', 'values', 0, $form, $parent);
  $first_node = array_shift($statements);
  $data = $first_node->data;
  // Check the value assigned to this first occurence of the string. It would
  // normally be either $form = array() or $form = array('#key...)
  $value = $data->getValue();
  $container = &$first_node->container;
  $string = $editor->textToStatements("\$form['#config'] = '$_coder_upgrade_module_name.settings';")->getElement(0);
  // If the form was initialized with an empty array, put our new '#config' key
  // after that, otherwise before.
  if ($value->toString() == 'array()') {
    $container->insertAfter($first_node, $string, 'global');
  }
  else {
    $container->insertBefore($first_node, $string, 'global');
  }
}

