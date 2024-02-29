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

// @todo: use BuilderFactory for building nodes.
 
use PhpParser\Node\Name;
use PhpParser\NodeFinder;
use PhpParser\BuilderFactory;
use PhpParser\Node\Arg;
use PhpParser\Node\Expr\Variable;
use PhpParser\Comment\Doc;
use PhpParser\Node\Stmt\Return_;
use PhpParser\Node\Stmt\Expression;
use PhpParser\Node\Expr\Assign;
use PhpParser\Node\Scalar\String_;
use PhpParser\PrettyPrinter;

/**
 * Implements hook_upgrade_call_alter().
 */
function coder_upgrade_upgrade_call_alter(&$node, $name) { // NEEDS WORK
  // Create helper objects.

  // Get the function call object.
  $item = &$node->data;

  // Process function call.
//  $name = &$item->name;

}

/**
 * Implements hook_upgrade_call_variable_get_alter().
 */
function coder_upgrade_upgrade_call_variable_get_alter(&$node, &$asts) { // DONE
  // Create helper objects.

  // Get the function call object.
  $item = &$node;

  // Process function call.
  $name = $item->name->toString();

  global $_coder_upgrade_module_name, $_coder_upgrade_filename;
    $line = $item->getAttribute('startLine');

    $prettyPrinter = new PrettyPrinter\Standard();
    $variable = $prettyPrinter->prettyPrintExpr($item->args[0]->value);
    $variable2 = !empty($item->args[1]->value) ? $prettyPrinter->prettyPrintExpr($item->args[1]->value) : NULL;

    $variable = str_replace('.', '-', $variable);
    $variable = str_replace(array("'", '"', ' '), '', $variable);

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
      $value = 'dynamic variable in file ' . $path . ' line ' . $line;
    }
    else {
      $raw_value = trim($variable2);  // Remove extra whitespace
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
      elseif (!preg_match('@^\'[^\']*\'$@', $raw_value) && preg_match('@[\.\s\$\[\]\#\>\(]@', $raw_value)) {
        $value = 'dynamic value in file ' . $path . ' line ' . $line;
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
        }
      }
    }

    $config->set($variable, $value);
    $config->save();

    // Now replace the variable_get with config.
    // Process function call.
    $item->name = new Name('config_get');

    $temp = $_coder_upgrade_module_name . '.settings';

    $builderFactory = new BuilderFactory();
    $args = $builderFactory->args(array($temp, $variable));
    $item->args = $args;
  }
  else {
    module_load_include('inc', 'coder_upgrade', 'includes/variables');
    $variable_data = coder_upgrade_drupal_variables($variable);
    if ($variable_data) {
      if ($variable_data['storage'] == 'to_config') {
        $storage = coder_upgrade_backdrop_configs($variable_data['variable']);
        // Now replace the variable_get with config.
        // Process function call.
        $item->name = new Name('config_get');

        $item->args[0]->value->value = $storage;
        $item->args[1]->value->value = $variable_data['variable'];
      }
      elseif ($variable_data['storage'] == 'to_settings') {
        $item->name = new Name('settings_get');
        $item->args[0]->value->value = $variable_data['variable'];
      }
      elseif ($variable_data['storage'] == 'to_states') {
        $item->name = new Name('state_get');
        $item->args[0]->value->value = $variable_data['variable'];
      }
      // @todo: setting comments on funCalls doesnt work?
      elseif ($variable_data['storage'] == 'removed') {
        // This variable was removed in Backdrop without replacement.
        $statement = "// TODO This variable was removed in Backdrop without replacement.";
        $doc = new Doc($statement);
        $item->getAttribute('parent')->getAttribute('parent')->setDocComment($doc);
      }
      elseif ($variable_data['storage'] == 'unknown') {
        // This variable is a known Drupal variable but we're not sure about it.
        $statement = "// TODO This variable was probably removed in Backdrop without replacement.";
        $doc = new Doc($statement);
        $item->getAttribute('parent')->getAttribute('parent')->setDocComment($doc);
      }
    }
    else {
      // This variable is not a known Drupal core variable.
      $statement = "// TODO This variable was probably removed in Backdrop without replacement.";
      $doc = new Doc($statement);
      $item->getAttribute('parent')->getAttribute('parent')->setDocComment($doc);
    }
  }
}

