<?php

$file = file('./opcodes.txt', FILE_IGNORE_NEW_LINES);
// throw the first two lines away
$file = array_slice($file, 2);

foreach ($file as $line) {
    // throw the first two lines away
    $params = preg_split('/\s+/', $line);
    $opcode = $params[1];
    $address = $params[2];

    $addressChopped = substr($address, 1);
    echo '#define '.$opcode.'_'.$addressChopped.' 0x'.$addressChopped."\n";
}
