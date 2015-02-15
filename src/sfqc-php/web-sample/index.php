<!DOCTYPE html>
<html lang="ja">
<head>
<meta http-equiv="content-type" content="text/html; charset=utf-8">
<meta name="description" content="File-based Queue C java php"">

<title>Simple File-based Queue sample page</title>

<?php include_once("analyticstracking.php"); ?>

</head>
<body>
<h1>sfq</h1>
<h2>Simple File-based Queue sample page</h2>

[github]
<ul>
 <li><a href="https://github.com/cbh34680/sfq/blob/master/README.md" target="_blank">README</a>
 <li><a href="https://github.com/cbh34680/sfq/blob/master/README.ja.md" target="_blank">README (ja)</a>
 <li><a href="https://github.com/cbh34680/sfq" target="_blank">project top</a>
 <li><a href="https://github.com/cbh34680/sfq/tree/master/src/sfqc-php/web-sample"
        target="_blank">this directory</a>
</ul>

<hr />
[webque-0]<br />
<br />
CREATE: <i>sudo /project/sfq/bin/sfqc-init -N webque-0 -U apache -G devgroup</i>
<br />
<ul>
 <li><a href="sfqc-push.php" target="_blank">push</a></li>
 <li><a href="sfqc-pop.php" target="_blank">pop</a></li>
 <li><a href="sfqc-shift.php" target="_blank">shift</a></li>
</ul>

<hr />
[webque-1]<br />
<br />
CREATE: <i>sudo /project/sfq/bin/sfqc-init -N webque-1 -U apache -G devgroup -B 1 -o -e</i>
<br />
<ul>
 <li><a href="sfqc-push-env.php" target="_blank">push (`env`)</a></li>
 <li><a href="sfqc-push-hosts.php" target="_blank">push (cat -n /etc/hosts)</a></li>
 <li><a href="sfqc-push-sh.php" target="_blank">push (command | /bin/sh)</a></li>
 <li><a href="sfqc-push-err.php" target="_blank">push (ls -l /home/NotFound/)</a></li>
 <li><a href="sfqc-push-cut.php" target="_blank">push (payload-size)</a></li>
</ul>

<hr />
<a href="phpinfo.php" target="_blank">phpinfo</a>
</body>
</html>