/**
 * Implements hook_upgrade_call_variable_set_alter().
 */
function coder_upgrade_upgrade_call_variable_set_alter(&$node, &$asts) { // DONE
  // Create helper objects.

  // Get the function call object.
  $item = &$node;

  // Process function call.
  $name = $item->name->toString();

  $p0 = $item->args[0]->value->value;
  $p1 = $item->args[1]->value->value;

  // Process function call.
  global $_coder_upgrade_module_name, $_coder_upgrade_filename;
  $variable = str_replace("'", '', $item->args[0]->value->value);

  // Only store the variable in the module's config if it starts with the module
  // name. This is necessary since we cannot know if otherwise named variables
  // are core variables or other contrib variables.
  if (strpos($variable, $_coder_upgrade_module_name) !== FALSE) { // todo change this to a check for core variables.
    $item->name = new Name('config_set');

    $temp = $_coder_upgrade_module_name . '.settings';
    $builderFactory = new BuilderFactory();
    $args = $builderFactory->args(array($temp, $p0, $p1));
    $item->args = $args;
  }
  else {
    module_load_include('inc', 'coder_upgrade', 'includes/variables');
    $variable_data = coder_upgrade_drupal_variables($variable);
    if ($variable_data) {
      if ($variable_data['storage'] == 'to_config') {
        $storage = coder_upgrade_backdrop_configs($variable_data['variable']);
        // Now replace the variable_del with config.
        // Process function call.
        $item->name = new Name('config_set');

        $builderFactory = new BuilderFactory();
        $args = $builderFactory->args(array($storage, $variable_data['variable'], $p1));
        $item->args = $args;
      }
      elseif ($variable_data['storage'] == 'to_states') {
        $item->name = new Name('state_set');
        $item->args[0]->value->value = $variable_data['variable'];
      }
      elseif ($variable_data['storage'] == 'to_settings') {
        // This variable is a settings variable and cannot be set.
        $statement = "// TODO In Backdrop this variable is a settings variable and cannot be set, only retrieved.";
        $doc = new Doc($statement);
        $item->getAttribute('parent')->setDocComment($doc);
      }
      elseif ($variable_data['storage'] == 'removed') {
        // This variable was removed in Backdrop without replacement.
        $statement = "// TODO This variable was removed in Backdrop without replacement.";
        $doc = new Doc($statement);
        $item->getAttribute('parent')->setDocComment($doc);
      }
      elseif ($variable_data['storage'] == 'unknown') {
        // This variable is a known Drupal variable but we're not sure about it.
        $statement = "// TODO This variable was probably removed in Backdrop without replacement.";
        $doc = new Doc($statement);
        $item->getAttribute('parent')->setDocComment($doc);
      }
    }
    else {
      // This variable is not a known Drupal core variable.
        $statement = "// TODO This variable was probably removed in Backdrop without replacement.";
        $doc = new Doc($statement);
        $item->getAttribute('parent')->setDocComment($doc);
    }
  }
}

/**
 * Implements hook_upgrade_call_variable_del_alter().
 */
