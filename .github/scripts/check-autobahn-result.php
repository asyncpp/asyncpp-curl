<?php

$non_strict = [
    "6.4.3",
    "6.4.4",
];
$informational = [
    "7.1.6",
    "7.13.1",
    "7.13.2"
];

$data = file_get_contents($argv[1]);
if($data === false) exit(-1);
$data = json_decode($data, true);
if(!is_array($data) || !isset($data["asyncpp-curl"])) exit(-1);
$data = $data["asyncpp-curl"];
$failed = false;
foreach($data as $k=>$v) {
    $ok = true;
    if($v["behavior"] == "UNIMPLEMENTED" || $v["behaviorClose"] == "UNIMPLEMENTED") {
        if(explode(".", $k)[0] == "12" || explode(".", $k)[0] == "13") continue;
        else $ok = false;
    } else if($v["behavior"] == "NON-STRICT" || $v["behaviorClose"] == "NON-STRICT") {
        if(!in_array($k, $non_strict)) $ok = false;
    } else if($v["behavior"] == "INFORMATIONAL" || $v["behaviorClose"] == "INFORMATIONAL") {
        if(!in_array($k, $informational)) $ok = false;
    } else if($v["behavior"] != "OK" || $v["behaviorClose"] != "OK") {
        $ok = false;
    }
    if(!$ok) {
        echo("Test ".$k." failed\n");
        var_dump($v);
        $failed = true;
    }
}
if($failed) exit(-1);
else exit(0);