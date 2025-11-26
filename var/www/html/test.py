#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import cgi
import cgitb

# Habilita mensajes de error en el navegador
cgitb.enable()

print("Content-Type: text/html; charset=utf-8")
print()

print("""
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>CGI Test</title>
</head>
<body>
    <h1>¡CGI funcionando!</h1>
    <p>Este es un test basico de un script Python ejecutado con CGI 😊</p>
</body>
</html>
""")
