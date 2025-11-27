#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import cgi
import cgitb
import html  # <--- este es el cambio
import time

# Habilita mensajes de error en el navegador
# cgitb.enable()

print("Content-Type: text/html; charset=utf-8")
print()  # línea vacía que separa headers del body

print(f"""
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>CGI Test</title>
</head>
<body>
    <h1>¡CGI funcionando!</h1>
    <p>Este es un test básico de un script Python ejecutado con CGI 😊</p>
</body>
</html>
""")

print()


