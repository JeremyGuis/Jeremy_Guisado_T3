# Operación Extracción - FPS 3D en OpenGL (Evaluación T3)

Videojuego de disparos en primera persona (FPS) desarrollado en **C++ y OpenGL 3.3 Core** utilizando GLFW, GLAD y GLM. El jugador debe atravesar un búnker infestado de enemigos, gestionar su munición mediante cajas de suministro y alcanzar la plataforma verde de extracción.

## Controles
* **W / A / S / D:** Desplazamiento del jugador en el espacio 3D.
* **Movimiento del Mouse:** Orientación de cámara en primera persona (Yaw / Pitch).
* **Clic Izquierdo:** Disparo único con retroceso y fogonazo dinámico.
* **Tecla 1:** Equipar Rifle de Plasma (15 balas iniciales).
* **Tecla 2:** Equipar Pistola Táctica (12 balas iniciales).
* **Tecla R:** Reiniciar la partida tras ganar o perder.
* **ESC:** Cerrar el videojuego.

## Características Implementadas
* **Cámara FPS y Colisiones:** Movimiento restringido al plano del búnker con límites de pared y altura de visión fija.
* **Sistema de Armas y HUD 2D:** Cambio visual de armas en primera persona con *weapon bobbing*, retroceso, mira central e indicador visual de balas restantes.
* **IA de Enemigos:** Detección por distancia, persecución activa hacia el jugador, resistencia de 2 impactos mediante *raycasting* y sistema de *respawn* tras 5 segundos.
* **Condiciones de Victoria y Derrota:** Plataforma de extracción funcional al final del búnker (`¡MISION COMPLETADA!`) y pantalla de `GAME OVER` al ser atrapado por un enemigo.

## Requisitos e Instalación
1. Contar con **Visual Studio 2022** con la carga de trabajo *Desarrollo para el escritorio con C++*.
2. Tener configuradas las librerías **GLFW**, **GLAD** y **GLM** en arquitectura **x64**.
3. Abrir `Jeremy_Guisado_T3.sln`, seleccionar configuración **Debug / x64** y presionar **F5** para compilar y ejecutar.
