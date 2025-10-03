Reflection (<=500 words)

Preguntas y decisiones
- Pregunté por el intervalo y la ruta del dispositivo; usé CLI flags para flexibilidad.
- Elegí `/dev/urandom` porque está disponible en la mayoría de sistemas Linux, es seguro y no requiere privilegios especiales.

Iteración
- Implementé manejo de SIGTERM y flushing del buffer para evitar líneas parciales.
- Añadí una ruta de respaldo `/var/tmp` si `/tmp` no es escribible.

Validación
- Compilé localmente con `go build` durante el desarrollo y añadí instrucciones en README para pruebas manuales.

Cambios manuales
- Ajusté permisos y documentación para uso sin root y agregué `Makefile` y unidad systemd.
