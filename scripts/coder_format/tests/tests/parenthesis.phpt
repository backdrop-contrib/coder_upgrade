<?php
TEST: Parenthesis

--INPUT--
function backdrop_mail_send($message) {
  return mail(
    $message['to'],
    mime_header_encode($message['subject']),
    str_replace("\r", '', $message['body']),
    join("\n", $mimeheaders)
  );
}

--INPUT--
function backdrop_to_js($var) {
  switch (gettype($var)) {
    case 'string':
      return '"'. str_replace(array("\r", "\n", "<", ">", "&"),
                              array('\r', '\n', '\x3c', '\x3e', '\x26'),
                              addslashes($var)) .'"';
    default:
      return 'null';
  }
}

--EXPECT--
function backdrop_to_js($var) {
  switch (gettype($var)) {
    case 'string':
      return '"'. str_replace(array("\r", "\n", "<", ">", "&"),
        array('\r', '\n', '\x3c', '\x3e', '\x26'),
        addslashes($var)
      ) .'"';

    default:
      return 'null';
  }
}

--INPUT--
function backdrop_urlencode($text) {
  if (variable_get('clean_url', '0')) {
    return str_replace(array('%2F', '%26', '%23', '//'),
                       array('/', '%2526', '%2523', '/%252F'),
                       rawurlencode($text));
  }
}

--EXPECT--
function backdrop_urlencode($text) {
  if (variable_get('clean_url', '0')) {
    return str_replace(array('%2F', '%26', '%23', '//'),
      array('/', '%2526', '%2523', '/%252F'),
      rawurlencode($text)
    );
  }
}

