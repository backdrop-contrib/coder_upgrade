<?php
TEST: JavaScript

--INPUT--
function _locale_rebuild_js($langcode = NULL) {
  if (!empty($translations)) {

    $data = "Backdrop.locale = { ";

    if (!empty($language->formula)) {
      $data .= "'pluralFormula': function(\$n) { return Number({$language->formula}); }, ";
    }

    $data .= "'strings': ". backdrop_to_js($translations) ." };";
    $data_hash = md5($data);
  }
}