function coder_upgrade_upgrade_call_variable_del_alter(&$node, &$asts) { // DONE
  // Create helper objects.

  // Get the function call object.
  $item = &$node;

  // Process function call.
  $name = $item->name->toString();

  // Process function call.
  global $_coder_upgrade_module_name, $_coder_upgrade_filename;
  $variable = str_replace("'", '', $item->args[0]->value->value);


  // Only store the variable in the module's config if it starts with the module
  // name. This is necessary since we cannot know if otherwise named variables
  // are core variables or other contrib variables.
  if (strpos($variable, $_coder_upgrade_module_name) !== FALSE) { // todo change this to a check for core variables.
    // $name['value'] = 'config_clear';
    $item->name = new Name('config_clear');

    $p0 = $item->args[0]->value->value;
    $temp = $_coder_upgrade_module_name . '.settings';
    $builderFactory = new BuilderFactory();
    $args = $builderFactory->args(array($temp, $p0));
    $item->args = $args;
  }
  else {
    module_load_include('inc', 'coder_upgrade', 'includes/variables');
    $variable_data = coder_upgrade_drupal_variables($variable);
    if ($variable_data) {
      if ($variable_data['storage'] == 'to_config') {
        $storage = coder_upgrade_backdrop_configs($variable_data['variable']);
        // Now replace the variable_del with config.
        // Process function call.
        $item->name = new Name('config_clear');

        $builderFactory = new BuilderFactory();
        $args = $builderFactory->args(array($storage, $variable_data['variable']));
        $item->args = $args;
      }
      elseif ($variable_data['storage'] == 'to_states') {
        $item->name = new Name('state_del');
        $item->args[0]->value->value = $variable_data['variable'];
      }
      elseif ($variable_data['storage'] == 'to_settings') {
        // This variable is a settings variable and cannot be deleted.
        $statement = "// TODO In Backdrop this variable is a settings variable and cannot be deleted, only retrieved.";
        $doc = new Doc($statement);
        $item->getAttribute('parent')->setDocComment($doc);
      }
      elseif ($variable_data['storage'] == 'removed') {
        // This variable was removed in Backdrop without replacement.
        $statement = "// TODO This variable was probably removed in Backdrop without replacement.";
        $doc = new Doc($statement);
        $item->getAttribute('parent')->setDocComment($doc);
      }
      elseif ($variable_data['storage'] == 'unknown') {
        // This variable is a known Drupal variable but we're not sure about it.
        $statement = "// TODO This variable was probably removed in Backdrop without replacement.";
        $doc = new Doc($statement);
        $item->getAttribute('parent')->setDocComment($doc);
      }
    }
    else {
      // This variable is not a known Drupal core variable.
      $statement = "// TODO This variable was probably removed in Backdrop without replacement.";
      $doc = new Doc($statement);
      $item->getAttribute('parent')->setDocComment($doc);
    }
  }
}

/**
 * Implements hook_upgrade_call_system_settings_form_alter().
 */
function coder_upgrade_upgrade_call_system_settings_form_alter(&$node, &$asts) { // DONE
  // Create helper objects.
  global $_coder_upgrade_module_name;

  $item = &$node;
  // Get the string used to represent the form (usually "$form") by checking the
  // parameter in system_settings_form return call.
  $form = $item->args[0]->value->name;

  // Find the parent function.
  $parent = $item->getAttribute('parent')->getAttribute('parent');
  $parent_function = NULL;
  foreach ($asts as $ast_key => $ast) {
    if ($ast->name->toString() == $parent->name->toString()) {
      $parent_function = $ast;
      break;
    }
  }

  $nodeFinder = new NodeFinder;

  // Now search for the first occurence of this string.
  $first_node = $nodeFinder->findFirst($parent_function->stmts, function (\PhpParser\Node $subNode) use ($form) {
      return $subNode instanceof \PhpParser\Node\Expr\Assign
      && ((isset($subNode->var->name) && $subNode->var->name == $form)
      || (isset($subNode->var->var->name) && $subNode->var->var->name == $form));
  });
  $var = new Variable($form . "['#config']");
  $assignee = new String_($_coder_upgrade_module_name . '.settings');
  $assign = new Expression(new Assign($var, $assignee));

  // Find where the first node is in the parent statements.
  $key = array_search($first_node->getAttribute('parent'), $parent_function->stmts);

  // If the form was initialized with an empty array, put our new '#config' key
  // after that, otherwise before.
  if ($first_node->expr->items == array()) {
    array_splice($parent_function->stmts, ($key + 1), 0, array($assign));
  }
  else {
    array_splice($parent_function->stmts, $key, 0, array($assign));
  }
}

